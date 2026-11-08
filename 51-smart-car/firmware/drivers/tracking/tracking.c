/*
 * tracking.c —— 18 路循迹传感器读取、滤波、位置解算与元素状态机
 *
 * 硬件：3 片 74HC165 级联，PL / CP / Q7 三线，24 位。
 *   移出顺序 A7..A0 -> B7..B0 -> C7..C0，
 *   对应 bit0..bit15 = S0..S15，bit16 = SIDE_L，bit17 = SIDE_R。
 *
 * 分层：
 *   tracking_read_raw()      只负责 165 时序，是唯一碰硬件的地方；
 *   tracking_filter()        逐通道 3 次一致确认（可脱离硬件单测）；
 *   tracking_update_geom()   有效通道数、两半区计数、Q4 位置；
 *   tracking_sm()            十字 / 环岛状态机。
 *
 * 极性约定：1 = 压黑线（比较器输出经 165 读入后为正逻辑）。
 * 若实测相反，在 tracking_read_raw() 返回前按位取反，不改其它层。
 */

#include "config.h"
#include "tracking.h"
#include "tick.h"
#include <intrins.h>

/* 状态机内部常量（config_tracking.h 未定义的部分） */
#define TRK_CROSS_CONFIRM     2U     /* 连续 2 次扫描满足才进十字 */
#define TRK_ROUND_MIN_COUNT   6U     /* 环岛候选的最小有效通道数 */
#define TRK_ROUND_EXIT_MS     100U   /* ROUND_IN 无侧向持续判定退出 */
#define TRK_ROUND_ENTER_ABORT 100U   /* 候选消失后的冷却 */

static u32 s_raw;                       /* 最近一次原始 24 位 */
static u32 s_stable;                    /* 确认后的位图 */
static u8  idata s_conf[TRK_TOTAL_CH];  /* 每通道确认计数 0..3 */

static u8  s_count;                     /* 主阵列有效通道数 */
static u8  s_left_count;                /* S0..S7 有效数 */
static u8  s_right_count;               /* S8..S15 有效数 */
static u8  s_pos_q4;                    /* 0..240 */
static s8  s_e_q4;                      /* -120..+120 */

static trk_state_t s_state;
static u8  s_cross_cnt;                 /* 十字连续满足次数 */
static u16 s_state_ms;                  /* 当前状态已持续时间 */
static u16 s_cooldown_ms;               /* 环岛冷却剩余时间 */
static u16 s_lost_ms;                   /* 丢线累计 */
static u16 s_last_scan_ms;              /* 上次扫描时间戳 */

u32 tracking_read_raw(void)
{
    u8 i;
    u32 v;

    /* 并行装载：PL 低脉冲，保持约 4us */
    PIN_TRK_PL = 0;
    _nop_();
    _nop_();
    _nop_();
    _nop_();
    PIN_TRK_PL = 1;
    _nop_();

    /* CP 空闲为高，逐位读取后打一个上升沿移出下一位 */
    v = 0UL;
    for (i = 0U; i < PIN_TRK_CHAIN_LEN; i++)
    {
        v <<= 1;
        if (PIN_TRK_Q7)
        {
            v |= 1UL;
        }

        PIN_TRK_CP = 0;
        _nop_();
        PIN_TRK_CP = 1;
        _nop_();
    }

    return (v & 0x00FFFFFFUL);
}

/* 逐通道 3 次一致确认：黑线需连续 3 次才置位，白底需连续 3 次才清除 */
static void tracking_filter(u32 raw)
{
    u8 i;
    u32 mask;

    for (i = 0U; i < TRK_TOTAL_CH; i++)
    {
        mask = TRK_BIT_MASK(i);

        if ((raw & mask) != 0UL)
        {
            if (s_conf[i] < TRK_FILTER_CONFIRM)
            {
                s_conf[i]++;
            }
        }
        else if (s_conf[i] > 0U)
        {
            s_conf[i]--;
        }

        if (s_conf[i] >= TRK_FILTER_CONFIRM)
        {
            s_stable |= mask;
        }
        else
        {
            s_stable &= ~mask;
        }
    }
}

/* 由确认位图算有效通道数与加权位置 */
static void tracking_update_geom(void)
{
    u8 i;
    u8 count;
    u8 left;
    u8 right;
    u16 sum;
    u16 pos;

    count = 0U;
    left = 0U;
    right = 0U;
    sum = 0U;

    for (i = 0U; i < TRK_MAIN_CH; i++)
    {
        if ((s_stable & TRK_BIT_MASK(i)) != 0UL)
        {
            count++;
            sum = (u16)(sum + (u16)i);
            if (i < (TRK_MAIN_CH / 2U))
            {
                left++;
            }
            else
            {
                right++;
            }
        }
    }

#if (TRK_EDGE_IGNORE != 0)
    /* 最外侧单独触发视为邻道或噪点，不参与位置计算 */
    if (count == 1U)
    {
        if ((s_stable & TRK_BIT_MASK(TRK_CH_LEFT)) ||
            (s_stable & TRK_BIT_MASK(TRK_CH_RIGHT)))
        {
            count = 0U;
        }
    }
#endif

    s_count = count;
    s_left_count = left;
    s_right_count = right;

    if (count > 0U)
    {
        pos = (u16)((sum * TRK_POS_SCALE) / (u16)count);
        if (pos > (u16)(TRK_CENTER_Q4 * 2U))
        {
            pos = (u16)(TRK_CENTER_Q4 * 2U);
        }
        s_pos_q4 = (u8)pos;
        s_e_q4 = (s8)((s16)pos - (s16)TRK_CENTER_Q4);
    }
}

/* 侧向位：bit0 = 左，bit1 = 右 */
static u8 tracking_side_bits(void)
{
    u8 side;

    side = 0U;
    if ((s_stable & TRK_BIT_MASK(TRK_BIT_SIDE_L)) != 0UL)
    {
        side |= 1U;
    }
    if ((s_stable & TRK_BIT_MASK(TRK_BIT_SIDE_R)) != 0UL)
    {
        side |= 2U;
    }
    return side;
}

/* 单侧分支是否明显：一侧比另一侧多出 TRK_ROUND_WIDTH_DELTA 路 */
static u8 tracking_round_asymmetry(void)
{
    if ((s_left_count >= (u8)(s_right_count + TRK_ROUND_WIDTH_DELTA)) ||
        (s_right_count >= (u8)(s_left_count + TRK_ROUND_WIDTH_DELTA)))
    {
        return 1U;
    }
    return 0U;
}

static void tracking_sm(u16 dt)
{
    u8 side;

    side = tracking_side_bits();

    if (s_cooldown_ms >= dt)
    {
        s_cooldown_ms = (u16)(s_cooldown_ms - dt);
    }
    else
    {
        s_cooldown_ms = 0U;
    }

    switch (s_state)
    {
    case TRK_STATE_NORMAL:
        if (s_count >= TRK_CROSS_COUNT)
        {
            if (s_cross_cnt < TRK_CROSS_CONFIRM)
            {
                s_cross_cnt++;
            }
            if (s_cross_cnt >= TRK_CROSS_CONFIRM)
            {
                s_state = TRK_STATE_CROSS;
                s_state_ms = 0U;
            }
        }
        else
        {
            s_cross_cnt = 0U;
        }

        if ((s_state == TRK_STATE_NORMAL) && (s_cooldown_ms == 0U))
        {
            if ((side != 0U) && (s_count >= TRK_ROUND_MIN_COUNT) &&
                (tracking_round_asymmetry() != 0U))
            {
                s_state = TRK_STATE_ROUND_ENTER;
                s_state_ms = 0U;
            }
        }
        break;

    case TRK_STATE_CROSS:
        s_state_ms = (u16)(s_state_ms + dt);
        if ((s_count <= TRK_CROSS_EXIT_COUNT) || (s_state_ms >= TRK_CROSS_HOLD_MS))
        {
            s_state = TRK_STATE_NORMAL;
            s_cross_cnt = 0U;
            s_cooldown_ms = TRK_ROUND_COOLDOWN_MS;
        }
        break;

    case TRK_STATE_ROUND_ENTER:
        s_state_ms = (u16)(s_state_ms + dt);
        if (side == 0U)
        {
            s_state = TRK_STATE_NORMAL;
            s_cooldown_ms = TRK_ROUND_ENTER_ABORT;
        }
        else if (s_state_ms >= TRK_ROUND_HOLD_MS)
        {
            s_state = TRK_STATE_ROUND_IN;
            s_state_ms = 0U;
        }
        break;

    case TRK_STATE_ROUND_IN:
        if (side == 0U)
        {
            s_state_ms = (u16)(s_state_ms + dt);
            if (s_state_ms >= TRK_ROUND_EXIT_MS)
            {
                s_state = TRK_STATE_ROUND_EXIT;
                s_state_ms = 0U;
            }
        }
        else
        {
            s_state_ms = 0U;
        }
        break;

    case TRK_STATE_ROUND_EXIT:
        s_state_ms = (u16)(s_state_ms + dt);
        if (s_state_ms >= TRK_ROUND_EXIT_MS)
        {
            s_state = TRK_STATE_NORMAL;
            s_cooldown_ms = TRK_ROUND_COOLDOWN_MS;
        }
        break;

    default:
        s_state = TRK_STATE_NORMAL;
        break;
    }
}

u8 tracking_scan(void)
{
    u16 now;
    u16 dt;

    s_raw = tracking_read_raw();
    tracking_filter(s_raw);
    tracking_update_geom();

    now = tick_get_ms();
    dt = (u16)(now - s_last_scan_ms);
    s_last_scan_ms = now;

    if (s_count < TRK_VALID_MIN)
    {
        if (s_lost_ms < 60000U)
        {
            s_lost_ms = (u16)(s_lost_ms + dt);
        }
    }
    else
    {
        s_lost_ms = 0U;
    }

    tracking_sm(dt);

    return (s_count >= TRK_VALID_MIN) ? 1U : 0U;
}

u8 tracking_channel(u8 ch)
{
    if (ch >= TRK_TOTAL_CH)
    {
        return 0U;
    }
    return ((s_stable & TRK_BIT_MASK(ch)) != 0UL) ? 1U : 0U;
}

u8 tracking_count(void)
{
    return s_count;
}

u8 tracking_position_q4(void)
{
    return s_pos_q4;
}

s8 tracking_error_q4(void)
{
    return s_e_q4;
}

u8 tracking_side_left(void)
{
    return ((s_stable & TRK_BIT_MASK(TRK_BIT_SIDE_L)) != 0UL) ? 1U : 0U;
}

u8 tracking_side_right(void)
{
    return ((s_stable & TRK_BIT_MASK(TRK_BIT_SIDE_R)) != 0UL) ? 1U : 0U;
}

trk_state_t tracking_state(void)
{
    return s_state;
}

u8 tracking_cross_active(void)
{
    return (s_state == TRK_STATE_CROSS) ? 1U : 0U;
}

u16 tracking_lost_ms(void)
{
    return s_lost_ms;
}

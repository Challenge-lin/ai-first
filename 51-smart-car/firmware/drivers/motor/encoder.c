/*
 * encoder.c —— 双路霍尔编码器计数、测速与里程
 *
 * 资源：
 *   左轮 A 相 P3.2/INT0（下降沿中断，interrupt 0，高优先级）
 *   右轮 A 相 P3.3/INT1（下降沿中断，interrupt 2，高优先级）
 *   B 相 P0.0 / P0.1 普通输入，在 A 相下降沿采样判方向
 *
 * 计数口径：
 *   A 相单沿（下降沿）计数，13 线 x 30 减速比 = 390 脉冲/输出轴转。
 *   正向（B 相为低）加 1，反向减 1；累计量减到 0 后不再减，避免 u32 下溢。
 *   总里程与单圈里程取左右轮平均，等价于车体中心行程。
 *
 * 测速口径：
 *   encoder_speed_mm_s() 读取并清零该轮窗口计数，用 tick 实测间隔换算，
 *   结果 mm/s。调用频率建议 20ms（CFG_SPEED_LOOP_HZ = 50）。
 *   encoder_xxx_pulses() 返回最近一次测速窗口的脉冲数。
 */

#include "config.h"
#include "encoder.h"
#include "tick.h"

/* 累计脉冲（左右轮分开，取平均作为车体里程） */
static volatile u32 idata s_left_total;
static volatile u32 idata s_right_total;

/* 本圈脉冲 */
static volatile u32 idata s_left_lap;
static volatile u32 idata s_right_lap;

/* 当前测速窗口内脉冲，速度接口读取后清零 */
static volatile u16 idata s_left_win;
static volatile u16 idata s_right_win;

/* 最近一次测速窗口的脉冲数（供遥测/调试） */
static u16 idata s_left_last;
static u16 idata s_right_last;

/* 最近一次测速的时间戳，单位 ms */
static u16 idata s_left_stamp;
static u16 idata s_right_stamp;

/* INT0：左轮 A 相下降沿 */
static void enc_left_isr(void) INT(0)
{
    u8 fwd;

    fwd = (PIN_ENC_L_B == 0) ? 1 : 0;
#if (ENC_LEFT_INVERT != 0)
    fwd = (u8)(fwd ^ 1U);
#endif

    if (fwd)
    {
        s_left_win++;
        s_left_total++;
        s_left_lap++;
    }
    else
    {
        if (s_left_win > 0U)   { s_left_win--; }
        if (s_left_total > 0UL) { s_left_total--; }
        if (s_left_lap > 0UL)   { s_left_lap--; }
    }
}

/* INT1：右轮 A 相下降沿 */
static void enc_right_isr(void) INT(2)
{
    u8 fwd;

    fwd = (PIN_ENC_R_B == 0) ? 1 : 0;
#if (ENC_RIGHT_INVERT != 0)
    fwd = (u8)(fwd ^ 1U);
#endif

    if (fwd)
    {
        s_right_win++;
        s_right_total++;
        s_right_lap++;
    }
    else
    {
        if (s_right_win > 0U)   { s_right_win--; }
        if (s_right_total > 0UL) { s_right_total--; }
        if (s_right_lap > 0UL)   { s_right_lap--; }
    }
}

void encoder_reset(void)
{
    bit ex0_save;
    bit ex1_save;

    ex0_save = EX0;
    ex1_save = EX1;
    EX0 = 0;
    EX1 = 0;

    s_left_total = 0UL;
    s_right_total = 0UL;
    s_left_lap = 0UL;
    s_right_lap = 0UL;
    s_left_win = 0U;
    s_right_win = 0U;

    EX0 = ex0_save;
    EX1 = ex1_save;

    s_left_last = 0U;
    s_right_last = 0U;
    s_left_stamp = tick_get_ms();
    s_right_stamp = s_left_stamp;
}

void encoder_init(void)
{
    /* P0 作输入先写 1，配合外部 10k 上拉 */
    PIN_ENC_L_A = 1;
    PIN_ENC_R_A = 1;
    PIN_ENC_L_B = 1;
    PIN_ENC_R_B = 1;

    encoder_reset();

    IT0 = 1;    /* INT0 下降沿触发 */
    IT1 = 1;    /* INT1 下降沿触发 */
    PX0 = 1;    /* 高优先级，避免被 Timer1/UART 拖延造成丢脉冲 */
    PX1 = 1;
    EX0 = 1;
    EX1 = 1;
}

u16 encoder_left_pulses(void)
{
    return s_left_last;
}

u16 encoder_right_pulses(void)
{
    return s_right_last;
}

u32 encoder_total_pulses(void)
{
    u32 l;
    u32 r;

    EX0 = 0;
    EX1 = 0;
    l = s_left_total;
    r = s_right_total;
    EX0 = 1;
    EX1 = 1;

    return (l + r) / 2UL;
}

u32 encoder_lap_pulses(void)
{
    u32 l;
    u32 r;

    EX0 = 0;
    EX1 = 0;
    l = s_left_lap;
    r = s_right_lap;
    EX0 = 1;
    EX1 = 1;

    return (l + r) / 2UL;
}

u16 encoder_speed_mm_s(u8 wheel)
{
    u16 pulses;
    u16 now;
    u16 dt;
    u32 mm_s;

    if (wheel == ENC_WHEEL_LEFT)
    {
        EX0 = 0;
        pulses = s_left_win;
        s_left_win = 0U;
        EX0 = 1;

        s_left_last = pulses;

        now = tick_get_ms();
        dt = (u16)(now - s_left_stamp);
        s_left_stamp = now;
    }
    else
    {
        EX1 = 0;
        pulses = s_right_win;
        s_right_win = 0U;
        EX1 = 1;

        s_right_last = pulses;

        now = tick_get_ms();
        dt = (u16)(now - s_right_stamp);
        s_right_stamp = now;
    }

    if (dt == 0U)
    {
        dt = 1U;    /* 同一毫秒内重复调用，按 1ms 计 */
    }

    /* pulses * 0.524mm / dt(ms) = mm/s */
    mm_s = ((u32)pulses * ENC_MM_PER_PULSE_X1000) / (u32)dt;
    if (mm_s > 65535UL)
    {
        mm_s = 65535UL;
    }

    return (u16)mm_s;
}

u32 encoder_distance_mm(void)
{
    return ENC_PULSES_TO_MM(encoder_total_pulses());
}

/*
 * app.c —— 整车状态机、循迹控制、计圈停车与保护
 *
 * 状态：IDLE -> ARMED（长按 START，倒计时）-> RUN -> FINISH / FAULT
 *
 * 10ms 控制周期：
 *   基础占空比按 |e_q4| 降速，环岛用固定低速，十字强制直行；
 *   PD 输出 u 做左右差速：left = base + u，right = base - u；
 *   每周期占空比变化不超过 DUTY_SLEW_STEP，保护齿轮箱和电池。
 *
 * 计圈：发车线是横贯赛道的黑线，表现为主阵列大量压线（>= TRK_STARTLINE_COUNT）。
 *   首次过线开始计时，之后每过一次线累加一圈，共 CROSSINGS_REQUIRED 次（含起跑）。
 *   两次过线之间必须超过圈长的 85%（LAP_WINDOW_PCT），否则视为十字或噪点。
 *
 * 停车：最后一次过线后按编码器里程行驶 STOP_TARGET_MM 刹车，
 *   超过 STOP_FORCE_MM 无论如何都刹。
 *
 * 保护：丢线 800ms、堵转 500ms、运行超时 120s、欠压 200ms 均进 FAULT。
 */

#include "config.h"
#include "app.h"
#include "tick.h"
#include "motor.h"
#include "encoder.h"
#include "tracking.h"
#include "pid.h"

#define APP_LINE_MS_MAX   200U   /* 发车线计时的上限保护 */

static sys_state_t s_state;
static fault_code_t s_fault;

static u16 s_state_ms;          /* 当前状态持续时间 */
static u32 s_run_ms;            /* RUN 累计时间 */
static u8  s_lap;               /* 已完成圈数 */
static u8  s_crossings;         /* 过线次数（含起跑） */
static u32 s_prev_cross_mm;     /* 上次过线时的里程 */
static u32 s_lap_len_mm;        /* 学习到的圈长，0 = 未知 */
static u8  s_line_ms;           /* 发车线连续压线时间 */
static u8  s_cross_armed;       /* 允许再次计线 */
static u32 s_stop_dist_mm;      /* 最后一次过线里程 */
static u8  s_stop_done;         /* 已完成停车 */

static u8  s_key_stable;        /* START 键稳定电平 */
static u16 s_key_ms;            /* 按住时间 */
static u16 s_stop_key_ms;       /* STOP 键按住时间 */

static s8  s_u_last;            /* 上次 PD 输出，丢线保持用 */
static s8  s_last_left;         /* 上次左右占空比，斜率限制用 */
static s8  s_last_right;

static u16 s_speed_l;           /* 最近测速窗口 mm/s */
static u16 s_speed_r;
static s16 s_speed_i;           /* 速度环积分 */
static s16 s_speed_corr;        /* 速度环对基础占空比的修正 */

static u16 s_stall_ms;          /* 堵转累计 */
static u16 s_bat_ms;            /* 欠压累计 */

/* ===== 小工具 ===== */
static s16 app_clamp_s16(s16 v, s16 lo, s16 hi)
{
    if (v > hi)
    {
        return hi;
    }
    if (v < lo)
    {
        return lo;
    }
    return v;
}

static s8 app_clamp_duty(s16 v)
{
    return (s8)app_clamp_s16(v, (s16)PWM_DUTY_MIN, (s16)PWM_DUTY_MAX);
}

/* 每 10ms 最多改变 DUTY_SLEW_STEP 级 */
static s8 app_slew(s8 target, s8 last)
{
    s16 d;

    d = (s16)target - (s16)last;
    if (d > (s16)DUTY_SLEW_STEP)
    {
        return (s8)(last + (s8)DUTY_SLEW_STEP);
    }
    if (d < (s16)(-DUTY_SLEW_STEP))
    {
        return (s8)(last - (s8)DUTY_SLEW_STEP);
    }
    return target;
}

static void app_stop(void)
{
    motor_brake();
    s_u_last = 0;
    s_last_left = 0;
    s_last_right = 0;
    s_speed_corr = 0;
    s_speed_i = 0;
}

static void app_set_fault(fault_code_t f)
{
    s_fault = f;
    s_state = SYS_STATE_FAULT;
    app_stop();
    PIN_LED_ERR = 1;
    PIN_LED_RUN = 0;
    PIN_BUZZER = 0;
}

/* 允许再次计线所需的最小里程：已知圈长用圈长，否则用估算值 */
static u32 app_lap_window_mm(void)
{
    u32 base;

    base = (s_lap_len_mm != 0UL) ? s_lap_len_mm : LAP_LENGTH_MM_EST;
    return (base * (u32)(100U - LAP_WINDOW_PCT)) / 100UL;
}

/* 含起跑一共需要几次过线；START_MODE = 1 时首次过线即第 1 圈 */
static u8 app_crossing_limit(void)
{
    return (u8)(CROSSINGS_REQUIRED - ((START_MODE != 0U) ? 1U : 0U));
}

/* 基础占空比：弯道降速、环岛低速、起步斜坡 */
static s8 app_base_duty(s16 e)
{
    s16 base;
    u16 dev;
    u16 slow;
    trk_state_t st;

    st = tracking_state();
    if ((st == TRK_STATE_ROUND_ENTER) || (st == TRK_STATE_ROUND_IN))
    {
        base = (s16)ROUND_SPEED;
    }
    else
    {
        dev = (e < 0) ? (u16)(-e) : (u16)e;
        slow = (u16)(((u16)SPEED_SLOW_K * dev) >> 4);
        base = (s16)BASE_FAST - (s16)slow;
        if (base < (s16)BASE_CORNER)
        {
            base = (s16)BASE_CORNER;
        }
    }

    if (s_run_ms < START_RAMP_MS)
    {
        base = (s16)BASE_START +
               (s16)(((s32)(base - (s16)BASE_START) * (s32)s_run_ms) / (s32)START_RAMP_MS);
    }

    return (s8)base;
}

/* ===== 状态切换 ===== */
static void app_start_run(void)
{
    s_state = SYS_STATE_RUN;
    s_state_ms = 0U;
    s_run_ms = 0UL;
    s_crossings = 0U;
    s_lap = 0U;
    s_prev_cross_mm = 0UL;
    s_lap_len_mm = 0UL;
    s_line_ms = 0U;
    s_cross_armed = 1U;
    s_stop_done = 0U;
    s_stall_ms = 0U;
    s_bat_ms = 0U;
    s_u_last = 0;
    s_last_left = 0;
    s_last_right = 0;
    s_speed_corr = 0;
    s_speed_i = 0;

    pid_reset();
    encoder_reset();

    PIN_BUZZER = 0;
    PIN_LED_RUN = 1;
    motor_set_duty((s8)0, (s8)0);
}

/* ===== 10ms：按键 ===== */
static void app_key_10ms(void)
{
    u8 k;

    k = (PIN_KEY_START == 0) ? 1U : 0U;

    if (k != s_key_stable)
    {
        s_key_stable = k;
        s_key_ms = 0U;
        return;
    }

    if (s_key_ms < 60000U)
    {
        s_key_ms = (u16)(s_key_ms + 10U);
    }

    if ((s_state == SYS_STATE_IDLE) && (k != 0U) && (s_key_ms >= ARM_HOLD_MS))
    {
        s_state = SYS_STATE_ARMED;
        s_state_ms = 0U;
        PIN_BUZZER = 1;
    }
}

/* ===== 10ms：发车线 / 计圈 ===== */
static void app_lap_10ms(u32 dist)
{
    u32 need;

    if (tracking_count() >= TRK_STARTLINE_COUNT)
    {
        if (s_line_ms < APP_LINE_MS_MAX)
        {
            s_line_ms = (u8)(s_line_ms + 10U);
        }
    }
    else
    {
        s_line_ms = 0U;
        s_cross_armed = 1U;
    }

    if (s_line_ms < TRK_STARTLINE_MIN_MS)
    {
        return;
    }
    if (s_cross_armed == 0U)
    {
        return;
    }

    if (s_crossings == 0U)
    {
        /* 起跑过线必须在起步窗口内，否则认为放错位置 */
        if (dist > START_CROSS_MAX_MM)
        {
            app_set_fault(FAULT_OVERRUN);
            return;
        }
    }
    else
    {
        need = app_lap_window_mm();
        if ((dist - s_prev_cross_mm) < need)
        {
            return;     /* 距离窗口不足，按十字/噪点忽略 */
        }
    }

    s_cross_armed = 0U;
    s_crossings++;

    if (s_crossings == 1U)
    {
        s_prev_cross_mm = dist;
        /* 起步即压线时，本次直接算第 1 圈 */
        s_lap = (u8)(s_lap + ((START_MODE != 0U) ? 1U : 0U));
        return;
    }

    s_lap_len_mm = dist - s_prev_cross_mm;
    s_prev_cross_mm = dist;
    s_lap++;

    if (s_crossings >= app_crossing_limit())
    {
        s_state = SYS_STATE_FINISH;
        s_stop_dist_mm = dist;
        s_stop_done = 0U;
        s_state_ms = 0U;
    }
}

/* ===== 10ms：RUN 循迹输出 ===== */
static void app_drive_10ms(void)
{
    s16 e;
    s16 u;
    s16 base;
    s16 left;
    s16 right;
    u16 lost;

    lost = tracking_lost_ms();

    if (lost >= LOST_STOP_MS)
    {
        app_set_fault(FAULT_LOST_LINE);
        return;
    }

    e = (s16)tracking_error_q4();
    if (tracking_cross_active())
    {
        e = 0;      /* 十字强制直行 */
    }

    base = (s16)app_base_duty(e);

    if (lost > 0U)
    {
        if (lost < LOST_HOLD_MS)
        {
            u = (s16)s_u_last;
            base = (s16)(((s32)base * (s32)LOST_SPEED_PCT) / 100L);
        }
        else
        {
            u = 0;
            base = 0;
        }
    }
    else
    {
        u = pid_update(e);
        s_u_last = (s8)u;
    }

    base = (s16)(base + s_speed_corr);
    if (base > (s16)PWM_DUTY_MAX)
    {
        base = (s16)PWM_DUTY_MAX;
    }
    if (base < (s16)(-PWM_DUTY_MAX))
    {
        base = (s16)(-PWM_DUTY_MAX);
    }

    left = (s16)(base + u);
    right = (s16)(base - u);

    left = (s16)app_slew(app_clamp_duty(left), s_last_left);
    right = (s16)app_slew(app_clamp_duty(right), s_last_right);

    s_last_left = (s8)left;
    s_last_right = (s8)right;
    motor_set_duty((s8)left, (s8)right);
}

/* ===== 10ms：FINISH 减速停车 ===== */
static void app_finish_10ms(void)
{
    u32 d;
    s16 e;
    s16 u;
    s16 left;
    s16 right;

    if (s_stop_done != 0U)
    {
        return;
    }

    d = encoder_distance_mm() - s_stop_dist_mm;
    if ((d >= STOP_TARGET_MM) || (d >= STOP_FORCE_MM))
    {
        app_stop();
        s_stop_done = 1U;
        return;
    }

    e = (s16)tracking_error_q4();
    if (tracking_cross_active())
    {
        e = 0;
    }
    u = pid_update(e);

    left = (s16)app_slew(app_clamp_duty((s16)((s16)BASE_CORNER + u)), s_last_left);
    right = (s16)app_slew(app_clamp_duty((s16)((s16)BASE_CORNER - u)), s_last_right);

    s_last_left = (s8)left;
    s_last_right = (s8)right;
    motor_set_duty((s8)left, (s8)right);
}

void app_init(void)
{
    s_state = SYS_STATE_IDLE;
    s_fault = FAULT_NONE;

    s_state_ms = 0U;
    s_run_ms = 0UL;
    s_lap = 0U;
    s_crossings = 0U;
    s_prev_cross_mm = 0UL;
    s_lap_len_mm = 0UL;
    s_line_ms = 0U;
    s_cross_armed = 1U;
    s_stop_dist_mm = 0UL;
    s_stop_done = 0U;

    s_key_stable = 0U;
    s_key_ms = 0U;
    s_stop_key_ms = 0U;

    s_u_last = 0;
    s_last_left = 0;
    s_last_right = 0;
    s_speed_l = 0U;
    s_speed_r = 0U;
    s_speed_i = 0;
    s_speed_corr = 0;

    s_stall_ms = 0U;
    s_bat_ms = 0U;

    pid_reset();
    encoder_reset();

    PIN_LED_RUN = 0;
    PIN_LED_ERR = 0;
    PIN_BUZZER = 0;
    motor_standby();
}

void app_control(void)
{
    u32 dist;

    app_key_10ms();

    switch (s_state)
    {
    case SYS_STATE_IDLE:
        break;

    case SYS_STATE_ARMED:
        if (s_state_ms < 60000U)
        {
            s_state_ms = (u16)(s_state_ms + 10U);
        }
        PIN_BUZZER = 1;
        if (s_state_ms >= ARM_COUNTDOWN_MS)
        {
            app_start_run();
        }
        break;

    case SYS_STATE_RUN:
        if (s_run_ms < RUN_TIMEOUT_MS)
        {
            s_run_ms = (u32)(s_run_ms + 10UL);
        }
        if (s_run_ms >= RUN_TIMEOUT_MS)
        {
            app_set_fault(FAULT_TIMEOUT);
            break;
        }

        dist = encoder_distance_mm();
        app_lap_10ms(dist);
        if (s_state == SYS_STATE_RUN)
        {
            app_drive_10ms();
        }
        break;

    case SYS_STATE_FINISH:
        app_finish_10ms();
        break;

    case SYS_STATE_FAULT:
    default:
        break;
    }
}

#if (CFG_ENABLE_SPEED_LOOP != 0)
/* 速度环目标速度：按元素类型和偏差粗分三档 */
static u16 app_target_speed(void)
{
    s16 e;
    trk_state_t st;

    st = tracking_state();
    if ((st == TRK_STATE_ROUND_ENTER) || (st == TRK_STATE_ROUND_IN))
    {
        return (u16)SPEED_ROUND_MM_S;
    }

    e = (s16)tracking_error_q4();
    if (e < 0)
    {
        e = (s16)(-e);
    }
    if (e >= 32)
    {
        return (u16)SPEED_CORNER_MM_S;
    }
    return (u16)SPEED_STRAIGHT_MM_S;
}
#endif

void app_speed(void)
{
    s_speed_l = encoder_speed_mm_s(ENC_WHEEL_LEFT);
    s_speed_r = encoder_speed_mm_s(ENC_WHEEL_RIGHT);

#if (CFG_ENABLE_SPEED_LOOP != 0)
    if (s_state == SYS_STATE_RUN)
    {
        u16 avg;
        u16 target;
        s16 err;
        s16 corr;

        avg = (u16)(((u32)s_speed_l + (u32)s_speed_r) / 2UL);
        target = app_target_speed();
        err = (s16)((s16)target - (s16)avg);

        s_speed_i = (s16)(s_speed_i +
                         (s16)(((s32)SPEED_KI_Q8 * (s32)err) >> PID_SHIFT));
        s_speed_i = app_clamp_s16(s_speed_i, (s16)(-SPEED_I_MAX), (s16)SPEED_I_MAX);

        corr = (s16)(((s32)SPEED_KP_Q8 * (s32)err) >> PID_SHIFT);
        corr = (s16)(corr + s_speed_i);
        s_speed_corr = app_clamp_s16(corr, (s16)(-SPEED_CORR_MAX), (s16)SPEED_CORR_MAX);
    }
    else
    {
        s_speed_i = 0;
        s_speed_corr = 0;
    }
#else
    s_speed_i = 0;
    s_speed_corr = 0;
#endif
}

void app_monitor(void)
{
    u16 spd;
    s16 duty;

    /* STOP 键：按住 200ms 直接停车，可从其它状态退回 IDLE */
    if (PIN_KEY_STOP == 0)
    {
        if (s_stop_key_ms < 60000U)
        {
            s_stop_key_ms = (u16)(s_stop_key_ms + 100U);
        }
    }
    else
    {
        s_stop_key_ms = 0U;
    }

    if ((s_stop_key_ms >= KEY_STOP_HOLD_MS) && (s_state != SYS_STATE_IDLE))
    {
        app_stop();
        s_state = SYS_STATE_IDLE;
        s_fault = FAULT_NONE;
        s_state_ms = 0U;
        s_stop_key_ms = 0U;
        PIN_LED_RUN = 0;
        PIN_LED_ERR = 0;
        PIN_BUZZER = 0;
        return;
    }

    if (s_state == SYS_STATE_RUN)
    {
        /* 堵转：占空比够大但轮速持续很低 */
        spd = (u16)(((u32)s_speed_l + (u32)s_speed_r) / 2UL);
        duty = (s16)s_last_left;
        if (duty < 0)
        {
            duty = (s16)(-duty);
        }

        if ((duty >= (s16)STALL_DUTY_MIN) && (spd < STALL_SPEED_MM_S))
        {
            if (s_stall_ms < 60000U)
            {
                s_stall_ms = (u16)(s_stall_ms + 100U);
            }
            if (s_stall_ms >= STALL_MS)
            {
                app_set_fault(FAULT_STALL);
            }
        }
        else
        {
            s_stall_ms = 0U;
        }

        /* 欠压：比较器输出低有效 */
        if ((CFG_ENABLE_LOW_BATTERY != 0) && (PIN_VBAT_LOW == 0))
        {
            if (s_bat_ms < 60000U)
            {
                s_bat_ms = (u16)(s_bat_ms + 100U);
            }
            if (s_bat_ms >= VBAT_FILTER_MS)
            {
                app_set_fault(FAULT_LOW_BATTERY);
            }
        }
        else
        {
            s_bat_ms = 0U;
        }
    }
    else
    {
        s_stall_ms = 0U;
        s_bat_ms = 0U;
    }

    if ((s_state == SYS_STATE_RUN) || (s_state == SYS_STATE_FINISH))
    {
        PIN_LED_RUN = 1;
    }
    else
    {
        PIN_LED_RUN = 0;
    }
}

sys_state_t app_state(void)
{
    return s_state;
}

fault_code_t app_fault(void)
{
    return s_fault;
}

u8 app_crossings(void)
{
    return s_crossings;
}

u8 app_lap(void)
{
    return s_lap;
}

u32 app_run_ms(void)
{
    return s_run_ms;
}

u32 app_distance_mm(void)
{
    return encoder_distance_mm();
}

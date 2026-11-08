/*
 * pid.c —— 循线 PD 控制器（Q8 增益，Q4 误差）
 *
 * 全部整型运算，不使用浮点：
 *   de     = e_q4 - e_prev_q4
 *   d_filt = (3 * d_filt + de) / 4
 *   u      = (Kp_q8 * e_q4 + Kd_q8 * d_filt) >> 8 + i_acc
 *   输出限幅 ±U_MAX（config_motor.h），与舵量/差速限幅保持一致。
 *
 * 积分默认关闭（PID_KI_Q8 = 0），保留接口供后续速度环复用。
 * 除法用常数 4，编译器会优化成移位，同时避免有符号右移的语义差异。
 */

#include "config.h"
#include "pid.h"

static s16 idata s_e_prev;        /* 上次误差 */
static s16 idata s_d_filt;        /* 滤波后的误差变化率 */
static s16 idata s_i_acc;         /* 积分累加（限幅） */
static s16 idata s_last_u;        /* 最近一次输出 */
static u8  idata s_kp_q8;
static u8  idata s_kd_q8;

static s16 pid_clamp(s16 v, s16 lo, s16 hi)
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

void pid_reset(void)
{
    s_e_prev = 0;
    s_d_filt = 0;
    s_i_acc = 0;
    s_last_u = 0;
    s_kp_q8 = (u8)PID_KP_Q8;
    s_kd_q8 = (u8)PID_KD_Q8;
}

void pid_set_gain(u8 kp_q8, u8 kd_q8)
{
    s_kp_q8 = kp_q8;
    s_kd_q8 = kd_q8;
}

s16 pid_update(s16 e_q4)
{
    s16 de;
    s16 u;
    s32 acc;

    de = (s16)(e_q4 - s_e_prev);
    s_e_prev = e_q4;

    /* 一阶低通：新值占 1/4，历史占 3/4 */
    s_d_filt = (s16)(((s32)3 * (s32)s_d_filt + (s32)de) / 4L);
    s_d_filt = pid_clamp(s_d_filt, (s16)(-PID_D_FILT_MAX), (s16)PID_D_FILT_MAX);

    /* 积分：KI = 0 时该项恒为 0，保留结构供速度环使用 */
    s_i_acc = (s16)(s_i_acc + (s16)(((s32)PID_KI_Q8 * (s32)e_q4) >> PID_SHIFT));
    s_i_acc = pid_clamp(s_i_acc, (s16)(-PID_KI_ACC_MAX), (s16)PID_KI_ACC_MAX);

    /* 小误差死区，抑制直线上的左右抖动 */
    if ((e_q4 < (s16)PID_E_DEADZONE_Q4) && (e_q4 > (s16)(-PID_E_DEADZONE_Q4)))
    {
        s_last_u = 0;
        return 0;
    }

    acc = (s32)s_kp_q8 * (s32)e_q4 + (s32)s_kd_q8 * (s32)s_d_filt;
    u = (s16)(acc >> PID_SHIFT);
    u = (s16)(u + s_i_acc);
    u = pid_clamp(u, (s16)U_MIN, (s16)U_MAX);

    s_last_u = u;
    return u;
}

s16 pid_last_output(void)
{
    return s_last_u;
}

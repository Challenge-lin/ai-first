#ifndef CONFIG_MOTOR_H
#define CONFIG_MOTOR_H

/*
 * 电机、PWM 与差速参数
 *
 * PWM：Timer0 模式 2 产生 10.017kHz 时基，软件计数 0 .. PWM_PERIOD，
 *      32 级时 PWM 频率 313Hz，分辨率 3.125%。
 * 占空比约定：s8 有符号，范围 -32 .. +32，正负表示转向；
 *      小占空比（<= 3）电机不转，属死区，由速度环补偿。
 */

#include "config_types.h"
#include "config_build.h"
#include "config_timer.h"

/* ===== PWM ===== */
#define PWM_PERIOD            CFG_PWM_LEVELS                 /* 32 */
#define PWM_FREQ_HZ           (TIMER0_PWM_HZ / PWM_PERIOD)   /* 313 */
#define PWM_DUTY_MAX          ((s8)PWM_PERIOD)               /* +32 */
#define PWM_DUTY_MIN          ((s8)(-PWM_PERIOD))            /* -32 */
#define PWM_DUTY_DEADZONE     3U

/* ===== 基础占空比（单位：PWM 级）===== */
#define BASE_FAST             24U   /* 直线，约 0.9 ~ 1.1m/s */
#define BASE_CORNER           14U   /* 弯道，约 0.5 ~ 0.7m/s */
#define ROUND_SPEED           14U   /* 环岛，约 0.4 ~ 0.5m/s */
#define BASE_START            6U    /* 起步第一档 */

/* 弯道降速：base = BASE_FAST - (SPEED_SLOW_K * |e_q4| >> 4)，限幅到 BASE_CORNER */
#define SPEED_SLOW_K          2U

/* ===== 差速 ===== */
#define U_MAX                 18    /* 差速限幅，|u| <= 18 级 */
#define U_MIN                 (-18)
#define DUTY_SLEW_STEP        2U    /* 每 10ms 占空比最大变化 2 级 */

/* ===== 目标速度（mm/s，仅用于速度环和遥测）===== */
#define SPEED_STRAIGHT_MM_S   1000U
#define SPEED_CORNER_MM_S     600U
#define SPEED_ROUND_MM_S      450U

/* ===== 起步斜坡 ===== */
#define START_RAMP_MS         300U  /* 0 -> BASE_FAST 线性爬升 */

#endif /* CONFIG_MOTOR_H */

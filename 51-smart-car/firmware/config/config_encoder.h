#ifndef CONFIG_ENCODER_H
#define CONFIG_ENCODER_H

/*
 * 编码器与里程换算
 *
 * 电机：25GA-370，12V，1:30，霍尔编码器 13 线（A 相单沿计数）
 * 车轮：65mm 橡胶轮，周长 204.2mm
 * 输出轴每转脉冲 = 13 * 30 = 390
 * 每脉冲里程   = 204.2 / 390 ≈ 0.524mm
 *
 * 全部整数运算，不引入浮点：
 *   距离(0.001mm) = pulses * 524
 *   距离(mm)      = pulses * 524 / 1000
 * 溢出核算：两圈约 69000 脉冲，69000 * 524 = 36,156,000 < 2^32，u32 安全。
 */

#include "config_types.h"

/* ===== 机械参数 ===== */
#define ENC_PPR                13U    /* 电机轴每转脉冲数（A 相单沿）*/
#define ENC_GEAR_RATIO         30U    /* 减速比 1:30 */
#define ENC_PULSES_PER_REV     (ENC_PPR * ENC_GEAR_RATIO)   /* 390 */
#define WHEEL_DIAMETER_MM      65U
#define WHEEL_CIRC_MM_X10      2042U  /* 周长 204.2mm，放大 10 倍存整数 */

/* 每脉冲 0.524mm：以 0.001mm 为单位存为 524 */
#define ENC_MM_PER_PULSE_X1000 524U
#define ENC_MM_PER_PULSE_X100  52U

/* ===== 换算宏 ===== */
/* 脉冲 -> 0.001mm，输入应为 u32；单圈 3.4 万脉冲时约 1.8e7，不溢出 */
#define ENC_PULSES_TO_UM(p)    ((u32)(p) * ENC_MM_PER_PULSE_X1000)
/* 脉冲 -> 毫米 */
#define ENC_PULSES_TO_MM(p)    (ENC_PULSES_TO_UM(p) / 1000UL)
/* 脉冲 -> 厘米（停车窗口判定用）*/
#define ENC_PULSES_TO_CM(p)    (ENC_PULSES_TO_UM(p) / 10000UL)

/* ===== 测速 ===== */
#define ENC_SPEED_WINDOW_MS    20U    /* 与 CFG_SPEED_LOOP_HZ 一致 */
#define ENC_MAX_HZ             2000U  /* 1m/s 时单轮约 1910Hz */

/* ===== 方向极性：1 = 取反，装配后按实际转向标定 ===== */
#define ENC_LEFT_INVERT        0U
#define ENC_RIGHT_INVERT       0U

/* ===== 轮子编号，用于 encoder_speed_mm_s(u8 wheel) ===== */
#define ENC_WHEEL_LEFT         0U
#define ENC_WHEEL_RIGHT        1U

/* ===== 赛道里程估算（用于圈数窗口初值，实测后修正）===== */
#define LAP_LENGTH_MM_EST      18000UL   /* 单圈 18m */
#define LAPS_REQUIRED          2U        /* 规则要求连续两圈 */

#endif /* CONFIG_ENCODER_H */

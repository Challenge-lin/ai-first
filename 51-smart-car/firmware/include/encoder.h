#ifndef ENCODER_H
#define ENCODER_H

/*
 * 编码器与里程对外接口（声明）
 *
 * 实现位置：firmware/drivers/motor/ 或 firmware/drivers/encoder/
 * 左轮 A 相 -> INT0，右轮 A 相 -> INT1，B 相普通 I/O 判方向。
 */

#include "config.h"

extern void encoder_init(void);

/* 清零脉冲计数、里程和速度历史 */
extern void encoder_reset(void);

/* 最近一个测速窗口内的脉冲数 */
extern u16 encoder_left_pulses(void);
extern u16 encoder_right_pulses(void);

/* 上电以来的累计脉冲，32 位；两圈约 6.9 万，不会溢出 */
extern u32 encoder_total_pulses(void);

/* 本圈里程脉冲数，用于圈长学习和停车 */
extern u32 encoder_lap_pulses(void);

/* 速度，mm/s，内部按 ENC_MM_PER_PULSE_X1000 换算 */
extern u16 encoder_speed_mm_s(u8 wheel);

/* 里程，mm，等价于 ENC_PULSES_TO_MM(encoder_total_pulses()) */
extern u32 encoder_distance_mm(void);

#endif /* ENCODER_H */

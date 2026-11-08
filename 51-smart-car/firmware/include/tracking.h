#ifndef TRACKING_H
#define TRACKING_H

/*
 * 循迹传感器对外接口（声明）
 *
 * 实现位置：firmware/drivers/tracking/
 * 本文件只做声明，当前工程不含 .c 实现。
 */

#include "config.h"

/* 读 3 片 74HC165 的 24 位原始数据，位映射见 config_tracking.h */
extern u32 tracking_read_raw(void);

/* 单通道状态：ch = 0..15 主传感器，16 = SIDE_L，17 = SIDE_R */
extern u8 tracking_channel(u8 ch);

/* 每 5ms 调用一次：读 165、滤波、更新 count / pos_q4 / e_q4 / 状态 */
extern u8 tracking_scan(void);

/* 有效通道数（主阵列） */
extern u8 tracking_count(void);

/* 位置，Q4：0..240，中心 120 */
extern u8 tracking_position_q4(void);

/* 偏差，Q4：-120..+120 */
extern s8 tracking_error_q4(void);

/* 侧向传感器单次状态，1 = 压线 */
extern u8 tracking_side_left(void);
extern u8 tracking_side_right(void);

/* 当前循迹状态（NORMAL / CROSS / ROUND_*） */
extern trk_state_t tracking_state(void);

/* 十字标志：1 = 当前处于强制直行 */
extern u8 tracking_cross_active(void);

/* 丢线计时，由 5ms 扫描累加，单位 ms */
extern u16 tracking_lost_ms(void);

#endif /* TRACKING_H */

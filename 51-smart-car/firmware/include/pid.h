#ifndef PID_H
#define PID_H

/*
 * 循线 PD 控制器对外接口（声明）
 *
 * 实现位置：firmware/app/ 或 firmware/common/
 * 输入、输出均为 Q4 定点：e_q4 = (pos_q4 - 120)，u 为 PWM 级数。
 * 全部整数运算，乘法使用 s32，不使用浮点。
 */

#include "config.h"

/* 清零历史误差、d_filt、积分累加 */
extern void pid_reset(void);

/* 每 10ms 调用一次，返回差速量 u（-18 .. +18） */
extern s16 pid_update(s16 e_q4);

/* 在线整定：kp_q8 / kd_q8 为 Q8 增益 */
extern void pid_set_gain(u8 kp_q8, u8 kd_q8);

/* 最近一次输出，用于遥测 */
extern s16 pid_last_output(void);

#endif /* PID_H */

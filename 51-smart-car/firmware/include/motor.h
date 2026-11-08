#ifndef MOTOR_H
#define MOTOR_H

/*
 * 电机驱动对外接口（声明）
 *
 * 实现位置：firmware/drivers/motor/
 * 占空比为有符号数：正 = 前进，负 = 后退，0 = 停。
 * 单侧反转用于 90 度折角原地转向。
 */

#include "config.h"

extern void motor_init(void);

/* 设置左右轮占空比，内部限幅到 -32 .. +32 */
extern void motor_set_duty(s8 left, s8 right);

/* 刹车：IN1 = IN2 = 1，保持短接 */
extern void motor_brake(void);

/* 滑行：IN1 = IN2 = 0，高阻 */
extern void motor_coast(void);

/* 待机：STBY 拉低，输出关闭 */
extern void motor_standby(void);

/* 读回当前占空比，用于遥测 */
extern s8 motor_duty_left(void);
extern s8 motor_duty_right(void);

/* 直接设置方向脚，dir 取 MOTOR_DIR_* */
extern void motor_set_dir(u8 wheel, u8 dir);

#endif /* MOTOR_H */

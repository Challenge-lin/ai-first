#ifndef DELAY_H
#define DELAY_H

/*
 * 软件延时对外接口
 *
 * 实现位置：firmware/common/delay.c
 *
 * delay_ms() 基于 g_tick_ms，精度 1ms，调用前必须先 tick_init()。
 * 暂不提供微秒级延时：C51 的最小循环开销约 6~10 个机器周期，
 * 做不出真正的 1us 粒度，74HC165 的时序也不依赖延时。
 */

#include "config.h"

/* 毫秒级延时，依赖 tick_init()，会阻塞等待 */
extern void delay_ms(u16 ms);

#endif /* DELAY_H */

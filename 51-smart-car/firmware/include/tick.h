#ifndef TICK_H
#define TICK_H

/*
 * 1ms 系统节拍（Timer1）对外接口
 *
 * 实现位置：firmware/common/tick.c
 * 资源：Timer1 模式 1，重装值 TIMER1_TICK_RELOAD = 64615
 * 中断：timer1_isr，interrupt 3，低优先级
 *
 * 说明：
 *   - g_tick_ms 在中断里累加，是 u16，约 65.5s 回绕；
 *     计算时间差必须用 tick_elapsed_ms()，不能直接比较大小。
 *   - 5/10/20/100ms 标志由中断置 1，主循环处理完后自行清 0。
 */

#include "config.h"

/* 启动 Timer1 节拍，并使能全局中断 */
extern void tick_init(void);

/* 读取当前毫秒计数（u16，会回绕） */
extern u16 tick_get_ms(void);

/* 自 start_ms 起经过的毫秒数，已处理 u16 回绕 */
extern u16 tick_elapsed_ms(u16 start_ms);

/* 任务节拍标志：中断置 1，主循环消费后清 0 */
extern volatile u8 g_flag_5ms;
extern volatile u8 g_flag_10ms;
extern volatile u8 g_flag_20ms;
extern volatile u8 g_flag_100ms;

#endif /* TICK_H */

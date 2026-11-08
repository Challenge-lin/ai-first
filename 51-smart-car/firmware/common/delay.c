/*
 * delay.c —— 软件延时
 *
 * delay_ms 基于 1ms 节拍，精度 1ms。
 */

#include "config.h"
#include "delay.h"
#include "tick.h"

void delay_ms(u16 ms)
{
    u16 start;

    if (ms == 0U)
    {
        return;
    }

    start = tick_get_ms();

    /* g_tick_ms 是 volatile，循环不会被优化掉；差值自动处理 u16 回绕 */
    while (tick_elapsed_ms(start) < ms)
    {
    }
}

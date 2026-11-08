/*
 * tick.c —— 1ms 系统节拍
 *
 * Timer1 模式 1，16 位定时，重装值 64615（921 个机器周期 ≈ 1ms）。
 * 中断里只做重装、计数和置标志，不放乘法、除法或外设访问。
 */

#include "config.h"
#include "tick.h"

volatile u16 g_tick_ms;
volatile u8  g_flag_5ms;
volatile u8  g_flag_10ms;
volatile u8  g_flag_20ms;
volatile u8  g_flag_100ms;

static u8 s_cnt_5ms;
static u8 s_cnt_10ms;
static u8 s_cnt_20ms;
static u8 s_cnt_100ms;

void tick_init(void)
{
    /* 只改 Timer1 的 4 位，保留 Timer0 的模式 2 设置 */
    TMOD = (u8)((TMOD & 0x0F) | 0x10);

    TH1 = (u8)(TIMER1_TICK_RELOAD >> 8);
    TL1 = (u8)(TIMER1_TICK_RELOAD & 0x00FF);

    g_tick_ms    = 0;
    g_flag_5ms   = 0;
    g_flag_10ms  = 0;
    g_flag_20ms  = 0;
    g_flag_100ms = 0;

    s_cnt_5ms   = 0;
    s_cnt_10ms  = 0;
    s_cnt_20ms  = 0;
    s_cnt_100ms = 0;

    ET1 = 1;
    TR1 = 1;
    EA  = 1;
}

u16 tick_get_ms(void)
{
    u16 now;

    /* u16 读不是原子操作，关中断取一次快照 */
    ET1 = 0;
    now = g_tick_ms;
    ET1 = 1;

    return now;
}

u16 tick_elapsed_ms(u16 start_ms)
{
    u16 now;

    ET1 = 0;
    now = g_tick_ms;
    ET1 = 1;

    return (u16)(now - start_ms);
}

/*
 * Timer1 溢出中断：每 1ms 一次。
 * 先重装再处理标志，保证下一次定时周期不被中断服务时间拉长。
 */
void timer1_isr(void) interrupt 3
{
    TH1 = (u8)(TIMER1_TICK_RELOAD >> 8);
    TL1 = (u8)(TIMER1_TICK_RELOAD & 0x00FF);

    g_tick_ms++;

    if (++s_cnt_5ms >= TICK_SENSOR_MS)
    {
        s_cnt_5ms = 0;
        g_flag_5ms = 1;
    }

    if (++s_cnt_10ms >= TICK_CONTROL_MS)
    {
        s_cnt_10ms = 0;
        g_flag_10ms = 1;
    }

    if (++s_cnt_20ms >= TICK_SPEED_MS)
    {
        s_cnt_20ms = 0;
        g_flag_20ms = 1;
    }

    if (++s_cnt_100ms >= TICK_UI_MS)
    {
        s_cnt_100ms = 0;
        g_flag_100ms = 1;
    }
}

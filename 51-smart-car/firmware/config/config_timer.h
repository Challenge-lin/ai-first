#ifndef CONFIG_TIMER_H
#define CONFIG_TIMER_H

/*
 * 定时器与中断资源分配
 *
 * | 资源   | 模式            | 用途               | 频率        | 优先级 |
 * | Timer0 | 模式 2 自动重装 | 双通道软件 PWM 时基 | 10.017kHz  | 高 PT0 |
 * | Timer1 | 模式 1 16 位    | 1ms 系统节拍        | 1kHz       | 低 PT1 |
 * | Timer2 | 波特率发生器    | 串口                | 9600       | 无中断 |
 * | INT0   | 下降沿          | 左编码器 A 相       | <= 2kHz    | 高 PX0 |
 * | INT1   | 下降沿          | 右编码器 A 相       | <= 2kHz    | 高 PX1 |
 * | UART   | 中断接收        | 调试/遥测           | 9600 8N1   | 低 PS  |
 *
 * 两级优先级：PWM 与编码器为高，Timer1/UART 为低；中断内不做乘除和长任务。
 */

#include "config_types.h"
#include "config_build.h"

/* ===== 时钟 ===== */
#define CFG_FOSC_HZ              11059200UL
#define CFG_MACHINE_CYCLE_NS     1085U    /* 12T，1 机器周期 ≈ 1.085us */
#define CFG_MACHINE_CYCLE_HZ     921600UL /* 11.0592MHz / 12 */

/*
 * Timer0：模式 2，8 位自动重装
 * 周期 = (256 - 164) * 1.085us = 99.83us -> 10017Hz
 * 32 级 PWM -> 10017 / 32 ≈ 313Hz
 */
#define TIMER0_PWM_RELOAD        164U
#define TIMER0_PWM_HZ            10017U
#define TIMER0_PWM_DIV           CFG_PWM_LEVELS

/*
 * Timer1：模式 1，16 位
 * 1000us / 1.085us = 921 个机器周期 -> 65536 - 921 = 64615
 */
#define TIMER1_TICK_RELOAD       64615U
#define TIMER1_TICK_HZ           1000U

/*
 * Timer2：波特率发生器（12T）
 * baud = FOSC / 32 / (65536 - RCAP2)
 *   9600   : 345600 / 9600   = 36   -> RCAP2 = 65500
 *   115200 : 345600 / 115200 = 3    -> RCAP2 = 65533
 */
#define TIMER2_RCAP2_9600        65500U
#define TIMER2_RCAP2_115200      65533U
#define CFG_UART_BAUD            9600U

/* ===== 软件节拍分频（单位 ms）===== */
#define TICK_1MS                 1U
#define TICK_SENSOR_MS           (1000U / CFG_SENSOR_SCAN_HZ)   /* 5  */
#define TICK_CONTROL_MS          (1000U / CFG_CONTROL_HZ)       /* 10 */
#define TICK_SPEED_MS            (1000U / CFG_SPEED_LOOP_HZ)    /* 20 */
#define TICK_UI_MS               (1000U / CFG_UI_HZ)            /* 100 */

/*
 * 以下全局量由 firmware/common/tick.c 的 Timer1 中断实现，
 * 本工程当前只提供声明，实现阶段补齐定义。
 */
extern volatile u16 g_tick_ms;     /* 上电后毫秒计数，溢出周期约 65.5s */
extern volatile u8  g_flag_5ms;
extern volatile u8  g_flag_10ms;
extern volatile u8  g_flag_20ms;
extern volatile u8  g_flag_100ms;

#endif /* CONFIG_TIMER_H */

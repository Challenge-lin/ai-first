/*
 * main.c —— 工程入口
 *
 * 初始化顺序：
 *   tick_init()   先建立 1ms 时基，后续 delay/测速/状态机都依赖它；
 *   motor_init()  只配置引脚与 PWM，驱动保持关闭；
 *   encoder_init() 打开 INT0/INT1；
 *   tracking_scan() 先读一次 165，建立滤波与位置基线；
 *   app_init()    状态机进入 IDLE，电机待机。
 *
 * 主循环按 config_build.h 的任务频率调度，全部由节拍标志驱动：
 *   5ms   循迹扫描（165 读取 + 滤波 + 元素状态机）
 *   10ms  按键、系统状态机、PD 输出
 *   20ms  编码器测速、速度环
 *   100ms 保护与指示灯
 */

#include "config.h"
#include "tick.h"
#include "motor.h"
#include "encoder.h"
#include "tracking.h"
#include "app.h"

void main(void)
{
    tick_init();
    motor_init();
    encoder_init();
    tracking_scan();
    app_init();

    while (1)
    {
        if (g_flag_5ms != 0U)
        {
            g_flag_5ms = 0U;
            tracking_scan();
        }

        if (g_flag_10ms != 0U)
        {
            g_flag_10ms = 0U;
            app_control();
        }

        if (g_flag_20ms != 0U)
        {
            g_flag_20ms = 0U;
            app_speed();
        }

        if (g_flag_100ms != 0U)
        {
            g_flag_100ms = 0U;
            app_monitor();
        }
    }
}

#ifndef CONFIG_H
#define CONFIG_H

/*
 * 配置总入口（Keil C51）
 *
 * 包含顺序固定如下，模块内只写 #include "config.h"，不要单独包含下层文件：
 *   1. config_types.h     数据类型别名（最先，其他头文件依赖 u8/s16/u32）
 *   2. config_build.h     编译开关与频率配置
 *   3. config_pin.h       引脚宏 / sbit 位定义
 *   4. config_timer.h     定时器重装值、节拍分频
 *   5. config_motor.h     PWM、基础占空比、差速限幅
 *   6. config_tracking.h  18 路传感器、74HC165 位映射、循迹阈值
 *   7. config_encoder.h   编码器与里程换算
 *   8. config_control.h   PID 参数、状态机枚举、保护/停车阈值
 *
 * 约束：本目录只放宏、类型、位声明、常量与 extern 声明，不放函数实现。
 */

#include "config_types.h"
#include "config_build.h"
#include "config_pin.h"
#include "config_timer.h"
#include "config_motor.h"
#include "config_tracking.h"
#include "config_encoder.h"
#include "config_control.h"

#endif /* CONFIG_H */

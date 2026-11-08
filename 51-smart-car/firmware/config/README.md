# 配置头文件说明（firmware/config）

本目录集中存放引脚、时钟、控制参数与编译开关，是固件唯一的常量来源。
当前为**头文件骨架**：只含宏、类型、`sbit` 位定义、枚举与 `extern` 声明，**不含任何 `.c` 实现**。

## 1. 文件清单

| 文件 | 职责 | 主要内容 |
| --- | --- | --- |
| `config.h` | 配置总入口 | 按固定顺序包含下列 7 个文件；模块只包含它 |
| `config_types.h` | 数据类型统一别名 | `u8 / s8 / u16 / s16 / u32 / s32`、`CFG_TRUE/FALSE`、取绝对值与限幅宏 |
| `config_build.h` | 编译开关与任务频率 | 传感器 200Hz、控制 100Hz、速度环 50Hz、UI 10Hz；OLED / 蓝牙 / 速度环 / 环岛 / 欠压开关；PWM 级数 |
| `config_pin.h` | 引脚宏与位定义 | 40 脚分配，与 `hardware/pinmap/pinmap.md` 逐脚一致；TB6612 方向真值常量 |
| `config_timer.h` | 定时器与节拍 | Timer0 重装值 164（10.017kHz）、Timer1 重装值 64615（1ms）、Timer2 波特率重装值、节拍分频、tick 的 `extern` 声明 |
| `config_motor.h` | 电机与 PWM | `PWM_PERIOD=32`、313Hz、基础占空比、差速限幅 ±18、弯道降速系数、起步斜坡 |
| `config_tracking.h` | 循迹传感器 | 18 路几何参数、74HC165 的 24 位位映射与掩码、位置 Q4 常量、十字/环岛/发车线阈值 |
| `config_encoder.h` | 编码器与里程 | 390 脉冲/转、0.524mm/脉冲、里程换算宏、单圈里程估算 |
| `config_control.h` | 控制与保护 | PID 增益（Q8）、状态机/故障枚举、起跑、圈数与停车、丢线 800ms、堵转 500ms、低压阈值 |

`firmware/include/` 下为模块对外接口声明，与配置目录分工如下：

| 文件 | 职责 |
| --- | --- |
| `tracking.h` | 74HC165 读取（`tracking_read_raw`）、5ms 扫描、位置/偏差/侧向/状态查询 |
| `motor.h` | 占空比设置、刹车、滑行、待机、方向脚控制 |
| `encoder.h` | 脉冲计数、里程、速度查询 |
| `pid.h` | PD 更新、增益在线整定、输出查询 |

## 2. 包含顺序

`config.h` 内部顺序不可随意调整，存在依赖关系：

```text
config_types.h      类型别名，其他文件的基础
      |
config_build.h      频率/开关，config_timer.h 依赖
      |
config_pin.h        <reg52.h> + sbit，独立
      |
config_timer.h      依赖 config_types.h、config_build.h
      |
config_motor.h      依赖 config_timer.h（TIMER0_PWM_HZ）
      |
config_tracking.h   依赖 config_types.h
      |
config_encoder.h    依赖 config_types.h
      |
config_control.h    依赖 config_types.h、config_encoder.h（LAPS_REQUIRED）
```

使用方式：

```c
#include "config.h"
```

不要单独包含 `config_motor.h` 等下层文件，也不要在业务代码里重复写常量。

## 3. Keil 工程设置要求

| 项目 | 设置 |
| --- | --- |
| 器件 | STC89C52RC（或同内核 AT89C52，8KB Flash / 512B RAM） |
| 编译器 | Keil C51，不使用 C99 模式 |
| Memory Model | Small（变量默认 `data`） |
| 代码模式 | 不使用外部程序存储器，EA 上拉 |
| 头文件路径 | 添加 `firmware\config` 与 `firmware\include` |
| 优化 | 默认 8 级即可；调试时降为 0 级，避免变量被优化掉 |
| 浮点库 | 不链接，代码中禁止 `float` / `double` / `printf` |

## 4. 编码约定

1. 只使用 `/* */` 注释，不用 `//`，避免旧版 C51 预处理差异。
2. 不引入 C99 特性：不使用 `stdint.h`、行内声明、变长数组、`inline`。
3. 常量一律带无符号后缀：16 位用 `U`，32 位用 `UL`；涉及乘法的先转 `u32`。
4. 里程换算只用整数：`pulses * 524 / 1000`，不使用 0.524 浮点字面量。
5. 增益用定点放大：位置 Q4（×16）、PID 增益 Q8（×256），注释中同时写实际值。
6. 引脚变更流程：先改 `hardware/pinmap/pinmap.md`，再改 `config_pin.h`，最后在 `docs/notes/` 记录。
7. `sbit` 定义可被多个 `.c` 包含，不会产生重复符号；不要改写成宏或变量。

## 5. 尚无实现的符号

骨架阶段下列符号只有声明，进入实现阶段后需在对应 `.c` 中定义，否则链接失败：

| 符号 | 建议实现位置 |
| --- | --- |
| `g_tick_ms`、`g_flag_5ms/10ms/20ms/100ms` | `firmware/common/tick.c`（Timer1 中断） |
| `tracking_*` | `firmware/drivers/tracking/` |
| `motor_*` | `firmware/drivers/motor/` |
| `encoder_*` | `firmware/drivers/motor/` 或 `firmware/drivers/encoder/` |
| `pid_*` | `firmware/app/` 或 `firmware/common/` |

## 6. 关键参数速查

| 参数 | 值 | 位置 |
| --- | --- | --- |
| 晶振 / 机器周期 | 11.0592MHz / 1.085µs | `config_timer.h` |
| PWM 时基 | 10.017kHz，重装 164 | `config_timer.h` |
| PWM | 32 级，313Hz | `config_motor.h` |
| 系统节拍 | 1ms，重装 64615 | `config_timer.h` |
| 主传感器 | 16 路，10mm 间距，150mm | `config_tracking.h` |
| 侧向传感器 | 2 路，±82mm，外偏 30° | `config_tracking.h` |
| 位置分辨率 | Q4，中心 120 | `config_tracking.h` |
| PD 增益 | Kp=31/256，Kd=115/256 | `config_control.h` |
| 差速限幅 | ±18 级 | `config_control.h` |
| 每脉冲里程 | 0.524mm（524 / 1000mm） | `config_encoder.h` |
| 丢线停车 | 800ms | `config_control.h` |
| 堵转停车 | 500ms，占空比 ≥10 | `config_control.h` |
| 停车目标 | 过线后 250mm，窗口 500mm | `config_control.h` |

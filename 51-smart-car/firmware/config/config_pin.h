#ifndef CONFIG_PIN_H
#define CONFIG_PIN_H

/*
 * 引脚分配（与 hardware/pinmap/pinmap.md 逐脚对应）
 *
 * 规则：本文件是固件侧唯一定义处。改引脚必须先改 pinmap.md，再改本文件，
 *      并在 docs/notes/ 留变更记录。
 *
 * 说明：
 *   - 使用 sbit 定义位寻址引脚，可直接读写；sbit 不分配存储空间，
 *     被多个 .c 包含不会产生重复定义。
 *   - P0 作输入必须外接 10k 上拉（硬件已加），固件不再设置。
 *   - 未列出的引脚为备用：P0.2 / P0.5 / P0.6 / P0.7 / P3.5 / P3.6 / P3.7。
 */

#include <reg52.h>

/* ===== 传感器板 74HC165（脚 1、2、3）===== */
sbit PIN_TRK_PL  = P1^0;   /* 脚 1  P1.0  输出  并行装载，低脉冲有效 */
sbit PIN_TRK_CP  = P1^1;   /* 脚 2  P1.1  输出  移位时钟，上升沿移出 */
sbit PIN_TRK_Q7  = P1^2;   /* 脚 3  P1.2  输入  QH 串行数据，推挽 */
#define PIN_TRK_CHAIN_LEN   24U   /* 3 片 74HC165 级联 */

/* ===== 显示与指示（脚 4、5、6、7）===== */
sbit PIN_OLED_SCL = P1^3;  /* 脚 4  P1.3  输出  软件 I2C，4.7k 上拉 */
sbit PIN_OLED_SDA = P1^4;  /* 脚 5  P1.4  双向  软件 I2C，4.7k 上拉 */
sbit PIN_BUZZER   = P1^5;  /* 脚 6  P1.5  输出  S8050 基极，高电平鸣叫 */
sbit PIN_LED_RUN  = P1^6;  /* 脚 7  P1.6  输出  运行指示，高电平点亮 */
sbit PIN_KEY_MODE = P1^7;  /* 脚 8  P1.7  输入  低有效，10k 上拉 + 100nF */

/* ===== 串口（脚 10、11）===== */
sbit PIN_RXD      = P3^0;  /* 脚 10 P3.0  输入  HC-05 TX / ISP，J8 切换 */
sbit PIN_TXD      = P3^1;  /* 脚 11 P3.1  输出  HC-05 RX / ISP，J8 切换 */

/* ===== 编码器（脚 12、13、38、39）===== */
sbit PIN_ENC_L_A  = P3^2;  /* 脚 12 P3.2/INT0  左 A 相，下降沿中断 */
sbit PIN_ENC_R_A  = P3^3;  /* 脚 13 P3.3/INT1  右 A 相，下降沿中断 */
sbit PIN_ENC_R_B  = P0^1;  /* 脚 38 P0.1       右 B 相，普通输入 */
sbit PIN_ENC_L_B  = P0^0;  /* 脚 39 P0.0       左 B 相，普通输入 */

/* ===== 按键（脚 14、36）===== */
sbit PIN_KEY_START = P3^4; /* 脚 14 P3.4  输入  低有效，长按 500ms 起跑 */
sbit PIN_KEY_STOP  = P0^3; /* 脚 36 P0.3  输入  低有效，10k 上拉 */

/* ===== 电机驱动 TB6612（脚 21 ~ 27）===== */
sbit PIN_PWMA  = P2^0;     /* 脚 21 P2.0  输出  左轮 PWM，Timer0 软件 PWM */
sbit PIN_PWMB  = P2^1;     /* 脚 22 P2.1  输出  右轮 PWM */
sbit PIN_AIN1  = P2^2;     /* 脚 23 P2.2  输出  左轮方向 */
sbit PIN_AIN2  = P2^3;     /* 脚 24 P2.3  输出  左轮方向 */
sbit PIN_BIN1  = P2^4;     /* 脚 25 P2.4  输出  右轮方向 */
sbit PIN_BIN2  = P2^5;     /* 脚 26 P2.5  输出  右轮方向 */
sbit PIN_STBY  = P2^6;     /* 脚 27 P2.6  输出  高电平使能，10k 下拉 */
sbit PIN_LED_ERR = P2^7;   /* 脚 28 P2.7  输出  故障指示，高电平点亮 */

/* ===== 电源检测（脚 35）===== */
sbit PIN_VBAT_LOW = P0^4;  /* 脚 35 P0.4  输入  低电平 = 欠压（主板 LM339）*/

/*
 * TB6612 方向真值（STBY = 1 使能）：
 *   IN1=1, IN2=0  正转
 *   IN1=0, IN2=1  反转
 *   IN1=1, IN2=1  刹车（电机两端短接）
 *   IN1=0, IN2=0  滑行（高阻）
 */
#define MOTOR_DIR_FORWARD   0U
#define MOTOR_DIR_REVERSE   1U
#define MOTOR_DIR_BRAKE     2U
#define MOTOR_DIR_COAST     3U

#endif /* CONFIG_PIN_H */

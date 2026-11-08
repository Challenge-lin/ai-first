#ifndef CONFIG_BUILD_H
#define CONFIG_BUILD_H

/*
 * 编译开关与任务频率
 *
 * 只允许取 0 / 1 的开关用 #if 判断，板级差异全部集中在这里，
 * 驱动层和应用层不要出现 #ifdef 板级分支。
 */

#define CFG_BUILD_VERSION_MAJOR   0U
#define CFG_BUILD_VERSION_MINOR   1U

/* ===== 任务频率（Hz）=====
 * 这几个值会参与 #if 整除检查，故写成不带后缀的普通整数常量（均 < 32767）。
 */
#define CFG_SENSOR_SCAN_HZ        200    /* 5ms 扫描 24 位传感器 */
#define CFG_CONTROL_HZ            100    /* 10ms 循线 PD */
#define CFG_SPEED_LOOP_HZ         50     /* 20ms 速度环 */
#define CFG_UI_HZ                 10     /* 100ms 显示/遥测/保护检查 */

/* ===== 功能开关 ===== */
#define CFG_DEBUG_UART            1      /* 串口遥测，比赛可关 */
#define CFG_USE_OLED              1      /* OLED 状态显示 */
#define CFG_USE_BLUETOOTH         0      /* 蓝牙仅调试，比赛必须断开 */
#define CFG_ENABLE_SPEED_LOOP     0      /* 速度环默认关闭，先整定 PD */
#define CFG_ENABLE_CROSS_STATE    1      /* 十字强制直行 */
#define CFG_ENABLE_ROUND_STATE    1      /* 环岛状态机 */
#define CFG_ENABLE_LOW_BATTERY    1      /* 欠压降速/停车 */
#define CFG_USE_WATCHDOG          0      /* 不依赖 STC89C52RC 内部 WDT */
#define CFG_SAFE_MODE             1      /* 安全模式：故障必须停车 */

/* ===== PWM 级数 ===== */
#define CFG_PWM_LEVELS            32     /* 32 级 -> 313Hz；16 级 -> 625Hz */

#if (CFG_PWM_LEVELS != 16) && (CFG_PWM_LEVELS != 32)
#error "CFG_PWM_LEVELS must be 16 or 32"
#endif

/* ===== 频率一致性检查（整数分频必须整除）===== */
#if (1000 % CFG_SENSOR_SCAN_HZ) != 0
#error "CFG_SENSOR_SCAN_HZ must divide 1000"
#endif

#if (1000 % CFG_CONTROL_HZ) != 0
#error "CFG_CONTROL_HZ must divide 1000"
#endif

#if (1000 % CFG_SPEED_LOOP_HZ) != 0
#error "CFG_SPEED_LOOP_HZ must divide 1000"
#endif

#endif /* CONFIG_BUILD_H */

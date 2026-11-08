#ifndef APP_H
#define APP_H

/*
 * 整车应用层对外接口
 *
 * 调度约定（main 按 config_build.h 的任务频率调用）：
 *   app_control()  10ms  按键、系统状态机、循迹 PD、差速输出
 *   app_speed()    20ms  编码器测速、速度环修正
 *   app_monitor()  100ms 堵转/欠压/STOP 键保护、指示灯
 *   5ms 的 tracking_scan() 由 main 直接调用。
 */

#include "config.h"

extern void app_init(void);

extern void app_control(void);
extern void app_speed(void);
extern void app_monitor(void);

/* ===== 遥测 ===== */
extern sys_state_t app_state(void);
extern fault_code_t app_fault(void);
extern u8 app_crossings(void);   /* 已过发车线次数 */
extern u8 app_lap(void);         /* 已完成圈数 0..2 */
extern u32 app_run_ms(void);     /* RUN 状态累计时间 */
extern u32 app_distance_mm(void);/* 上电以来里程 */

#endif /* APP_H */

#ifndef CONFIG_CONTROL_H
#define CONFIG_CONTROL_H

/*
 * 控制参数、状态机与保护阈值
 *
 * 定点约定：
 *   位置/误差用 Q4：数值 = 实际值 * 16，e_q4 范围 -120 .. +120
 *   PID 增益用 Q8：数值 = 实际值 * 256
 *   输出 u 为 PWM 级数（有符号，-18 .. +18），不再放大
 *
 * PD 计算（每 10ms）：
 *   de_q4  = e_q4 - e_prev_q4
 *   d_filt = (3 * d_filt + de_q4) / 4
 *   u      = (PID_KP_Q8 * e_q4 + PID_KD_Q8 * d_filt) >> 8
 *   乘法用 s32，结果再限幅
 */

#include "config_types.h"
#include "config_encoder.h"   /* 使用 LAPS_REQUIRED */

/* ===== PID 参数 =====
 * 输出限幅与斜率限制统一放在 config_motor.h（U_MAX / U_MIN / DUTY_SLEW_STEP），
 * 本文件不重复定义，避免两处参数不一致。
 */
#define PID_SHIFT             8U      /* Q8 */
#define PID_KP_Q8             31U     /* Kp = 31/256 ≈ 0.121 */
#define PID_KD_Q8             115U    /* Kd = 115/256 ≈ 0.449 */
#define PID_KI_Q8             0U      /* 默认关闭积分 */
#define PID_KI_ACC_MAX        4      /* 积分累加限幅 ±4 */
#define PID_D_FILT_MAX        120     /* d_filt 限幅 ±120 */
#define PID_E_DEADZONE_Q4     4       /* |e_q4| < 4 时 u = 0 */
#define PID_D_FILT_DIV_SHIFT  2U      /* (3*d + de)/4 */

/* ===== 速度环（CFG_ENABLE_SPEED_LOOP = 1 时生效）=====
 * 与 PID 相同的 Q8 定点：corr = (Kp*err + Ki*i) >> 8，err 单位 mm/s。
 * 输出只作为公共基础占空比的修正量，不直接改左右差速。
 */
#define SPEED_KP_Q8           64U     /* Kp = 64/256 = 0.25 */
#define SPEED_KI_Q8           8U      /* Ki = 8/256 = 0.031 */
#define SPEED_I_MAX           300     /* 积分累加限幅 ±300 */
#define SPEED_CORR_MAX        6       /* 速度环修正量限幅 ±6 级 */
#define STALL_SPEED_MM_S      50U     /* 低于此速度且占空比足够，判堵转 */

/* ===== 按键与电源滤波 ===== */
#define KEY_STOP_HOLD_MS      200U    /* STOP 键确认时间，避免误停 */
#define VBAT_FILTER_MS        200U    /* 欠压标志连续有效时间 */

/* ===== 系统状态机 ===== */
typedef enum {
    SYS_STATE_IDLE = 0,     /* 电机停，等待起跑 */
    SYS_STATE_ARMED,        /* 倒计时中 */
    SYS_STATE_RUN,          /* 循迹运行 */
    SYS_STATE_FINISH,       /* 两圈完成，减速停车 */
    SYS_STATE_FAULT         /* 故障停机 */
} sys_state_t;

typedef enum {
    TRK_STATE_NORMAL = 0,
    TRK_STATE_CROSS,
    TRK_STATE_ROUND_ENTER,
    TRK_STATE_ROUND_IN,
    TRK_STATE_ROUND_EXIT
} trk_state_t;

typedef enum {
    FAULT_NONE = 0,
    FAULT_LOST_LINE,
    FAULT_STALL,
    FAULT_TIMEOUT,
    FAULT_LOW_BATTERY,
    FAULT_OVERRUN
} fault_code_t;

/* ===== 起跑 ===== */
#define ARM_HOLD_MS           500U    /* START 长按时间 */
#define ARM_COUNTDOWN_MS      1000U   /* 倒计时 */
#define BEEP_ARM_COUNT        3U      /* 起跑提示音次数 */

/* ===== 圈数与发车线 ===== */
/*
 * 规则要求连续两圈，完整流程共 3 次过发车线：
 *   起跑过线 -> 第 1 圈完成 -> 第 2 圈完成（进入 FINISH）
 * START_MODE = 0：车停在发车线后方，首次过线算起跑；
 * START_MODE = 1：车停在线上/线后，首次过线算第 1 圈。
 */
#define START_MODE            0U
#define CROSSINGS_REQUIRED    (LAPS_REQUIRED + 1U)   /* 3 */
#define START_CROSS_MAX_MM    1500U   /* 起跑过线的里程窗口 */
#define LAP_WINDOW_PCT        15U     /* 圈长窗口 ±15% */

/* ===== 停车 ===== */
#define STOP_TARGET_MM        250U    /* 过线后目标停车距离 */
#define STOP_WINDOW_MM        500U    /* 规则窗口 0 ~ 50cm */
#define STOP_FORCE_MM         450U    /* 超过此距离强制刹车 */
#define STOP_BRAKE_MS         100U    /* 短接电机刹车时间 */
#define STOP_HOLD_MS          2000U   /* 停稳保持 */

/* ===== 异常保护 ===== */
#define LOST_HOLD_MS          150U    /* 丢线保持：沿用上次舵量，降速 50% */
#define LOST_STOP_MS          800U    /* 丢线停车（规则 1s 判无效）*/
#define LOST_SPEED_PCT        50U     /* 丢线保持期的速度百分比 */
#define STALL_DUTY_MIN        10U     /* 判定堵转的最小占空比 */
#define STALL_MS              500U    /* 堵转判定时间 */
#define RUN_TIMEOUT_MS        120000UL /* 单次运行超时 120s */
#define STOP_FALLBACK_EXTRA_MM 2000U  /* 兜底：2.2 圈 + 2m 仍未识别发车线则强制停车 */

/* ===== 电源 =====
 * 硬件用分压 + 主板 LM339 比较器产生数字欠压标志（P0.4 低电平有效），
 * 阈值由比较器电位器设定；下列电压值仅作为整定参考，固件不直接测电压。
 */
#define VBAT_WARN_MV          10500U  /* 10.5V 降速提示 */
#define VBAT_STOP_MV          9600U   /* 9.6V 停车 */

#endif /* CONFIG_CONTROL_H */

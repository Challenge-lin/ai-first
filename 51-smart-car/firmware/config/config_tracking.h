#ifndef CONFIG_TRACKING_H
#define CONFIG_TRACKING_H

/*
 * 循迹传感器与 74HC165 位映射
 *
 * 阵列：16 路主传感器（10mm 间距，总宽 150mm）+ 左右各 1 路侧向（±82mm，外偏 30°）。
 * 安装：中心到前轴前瞻 120mm，距地 7mm（可调 4 ~ 10mm）。
 * 读取：3 片 74HC165 级联，PL/CP/Q7 三线，移出顺序 A7..A0 -> B7..B0 -> C7..C0。
 * 位映射见 hardware/pinmap/pinmap.md 4.2，本文件与之一一对应。
 */

#include "config_types.h"

/* ===== 24 位原始数据的位定义 ===== */
#define TRK_BIT_S0            0
#define TRK_BIT_S1            1
#define TRK_BIT_S2            2
#define TRK_BIT_S3            3
#define TRK_BIT_S4            4
#define TRK_BIT_S5            5
#define TRK_BIT_S6            6
#define TRK_BIT_S7            7
#define TRK_BIT_S8            8
#define TRK_BIT_S9            9
#define TRK_BIT_S10           10
#define TRK_BIT_S11           11
#define TRK_BIT_S12           12
#define TRK_BIT_S13           13
#define TRK_BIT_S14           14
#define TRK_BIT_S15           15
#define TRK_BIT_SIDE_L        16
#define TRK_BIT_SIDE_R        17
/* bit18 ~ bit23 备用，必须为 0 */

#define TRK_BIT_MASK(n)       (1UL << (n))

#define TRK_MASK_MAIN         0x0000FFFFUL   /* bit0  ~ bit15 */
#define TRK_MASK_SIDE         0x00030000UL   /* bit16 / bit17 */
#define TRK_MASK_SPARE        0x00FC0000UL   /* bit18 ~ bit23 */

/* S0 为最左侧传感器，S15 为最右侧 */
#define TRK_CH_LEFT           0U
#define TRK_CH_RIGHT          15U

/* ===== 几何参数 ===== */
#define TRK_MAIN_CH           16U    /* 主通道数 */
#define TRK_SIDE_CH           2U     /* 侧向通道数 */
#define TRK_TOTAL_CH          18U    /* 有效通道总数 */
#define TRK_SPACING_MM        10U    /* 通道间距 */
#define TRK_SPAN_MM           150U   /* 主阵列覆盖宽度 */
#define TRK_SIDE_OFFSET_MM    82U    /* 侧向传感器横向偏置 */
#define TRK_SIDE_ANGLE_DEG    30U    /* 侧向外偏角 */
#define TRK_FORWARD_MM        120U   /* 前瞻：排中心到前轴 */
#define TRK_HEIGHT_MM         7U     /* 距地高度 */

/* ===== 位置计算（Q4 定点）=====
 * sum_i  = Σ i * w_i           i = 0..15
 * pos_q4 = (sum_i * 16) / count
 * e_q4   = pos_q4 - 120        中心 7.5 * 16 = 120
 * 范围：pos_q4 0..240，e_q4 -120..+120
 */
#define TRK_POS_SHIFT         4U
#define TRK_POS_SCALE         (1U << TRK_POS_SHIFT)   /* 16 */
#define TRK_CENTER_Q4         120U
#define TRK_ERR_MAX_Q4        120

/* ===== 判定阈值 ===== */
#define TRK_CROSS_COUNT       14U   /* 有效通道 >= 14 视为十字候选 */
#define TRK_CROSS_EXIT_COUNT  8U    /* 回落到 <= 8 退出十字 */
#define TRK_CROSS_HOLD_MS     200U  /* 十字强制直行时间 */
#define TRK_VALID_MIN         1U    /* count >= 1 视为看到线 */
#define TRK_FILTER_CONFIRM    3U    /* 连续 3 次一致才确认 */
#define TRK_EDGE_IGNORE       1U    /* 最外侧 1 路单独触发视为邻道/噪点 */

/* 环岛：单侧分支 + 侧向触发 + 持续时间 */
#define TRK_ROUND_WIDTH_DELTA 4U    /* 单侧有效通道突增阈值 */
#define TRK_ROUND_HOLD_MS     30U   /* 分支持续确认时间 */
#define TRK_ROUND_COOLDOWN_MS 300U  /* 退出后冷却 */
#define TRK_SIDE_MIN_HOLD_MS  20U   /* 侧向触发确认 */

/*
 * 环岛绕行方向：1 = 顺时针（先向右打舵），0 = 逆时针。
 * 规则未明确绕行方向，赛前按实际赛道用按键/参数修改。
 */
#define TRK_ROUND_DIR_CW      1U
#define TRK_ROUND_DIR         TRK_ROUND_DIR_CW

/* 发车线 / 十字区分：全黑模式的最小有效通道数与最短持续时间 */
#define TRK_STARTLINE_COUNT   14U
#define TRK_STARTLINE_MIN_MS  15U

#endif /* CONFIG_TRACKING_H */

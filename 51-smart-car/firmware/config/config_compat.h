#ifndef CONFIG_COMPAT_H
#define CONFIG_COMPAT_H

/*
 * 编译器兼容层（Keil C51 ↔ SDCC）
 *
 * 用法：所有模块只 include "config.h"，不要直接 include <reg52.h>/<intrins.h>。
 * 本文件在 config.h 中被最先包含。
 *
 * Keil C51 与 SDCC(mcs51) 的主要差异：
 *   1. 寄存器头：Keil <reg52.h>  /  SDCC <8052.h>
 *   2. 位定义  ：Keil sbit X = P1^0;  /  SDCC __sbit __at (0x90) X;
 *      → 统一用 SBIT(name, keil_expr, bit_addr) 宏
 *   3. 存储类  ：Keil idata/xdata/pdata/bdata/code/bit
 *               SDCC  __idata/__xdata/__pdata/__bdata/__code/__bit
 *      → 在 SDCC 下用宏把 Keil 写法映射过去，Keil 下不动
 *   4. 中断    ：Keil interrupt n  /  SDCC __interrupt (n)
 *      → 统一用 INT(n) 宏
 *   5. 空操作  ：Keil <intrins.h> 的 _nop_()  /  SDCC 内联汇编
 *      → 统一用 _nop_()，SDCC 下重定义
 */

#if defined(__SDCC)
  /* ---------- SDCC ---------- */
  #include <8052.h>

  /* 位寻址引脚：__sbit __at (位地址) 名字 */
  #define SBIT(name, keil_expr, bit_addr)  __sbit __at (bit_addr) name

  /* 存储类关键字映射（仅 SDCC 需要加双下划线） */
  #define idata   __idata
  #define xdata   __xdata
  #define pdata   __pdata
  #define bdata   __bdata
  #define code    __code
  #define bit     __bit

  /* 中断：SDCC 要求中断号带括号 */
  #define INT(n)  __interrupt (n)

  /* 空操作：用内联汇编替代 <intrins.h> */
  #define _nop_()  __asm nop __endasm

#else
  /* ---------- Keil C51 ---------- */
  #include <reg52.h>
  #include <intrins.h>

  #define SBIT(name, keil_expr, bit_addr)  sbit name = keil_expr
  #define INT(n)  interrupt n
  /* _nop_() 由 <intrins.h> 提供，无需重定义 */

#endif

#endif /* CONFIG_COMPAT_H */

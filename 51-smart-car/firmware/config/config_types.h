#ifndef CONFIG_TYPES_H
#define CONFIG_TYPES_H

/*
 * 统一数据类型别名
 *
 * Keil C51 的基本类型宽度：char = 8 位，int = 16 位，long = 32 位。
 * 不使用 C99 的 <stdint.h> / uint8_t，也不使用 float/double。
 * 所有模块对外接口只允许使用下列别名。
 */

typedef unsigned char  u8;    /* 0 .. 255 */
typedef signed   char  s8;    /* -128 .. 127 */
typedef unsigned int   u16;   /* 0 .. 65535 */
typedef signed   int   s16;   /* -32768 .. 32767 */
typedef unsigned long  u32;   /* 0 .. 4294967295 */
typedef signed   long  s32;

#define CFG_TRUE    1
#define CFG_FALSE   0

/*
 * 取绝对值。参数会被求值两次，调用时不要传入带副作用的表达式。
 */
#define CFG_ABS_S16(v)  ((s16)(((v) < 0) ? (-(v)) : (v)))

/*
 * 16 位有符号限幅。同样只用于简单变量。
 */
#define CFG_CLAMP_S16(v, lo, hi) \
    ((s16)(((v) < (lo)) ? (lo) : (((v) > (hi)) ? (hi) : (v))))

#endif /* CONFIG_TYPES_H */

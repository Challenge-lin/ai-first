/*
 * motor.c —— TB6612 双电机驱动 + Timer0 软件 PWM
 *
 * 资源：
 *   Timer0 模式 2 自动重装，重装值 164 -> 10.017kHz 时基；
 *   32 级软件 PWM -> 313Hz，分辨率 3.125%；
 *   中断 timer0_pwm_isr，interrupt 1，高优先级（PT0 = 1）。
 *
 * 约定：
 *   占空比 s8 有符号，-32..+32，正 = 前进，负 = 后退，0 = 停（滑行）。
 *   中断内只做计数与两路引脚输出，不做乘除、不调用函数。
 *
 * STBY 语义：
 *   motor_init() 后驱动处于关闭（STBY = 0）。
 *   任何会输出 PWM 的接口（set_duty / brake / coast）都会先拉高 STBY。
 *   motor_standby() 拉低 STBY 并清占空比，用于故障或等待起跑。
 */

#include "config.h"
#include "motor.h"

#define WHEEL_LEFT    0U
#define WHEEL_RIGHT   1U

/* PWM 占空比（当前周期内的高电平计数），0..PWM_PERIOD */
static volatile u8 s_pwm_left;
static volatile u8 s_pwm_right;

/* 自由运行计数器，0..PWM_PERIOD-1，只在 Timer0 中断内修改 */
static u8 s_pwm_cnt;

/* 对外读回的占空比 */
static s8 s_duty_left;
static s8 s_duty_right;

/* 限幅到 -32..+32 */
static s8 motor_clamp(s8 duty)
{
    if (duty > PWM_DUTY_MAX)
    {
        return PWM_DUTY_MAX;
    }
    if (duty < PWM_DUTY_MIN)
    {
        return PWM_DUTY_MIN;
    }
    return duty;
}

/* 取绝对值，返回 0..32 */
static u8 motor_magnitude(s8 duty)
{
    if (duty < 0)
    {
        return (u8)(-duty);
    }
    return (u8)duty;
}

/* Timer0 中断：10.017kHz，输出两路软件 PWM */
static void timer0_pwm_isr(void) interrupt 1
{
    if (s_pwm_cnt >= (u8)PWM_PERIOD)
    {
        s_pwm_cnt = 0;
    }

    PIN_PWMA = (s_pwm_cnt < s_pwm_left) ? 1 : 0;
    PIN_PWMB = (s_pwm_cnt < s_pwm_right) ? 1 : 0;

    s_pwm_cnt++;
}

void motor_init(void)
{
    /* 先关闭驱动，避免上电瞬间电机抽动 */
    PIN_STBY = 0;
    PIN_PWMA = 0;
    PIN_PWMB = 0;
    PIN_AIN1 = 0;
    PIN_AIN2 = 0;
    PIN_BIN1 = 0;
    PIN_BIN2 = 0;

    s_pwm_left = 0;
    s_pwm_right = 0;
    s_pwm_cnt = 0;
    s_duty_left = 0;
    s_duty_right = 0;

    /* Timer0：模式 2 自动重装，保留 Timer1 的模式位 */
    TMOD = (u8)((TMOD & 0xF0) | 0x02);
    TH0 = (u8)TIMER0_PWM_RELOAD;
    TL0 = (u8)TIMER0_PWM_RELOAD;

    PT0 = 1;    /* 高优先级，保证 PWM 抖动小 */
    ET0 = 1;
    TR0 = 1;
}

void motor_set_dir(u8 wheel, u8 dir)
{
    if (wheel == WHEEL_LEFT)
    {
        switch (dir)
        {
        case MOTOR_DIR_FORWARD:
            PIN_AIN1 = 1;
            PIN_AIN2 = 0;
            break;
        case MOTOR_DIR_REVERSE:
            PIN_AIN1 = 0;
            PIN_AIN2 = 1;
            break;
        case MOTOR_DIR_BRAKE:
            PIN_AIN1 = 1;
            PIN_AIN2 = 1;
            break;
        default:                /* MOTOR_DIR_COAST */
            PIN_AIN1 = 0;
            PIN_AIN2 = 0;
            break;
        }
    }
    else
    {
        switch (dir)
        {
        case MOTOR_DIR_FORWARD:
            PIN_BIN1 = 1;
            PIN_BIN2 = 0;
            break;
        case MOTOR_DIR_REVERSE:
            PIN_BIN1 = 0;
            PIN_BIN2 = 1;
            break;
        case MOTOR_DIR_BRAKE:
            PIN_BIN1 = 1;
            PIN_BIN2 = 1;
            break;
        default:                /* MOTOR_DIR_COAST */
            PIN_BIN1 = 0;
            PIN_BIN2 = 0;
            break;
        }
    }
}

void motor_set_duty(s8 left, s8 right)
{
    left = motor_clamp(left);
    right = motor_clamp(right);

    s_duty_left = left;
    s_duty_right = right;

    motor_set_dir(WHEEL_LEFT, (left < 0) ? MOTOR_DIR_REVERSE : MOTOR_DIR_FORWARD);
    motor_set_dir(WHEEL_RIGHT, (right < 0) ? MOTOR_DIR_REVERSE : MOTOR_DIR_FORWARD);

    PIN_STBY = 1;

    /* 两个占空比同时生效，避免中断取到一新一旧 */
    ET0 = 0;
    s_pwm_left = motor_magnitude(left);
    s_pwm_right = motor_magnitude(right);
    ET0 = 1;
}

void motor_brake(void)
{
    ET0 = 0;
    s_pwm_left = 0;
    s_pwm_right = 0;
    ET0 = 1;

    s_duty_left = 0;
    s_duty_right = 0;

    PIN_PWMA = 0;
    PIN_PWMB = 0;
    PIN_STBY = 1;
    motor_set_dir(WHEEL_LEFT, MOTOR_DIR_BRAKE);
    motor_set_dir(WHEEL_RIGHT, MOTOR_DIR_BRAKE);
}

void motor_coast(void)
{
    ET0 = 0;
    s_pwm_left = 0;
    s_pwm_right = 0;
    ET0 = 1;

    s_duty_left = 0;
    s_duty_right = 0;

    PIN_PWMA = 0;
    PIN_PWMB = 0;
    PIN_STBY = 1;
    motor_set_dir(WHEEL_LEFT, MOTOR_DIR_COAST);
    motor_set_dir(WHEEL_RIGHT, MOTOR_DIR_COAST);
}

void motor_standby(void)
{
    ET0 = 0;
    s_pwm_left = 0;
    s_pwm_right = 0;
    ET0 = 1;

    s_duty_left = 0;
    s_duty_right = 0;

    PIN_PWMA = 0;
    PIN_PWMB = 0;
    PIN_STBY = 0;
}

s8 motor_duty_left(void)
{
    return s_duty_left;
}

s8 motor_duty_right(void)
{
    return s_duty_right;
}

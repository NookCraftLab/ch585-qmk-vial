/*
 * LED指示灯驱动实现
 * LED0(PB14) = 连接状态灯，LED1(PB15) = 电源/充电灯
 * 低电平点亮
 */

#include "led_indicator.h"
#include "CH58x_common.h"
#include "timer.h"

/* LED引脚定义 */
#define LED0_PIN    GPIO_Pin_14   /* PB14，连接状态灯 */
#define LED1_PIN    GPIO_Pin_15   /* PB15，电源/充电灯 */

/* 闪烁参数（毫秒） */
#define FLASH_PAIRING_ON_MS       100    /* 配对快闪：亮100ms */
#define FLASH_PAIRING_OFF_MS      233    /* 配对快闪：灭233ms（3Hz） */
#define FLASH_RECONNECT_ON_MS     200    /* 回连慢闪：亮200ms */
#define FLASH_RECONNECT_OFF_MS    800    /* 回连慢闪：灭800ms（1Hz） */
#define FLASH_LOW_BAT_ON_MS       200    /* 低电量：亮200ms */
#define FLASH_LOW_BAT_OFF_MS      2800   /* 低电量：灭2800ms（1次/3s） */
#define SOLID_DURATION_MS         2000   /* 常亮持续时间：2s */

/* 内部状态 */
static led_conn_state_t conn_state = LED_CONN_IDLE;
static led_pwr_state_t  pwr_state  = LED_PWR_IDLE;

static uint32_t conn_timer = 0;       /* LED0 状态计时器 */
static bool     conn_led_on = false;  /* LED0 当前亮灭 */
static uint32_t pwr_timer = 0;        /* LED1 状态计时器 */
static bool     pwr_led_on = false;   /* LED1 当前亮灭 */

/* 硬件控制：低电平点亮 */
static inline void led0_on(void)  { GPIOB_ResetBits(LED0_PIN); }
static inline void led0_off(void) { GPIOB_SetBits(LED0_PIN); }
static inline void led1_on(void)  { GPIOB_ResetBits(LED1_PIN); }
static inline void led1_off(void) { GPIOB_SetBits(LED1_PIN); }

void led_indicator_init(void)
{
    /* PB14/PB15 推挽输出，5mA */
    GPIOB_ModeCfg(LED0_PIN | LED1_PIN, GPIO_ModeOut_PP_5mA);

    /* 默认熄灭 */
    led0_off();
    led1_off();

    conn_state = LED_CONN_IDLE;
    pwr_state  = LED_PWR_IDLE;
    conn_led_on = false;
    pwr_led_on  = false;
}

void led_set_conn_state(led_conn_state_t state)
{
    if (conn_state == state) return;

    conn_state = state;
    conn_timer = timer_read32();
    conn_led_on = false;

    /* 进入状态时立即更新一次 */
    switch (state) {
    case LED_CONN_WIRED:
    case LED_CONN_SUCCESS:
        led0_on();
        conn_led_on = true;
        break;
    case LED_CONN_PAIRING:
    case LED_CONN_RECONNECTING:
        led0_on();
        conn_led_on = true;
        break;
    case LED_CONN_IDLE:
    default:
        led0_off();
        conn_led_on = false;
        break;
    }
}

void led_set_pwr_state(led_pwr_state_t state)
{
    if (pwr_state == state) return;

    pwr_state = state;
    pwr_timer = timer_read32();
    pwr_led_on = false;

    switch (state) {
    case LED_PWR_CHARGING:
        led1_on();
        pwr_led_on = true;
        break;
    case LED_PWR_LOW_BATTERY:
        led1_on();
        pwr_led_on = true;
        break;
    case LED_PWR_IDLE:
    case LED_PWR_CHARGED:
    default:
        led1_off();
        pwr_led_on = false;
        break;
    }
}

/* LED0 连接状态机处理 */
static void process_conn_led(void)
{
    uint32_t elapsed = timer_elapsed32(conn_timer);

    switch (conn_state) {
    case LED_CONN_WIRED:
    case LED_CONN_SUCCESS:
        /* 常亮2s后自动熄灭 */
        if (elapsed >= SOLID_DURATION_MS) {
            led0_off();
            conn_led_on = false;
            conn_state = LED_CONN_IDLE;
        }
        break;

    case LED_CONN_PAIRING:
        /* 快闪3Hz：亮100ms/灭233ms */
        if (conn_led_on && elapsed >= FLASH_PAIRING_ON_MS) {
            led0_off();
            conn_led_on = false;
            conn_timer = timer_read32();
        } else if (!conn_led_on && elapsed >= FLASH_PAIRING_OFF_MS) {
            led0_on();
            conn_led_on = true;
            conn_timer = timer_read32();
        }
        break;

    case LED_CONN_RECONNECTING:
        /* 慢闪1Hz：亮200ms/灭800ms */
        if (conn_led_on && elapsed >= FLASH_RECONNECT_ON_MS) {
            led0_off();
            conn_led_on = false;
            conn_timer = timer_read32();
        } else if (!conn_led_on && elapsed >= FLASH_RECONNECT_OFF_MS) {
            led0_on();
            conn_led_on = true;
            conn_timer = timer_read32();
        }
        break;

    case LED_CONN_IDLE:
    default:
        break;
    }
}

/* LED1 电源状态机处理 */
static void process_pwr_led(void)
{
    uint32_t elapsed = timer_elapsed32(pwr_timer);

    switch (pwr_state) {
    case LED_PWR_CHARGING:
        /* 常亮，不处理 */
        break;

    case LED_PWR_LOW_BATTERY:
        /* 慢闪1次/3s：亮200ms/灭2800ms */
        if (pwr_led_on && elapsed >= FLASH_LOW_BAT_ON_MS) {
            led1_off();
            pwr_led_on = false;
            pwr_timer = timer_read32();
        } else if (!pwr_led_on && elapsed >= FLASH_LOW_BAT_OFF_MS) {
            led1_on();
            pwr_led_on = true;
            pwr_timer = timer_read32();
        }
        break;

    case LED_PWR_IDLE:
    case LED_PWR_CHARGED:
    default:
        break;
    }
}

void led_indicator_task(void)
{
    process_conn_led();
    process_pwr_led();
}

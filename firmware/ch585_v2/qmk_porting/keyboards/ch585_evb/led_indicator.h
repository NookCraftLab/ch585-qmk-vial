/*
 * LED指示灯驱动
 * LED0(PB14) = 连接状态灯，LED1(PB15) = 电源/充电灯
 * 正常使用时全部熄灭，状态变化时才提示
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

/* LED0 连接状态 */
typedef enum {
    LED_CONN_IDLE = 0,          /* 熄灭（正常使用） */
    LED_CONN_WIRED,              /* 有线插入：常亮2s后熄灭 */
    LED_CONN_PAIRING,            /* 配对中：快闪3Hz */
    LED_CONN_RECONNECTING,       /* 回连中：慢闪1Hz */
    LED_CONN_SUCCESS,            /* 连接成功：常亮2s后熄灭 */
} led_conn_state_t;

/* LED1 电源状态 */
typedef enum {
    LED_PWR_IDLE = 0,            /* 熄灭（正常使用） */
    LED_PWR_CHARGING,            /* 充电中：常亮 */
    LED_PWR_CHARGED,             /* 充满：熄灭 */
    LED_PWR_LOW_BATTERY,         /* 低电量：慢闪1次/3s */
} led_pwr_state_t;

/* 初始化LED硬件 */
void led_indicator_init(void);

/* 主循环调用，非阻塞 */
void led_indicator_task(void);

/* 设置LED0连接状态 */
void led_set_conn_state(led_conn_state_t state);

/* 设置LED1电源状态 */
void led_set_pwr_state(led_pwr_state_t state);

/*
 * power_management.c — 电源管理
 * 功能：10分钟无操作进睡眠，按键唤醒；有线模式不进睡眠；
 *       睡眠时关LED，低电量LED1慢闪；WFI低功耗；
 *       阶梯式蓝牙连接间隔（100/300/500/1000ms，按睡眠时长30/90/180分钟切换）
 */

#include "power_management.h"
#include "battery.h"
#include "ble_app.h"
#include "ble_config.h"
#include "timer.h"
#include "led_indicator.h"
#include "CH58x_common.h"

/* ========================================================================
 * 内部状态
 * ======================================================================== */

static power_state_t current_power_state = POWER_STATE_ACTIVE;
static uint32_t last_activity_time = 0;
static bool low_battery_active = false;
static bool s_wired_mode = false;
static bool s_mobile_mode = false;  /* 携带模式：禁用阶梯式连接间隔，固定2000ms */
static uint32_t sleep_start_time = 0;

/* ========================================================================
 * 初始化
 * ======================================================================== */

void power_init(void)
{
    current_power_state = POWER_STATE_ACTIVE;
    last_activity_time = timer_read32();
    low_battery_active = false;

    /* LED0 连接状态灯由 led_indicator 管理 */
}

/* ========================================================================
 * 主循环轮询
 * ======================================================================== */

void power_task(void)
{
    uint32_t now = timer_read32();

    /* 低电量检测 — 蓝牙模式下低电量时 LED1 慢闪提醒
     * 有线模式下 LED1 常亮（表示外部供电），不受低电量影响
     */
    uint8_t battery_level = battery_get_level();
    bool is_low_battery = (battery_level < POWER_LOW_BATTERY_THRESHOLD);
    if (is_low_battery != low_battery_active) {
        low_battery_active = is_low_battery;
        /* 只在蓝牙模式下更新 LED1 状态，有线模式保持常亮 */
        if (!s_wired_mode) {
            if (is_low_battery) {
                led_set_pwr_state(LED_PWR_LOW_BATTERY);  /* 低电量慢闪 */
            } else {
                led_set_pwr_state(LED_PWR_IDLE);  /* 正常电量熄灭 */
            }
        }
    }

    /* 睡眠状态：按睡眠时长阶梯式切换蓝牙连接间隔
     * 携带模式下跳过（固定2000ms，由携带模式进入时设置）
     */
    if (current_power_state == POWER_STATE_SLEEPING && !s_mobile_mode) {
        uint32_t sleep_duration = now - sleep_start_time;
        uint8_t target_level = 0;

        if (sleep_duration >= BLE_CONN_INTERVAL_L2_TIME) {
            target_level = 3;  /* 180分钟+：1000ms */
        } else if (sleep_duration >= BLE_CONN_INTERVAL_L1_TIME) {
            target_level = 2;  /* 90~180分钟：500ms */
        } else if (sleep_duration >= BLE_CONN_INTERVAL_L0_TIME) {
            target_level = 1;  /* 30~90分钟：300ms */
        } else {
            target_level = 0;  /* 0~30分钟：100ms */
        }

        if (target_level != ble_get_current_interval_level()) {
            ble_request_conn_interval(target_level);
        }
        return;
    }

    /* 有线模式不进睡眠 */
    if (s_wired_mode) {
        last_activity_time = now;
        return;
    }

    /* 无操作超时进睡眠 */
    uint32_t elapsed = now - last_activity_time;
    if (elapsed >= POWER_SLEEP_TIMEOUT_MS) {
        power_enter_sleep();
    }
}

/* ========================================================================
 * 用户活动通知 — 按键按下时调用，重置无操作计时
 * ======================================================================== */

void power_notify_activity(void)
{
    last_activity_time = timer_read32();

    /* 如果在睡眠状态，唤醒 */
    if (current_power_state == POWER_STATE_SLEEPING) {
        power_wake_up();
    }
}

/* ========================================================================
 * 进入睡眠
 * ======================================================================== */

void power_enter_sleep(void)
{
    if (current_power_state == POWER_STATE_SLEEPING) return;

    current_power_state = POWER_STATE_SLEEPING;
    sleep_start_time = timer_read32();
    ble_request_conn_interval(0);  /* 刚睡眠保持100ms低延迟 */

    /* LED0 连接状态灯由 led_indicator 管理，睡眠时自动熄灭 */
    low_battery_active = false;
}

void power_wake_up(void)
{
    if (current_power_state == POWER_STATE_ACTIVE) return;

    current_power_state = POWER_STATE_ACTIVE;
    last_activity_time = timer_read32();
    ble_request_conn_interval(0);  /* 唤醒恢复100ms低延迟 */

    /* LED0 连接状态灯由 led_indicator 管理 */
}

/* ========================================================================
 * 状态查询
 * ======================================================================== */

power_state_t power_get_state(void)
{
    return current_power_state;
}

uint32_t power_get_time_until_sleep(void)
{
    if (current_power_state == POWER_STATE_SLEEPING) {
        return 0;
    }
    uint32_t elapsed = timer_read32() - last_activity_time;
    if (elapsed >= POWER_SLEEP_TIMEOUT_MS) {
        return 0;
    }
    return POWER_SLEEP_TIMEOUT_MS - elapsed;
}

/* ========================================================================
 * 有线模式设置 — 有线模式下不进睡眠
 * ======================================================================== */

void power_set_wired_mode(bool wired)
{
    s_wired_mode = wired;

    if (wired) {
        if (current_power_state == POWER_STATE_SLEEPING) {
            power_wake_up();
        }
        last_activity_time = timer_read32();
    } else {
        last_activity_time = timer_read32();
    }
}

/* 携带模式设置 — 携带模式下禁用阶梯式连接间隔（固定2000ms）*/
void power_set_mobile_mode(bool enabled)
{
    s_mobile_mode = enabled;
}

bool power_is_mobile_mode(void)
{
    return s_mobile_mode;
}

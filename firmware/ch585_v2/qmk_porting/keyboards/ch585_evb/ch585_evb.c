/*
 * CH585 EVB 板级入口
 *
 * 模式状态机（参考量产键盘逻辑）：
 *   WIRED（有线模式）：USB 已连接，停止蓝牙广播，LED 亮2秒提示后熄灭
 *   BLE_IDLE（无线空闲）：USB 未连接，没有主动连接蓝牙，LED 灭
 *   BLE_RECONNECTING（回连中）：定向广播→普通广播，LED 慢闪
 *   BLE_PAIRING（配对中）：普通广播可被搜索，LED 快闪
 *   BLE_CONNECTED（已连接）：正常工作，LED 亮2秒后熄灭
 *   BLE_STANDBY（待机）：回连/配对超时后进入，LED 灭，按 BT 键唤醒
 *
 * 用户逻辑（参考量产键盘）：
 *   上电 → 恢复上次模式（有线/蓝牙），蓝牙模式自动回连上次设备
 *   运行中插 USB → 自动切有线（断开蓝牙，停止广播）
 *   运行中拔 USB → 不自动切无线，保持当前状态
 *   短按 BT_1/2/3 → 切到蓝牙模式，切换到对应设备（有配对则回连，无配对则广播）
 *   长按 BT_1/2/3（≥3秒）→ 进入配对模式（不删旧绑定，新配对覆盖）
 *   按 WIRED_MODE → 切到有线模式
 *   按 BT_CLEAR → 清除所有配对
 *   模式和设备记忆到 EEPROM，上电恢复
 */

#include QMK_KEYBOARD_H
#include "ble_app.h"
#include "usb_device_state.h"
#include "power_management.h"
#include "battery.h"
#include "led_indicator.h"
#include "CH58x_common.h"

/* 有线模式激活标志 — 全局变量，供 ble_app.c 里的状态回调检查
 * 有线模式下，蓝牙状态变化不应覆盖 LED 状态
 */
bool wired_mode_active = false;

/* 记录上一次的 USB 状态，用于检测状态变化 */
static bool last_usb_connected = false;

/* 判断 USB 是否已连接并枚举成功 */
static bool usb_is_connected(void) {
    return (usb_device_state_get_configure_state() == USB_DEVICE_STATE_CONFIGURED);
}

/* 切换到有线模式：断开蓝牙，停止广播，LED 亮2秒提示 */
void enter_wired_mode(void) {
    /* 设置有线模式标志 — 必须在断开蓝牙之前设置，
     * 否则 ble_state_cb 里的状态变化会覆盖 LED 状态
     */
    wired_mode_active = true;

    /* 通知电源管理进入有线模式 — 有线模式不进睡眠 */
    power_set_wired_mode(true);

    /* 保存模式记忆：有线模式 */
    ble_save_mode(true, 1);

    /* 断开蓝牙连接（如果已连接）*/
    if (ble_is_connected()) {
        ble_disconnect();
    }

    /* 停止蓝牙广播 */
    ble_stop_advertising();

    /* LED1 电源灯：有线模式常亮（表示外部供电）*/
    led_set_pwr_state(LED_PWR_CHARGING);

    /* LED0 连接状态灯由 led_indicator 管理 */
}

/* 退出有线模式：清除标志（蓝牙模式由 ble_app.c 管理 LED）*/
static void exit_wired_mode(void) {
    wired_mode_active = false;

    /* 通知电源管理退出有线模式 — 无线模式下可以进睡眠 */
    power_set_wired_mode(false);

    /* LED1 电源灯：蓝牙模式熄灭（电池供电，省电）
     * 低电量时由 power_management.c 统一设置为慢闪
     */
    led_set_pwr_state(LED_PWR_IDLE);
}

/* ========================================================================
 * 主循环里的 USB 状态检测和模式切换
 *
 * QMK 会在主循环里定期调用 housekeeping_task_kb()
 * 这里检测 USB 状态变化，触发模式切换
 * ======================================================================== */

void housekeeping_task_kb(void) {
    /* 电源管理任务 — 检查无操作超时、低电量提醒 */
    power_task();

    bool current_usb = usb_is_connected();

    /* USB 状态变化检测 */
    if (current_usb != last_usb_connected) {
        if (current_usb) {
            /* USB 从断开 → 连接：自动切换到有线模式 */
            enter_wired_mode();
        } else {
            /* USB 从连接 → 断开：不自动切无线，保持当前状态
             * 用户需要手动按 BT_1/2/3 才会切到蓝牙
             */
            exit_wired_mode();
            /* LED0 连接状态灯由 led_indicator 管理 */
        }
        last_usb_connected = current_usb;
    }

    /* 有线模式下持续确保蓝牙广播已停止（防止 BLE 协议栈内部重新启动广播）*/
    if (wired_mode_active) {
        if (ble_get_state() == BLE_STATE_ADVERTISING || ble_get_state() == BLE_STATE_PAIRING) {
            ble_stop_advertising();
        }
        /* 注意：有线模式下 LED 不常亮，只在进入时亮2秒提示，之后熄灭（省电且不干扰）*/
    }
}

int main(void) {
    extern void protocol_setup();
    extern void protocol_pre_init();
    extern void protocol_post_init();
    extern void platform_run();

    platform_setup();

    /* LED指示灯初始化 */
    led_indicator_init();

    protocol_setup();
#if !defined ESB_ENABLE || ESB_ENABLE != 2
    keyboard_setup();
#endif

    protocol_pre_init();
    keyboard_init();
    protocol_post_init();

    /* 初始化 USB 状态记录（只用于后续状态变化检测，不自动切换模式）
     * 必须在 ble_restore_last_mode() 之前设置，避免上电时 housekeeping
     * 检测到"USB 从无到有"而自动切有线，覆盖上电恢复的模式
     */
    last_usb_connected = usb_is_connected();

    /* 初始化电池管理（评估板用模拟值，预留接口）*/
    battery_init();

    /* 初始化电源管理（上电点亮 LED1，启动无操作计时）*/
    power_init();

    /* 上电恢复上次模式（参考量产键盘逻辑）：
     *   上次是有线 → 进入有线模式
     *   上次是蓝牙 → 切换到上次设备槽位，自动回连
     * USB 是否连接不影响恢复结果，模式由用户上次选择决定
     */
    ble_restore_last_mode();

    /* Main loop */
    for (;;) {
        platform_run();
        //! housekeeping_task() is handled by platform

        /* LED指示灯任务 — 处理闪烁和状态超时 */
        led_indicator_task();

        /* WFI（Wait For Interrupt）— CPU 空闲时进入低功耗休眠
         * 任何中断（定时器/矩阵扫描/BLE/USB/GPIO）都会唤醒 CPU
         * 不影响实时性：所有任务都是中断驱动的，唤醒后立即处理
         * 零睡死风险：WFI 只暂停 CPU，所有外设/RAM/寄存器保持状态
         */
        __WFI();
    }
}

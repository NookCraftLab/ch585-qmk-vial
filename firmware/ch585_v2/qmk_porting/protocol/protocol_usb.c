/*
Copyright 2022 Huckies <https://github.com/Huckies>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "protocol_usb.h"
#include "usb_device_state.h"
#include "keycode_config.h"
#include "usb_main.h"
#include "usb_interface.h"
#include "ble_app.h"
#include "led_indicator.h"
#ifdef RAW_ENABLE
#include "raw_hid.h"
#endif
#ifdef RGB_RAW_ENABLE
#include "auxiliary_rgb.h"
#include "dynamic_lighting.h"
#endif

extern int usbd_deinitialize(uint8_t busid);
extern bool wired_mode_active;  /* 有线模式标志，定义在 ch585_evb.c */

/* 上一个键盘报告 — 用于报告去重，相同状态不重复发送 */
static report_keyboard_t last_keyboard_report;
static bool last_report_valid = false;

static void send_keyboard(report_keyboard_t *report)
{
    /* 报告去重：和上一个报告完全相同则跳过，避免矩阵扫描重复发送导致连击 */
    if (last_report_valid && memcmp(report, &last_keyboard_report, sizeof(report_keyboard_t)) == 0) {
        return;
    }
    memcpy(&last_keyboard_report, report, sizeof(report_keyboard_t));
    last_report_valid = true;

    /* 有线模式：通过USB发送 */
    if (wired_mode_active) {
        if (!keyboard_protocol) {
            hid_keyboard_send_report(KEYBOARD_MODE_BIOS, &report->mods, 8);
        } else {
            hid_keyboard_send_report(KEYBOARD_MODE_BIOS, (uint8_t *)report, KEYBOARD_REPORT_SIZE);
        }
    }

    /* 蓝牙模式：通过蓝牙发送（有线模式下蓝牙已断开，不会重复发送）*/
    if (!wired_mode_active) {
        ble_send_keyboard_report(report->mods, report->keys);
    }
}

static void send_nkro(report_nkro_t *report)
{
    if (wired_mode_active) {
        hid_keyboard_send_report(KEYBOARD_MODE_NKRO, (uint8_t *)report, EXKEY_IN_EP_SIZE);
    }
}

static void send_mouse(report_mouse_t *report)
{
#ifdef MOUSE_ENABLE
    if (wired_mode_active) {
        uint8_t report_to_send[6];
        report_to_send[0] = REPORT_ID_MOUSE;
        memcpy(report_to_send + 1, report, 5);
        hid_exkey_send_report(report_to_send, 6);
    }
#endif
}

static void send_extra(report_extra_t *report)
{
    if (wired_mode_active) {
        hid_exkey_send_report((uint8_t *)report, sizeof(report_extra_t));
    }
}

#ifdef RAW_ENABLE
static void send_qmk_raw(uint8_t *data, uint8_t length)
{
    hid_qmk_raw_send_report(data, length);
}
#endif

#ifdef RGB_RAW_ENABLE
static void send_rgb_raw(uint8_t *data, uint8_t length)
{
    hid_rgb_raw_send_report(data, length);
}

static void control_send_rgb_raw(uint8_t report_id, uint8_t **data, uint32_t *len)
{
    dynamic_lighting_handle_get_report(report_id, data, len);
}
#endif

static void platform_initialize()
{
#ifndef PLF_DEBUG
    while ((R8_UART1_LSR & RB_LSR_TX_ALL_EMP) == 0) {
        __nop();
    }
    UART1_Reset();
    gpio_set_pin_input_low(A8);
    gpio_set_pin_input_low(A9);
#endif

    /* 初始化 BLE 蓝牙协议栈和 HID 服务 */
    ble_app_init();
}

static void protocol_setup()
{
    usb_device_state_init();
    usb_task_init();
}

static void protocol_init()
{
    init_usb_driver();
}

static void platform_reboot()
{
    // wait for usb sending done
    DelayMs(10);
    PRINT("Reboot execute.\n");
    WAIT_FOR_DBG;
    SYS_ResetExecute();
}

static void protocol_pre_task()
{
}

static void protocol_post_task()
{
}

__HIGH_CODE static void platform_run()
{
    /* LED指示灯任务 — 处理闪烁和状态超时
     * 必须在 TMOS_SystemProcess() 之前调用，因为 platform_run() 在无限循环里
     */
    led_indicator_task();

    /* 处理 BLE 协议栈任务（TMOS 调度器） */
    TMOS_SystemProcess();
}

const ch582_interface_t ch582_protocol_usb = {
    .ch582_common_driver.keyboard_leds = keyboard_leds,
    .ch582_common_driver.send_keyboard = send_keyboard,
    .ch582_common_driver.send_nkro = send_nkro,
    .ch582_common_driver.send_mouse = send_mouse,
    .ch582_common_driver.send_extra = send_extra,
#ifdef RAW_ENABLE
    .send_qmk_raw = send_qmk_raw,
    .receive_qmk_raw = raw_hid_receive,
#endif
#ifdef RGB_RAW_ENABLE
    .send_rgb_raw = send_rgb_raw,
    .receive_rgb_raw = rgb_raw_hid_receive,
    .control_send_rgb_raw = control_send_rgb_raw,
    .receive_rgb_raw_control = rgb_raw_control_receive,
#endif
    .ch582_platform_initialize = platform_initialize,
    .ch582_protocol_setup = protocol_setup,
    .ch582_protocol_init = protocol_init,
    .ch582_protocol_pre_task = protocol_pre_task,
    .ch582_protocol_post_task = protocol_post_task,
    .ch582_platform_run = platform_run,
    .ch582_platform_reboot = platform_reboot,
};

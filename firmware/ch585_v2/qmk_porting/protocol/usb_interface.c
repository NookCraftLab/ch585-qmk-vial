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

#include "usb_interface.h"
#include "usb_config.h"
#include "ch585_usbhs_device.h"
#include "ch585_usbfs_device.h"
#include "led_indicator.h"
#include <stdbool.h>
#include <string.h>

static uint8_t keyboard_last_bios_report[8] = {0};

void init_usb_driver()
{
#if (USB_PORT_CONFIG == USB_PORT_HIGH_SPEED)
    USBHS_Device_Init(ENABLE);
    PFIC_EnableIRQ(USB2_DEVICE_IRQn);
#elif (USB_PORT_CONFIG == USB_PORT_FULL_SPEED)
    USBFS_Device_Init();
    PFIC_EnableIRQ(USB_IRQn);
#elif (USB_PORT_CONFIG == USB_PORT_DUAL_BOTH)
    USBHS_Device_Init(ENABLE);
    PFIC_EnableIRQ(USB2_DEVICE_IRQn);
    USBFS_Device_Init();
    PFIC_EnableIRQ(USB_IRQn);
#endif

    /* 有线模式提示：LED0 常亮2s后熄灭 */
    led_set_conn_state(LED_CONN_WIRED);
}

bool usb_remote_wakeup()
{
    return true;
}

bool hid_keyboard_send_report(uint8_t mode, uint8_t *data, uint8_t len)
{
    (void)mode;

#if (USB_PORT_CONFIG == USB_PORT_HIGH_SPEED)
    USBHS_Endp_DataUp(DEF_UEP1, data, len, DEF_UEP_CPY_LOAD);
#elif (USB_PORT_CONFIG == USB_PORT_FULL_SPEED)
    USBFS_SendKeyboardReport(data);
#elif (USB_PORT_CONFIG == USB_PORT_DUAL_BOTH)
    USBHS_Endp_DataUp(DEF_UEP1, data, len, DEF_UEP_CPY_LOAD);
    USBFS_SendKeyboardReport(data);
#endif

    memcpy(keyboard_last_bios_report, data, (len < 8) ? len : 8);
    return true;
}

void hid_keyboard_send_last_bios_report()
{
    hid_keyboard_send_report(KEYBOARD_MODE_BIOS, keyboard_last_bios_report, 8);
}

bool hid_exkey_send_report(uint8_t *data, uint8_t len)
{
    (void)data;
    (void)len;
    return true;
}

bool hid_qmk_raw_send_report(uint8_t *data, uint8_t len)
{
#if (USB_PORT_CONFIG == USB_PORT_HIGH_SPEED)
    return USBHS_RawHID_Send(data, len);
#elif (USB_PORT_CONFIG == USB_PORT_FULL_SPEED)
    return USBFS_RawHID_Send(data, len);
#elif (USB_PORT_CONFIG == USB_PORT_DUAL_BOTH)
    uint8_t ret1 = USBHS_RawHID_Send(data, len);
    uint8_t ret2 = USBFS_RawHID_Send(data, len);
    return (ret1 || ret2);
#else
    return false;
#endif
}

bool hid_rgb_raw_send_report(uint8_t *data, uint8_t len)
{
    (void)data;
    (void)len;
    return true;
}

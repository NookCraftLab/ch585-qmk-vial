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

#include "platform_deps.h"
#include <stdio.h>
#include "HAL.h"
#include "gpio.h"
#include "quantum.h"
#include "quantum_keycodes.h"

volatile uint8_t kbd_protocol_type = 0;
#if defined BLE_ENABLE || (defined ESB_ENABLE && (ESB_ENABLE == 1 || ESB_ENABLE == 2))
extern void wireless_indicator_status_reset();
#else
__attribute__((aligned(4))) uint32_t MEM_BUF[BLE_MEMHEAP_SIZE / 4];
#endif

__HIGH_CODE _PUTCHAR_CLAIM;

int8_t sendchar(uint8_t c)
{
    _putchar(c);
    return 0;
}

#if !defined ESB_ENABLE || ESB_ENABLE == 1
bool shutdown_kb(bool jump_to_bootloader)
{
    rgbled_power_off();
#if defined BLE_ENABLE || (defined ESB_ENABLE && (ESB_ENABLE == 1 || ESB_ENABLE == 2))
    wireless_indicator_status_reset();
#endif

#ifdef ENCODER_ENABLE
    pin_t encoders_pad_a[] = ENCODER_A_PINS, encoders_pad_b[] = ENCODER_B_PINS;

    for (uint8_t i = 0; i < sizeof(encoders_pad_a) / sizeof(encoders_pad_a[0]); i++) {
        gpio_set_pin_input_low(encoders_pad_a[i]);
    }
    for (uint8_t i = 0; i < sizeof(encoders_pad_b) / sizeof(encoders_pad_b[0]); i++) {
        gpio_set_pin_input_low(encoders_pad_b[i]);
    }
#endif

    if (!shutdown_user(jump_to_bootloader)) {
        return false;
    }

    return true;
}
#endif

void platform_setup()
{
    /* CH585: 显式配置系统时钟为 HSE PLL 62.4MHz（外部晶振）
     * USBFS 必须在 PLL 时钟下才能产生准确的 48MHz USB 时钟。
     * 必须在任何 USB/GPIO 外设初始化之前完成。*/

    /* HSE外部晶振负载电容配置（18pF），必须在SetSysClock之前调用，确保HSE稳定起振，蓝牙才能正常工作 */
    HSECFG_Capacitance(HSECap_18p);
    SetSysClock(CLK_SOURCE_HSE_PLL_62_4MHz);

    /* BLE 鍗忚鏍堝垵濮嬪寲蹇呴』绱ц窡鍦?SetSysClock 涔嬪悗锛屽拰瀹樻柟 HID_Keyboard 绀轰緥瀹屽叏涓€鑷淬€?     * 涓棿涓嶈兘鎻掑叆 UART/EEPROM/GPIO 绛変换浣曞璁炬搷浣滐紝鍚﹀垯鍙兘淇敼瀵勫瓨鍣ㄧ姸鎬侊紝
     * 瀵艰嚧 BLE 鍗忚鏍堝垵濮嬪寲寮傚父銆佸箍鎾棤娉曞彂鍑恒€?*/
    PRINT(">> stage: BLE_LibInit begin...\n");
    CH58X_BLEInit();
    PRINT(">> stage: BLE_LibInit done\n");

    HAL_Init();
    PRINT(">> stage: HAL_Init done\n");

#if LSE_ENABLE
    R16_PIN_ANALOG_IE |= RB_PIN_XT32K_IE;
#endif
#ifdef PLF_DEBUG
    DBG_INIT;
    PRINT("App " MACRO2STR(__GIT_VERSION__) ", build on %s\n", QMK_BUILDDATE);
#else
    gpio_write_pin_high(A9);
    gpio_set_pin_output(A9);
    gpio_set_pin_input_high(A8);
    UART1_DefInit();
    UART1_BaudRateCfg(DEBUG_BAUDRATE);

    char buffer[UINT8_MAX];
    uint8_t len = sprintf(buffer, "App " MACRO2STR(__GIT_VERSION__) "\n");

    while (len) {
        if (R8_UART1_TFC != UART_FIFO_SIZE) {
            R8_UART1_THR = buffer[strlen(buffer) - len];
            len--;
        }
    }
    while ((R8_UART1_LSR & RB_LSR_TX_ALL_EMP) == 0) {
        __nop();
    }
    R8_UART1_IER = RB_IER_RESET;
    gpio_set_pin_input_low(A8);
    gpio_set_pin_input_low(A9);
#endif

    PRINT(">> stage: clock+uart ok\n");

    {
        // preserve BOOTMAGIC_ROW and BOOTMAGIC_COLUMN to eeprom for future use
        uint8_t buffer[EEPROM_PAGE_SIZE], ret;

        do {
            ret = EEPROM_READ(QMK_EEPROM_RESERVED_START_POSITION, buffer, sizeof(buffer));
        } while (ret);
    PRINT(">> stage: eeprom read ok (ret=%d)\n", ret);
        if (buffer[1] != BOOTMAGIC_ROW || buffer[2] != BOOTMAGIC_COLUMN) {
            buffer[1] = BOOTMAGIC_ROW;
            buffer[2] = BOOTMAGIC_COLUMN;
            do {
                ret = EEPROM_ERASE(QMK_EEPROM_RESERVED_START_POSITION, EEPROM_PAGE_SIZE) ||
                      EEPROM_WRITE(QMK_EEPROM_RESERVED_START_POSITION, buffer, EEPROM_PAGE_SIZE);
            } while (ret);
        }
    }

    bootloader_select_boot_mode();
    PRINT(">> stage: boot mode selected\n");

    /* 娉ㄦ剰锛欳H58X_BLEInit() 鍜?HAL_Init() 宸茬粡鍦?SetSysClock 涔嬪悗绔嬪嵆璋冪敤浜嗭紝
     * 杩欓噷涓嶈兘閲嶅璋冪敤锛屽惁鍒?BLE 鍗忚鏍堜細閲嶅鍒濆鍖栵紝瀵艰嚧骞挎挱鏃犳硶鍙戝嚭銆?*/

    ch582_interface_t *interface = ch582_get_protocol_interface();

    if (interface) {
        interface->ch582_platform_initialize();
    }
    PRINT(">> stage: platform_initialize done (iface=%d)\n", interface != 0);

#ifdef BLE_ENABLE
    if (kbd_protocol_type != kbd_protocol_ble) {
        ch582_protocol_ble.ch582_protocol_setup();
    }
#endif
}

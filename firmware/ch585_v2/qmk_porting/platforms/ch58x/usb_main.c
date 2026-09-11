#include "usb_main.h"
#include "usb_interface.h"
#include "keycode_config.h"
#include "usb_device_state.h"
#include "suspend.h"
#include "rgb_led.h"
#include "protocol_supplement.h"
#include "wait.h"

static uint8_t usbTaskId = INVALID_TASK_ID;
extern void suspend_power_down_quantum();
extern void suspend_wakeup_init_quantum();

static uint16_t usb_ProcessEvent(uint8_t task_id, uint16_t events)
{
    if (events & USB_RUN_QMK_TASK_EVT) {
#if defined BLE_ENABLE || (defined ESB_ENABLE && (ESB_ENABLE == 1 || ESB_ENABLE == 2))
        extern void wireless_indicator_daemon();

        wireless_indicator_daemon();
#endif

#if !defined(NO_USB_STARTUP_CHECK)
        static bool suspended = false;

        do {
            if (usb_device_state_get_configure_state() == USB_DEVICE_STATE_SUSPEND) {
                suspended = true;
                /* Do this in the suspended state */
                suspend_power_down_quantum();
                rgbled_power_off();
                /* Remote wakeup */
                if (suspend_wakeup_condition()) {
                    while (!usb_remote_wakeup()) {
#if USB_SUSPEND_WAKEUP_DELAY > 0
                        // Some hubs, kvm switches, and monitors do
                        // weird things, with USB device state bouncing
                        // around wildly on wakeup, yielding race
                        // conditions that can corrupt the keyboard state.
                        //
                        // Pause for a while to let things settle...
                        wait_ms(USB_SUSPEND_WAKEUP_DELAY);
#endif
                    }
                    break;
                }
                goto exit_usb_run_qmk_task_evt;
            }
        } while (0);

        if (suspended) {
            suspended = false;
            suspend_wakeup_init_quantum();
        }
#endif

        run_qmk_task();

        keyboard_check_protocol_mode();
#ifdef POWER_DETECT_PIN
        if (!gpio_read_pin(POWER_DETECT_PIN)) {
            // cable removed
            PRINT("Cable pulled out.\n");
            mcu_reset();
        }
#endif

    exit_usb_run_qmk_task_evt:
        tmos_start_task(task_id, USB_RUN_QMK_TASK_EVT, 0);

        return (events ^ USB_RUN_QMK_TASK_EVT);
    }

    if (events & USB_PERIODICAL_BIOS_REPORT_EVT) {
#ifdef NKRO_ENABLE
        if (!keymap_config.nkro && keyboard_idle && keyboard_protocol) {
#else
        if (keyboard_idle && keyboard_protocol) {
#endif
            hid_keyboard_send_last_bios_report();
            usb_start_periodical_bios_report();
        }
    }

    return 0;
}

void usb_task_init()
{
    usbTaskId = TMOS_ProcessEventRegister(usb_ProcessEvent);
    tmos_start_task(usbTaskId, USB_RUN_QMK_TASK_EVT, 0);
}

void usb_start_periodical_bios_report()
{
    tmos_stop_task(usbTaskId, USB_PERIODICAL_BIOS_REPORT_EVT);
    tmos_clear_event(usbTaskId, USB_PERIODICAL_BIOS_REPORT_EVT);
    tmos_start_task(usbTaskId, USB_PERIODICAL_BIOS_REPORT_EVT, SYS_TICK_MS(keyboard_idle));
}

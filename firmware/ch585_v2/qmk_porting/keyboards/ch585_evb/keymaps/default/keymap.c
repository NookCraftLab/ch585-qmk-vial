/*
 * CH585 EVB 默认键位（2x4，常规矩阵扫描，杜邦线短接测试）
 * 层 0：qwer / asd + MO(_FN)
 * 层 1：FN（功能键，后续在 Vial 里自定义）
 *
 * 自定义按键码（Vial User Keycodes）：
 *   BT_1 / BT_2 / BT_3 — 短按切换设备，长按3秒配对设备
 *   BT_CLEAR — 清除所有配对
 *   WIRED_MODE — 切换到有线模式
 *   FACTORY_RESET — 长按5秒恢复出厂设置
 *   MOBILE_MODE — 长按3秒切换携带模式（锁定键盘+2000ms连接间隔）
 */

#include QMK_KEYBOARD_H
#include "ble_app.h"
#include "eeconfig.h"
#include "power_management.h"

enum layer_names {
    _BASE = 0,
    _FN,
};

/* 自定义按键码 — 对应 vial.json 里的 customKeycodes 顺序 */
enum custom_keycodes {
    BT_1 = QK_USER_0,
    BT_2,
    BT_3,
    BT_CLEAR,
    WIRED_MODE,
    FACTORY_RESET,
    MOBILE_MODE,
};

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    /* 第0层：纯字母，用户通过 Vial 改键把自定义按键放到这层 */
    [_BASE] = LAYOUT_all(
        KC_Q,   KC_W,   KC_E,   KC_R,
        KC_A,   KC_S,   KC_D,   KC_F
    ),

    /* 第1层：全透明，默认空，用户可在 Vial 里自定义 */
    [_FN] = LAYOUT_all(
        KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,
        KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS
    ),
};

/* ========================================================================
 * 外部函数声明 — 有线模式切换（定义在 ch585_evb.c）
 * ======================================================================== */
extern void enter_wired_mode(void);

/* ========================================================================
 * 长短按状态管理
 *
 * 自定义按键支持长短按：
 *   BT_1/BT_2/BT_3：短按切换设备，长按3秒配对
 *   FACTORY_RESET：长按5秒恢复出厂设置
 *   BT_CLEAR / WIRED_MODE：短按直接触发，不需要长按
 * ======================================================================== */

#define BT_LONG_PRESS_MS    3000  /* 蓝牙配对长按阈值：3秒 */
#define FACTORY_RESET_MS    5000  /* 恢复出厂设置长按阈值：5秒 */

static uint16_t long_press_keycode = 0;   /* 当前正在检测长按的按键码，0=无 */
static uint32_t long_press_start_time = 0; /* 按下时间（ms）*/
static bool long_press_triggered = false;   /* 长按是否已经触发 */

/* 从自定义按键码获取设备编号（1/2/3）*/
static uint8_t get_device_idx(uint16_t keycode) {
    switch (keycode) {
        case BT_1: return 1;
        case BT_2: return 2;
        case BT_3: return 3;
        default: return 0;
    }
}

/* 获取某个按键的长按阈值（ms）*/
static uint32_t get_long_press_threshold(uint16_t keycode) {
    if (keycode == FACTORY_RESET) {
        return FACTORY_RESET_MS;
    }
    return BT_LONG_PRESS_MS;
}

/* ========================================================================
 * 携带模式（Mobile Mode）
 *
 * 长按 MOBILE_MODE 键3秒切换：
 *   进入携带模式：锁定键盘（只有模式键有效）+ 连接间隔固定2000ms + LED1熄灭
 *   退出携带模式：解锁键盘 + 恢复100ms连接间隔 + LED1恢复正常
 *
 * 防重复触发（方案A）：一次按下只触发一次，必须松开再按才能再次触发
 *
 * LED提示：
 *   模式键按下：LED1亮
 *   模式键松开（未触发）：LED1灭，闪3下（亮1秒灭0.5秒×3）提示失败
 *   达到3秒触发：闪3下提示成功
 * ======================================================================== */

#define MOBILE_MODE_LONG_PRESS_MS  3000  /* 携带模式长按阈值：3秒 */
#define MOBILE_CONN_INTERVAL       1600  /* 携带模式连接间隔：2000ms = 1600×1.25ms */

static bool mobile_mode_active = false;     /* 是否在携带模式 */
static bool mobile_key_pressed = false;      /* 模式键是否按下 */
static uint32_t mobile_key_press_time = 0;   /* 模式键按下时间 */
static bool mobile_triggered = false;         /* 本次按下是否已经触发（防重复）*/

/* LED闪烁3下状态机（亮1秒灭0.5秒×3）*/
static uint8_t flash_count = 0;               /* 剩余闪烁次数，0=不闪 */
static bool flash_led_on = false;              /* 当前闪烁状态：亮/灭 */
static uint32_t flash_next_switch_time = 0;    /* 下次状态切换时间 */

/* 启动闪烁3下（LED0 是连接状态灯，由 led_indicator 管理）*/
static void mobile_start_flash(void)
{
    flash_count = 3;
    flash_led_on = true;
    flash_next_switch_time = timer_read32() + 1000;  /* 亮1秒 */
}

/* 处理闪烁状态机（主循环里调用）— LED0 是连接状态灯，由 led_indicator 管理 */
static void mobile_process_flash(void)
{
    if (flash_count == 0) return;

    uint32_t now = timer_read32();
    if ((int32_t)(now - flash_next_switch_time) < 0) return;

    if (flash_led_on) {
        /* 亮→灭 */
        flash_led_on = false;
        flash_next_switch_time = now + 500;  /* 灭0.5秒 */
    } else {
        /* 灭→亮，次数减1 */
        flash_count--;
        if (flash_count > 0) {
            flash_led_on = true;
            flash_next_switch_time = now + 1000;  /* 亮1秒 */
        }
    }
}

/* 切换携带模式 */
static void mobile_toggle_mode(void)
{
    mobile_mode_active = !mobile_mode_active;
    power_set_mobile_mode(mobile_mode_active);

    if (mobile_mode_active) {
        /* 进入携带模式：连接间隔固定2000ms */
        ble_set_conn_interval_raw(MOBILE_CONN_INTERVAL);
    } else {
        /* 退出携带模式：恢复100ms连接间隔 */
        ble_request_conn_interval_force(0);
    }

    /* LED0 是连接状态灯，由 led_indicator 管理 */
    mobile_start_flash();
}

/* 恢复出厂设置：清除蓝牙配对 + 重置 EEPROM + 软重启 */
static void do_factory_reset(void) {
    /* 1. 清除所有蓝牙配对 */
    ble_clear_bonds();

    /* 2. 重置 EEPROM 到默认值（键位配置等）*/
    eeconfig_init();

    /* LED0 是连接状态灯，恢复出厂设置不控制 LED */

    /* 3. 软重启 — CH585 系统复位
     * 延时一小段时间让 EEPROM 写入完成，然后复位
     */
    for (volatile int i = 0; i < 1000000; i++) {}
    PFIC_SystemReset();
}

/* ========================================================================
 * 按键处理 — 自定义按键码的长短按
 * ======================================================================== */

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    /* 通知电源管理有用户活动 */
    if (record->event.pressed) {
        power_notify_activity();
    }

    /* Vial 自定义按键码兼容处理（7个自定义按键）*/
    if ((keycode >> 8) == 0x7E && (keycode & 0xFF) < 7) {
        keycode = QK_USER_0 + (keycode & 0xFF);
    }

    /* 携带模式下锁定键盘：只有 MOBILE_MODE 键有效 */
    if (mobile_mode_active && keycode != MOBILE_MODE) {
        return false;
    }

    /* 只处理自定义按键码，其他按键正常传递 */
    if (keycode < BT_1 || keycode > MOBILE_MODE) {
        return true;
    }

    if (record->event.pressed) {
        /* 按键按下 */
        if (keycode == BT_CLEAR) {
            ble_clear_bonds();
            return false;
        }

        if (keycode == WIRED_MODE) {
            enter_wired_mode();
            return false;
        }

        if (keycode == MOBILE_MODE) {
            /* 携带模式键：记录按下时间，开始检测长按 */
            mobile_key_pressed = true;
            mobile_key_press_time = timer_read32();
            mobile_triggered = false;
            return false;
        }

        /* BT_1/BT_2/BT_3 / FACTORY_RESET：记录按下时间，开始检测长按 */
        long_press_keycode = keycode;
        long_press_start_time = timer_read32();
        long_press_triggered = false;
    } else {
        /* 按键释放 */
        if (keycode == BT_CLEAR || keycode == WIRED_MODE) {
            return false;
        }

        if (keycode == MOBILE_MODE) {
            /* 携带模式键松开 */
            mobile_key_pressed = false;
            if (!mobile_triggered && flash_count == 0) {
                /* 没触发过长按：闪3下提示失败 */
                mobile_start_flash();
            }
            return false;
        }

        if (long_press_keycode == keycode) {
            if (!long_press_triggered) {
                /* 没有触发过长按 → 短按 */
                if (keycode == FACTORY_RESET) {
                    /* FACTORY_RESET 短按不做任何事 */
                } else {
                    /* BT_1/BT_2/BT_3 短按：切换设备 */
                    uint8_t dev = get_device_idx(keycode);
                    if (dev > 0) {
                        ble_switch_device(dev);
                    }
                }
            }
            long_press_keycode = 0;
            long_press_triggered = false;
        }
    }

    return false;
}

/* ========================================================================
 * 主循环轮询 — 检测长按超时
 * ======================================================================== */

void housekeeping_task_user(void) {
    /* 检测蓝牙按键长按超时 */
    if (long_press_keycode != 0 && !long_press_triggered) {
        uint32_t threshold = get_long_press_threshold(long_press_keycode);
        uint32_t elapsed = timer_read32() - long_press_start_time;
        if (elapsed >= threshold) {
            if (long_press_keycode == FACTORY_RESET) {
                do_factory_reset();
            } else {
                uint8_t dev = get_device_idx(long_press_keycode);
                if (dev > 0) {
                    ble_pair_device(dev);
                }
            }
            long_press_triggered = true;
        }
    }

    /* 检测携带模式键长按超时（3秒）*/
    if (mobile_key_pressed && !mobile_triggered) {
        uint32_t elapsed = timer_read32() - mobile_key_press_time;
        if (elapsed >= MOBILE_MODE_LONG_PRESS_MS) {
            /* 达到3秒：切换携带模式，闪3下提示 */
            mobile_toggle_mode();
            mobile_triggered = true;  /* 本次按下不再触发（防重复）*/
        }
    }

    /* 处理LED闪烁3下状态机 */
    mobile_process_flash();

    /* 重试待发送的HID报告（非阻塞重试）*/
    ble_retry_pending_report();
}

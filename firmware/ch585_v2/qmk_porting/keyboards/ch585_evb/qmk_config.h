/*
 * CH585 EVB 板级 QMK 配置
 * 引脚定义与 M0 裸机测试固件一致（已实测）。
 */

#pragma once

/* USB Device descriptor parameter */
#define VENDOR_ID    0xCAFE
#define PRODUCT_ID   0x5850
#define DEVICE_VER   0x0001
#define MANUFACTURER NookCraftLab
#define PRODUCT      CH585_Test

/* 矩阵：2 行 x 4 列（8个按键，常规矩阵扫描，杜邦线短接测试）
 * 行(row)：PA4(P2-1), PA1(P2-5)
 * 列(col)：PB0(P3-7), PB2(P3-9), PB4(P3-11), PB6(P3-13)
 * 二极管方向：COL2ROW（列到行）
 */
#define MATRIX_ROWS 2
#define MATRIX_COLS 4

#define MATRIX_ROW_PINS         \
    {                           \
        A4, A1                  \
    }
#define MATRIX_COL_PINS         \
    {                           \
        B0, B2, B4, B6          \
    }

#define DIODE_DIRECTION  COL2ROW
#define BOOTMAGIC_ROW    0
#define BOOTMAGIC_COLUMN 0
#define HOLD_ON_OTHER_KEY_PRESS

#define EARLY_INIT_PERFORM_BOOTLOADER_JUMP FALSE

#ifdef ENCODER_ENABLE
/* 编码器：A 相 PB5，B 相 PB6 */
#define ENCODER_A_PINS \
    {                  \
        B5             \
    }
#define ENCODER_B_PINS \
    {                  \
        B6             \
    }
#define ENCODER_RESOLUTION 4
#endif

/* Set 0 if debouncing isn't needed */
#define DEBOUNCE 10

/* disable debug print */
// #define NO_DEBUG
// #define NO_PRINT

/* Vial: 8字节唯一键盘ID（CH585EVB 的 ASCII） */
#define VIAL_KEYBOARD_UID {0x43, 0x48, 0x35, 0x38, 0x35, 0x45, 0x56, 0x42}

/* Vial: 默认解除锁定（不需要按组合键解锁，用户可直接修改键位）*/
#define VIAL_INSECURE

/* Vial: 解锁组合键（VIAL_INSECURE 模式下不使用，但保留定义避免编译错误） */
#define VIAL_UNLOCK_COMBO_ROWS {0, 1}
#define VIAL_UNLOCK_COMBO_COLS {0, 3}

/* Vial: 默认 16 层（QMK 默认 4 层，这里扩展到 16 层） */
#define DYNAMIC_KEYMAP_LAYER_COUNT 16

/* ========================================================================
 * Vial 高阶功能配置（界面可直接配置）
 * ======================================================================== */

/* 核心功能开关 — 确保 vial.h 能检测到并自动开启 VIAL_xxx_ENABLE */
#define TAP_DANCE_ENABLE
#define VIAL_TAP_DANCE_ENABLE
#define COMBO_ENABLE
#define VIAL_COMBO_ENABLE
#define KEY_OVERRIDE_ENABLE
#define VIAL_KEY_OVERRIDE_ENABLE
#define REPEAT_KEY_ENABLE
#define VIAL_ALT_REPEAT_KEY_ENABLE

/* Tap Dance 按键舞蹈：按1次/2次/3次/长按触发不同功能 */
#define VIAL_TAP_DANCE_COUNT 16
#define TAPPING_TERM 200
#define TAPPING_TERM_PER_KEY

/* Combo 组合键：同时按多个键触发一个功能 */
#define VIAL_COMBO_COUNT 16
#define COMBO_TERM 50

/* Key Override 按键覆盖：修饰键+普通键的组合重映射 */
#define VIAL_KEY_OVERRIDE_COUNT 16

/* ========================================================================
 * QMK 原生功能参数配置
 * ========================================================================
 * 注意：以下参数只有在 keymaps/default/rules.cmake 里把对应功能的
 * 开关改成 ON 之后才会生效。功能关闭时，这些参数定义了也不会被用到。
 * 想开启某个功能，去 rules.cmake 里找对应的 set(XXX_ENABLE OFF ...) 改成 ON
 * ======================================================================== */

/* --- One Shot 一次性修饰键 --- */
/* 按一下修饰键（Ctrl/Shift/Alt），它会保持到下一个普通按键，然后自动松开 */
#define ONESHOT_TAP_TOGGLE 5       /* 连按5次切换该修饰键的永久锁定状态 */
#define ONESHOT_TIMEOUT 5000        /* 一次性修饰键超时时间（毫秒），超时自动取消 */

/* --- Leader 引导键 --- */
/* 按一下 Leader 键，然后输入序列（如 L+B+T）触发一个功能，用少量按键扩展大量功能 */
#define LEADER_TIMEOUT 300           /* 引导序列超时时间（毫秒），超时取消序列 */
#define LEADER_PER_KEY_TIMING        /* 每个键单独计时，输入慢也不会超时 */

/* --- Auto Shift 自动换挡 --- */
/* 长按字母键自动输出大写，短按输出小写，可以省掉 Shift 键 */
#define AUTO_SHIFT_TIMEOUT 150       /* 长按多久算大写（毫秒） */
#define NO_AUTO_SHIFT_SPECIAL        /* 特殊键（F键/方向键等）不触发自动大写 */
#define NO_AUTO_SHIFT_NUMERIC        /* 数字键不触发自动大写 */

/* --- Space Cadet 太空学员 --- */
/* 左 Shift 单击输出 ( ，右 Shift 单击输出 ) ，长按还是 Shift */
/* 需要键盘上有左右两个 Shift 键才有意义 */

/* --- Caps Word 大写单词 --- */
/* 双击 Shift 后，下一个单词自动大写，输完空格/标点后自动恢复小写 */
#define BOTH_SHIFTS_TURNS_ON_CAPS_WORD  /* 同时按左右Shift触发大写单词模式 */
#define CAPS_WORD_IDLE_TIMEOUT 5000     /* 大写单词模式空闲超时（毫秒），超时自动退出 */

/* --- Secure 安全锁定 --- */
/* 锁定键盘，需要输入特定解锁序列才能使用，适合蓝牙键盘离开座位时锁键盘 */
#define SECURE_UNLOCK_TIMEOUT 5000   /* 解锁输入超时（毫秒），超时取消解锁 */
#define SECURE_IDLE_TIMEOUT 60000    /* 空闲多久自动锁定（毫秒），设0禁用自动锁 */

/* --- Achordion 和弦防误触 --- */
/* 修饰键需要另一只手的键先按时才生效，防止单手误触修饰键 */
/* 主要适合家用大键盘，小配列意义不大 */
#define ACHORDION_STREAK              /* 启用 Streak 模式（连续同手按键不触发） */

/* --- Sentence Case 句首大写 --- */
/* 检测到句号+空格后，下一个字母自动大写，打字体验优化 */

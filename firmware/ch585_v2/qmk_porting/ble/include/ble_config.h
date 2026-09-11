/*
 * ble_config.h — BLE 蓝牙配置
 *
 * 本文件定义蓝牙相关的所有配置参数，每项都有注释说明用途。
 * 修改参数只需要改这里，不用动逻辑代码。
 */

#ifndef __BLE_CONFIG_H
#define __BLE_CONFIG_H

#include "config.h"

/* ========================================================================
 * 设备信息配置
 * ======================================================================== */

/* 蓝牙广播设备名 — 手机/电脑搜索蓝牙时显示的名字 */
#define BLE_DEVICE_NAME             "CH585 Keyboard"

/* 设备信息服务里的厂商名 */
#define BLE_MANUFACTURER_NAME       "NookCraftLab"

/* 设备信息服务里的型号 */
#define BLE_MODEL_NUMBER            "CH585-EVB"

/* ========================================================================
 * 连接参数配置
 * ======================================================================== */

/* 从机延迟 — 0 表示每个连接事件都响应，延迟最低但耗电 */
#define BLE_SLAVE_LATENCY           0

/* 监控超时（单位：10ms）— 500 = 5秒没响应就断开
 * 必须大于 (1 + slave_latency) × max_connection_interval
 * 1000ms间隔时：(1+0)×1000ms = 1000ms < 5000ms，安全
 */
#define BLE_CONN_TIMEOUT            500

/* 低延迟模式连接间隔（单位：1.25ms）— 30ms = 24 */
#define BLE_LOW_LATENCY_INTERVAL    24

/* ========================================================================
 * 阶梯式连接间隔（睡眠时动态调整）
 * 档位0~3 对应 100/300/500/1000ms
 * 时间阈值：0~30/30~90/90~180/180+分钟
 * 连接间隔单位：1.25ms（100ms=80, 300ms=240, 500ms=400, 1000ms=800）
 * ======================================================================== */

/* 档位数量 */
#define BLE_CONN_INTERVAL_LEVELS    4

/* 各档位的最小/最大连接间隔（单位：1.25ms）*/
#define BLE_CONN_INTERVAL_L0_MIN    80    /* 100ms */
#define BLE_CONN_INTERVAL_L0_MAX    80
#define BLE_CONN_INTERVAL_L1_MIN    240   /* 300ms */
#define BLE_CONN_INTERVAL_L1_MAX    240
#define BLE_CONN_INTERVAL_L2_MIN    400   /* 500ms */
#define BLE_CONN_INTERVAL_L2_MAX    400
#define BLE_CONN_INTERVAL_L3_MIN    800   /* 1000ms */
#define BLE_CONN_INTERVAL_L3_MAX    800

/* 各档位的睡眠时长阈值（毫秒）— 达到此时长切换到对应档位 */
#define BLE_CONN_INTERVAL_L0_TIME   (30 * 60 * 1000)   /* 30分钟 */
#define BLE_CONN_INTERVAL_L1_TIME   (90 * 60 * 1000)   /* 90分钟 */
#define BLE_CONN_INTERVAL_L2_TIME   (180 * 60 * 1000)  /* 180分钟 */
/* L3 是 L2_TIME 以上，不需要额外阈值 */

/* 兼容旧代码：日常模式用档位0（100ms）*/
#define BLE_MIN_CONN_INTERVAL       BLE_CONN_INTERVAL_L0_MIN
#define BLE_MAX_CONN_INTERVAL       BLE_CONN_INTERVAL_L0_MAX

/* ========================================================================
 * 广播配置
 * ======================================================================== */

/* 广播间隔（单位：0.625ms）— 100ms = 160，广播越快越耗电但连接越快 */
#define BLE_ADV_INTERVAL            160

/* 广播超时时间（秒）— 0 表示一直广播，不超时 */
#define BLE_ADV_TIMEOUT             0

/* ========================================================================
 * 断开状态阶梯式广播频率
 * 刚断开高频广播（快速重连），然后逐步降低（省电）
 * 广播间隔单位：0.625ms（100ms=160, 500ms=800, 2s=3200）
 * ======================================================================== */

/* 断开0~30秒：高频广播，快速重连 */
#define BLE_ADV_FAST_INTERVAL       160   /* 100ms */
#define BLE_ADV_FAST_TIME           (30 * 1000)  /* 30秒 */

/* 断开30秒~5分钟：中频广播，平衡重连速度和功耗 */
#define BLE_ADV_MID_INTERVAL        800   /* 500ms */
#define BLE_ADV_MID_TIME            (5 * 60 * 1000)  /* 5分钟 */

/* 断开5分钟以上：低频广播，省电为主（Mac睡眠唤醒也能搜到） */
#define BLE_ADV_LOW_INTERVAL        3200  /* 2000ms */

/* ========================================================================
 * 配对/绑定配置
 * ======================================================================== */

/* 配对模式 — WAIT_FOR_REQ 表示等待主机发起配对请求 */
#define BLE_PAIRING_MODE            GAPBOND_PAIRING_MODE_WAIT_FOR_REQ

/* 是否启用 MITM 保护 — FALSE 表示不需要密码，简单配对（Just Works） */
#define BLE_MITM_MODE               FALSE

/* 是否启用绑定（Bonding）— TRUE 表示配对后保存密钥，下次自动重连 */
#define BLE_BONDING_MODE            TRUE

/* IO 能力 — NO_INPUT_NO_OUTPUT 表示没有输入输出设备，用 Just Works 配对 */
#define BLE_IO_CAPABILITIES         GAPBOND_IO_CAP_NO_INPUT_NO_OUTPUT

/* 默认配对密码 — MITM 模式下用，我们用 Just Works 所以不用 */
#define BLE_DEFAULT_PASSCODE        0

/* ========================================================================
 * HID 配置
 * ======================================================================== */

/* HID 键盘输入报告长度 — 标准 8 字节（1修饰键 + 1保留 + 6键码） */
#define HID_KEYBOARD_IN_RPT_LEN     8

/* HID LED 输出报告长度 — 1 字节（Caps Lock/Num Lock/Scroll Lock 状态） */
#define HID_LED_OUT_RPT_LEN         1

/* HID 空闲超时（ms）— 和官方 HID_Keyboard 示例一致用 60000，0=不超时 */
#define HID_IDLE_TIMEOUT            60000

/* ========================================================================
 * 电池服务配置
 * ======================================================================== */

/* 电池电量临界值（百分比）— 低于此值触发低电量提醒 */
#define BATT_CRITICAL_LEVEL         6

/* 初始电池电量（百分比）— 评估板没有电池检测，先用固定值 */
#define BATT_INITIAL_LEVEL          100

/* ========================================================================
 * 内存配置
 * ======================================================================== */

/* BLE 协议栈堆内存大小（字节）— 最小 6K，HID 键盘用 6K 足够 */
#ifndef BLE_MEMHEAP_SIZE
#define BLE_MEMHEAP_SIZE            (1024 * 6)
#endif

/* ========================================================================
 * 发射功率配置
 * ======================================================================== */

/* 蓝牙发射功率 — 0dBm，距离和功耗的平衡点 */
#ifndef BLE_TX_POWER
#define BLE_TX_POWER                LL_TX_PWR_0_DBM
#endif

#endif /* __BLE_CONFIG_H */

/*
 * ble_app.c — BLE 蓝牙应用层核心实现
 *
 * 基于沁恒官方 HID_Keyboard 示例移植，适配 QMK 主循环。
 * 负责：协议栈初始化、GAP 事件处理、HID 报告发送、LED 状态联动。
 */

#include "ble_app.h"
#include "ble_config.h"
#include "led_indicator.h"
#include "CONFIG.h"
#include "HAL.h"
#include "hiddev.h"
#include "hidkbdservice.h"
#include "battservice.h"
#include "devinfoservice.h"
#include "hidkbd.h"
#include "CH58xBLE_LIB.h"
#include "eeprom.h"  /* EEPROM 读写 — 设备绑定表持久化 */
#include "timer.h"   /* timer_read32 — 超时判断 */
#include "power_management.h"  /* 有线模式下不进睡眠，切换蓝牙时清除标志 */

/* 有线模式激活标志 — 定义在 ch585_evb.c
 * 有线模式下，蓝牙状态变化不应覆盖 LED 状态（LED 常亮表示有线）
 */
extern bool wired_mode_active;

/* ========================================================================
 * 内部状态变量
 * ======================================================================== */

static uint8_t ble_task_id = INVALID_TASK_ID;
static uint16_t conn_handle = GAP_CONNHANDLE_INIT;
static ble_state_t current_state = BLE_STATE_IDLE;

/* ========================================================================
 * 三设备切换 — 绑定表（3个槽位，EEPROM持久化）
 * ======================================================================== */

#define BLE_MAX_DEVICES         3       /* 最多3个设备槽位 */
#define BLE_BOND_TABLE_MAGIC    0xA5    /* 魔数，判断 EEPROM 数据是否有效 */
#define BLE_BOND_TABLE_EEPROM_ADDR  0x400  /* EEPROM 存储起始地址
                                               * 必须避开 QMK/Vial 动态键位映射区域：
                                               *   动态键位映射起始 ≈ 0x19
                                               *   动态键位映射大小 = 16层 × 2行 × 4列 × 2字节 = 256字节
                                               *   动态键位映射结束 ≈ 0x119
                                               *   后续还有 TapDance/Combo/KeyOverride/Macros 等区域
                                               * 0x400(1024) 足够靠后，确保不与任何 QMK/Vial 区域重叠 */
#define BLE_RECONNECT_TIMEOUT   30000   /* 回连总超时：30秒（定向广播1.28秒+普通广播约28秒） */
#define BLE_PAIRING_TIMEOUT     180000  /* 配对超时：3分钟（量产键盘标准） */
#define BLE_DIRECTED_ADV_TIMEOUT 1500   /* 定向广播协议栈超时1.28秒，留余量1.5秒后转普通广播 */

/* 单个设备的绑定信息 */
typedef struct {
    uint8_t addr_type;              /* 地址类型：0=公共地址，1=随机地址 */
    uint8_t bd_addr[6];             /* 设备 BD_ADDR（6字节） */
    uint8_t paired;                  /* 是否已配对：0=未配对，1=已配对 */
} ble_device_info_t;

/* 完整的设备绑定表（存在 EEPROM 里）
 * 同时存储模式记忆，上电时恢复到上次的模式和设备
 */
typedef struct {
    uint8_t magic;                   /* 魔数 BLE_BOND_TABLE_MAGIC */
    uint8_t current_device;          /* 当前选中的设备槽位（1-3） */
    uint8_t last_mode_wired;         /* 上次模式：0=蓝牙，1=有线（上电恢复用） */
    uint8_t reserved;                /* 保留字节，对齐用 */
    ble_device_info_t devices[BLE_MAX_DEVICES];  /* 3个设备槽位 */
} ble_bond_table_t;

static ble_bond_table_t bond_table;  /* 内存中的设备绑定表 */
static uint32_t state_enter_time = 0;  /* 进入当前状态的时间（用于超时判断） */

/* 待切换/待配对目标（断开完成后执行）*/
static uint8_t pending_switch_device = 0;
static uint8_t pending_pair_device = 0;

/* HID 报告去重：上一次成功发送的报告
 * 大连接间隔(500/1000ms)下，矩阵扫描重复生成相同报告，
 * HidDev_Report 内部缓存排队，导致连击（按1次输出10多个字符）。
 */
static uint8_t last_sent_report[HID_KEYBOARD_IN_RPT_LEN] = {0};
static bool last_sent_report_valid = false;

/* 前向声明 */
static void bond_table_load(void);
static void bond_table_save(void);
static bool bond_table_is_paired(uint8_t device_idx);
static void bond_table_set_device(uint8_t device_idx, uint8_t addr_type, uint8_t *bd_addr);
static void bond_table_clear_all(void);
static void ble_start_directed_advertising(uint8_t addr_type, uint8_t *bd_addr);
static void ble_start_general_advertising(void);
static void ble_check_timeout(void);
static void ble_set_device_name(uint8_t device_idx);

/* ========================================================================
 * 广播数据
 *
 * 广播包内容：标志位 + 外观（HID键盘） + HID 服务 UUID
 * 扫描响应包：设备名 + 连接间隔范围 + 电池服务 UUID + 发射功率
 * ======================================================================== */

/* 广播数据 — 包含标志位和外观（和官方 HID_Keyboard 示例完全一致） */
static uint8_t advertData[] = {
    /* 标志位 */
    0x02,                           /* 长度 */
    GAP_ADTYPE_FLAGS,               /* 类型：标志位 */
    GAP_ADTYPE_FLAGS_LIMITED | GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED,  /* 有限可发现，不支持 BR/EDR（和官方一致） */

    /* 外观 */
    0x03,                           /* 长度 */
    GAP_ADTYPE_APPEARANCE,          /* 类型：外观 */
    LO_UINT16(GAP_APPEARE_HID_KEYBOARD),  /* HID 键盘外观低字节 */
    HI_UINT16(GAP_APPEARE_HID_KEYBOARD),  /* HID 键盘外观高字节 */
};

/* 不同槽位不同设备名：CH585_Test_BLE1/2/3 */
static void ble_set_device_name(uint8_t device_idx)
{
    static const char *device_names[BLE_MAX_DEVICES] = {
        "CH585_Test_BLE1",
        "CH585_Test_BLE2",
        "CH585_Test_BLE3",
    };
    if (device_idx < 1 || device_idx > BLE_MAX_DEVICES) {
        return;
    }

    const char *name = device_names[device_idx - 1];
    uint8_t name_len = (uint8_t)strlen(name);

    /* 构建扫描响应数据 */
    uint8_t scan_rsp[31];
    uint8_t offset = 0;

    /* 完整本地名称 */
    scan_rsp[offset++] = name_len + 1;
    scan_rsp[offset++] = GAP_ADTYPE_LOCAL_NAME_COMPLETE;
    memcpy(&scan_rsp[offset], name, name_len);
    offset += name_len;

    /* 从机连接间隔范围 */
    scan_rsp[offset++] = 0x05;
    scan_rsp[offset++] = GAP_ADTYPE_SLAVE_CONN_INTERVAL_RANGE;
    scan_rsp[offset++] = LO_UINT16(BLE_MIN_CONN_INTERVAL);
    scan_rsp[offset++] = HI_UINT16(BLE_MIN_CONN_INTERVAL);
    scan_rsp[offset++] = LO_UINT16(BLE_MAX_CONN_INTERVAL);
    scan_rsp[offset++] = HI_UINT16(BLE_MAX_CONN_INTERVAL);

    /* 服务 UUID（HID + 电池）*/
    scan_rsp[offset++] = 0x05;
    scan_rsp[offset++] = GAP_ADTYPE_16BIT_MORE;
    scan_rsp[offset++] = LO_UINT16(HID_SERV_UUID);
    scan_rsp[offset++] = HI_UINT16(HID_SERV_UUID);
    scan_rsp[offset++] = LO_UINT16(BATT_SERV_UUID);
    scan_rsp[offset++] = HI_UINT16(BATT_SERV_UUID);

    /* 注意：发射功率字段已去掉（可选字段，不影响功能，腾出3字节给设备名）*/

    /* 更新 GAPRole 扫描响应数据（广播时生效）*/
    GAPRole_SetParameter(GAPROLE_SCAN_RSP_DATA, offset, scan_rsp);

    /* 更新 GGS 服务设备名（连接后主机读取到的名字）*/
    GGS_SetParameter(GGS_DEVICE_NAME_ATT, name_len, (void *)name);
}

/* ========================================================================
 * 不同槽位不同蓝牙地址
 *
 * 为什么需要不同地址：
 *   从设备1切换到设备2时，设备2启动普通可连接广播。
 *   但已配对过设备1的电脑（Windows/Mac）记录的是蓝牙地址，看到同地址在广播
 *   就会主动重新连接，导致"切换到设备2但自动回连到电脑"，手机搜不到。
 *
 * 解决方案：每个槽位用不同的静态随机地址（Static Random Address）。
 *   电脑配对过设备1的地址A，看到设备2的地址B（没配对过），不会主动连接，
 *   手机就能正常搜到并配对。这也是量产键盘的标准做法。
 *
 * 静态随机地址要求：
 *   - 第一个字节最高两位必须是 11（0xC0~0xFE，不能是 0xFF）
 *   - 不能全 0 或全 1
 *   - 每个槽位用一个固定地址，配对信息和地址一一对应
 *
 * 注意：地址必须在断开连接后、启动广播前设置。
 *       在我们的 pending 机制里，GAPROLE_WAITING 执行切换时设置是合适的。
 * ======================================================================== */

/* 三个槽位的静态随机地址（第一个字节 0xC1=11000001，最高两位11，符合要求）*/
static const uint8_t device_addresses[BLE_MAX_DEVICES][B_ADDR_LEN] = {
    {0xC1, 0x58, 0x35, 0x00, 0x00, 0x01},  /* 设备1 */
    {0xC1, 0x58, 0x35, 0x00, 0x00, 0x02},  /* 设备2 */
    {0xC1, 0x58, 0x35, 0x00, 0x00, 0x03},  /* 设备3 */
};

/* 根据设备槽位设置蓝牙地址（静态随机地址）*/
static void ble_set_device_address(uint8_t device_idx)
{
    if (device_idx < 1 || device_idx > BLE_MAX_DEVICES) {
        return;
    }

    /* 设置静态随机地址 — 必须在广播启动前调用 */
    GAP_ConfigDeviceAddr(ADDRTYPE_STATIC, (uint8_t *)device_addresses[device_idx - 1]);
}

/* ========================================================================
 * HID 设备配置
 * ======================================================================== */

static hidDevCfg_t ble_hid_cfg = {
    HID_IDLE_TIMEOUT,               /* Idle 超时（毫秒），0=禁用 */
    HID_FEATURE_FLAGS,              /* HID 特性标志 */
};

/* ========================================================================
 * 内部函数声明
 * ======================================================================== */

static uint16_t ble_process_event(uint8_t task_id, uint16_t events);
static void ble_process_tmos_msg(tmos_event_hdr_t *pMsg);
static uint8_t ble_hid_report_cb(uint8_t id, uint8_t type, uint16_t uuid,
                                  uint8_t oper, uint16_t *pLen, uint8_t *pData);
static void ble_hid_event_cb(uint8_t evt);
static void ble_state_cb(gapRole_States_t newState, gapRoleEvent_t *pEvent);

/* ========================================================================
 * HID 设备回调函数表
 *
 * hidDevCB_t 结构包含四个回调：
 * - pfnRptCB: 报告读写回调（主机读/写 HID 报告时调用）
 * - pfnEvtCB: HID 事件回调（挂起/退出挂起/设置启动模式/设置报告模式）
 * - pfnPasscodeCB: 配对密码回调（需要 MITM 时用）
 * - pfnStateCB: GAP 状态变化回调（连接/断开/广播等）
 * ======================================================================== */

static hidDevCB_t ble_hid_cbs = {
    ble_hid_report_cb,    /* 报告回调 */
    ble_hid_event_cb,     /* 事件回调 */
    NULL,                  /* 密码回调（不需要 MITM，设为 NULL） */
    ble_state_cb,          /* 状态变化回调 */
};

/* ========================================================================
 * BLE 中断处理函数
 *
 * 注意：LLE_IRQHandler 已经在 BLE 协议栈库的 ble_task_scheduler.S 里定义了
 * （强符号，放在 .highcode 段，手动保存/恢复所有31个寄存器，读取
 * g_LLE_IRQLibHandlerLocation 函数指针间接调用，最后 mret 返回）。
 * 这里不能用 C 语言重复定义 LLE_IRQHandler，否则编译器只保存部分寄存器，
 * 导致 BLE 中断处理不正确，射频无法正常工作。
 *
 * BB_IRQHandler 启动文件里的弱定义是死循环，这里强定义覆盖，指向 BLE 协议栈库。
 * ======================================================================== */

void BB_IRQHandler(void)
{
    BB_IRQLibHandler();  /* BLE 基带中断 — 协议栈库处理 */
}

/* ========================================================================
 * 初始化
 * ======================================================================== */

void ble_app_init(void)
{
    /* BLE 协议栈初始化完成提示 — LED0 常亮2s后熄灭 */
    led_set_conn_state(LED_CONN_SUCCESS);

    /* 加载设备绑定表（3个槽位的配对信息 + 模式记忆，存在 EEPROM 里）*/
    bond_table_load();

    /* 注意：CH58X_BLEInit() 和 HAL_Init() 已经在 platform_setup() 里调用过了，
     * 这里不能重复调用，否则 BLE 协议栈会重复初始化，导致内存池混乱、广播无法发出。
     * 官方示例里这两个函数只在 main 函数里调用一次。
     */

    /* 1. 初始化 GAP 外设角色
     * 必须调用，否则无法作为从设备广播和连接
     */
    GAPRole_PeripheralInit();

    /* 2. 初始化 HID 设备服务
     * 必须调用，否则 HID 服务无法正常工作
     */
    HidDev_Init();

    /* 5. 注册 TMOS 任务事件处理函数 */
    ble_task_id = TMOS_ProcessEventRegister(ble_process_event);

    /* 4. 设置 GAP 外设角色参数 */
    {
        /* 初始化时不自动广播 — 上电恢复由 ble_restore_last_mode() 处理
         * 如果上次是蓝牙模式，ble_restore_last_mode() 会调用 ble_switch_device() 启动回连
         * 如果上次是有线模式，不启动广播
         */
        uint8_t initial_advertising_enable = FALSE;
        GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(uint8_t), &initial_advertising_enable);
        GAPRole_SetParameter(GAPROLE_ADVERT_DATA, sizeof(advertData), advertData);
        /* 扫描响应数据和 GGS 设备名由 ble_set_device_name() 动态设置（默认设备1）*/
        ble_set_device_name(1);
    }

    /* 5. GGS 设备名已经在 ble_set_device_name() 里设置了，这里不重复设置 */

    /* 6. 设置 GAP Bond Manager（配对管理器）参数 */
    {
        uint32_t passkey = BLE_DEFAULT_PASSCODE;
        uint8_t pairMode = BLE_PAIRING_MODE;
        uint8_t mitm = BLE_MITM_MODE;
        uint8_t ioCap = BLE_IO_CAPABILITIES;
        uint8_t bonding = BLE_BONDING_MODE;
        GAPBondMgr_SetParameter(GAPBOND_PERI_DEFAULT_PASSCODE, sizeof(uint32_t), &passkey);
        GAPBondMgr_SetParameter(GAPBOND_PERI_PAIRING_MODE, sizeof(uint8_t), &pairMode);
        GAPBondMgr_SetParameter(GAPBOND_PERI_MITM_PROTECTION, sizeof(uint8_t), &mitm);
        GAPBondMgr_SetParameter(GAPBOND_PERI_IO_CAPABILITIES, sizeof(uint8_t), &ioCap);
        GAPBondMgr_SetParameter(GAPBOND_PERI_BONDING_ENABLED, sizeof(uint8_t), &bonding);
    }

    /* 7. 设置电池服务临界电量 */
    {
        uint8_t critical = BATT_CRITICAL_LEVEL;
        Batt_SetParameter(BATT_PARAM_CRITICAL_LEVEL, sizeof(uint8_t), &critical);
    }

    /* 8. 添加 HID 键盘服务 */
    Hid_AddService();

    /* 9. 注册 HID 设备配置和回调 */
    HidDev_Register(&ble_hid_cfg, &ble_hid_cbs);

    /* 10. 设置启动事件（和官方 HID_Keyboard 示例完全一致）
     * 虽然我们的 START_DEVICE_EVT 处理函数里只清除事件（广播启动由 hiddev.c 内部处理），
     * 但官方示例也设置了这个事件，为完全对齐加上。
     */
    tmos_set_event(ble_task_id, START_DEVICE_EVT);

    current_state = BLE_STATE_IDLE;
    conn_handle = GAP_CONNHANDLE_INIT;
}

/* ========================================================================
 * TMOS 任务事件处理
 * ======================================================================== */

static uint16_t ble_process_event(uint8_t task_id, uint16_t events)
{
    /* 处理系统消息事件 */
    if (events & SYS_EVENT_MSG) {
        uint8_t *pMsg;
        if ((pMsg = tmos_msg_receive(ble_task_id)) != NULL) {
            ble_process_tmos_msg((tmos_event_hdr_t *)pMsg);
            tmos_msg_deallocate(pMsg);
        }
        return (events ^ SYS_EVENT_MSG);
    }

    /* 启动设备事件 — 启动广播由 hiddev.c 内部处理，这里只清除事件 */
    if (events & START_DEVICE_EVT) {
        return (events ^ START_DEVICE_EVT);
    }

    /* 参数更新事件 — 请求更新连接间隔 */
    if (events & START_PARAM_UPDATE_EVT) {
        if (current_state == BLE_STATE_CONNECTED) {
            GAPRole_PeripheralConnParamUpdateReq(conn_handle,
                BLE_MIN_CONN_INTERVAL,
                BLE_MAX_CONN_INTERVAL,
                BLE_SLAVE_LATENCY,
                BLE_CONN_TIMEOUT,
                ble_task_id);
        }
        return (events ^ START_PARAM_UPDATE_EVT);
    }

    return 0;  /* 未处理的事件 */
}

/* ========================================================================
 * TMOS 消息处理
 * ======================================================================== */

static void ble_process_tmos_msg(tmos_event_hdr_t *pMsg)
{
    /* 目前没有需要处理的自定义消息 */
    (void)pMsg;
}

/* ========================================================================
 * GAP 状态变化回调
 *
 * 当 BLE 连接状态变化时调用（广播开始、连接建立、连接断开等）
 * 参数：
 *   newState - 新状态（需要和 GAPROLE_STATE_ADV_MASK 掩码后判断）
 *   pEvent - 事件详情（包含连接句柄、断开原因等）
 * ======================================================================== */

static void ble_state_cb(gapRole_States_t newState, gapRoleEvent_t *pEvent)
{
    switch (newState & GAPROLE_STATE_ADV_MASK) {
        case GAPROLE_STARTED:
            /* 协议栈启动完成 — 设置默认设备1的蓝牙地址（静态随机地址）*/
            ble_set_device_address(1);
            break;

        case GAPROLE_ADVERTISING:
            /* 正在广播 */
            if (pEvent->gap.opcode == GAP_MAKE_DISCOVERABLE_DONE_EVENT) {
                current_state = BLE_STATE_ADVERTISING;
                if (!wired_mode_active) {
                    led_set_conn_state(LED_CONN_PAIRING);  /* 快闪表示广播中 */
                }
            }
            break;

        case GAPROLE_CONNECTED:
            /* 连接建立成功 */
            if (pEvent->gap.opcode == GAP_LINK_ESTABLISHED_EVENT) {
                gapEstLinkReqEvent_t *event = (gapEstLinkReqEvent_t *)pEvent;
                conn_handle = event->connectionHandle;
                current_state = BLE_STATE_CONNECTED;

                /* 重置 HID 报告去重状态：重新连接后第一次报告必须发送 */
                last_sent_report_valid = false;

                if (!wired_mode_active) {
                    led_set_conn_state(LED_CONN_SUCCESS);  /* 常亮2秒表示连接成功，然后自动熄灭 */
                }

                /* 保存对端 BD_ADDR 到当前设备槽位（用于后续回连）
                 * 注意：连接成功时就保存，不管配对是否完成。
                 * 配对完成后协议栈会自动保存密钥，我们只需要 BD_ADDR 用于定向广播回连。
                 */
                bond_table_set_device(bond_table.current_device, event->devAddrType, event->devAddr);

                /* 延迟请求更新连接参数（让主机先完成服务发现） */
                tmos_start_task(ble_task_id, START_PARAM_UPDATE_EVT, 12800);
            }
            break;

        case GAPROLE_CONNECTED_ADV:
            /* 连接中同时广播（多连接时） */
            break;

        case GAPROLE_WAITING:
            /* 断开连接 / 广播超时
             *
             * 优先级从高到低：
             * 1. 有待切换目标（pending_switch_device）：断开完成后执行切换
             * 2. 有待配对目标（pending_pair_device）：断开完成后进入配对模式
             * 3. 定向广播超时（RECONNECTING + GAP_LINK_ESTABLISHED_EVENT）：转普通广播
             * 4. 主动切换到回连模式，断开完成（RECONNECTING + GAP_LINK_TERMINATED_EVENT）：重启定向广播
             * 5. 主动切换到广播模式，断开完成（ADVERTISING）：重启普通广播
             * 6. 主动切换到配对模式，断开完成（PAIRING）：重启普通广播
             * 7. 被动断开（CONNECTED）：自动回连当前设备
             * 8. 主动切换中间态（SWITCHING）：什么都不做
             */
            conn_handle = GAP_CONNHANDLE_INIT;

            /* 情况1：有待切换目标 — 断开完成后执行切换 */
            if (pending_switch_device != 0) {
                uint8_t dev_idx = pending_switch_device;
                pending_switch_device = 0;
                pending_pair_device = 0;

                /* 设置该槽位的蓝牙地址（必须在启动广播前设置）*/
                ble_set_device_address(dev_idx);

                if (bond_table_is_paired(dev_idx)) {
                    ble_device_info_t *dev = &bond_table.devices[dev_idx - 1];
                    ble_start_directed_advertising(dev->addr_type, dev->bd_addr);
                } else {
                    current_state = BLE_STATE_ADVERTISING;
                    state_enter_time = timer_read32();
                    ble_start_general_advertising();
                    if (!wired_mode_active) {
                        led_set_conn_state(LED_CONN_PAIRING);
                    }
                }
                break;
            }

            /* 情况2：有待配对目标 — 断开完成后进入配对模式 */
            if (pending_pair_device != 0) {
                uint8_t dev_idx = pending_pair_device;
                pending_pair_device = 0;
                pending_switch_device = 0;

                /* 设置该槽位的蓝牙地址（必须在启动广播前设置）*/
                ble_set_device_address(dev_idx);

                current_state = BLE_STATE_PAIRING;
                state_enter_time = timer_read32();
                ble_start_general_advertising();
                led_set_conn_state(LED_CONN_PAIRING);
                break;
            }

            if (current_state == BLE_STATE_RECONNECTING &&
                pEvent->gap.opcode == GAP_LINK_ESTABLISHED_EVENT) {
                /* 情况3：定向广播超时 — 转普通可连接广播继续等（回连第二阶段） */
                current_state = BLE_STATE_RECONNECTING;
                state_enter_time = timer_read32();
                ble_start_general_advertising();
                if (!wired_mode_active) {
                    led_set_conn_state(LED_CONN_RECONNECTING);
                }
            } else if (current_state == BLE_STATE_RECONNECTING &&
                       pEvent->gap.opcode == GAP_LINK_TERMINATED_EVENT) {
                /* 回连模式断开完成 — 重启定向广播 */
                ble_device_info_t *dev = &bond_table.devices[bond_table.current_device - 1];
                ble_start_directed_advertising(dev->addr_type, dev->bd_addr);
            } else if (current_state == BLE_STATE_ADVERTISING) {
                /* 广播模式断开完成 — 重启普通广播 */
                ble_start_general_advertising();
                if (!wired_mode_active) {
                    led_set_conn_state(LED_CONN_PAIRING);
                }
            } else if (current_state == BLE_STATE_PAIRING) {
                /* 配对模式断开完成 — 重启普通广播 */
                ble_start_general_advertising();
                if (!wired_mode_active) {
                    led_set_conn_state(LED_CONN_PAIRING);
                }
            } else if (current_state == BLE_STATE_CONNECTED) {
                /* 情况7：被动断开 — 自动回连当前设备 */
                /* 设置当前设备的蓝牙地址（确保地址正确）*/
                ble_set_device_address(bond_table.current_device);

                if (bond_table_is_paired(bond_table.current_device)) {
                    ble_device_info_t *dev = &bond_table.devices[bond_table.current_device - 1];
                    ble_start_directed_advertising(dev->addr_type, dev->bd_addr);
                } else {
                    current_state = BLE_STATE_ADVERTISING;
                    state_enter_time = timer_read32();
                    ble_start_general_advertising();
                    if (!wired_mode_active) {
                        led_set_conn_state(LED_CONN_PAIRING);
                    }
                }
            }
            /* SWITCHING 状态 — 不做处理 */
            break;

        case GAPROLE_ERROR:
            /* 错误 */
            current_state = BLE_STATE_IDLE;
            if (!wired_mode_active) {
                led_set_conn_state(LED_CONN_IDLE);
            }
            break;

        default:
            break;
    }
}

/* ========================================================================
 * HID 事件回调
 *
 * 处理 HID 服务相关事件（挂起、退出挂起、设置启动模式、设置报告模式）
 * ======================================================================== */

static void ble_hid_event_cb(uint8_t evt)
{
    switch (evt) {
        case HID_DEV_SUSPEND_EVT:
            /* 主机请求挂起 — 可以进入低功耗模式 */
            break;

        case HID_DEV_EXIT_SUSPEND_EVT:
            /* 主机请求退出挂起 — 恢复正常工作 */
            break;

        case HID_DEV_SET_BOOT_EVT:
            /* 主机设置启动协议模式（Boot Protocol） */
            break;

        case HID_DEV_SET_REPORT_EVT:
            /* 主机设置报告协议模式（Report Protocol） */
            break;

        default:
            break;
    }
}

/* ========================================================================
 * HID 报告读写回调
 *
 * 当主机读/写 HID 报告时调用（通常是写 LED 状态：Caps Lock/Num Lock）
 * 参数：
 *   id - 报告 ID
 *   type - 报告类型（输入/输出/特性）
 *   uuid - 特征 UUID
 *   oper - 操作类型（读/写/使能通知/禁用通知）
 *   pLen - 数据长度指针
 *   pData - 数据指针
 * 返回：成功返回 SUCCESS，失败返回错误码
 * ======================================================================== */

static uint8_t ble_hid_report_cb(uint8_t id, uint8_t type, uint16_t uuid,
                                  uint8_t oper, uint16_t *pLen, uint8_t *pData)
{
    uint8_t status = SUCCESS;

    /* 写操作 — 主机写 HID 报告（通常是 LED 状态：Caps Lock/Num Lock） */
    if (oper == HID_DEV_OPER_WRITE) {
        if (uuid == REPORT_UUID) {
            /* 处理 LED 输出报告，忽略其他 */
            if (type == HID_REPORT_TYPE_OUTPUT) {
                /* 验证数据长度 */
                if (*pLen == HID_LED_OUT_RPT_LEN) {
                    status = SUCCESS;  /* 暂时不处理 LED 状态 */
                } else {
                    status = ATT_ERR_INVALID_VALUE_SIZE;
                }
            }
        }

        if (status == SUCCESS) {
            status = Hid_SetParameter(id, type, uuid, *pLen, pData);
        }
    }
    /* 读操作 — 主机读 HID 报告 */
    else if (oper == HID_DEV_OPER_READ) {
        status = Hid_GetParameter(id, type, uuid, pLen, pData);
    }
    /* 通知使能 — 主机使能通知（官方示例在这里启动自动发送报告的测试定时器，我们不需要） */
    else if (oper == HID_DEV_OPER_ENABLE) {
        /* 暂时不启动自动发送报告的定时器 */
    }

    return status;
}

/* ========================================================================
 * HID 报告发送（非阻塞重试）
 * ======================================================================== */

/* 尝试发送报告 */
static bool ble_send_report_internal(uint8_t *report)
{
    if (current_state != BLE_STATE_CONNECTED) {
        return false;
    }

    /* 去重：和上一次成功发送的报告完全相同则跳过
     * 避免大连接间隔下 HidDev_Report 缓存排队导致连击
     */
    if (last_sent_report_valid &&
        memcmp(report, last_sent_report, HID_KEYBOARD_IN_RPT_LEN) == 0) {
        return true;  /* 假装发送成功，实际跳过 */
    }

    uint8_t status = HidDev_Report(HID_RPT_ID_KEY_IN, HID_REPORT_TYPE_INPUT,
                                    HID_KEYBOARD_IN_RPT_LEN, report);
    if (status == SUCCESS) {
        /* 发送成功，更新上一次报告 */
        memcpy(last_sent_report, report, HID_KEYBOARD_IN_RPT_LEN);
        last_sent_report_valid = true;
        return true;
    }
    return false;
}

void ble_send_keyboard_report(uint8_t modifiers, uint8_t *keys)
{
    uint8_t report[HID_KEYBOARD_IN_RPT_LEN] = {0};
    report[0] = modifiers;
    if (keys != NULL) {
        for (int i = 0; i < 6; i++) {
            report[2 + i] = keys[i];
        }
    }

    /* 发送失败直接放弃，不保存到待重试队列
     * QMK 矩阵扫描持续运行(10ms一次)，下次扫描会重新发送最新报告
     * 避免待重试队列重复调用 HidDev_Report 导致缓存排队连击
     */
    ble_send_report_internal(report);
}

void ble_send_keyboard_release(void)
{
    uint8_t report[HID_KEYBOARD_IN_RPT_LEN] = {0};
    ble_send_report_internal(report);
}

/* ========================================================================
 * 状态查询和控制
 * ======================================================================== */

ble_state_t ble_get_state(void)
{
    return current_state;
}

bool ble_is_connected(void)
{
    return (current_state == BLE_STATE_CONNECTED);
}

uint16_t ble_get_conn_handle(void)
{
    return conn_handle;
}

void ble_start_advertising(void)
{
    if (current_state != BLE_STATE_CONNECTED) {
        uint8_t advertising_enable = TRUE;
        GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(uint8_t), &advertising_enable);
        current_state = BLE_STATE_ADVERTISING;
        led_set_conn_state(LED_CONN_PAIRING);
    }
}

void ble_disconnect(void)
{
    /* 只判断连接句柄是否有效 */
    if (conn_handle != GAP_CONNHANDLE_INIT) {
        GAPRole_TerminateLink(conn_handle);
    }
}

/* ========================================================================
 * 三设备切换
 * 绑定表存3个槽位（EEPROM持久化），短按切换/回连，长按配对，BT_CLEAR清除
 * ======================================================================== */

/* ------------------------------------------------------------------------
 * EEPROM 读写 — 设备绑定表持久化
 * ------------------------------------------------------------------------ */

/* 从 EEPROM 加载设备绑定表 */
static void bond_table_load(void)
{
    uint8_t *p = (uint8_t *)&bond_table;
    for (uint16_t i = 0; i < sizeof(ble_bond_table_t); i++) {
        p[i] = eeprom_read_byte(BLE_BOND_TABLE_EEPROM_ADDR + i);
    }
    /* 魔数不对说明是第一次使用或数据损坏，初始化空表 */
    if (bond_table.magic != BLE_BOND_TABLE_MAGIC) {
        memset(&bond_table, 0, sizeof(ble_bond_table_t));
        bond_table.magic = BLE_BOND_TABLE_MAGIC;
        bond_table.current_device = 1;  /* 默认设备1 */
        bond_table.last_mode_wired = 1; /* 默认有线模式（首次使用时 USB 连接着）*/
        bond_table_save();
    }
}

/* 保存设备绑定表到 EEPROM */
static void bond_table_save(void)
{
    uint8_t *p = (uint8_t *)&bond_table;
    for (uint16_t i = 0; i < sizeof(ble_bond_table_t); i++) {
        eeprom_update_byte(BLE_BOND_TABLE_EEPROM_ADDR + i, p[i]);
    }
}

/* 检查指定槽位是否已配对 */
static bool bond_table_is_paired(uint8_t device_idx)
{
    if (device_idx < 1 || device_idx > BLE_MAX_DEVICES) return false;
    return bond_table.devices[device_idx - 1].paired != 0;
}

/* 保存配对设备的 BD_ADDR 到指定槽位 */
static void bond_table_set_device(uint8_t device_idx, uint8_t addr_type, uint8_t *bd_addr)
{
    if (device_idx < 1 || device_idx > BLE_MAX_DEVICES) return;
    ble_device_info_t *dev = &bond_table.devices[device_idx - 1];
    dev->addr_type = addr_type;
    memcpy(dev->bd_addr, bd_addr, 6);
    dev->paired = 1;
    bond_table_save();
}

/* 清除所有槽位的配对信息 */
static void bond_table_clear_all(void)
{
    for (uint8_t i = 0; i < BLE_MAX_DEVICES; i++) {
        memset(&bond_table.devices[i], 0, sizeof(ble_device_info_t));
    }
    bond_table.current_device = 1;
    bond_table_save();
}

/* ------------------------------------------------------------------------
 * 定向广播 — 回连已配对主机
 * ------------------------------------------------------------------------ */

/* 开始定向广播（回连模式），只向指定 BD_ADDR 的设备广播 */
static void ble_start_directed_advertising(uint8_t addr_type, uint8_t *bd_addr)
{
    /* 设置定向广播的目标地址 */
    GAPRole_SetParameter(GAPROLE_ADV_DIRECT_TYPE, sizeof(uint8_t), &addr_type);
    GAPRole_SetParameter(GAPROLE_ADV_DIRECT_ADDR, 6, bd_addr);

    /* 设置广播类型为定向广播 */
    uint8_t adv_event_type = GAP_ADRPT_ADV_DIRECT_IND;  /* 定向可连接广播 */
    GAPRole_SetParameter(GAPROLE_ADV_EVENT_TYPE, sizeof(uint8_t), &adv_event_type);

    /* 开始广播 */
    uint8_t advertising_enable = TRUE;
    GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(uint8_t), &advertising_enable);

    current_state = BLE_STATE_RECONNECTING;
    state_enter_time = timer_read32();
    led_set_conn_state(LED_CONN_RECONNECTING);  /* 慢闪表示回连中 */
}

/* 开始普通广播（可被新设备搜索到） */
static void ble_start_general_advertising(void)
{
    /* 恢复普通广播类型 */
    uint8_t adv_event_type = GAP_ADRPT_ADV_IND;  /* 普通可连接广播 */
    GAPRole_SetParameter(GAPROLE_ADV_EVENT_TYPE, sizeof(uint8_t), &adv_event_type);

    /* 清除定向广播地址 */
    uint8_t zero_addr[6] = {0};
    GAPRole_SetParameter(GAPROLE_ADV_DIRECT_ADDR, 6, zero_addr);

    /* 开始广播 */
    uint8_t advertising_enable = TRUE;
    GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(uint8_t), &advertising_enable);
}

/* ------------------------------------------------------------------------
 * 超时检查 — 在主循环里调用
 * ------------------------------------------------------------------------ */

static void ble_check_timeout(void)
{
    uint32_t now = timer_read32();

    if (current_state == BLE_STATE_RECONNECTING) {
        /* 回连总超时：30秒没连上（定向广播1.28秒+普通广播约28秒）
         * 超时后停止广播，进入待机（用户按 BT 键可重新唤醒）
         */
        if (now - state_enter_time > BLE_RECONNECT_TIMEOUT) {
            ble_stop_advertising();
            current_state = BLE_STATE_STANDBY;
            if (!wired_mode_active) {
                led_set_conn_state(LED_CONN_IDLE);
            }
        }
    } else if (current_state == BLE_STATE_PAIRING) {
        /* 配对超时：3分钟没新设备配对
         * 退出配对模式：
         *   当前槽位有旧绑定 → 回连旧设备
         *   当前槽位无旧绑定 → 进入待机
         */
        if (now - state_enter_time > BLE_PAIRING_TIMEOUT) {
            ble_stop_advertising();
            if (bond_table_is_paired(bond_table.current_device)) {
                /* 有旧绑定 → 回连旧设备 */
                ble_device_info_t *dev = &bond_table.devices[bond_table.current_device - 1];
                ble_start_directed_advertising(dev->addr_type, dev->bd_addr);
            } else {
                /* 无旧绑定 → 待机 */
                current_state = BLE_STATE_STANDBY;
                if (!wired_mode_active) {
                    led_set_conn_state(LED_CONN_IDLE);
                }
            }
        }
    }
}

void ble_stop_advertising(void)
{
    uint8_t advertising_enable = FALSE;
    GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(uint8_t), &advertising_enable);
    if (current_state == BLE_STATE_ADVERTISING ||
        current_state == BLE_STATE_RECONNECTING ||
        current_state == BLE_STATE_PAIRING) {
        current_state = BLE_STATE_IDLE;
    }
}

void ble_switch_device(uint8_t device_idx)
{
    if (device_idx < 1 || device_idx > 3) {
        return;  /* 只支持设备1/2/3 */
    }

    /* 用户主动切换到蓝牙模式，退出有线模式 */
    wired_mode_active = false;
    power_set_wired_mode(false);  /* 通知电源管理退出有线模式，无线模式下可以进睡眠 */

    /* 设置当前设备槽位 */
    bond_table.current_device = device_idx;

    /* 设置该槽位对应的蓝牙设备名（CH585_Test-BLE1/2/3）
     * 不同槽位不同名字，避免已配对的设备1（电脑）自动连接到设备2的广播
     */
    ble_set_device_name(device_idx);

    /* 保存模式记忆：蓝牙模式 + 当前设备 */
    ble_save_mode(false, device_idx);

    /* LED 提示切换设备 — 暂时用常亮2s代替闪N次 */
    led_set_conn_state(LED_CONN_SUCCESS);

    if (ble_is_connected()) {
        /* 已连接 — 先断开，记录 pending 目标，断开完成后再切换 */
        current_state = BLE_STATE_SWITCHING;
        pending_switch_device = device_idx;
        pending_pair_device = 0;
        ble_disconnect();
    } else {
        /* 当前未连接 — 直接执行切换 */
        pending_switch_device = 0;
        pending_pair_device = 0;

        /* 设置该槽位的蓝牙地址（必须在启动广播前设置）*/
        ble_set_device_address(device_idx);

        /* 根据槽位是否有配对信息，决定回连还是广播等待 */
        if (bond_table_is_paired(device_idx)) {
            /* 有配对信息 → 定向广播回连（第一阶段，1.28秒超时后自动转普通广播）*/
            ble_device_info_t *dev = &bond_table.devices[device_idx - 1];
            ble_start_directed_advertising(dev->addr_type, dev->bd_addr);
        } else {
            /* 没有配对信息 → 普通广播等待新设备配对 */
            current_state = BLE_STATE_ADVERTISING;
            state_enter_time = timer_read32();
            ble_start_general_advertising();
            if (!wired_mode_active) {
                led_set_conn_state(LED_CONN_PAIRING);  /* 快闪表示广播中 */
            }
        }
    }
}

void ble_pair_device(uint8_t device_idx)
{
    if (device_idx < 1 || device_idx > BLE_MAX_DEVICES) {
        return;
    }

    /* 用户主动进入配对模式，退出有线模式 */
    wired_mode_active = false;
    power_set_wired_mode(false);  /* 通知电源管理退出有线模式，无线模式下可以进睡眠 */

    /* 设置当前设备槽位 */
    bond_table.current_device = device_idx;

    /* 设置该槽位对应的蓝牙设备名（CH585_Test-BLE1/2/3）
     * 不同槽位不同名字，避免已配对的设备1（电脑）自动连接到设备2的广播
     */
    ble_set_device_name(device_idx);

    /* 保存模式记忆：蓝牙模式 + 当前设备 */
    ble_save_mode(false, device_idx);

    /* 注意：不清除旧绑定！
     * 配对成功后用新绑定覆盖当前槽位，超时则保持原绑定。
     * 协议栈的 GAPBondMgr 会处理新配对的密钥交换。
     */

    if (ble_is_connected()) {
        /* 当前已连接 — 先只断开连接，记录待配对目标
         * 等 GAPROLE_WAITING（断开完成事件）触发后再进入配对模式
         * 避免在断开过程中设置广播参数干扰断开流程
         */
        current_state = BLE_STATE_SWITCHING;
        pending_pair_device = device_idx;
        pending_switch_device = 0;  /* 清除可能的待切换目标 */
        ble_disconnect();
    } else {
        /* 当前未连接 — 直接进入配对模式 */
        pending_switch_device = 0;
        pending_pair_device = 0;

        /* 设置该槽位的蓝牙地址（必须在启动广播前设置）*/
        ble_set_device_address(device_idx);

        /* 进入配对模式 — 普通广播，可被新设备搜索到 */
        current_state = BLE_STATE_PAIRING;
        state_enter_time = timer_read32();
        ble_start_general_advertising();
        led_set_conn_state(LED_CONN_PAIRING);  /* 快闪表示配对模式 */
    }
}

void ble_clear_bonds(void)
{
    /* 清除协议栈所有绑定信息 */
    GAPBondMgr_SetParameter(GAPBOND_ERASE_ALLBONDS, 0, NULL);

    /* 清除设备表所有槽位 */
    bond_table_clear_all();

    /* 保存模式记忆：保持当前模式，设备重置为1 */
    ble_save_mode(wired_mode_active, 1);

    /* 断开当前连接 */
    if (ble_is_connected()) {
        current_state = BLE_STATE_SWITCHING;  /* 设中间态，避免 GAPROLE_WAITING 自动回连 */
        ble_disconnect();
    }

    /* 停止广播 */
    ble_stop_advertising();

    /* 进入待机 */
    current_state = BLE_STATE_STANDBY;
    if (!wired_mode_active) {
        led_set_conn_state(LED_CONN_IDLE);
    }
}

/* ========================================================================
 * 模式记忆 — 记住上次使用的模式（有线/蓝牙）和设备槽位
 *
 * 存储在 bond_table 的 last_mode_wired 字段里，和设备绑定表一起持久化。
 * 上电时恢复到上次的模式和设备，蓝牙模式自动回连上次设备。
 * ======================================================================== */

/* 保存当前模式到 EEPROM（切换模式/设备时调用）
 * 参数：wired - true=有线模式，false=蓝牙模式
 *       device - 蓝牙设备槽位（1-3，有线模式时忽略）
 */
void ble_save_mode(bool wired, uint8_t device)
{
    bond_table.last_mode_wired = wired ? 1 : 0;
    if (device >= 1 && device <= BLE_MAX_DEVICES) {
        bond_table.current_device = device;
    }
    bond_table_save();
}

/* 从 EEPROM 读取上次的模式
 * 参数：wired - 输出，true=上次是有线，false=上次是蓝牙
 *       device - 输出，上次的蓝牙设备槽位（1-3）
 */
void ble_load_mode(bool *wired, uint8_t *device)
{
    if (wired != NULL) {
        *wired = (bond_table.last_mode_wired != 0);
    }
    if (device != NULL) {
        *device = bond_table.current_device;
        if (*device < 1 || *device > BLE_MAX_DEVICES) {
            *device = 1;  /* 非法值默认设备1 */
        }
    }
}

/* 上电恢复上次模式 — 在 main 函数初始化完成后调用
 *
 * 恢复逻辑（参考量产键盘）：
 *   上次是有线模式 → 进入有线模式
 *   上次是蓝牙模式 → 切换到上次设备槽位，自动回连
 *
 * 注意：USB 是否连接不影响恢复结果，模式由用户上次选择决定。
 *       用户可以随时手动按 WIRED_MODE 或 BTn 切换。
 */
void ble_restore_last_mode(void)
{
    bool last_wired;
    uint8_t last_device;

    ble_load_mode(&last_wired, &last_device);

    if (last_wired) {
        /* 上次是有线模式 → 进入有线模式
         * 注意：这里不调用 enter_wired_mode()（那是 ch585_evb.c 里的函数），
         * 只设置 wired_mode_active 标志，ch585_evb.c 的 housekeeping 会处理 LED。
         * 实际上 enter_wired_mode() 是外部函数，这里直接调用。
         */
        extern void enter_wired_mode(void);
        enter_wired_mode();
    } else {
        /* 上次是蓝牙模式 → 切换到上次设备槽位，自动回连 */
        ble_switch_device(last_device);
    }
}

/* ========================================================================
 * 主循环轮询
 * ======================================================================== */

void ble_app_poll(void)
{
    /* 处理 BLE 协议栈任务（TMOS 调度器）— 必须在主循环里定期调用 */
    TMOS_SystemProcess();

    /* 检查回连/配对超时 */
    ble_check_timeout();
}

/* ========================================================================
 * 连接间隔动态调整
 * 调用 GAPRole_PeripheralConnParamUpdateReq() 发起请求，主机接受后生效
 * ======================================================================== */

/* 当前请求的连接间隔档位（0~4）*/
static uint8_t current_interval_level = 0;

/* 上次参数更新请求时间（防抖用）*/
static uint32_t last_param_update_time = 0;

/* 防抖最小间隔：两次请求至少间隔5秒 */
#define PARAM_UPDATE_MIN_INTERVAL_MS  5000

/* 核心：发送连接间隔更新请求 */
static void ble_send_conn_interval_req(uint16_t min_interval, uint16_t max_interval)
{
    if (current_state != BLE_STATE_CONNECTED) return;
    GAPRole_PeripheralConnParamUpdateReq(conn_handle,
        min_interval, max_interval,
        BLE_SLAVE_LATENCY, BLE_CONN_TIMEOUT,
        ble_task_id);
    last_param_update_time = timer_read32();
}

/* 请求切换连接间隔档位（带5秒防抖）*/
void ble_request_conn_interval(uint8_t level)
{
    if (level >= BLE_CONN_INTERVAL_LEVELS) return;
    if (level == current_interval_level) return;

    if (current_state != BLE_STATE_CONNECTED) {
        current_interval_level = level;
        return;
    }

    /* 防抖：5秒内不重复请求 */
    uint32_t now = timer_read32();
    if (last_param_update_time != 0 &&
        (now - last_param_update_time) < PARAM_UPDATE_MIN_INTERVAL_MS) {
        return;
    }

    uint16_t min_interval, max_interval;
    switch (level) {
        case 0: min_interval = BLE_CONN_INTERVAL_L0_MIN; max_interval = BLE_CONN_INTERVAL_L0_MAX; break;
        case 1: min_interval = BLE_CONN_INTERVAL_L1_MIN; max_interval = BLE_CONN_INTERVAL_L1_MAX; break;
        case 2: min_interval = BLE_CONN_INTERVAL_L2_MIN; max_interval = BLE_CONN_INTERVAL_L2_MAX; break;
        case 3:
        default: min_interval = BLE_CONN_INTERVAL_L3_MIN; max_interval = BLE_CONN_INTERVAL_L3_MAX; break;
    }

    ble_send_conn_interval_req(min_interval, max_interval);
    current_interval_level = level;
}

/* 强制切换连接间隔档位（绕过5秒防抖，用于携带模式切换）*/
void ble_request_conn_interval_force(uint8_t level)
{
    if (level >= BLE_CONN_INTERVAL_LEVELS) return;
    if (level == current_interval_level) return;

    uint16_t min_interval, max_interval;
    switch (level) {
        case 0: min_interval = BLE_CONN_INTERVAL_L0_MIN; max_interval = BLE_CONN_INTERVAL_L0_MAX; break;
        case 1: min_interval = BLE_CONN_INTERVAL_L1_MIN; max_interval = BLE_CONN_INTERVAL_L1_MAX; break;
        case 2: min_interval = BLE_CONN_INTERVAL_L2_MIN; max_interval = BLE_CONN_INTERVAL_L2_MAX; break;
        case 3:
        default: min_interval = BLE_CONN_INTERVAL_L3_MIN; max_interval = BLE_CONN_INTERVAL_L3_MAX; break;
    }

    ble_send_conn_interval_req(min_interval, max_interval);
    current_interval_level = level;
}

/* 直接设置连接间隔（单位1.25ms，用于携带模式2000ms=1600）*/
void ble_set_conn_interval_raw(uint16_t interval)
{
    ble_send_conn_interval_req(interval, interval);
    current_interval_level = 0xFF; /* 标记为非标准档位 */
}

/* 获取当前请求的连接间隔档位（0~3）*/
uint8_t ble_get_current_interval_level(void)
{
    return current_interval_level;
}

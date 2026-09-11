# CH585 QMK/Vial 移植决策记录

本文档记录 CH585 QMK/Vial 三模键盘固件移植过程中的关键决策和理由。

---

## 一、USB 库选择

### 决策：使用沁恒官方 USB 库，不使用 CherryUSB

**理由**：
1. CherryUSB 的 CH58x port 有 DMA 地址寄存器定义错误（把 32 位寄存器定义为 16 位），导致 USB 完全不工作
2. CherryUSB 的 string_descriptor_callback 有坑（必须返回纯 ASCII，不能返回完整 Unicode 描述符）
3. 沁恒官方 USB 库是芯片原厂维护，和芯片匹配度最高
4. v2 版本从零开始适配沁恒官方 USB 库，避免 CherryUSB 的历史包袱

**影响**：
- USB 驱动代码在 `user/usb/` 目录下
- 高速口驱动：`ch585_usbhs_device.c`
- 全速口驱动：`ch585_usbfs_device.c`
- 描述符：`usb_desc.c` / `usb_desc.h`

---

## 二、系统时钟选择

### 决策：使用 HSE（外部晶振），`CLK_SOURCE_HSE_PLL_62_4MHz`

**理由**：
1. v2 版本使用沁恒官方 USB 库，HSE 可以正常工作
2. 和 SDK 默认 `highcode_init` 一致
3. HSE 精度比 HSI（内部 RC 振荡器）高，USB 时序更稳定

**对比**：
- v1 版本（CherryUSB）：必须用 HSI，用 HSE 时 USB 完全不工作
- v2 版本（沁恒官方 USB 库）：HSE 可用，且更稳定

---

## 三、USB 双口工作模式

### 决策：双口同时工作（`USB_PORT_DUAL_BOTH`，值4），不做切换

**理由**：
1. 双口自动切换（`USB_PORT_DUAL_AUTO`）做不通，`USB_DualPort_CheckAndSwitch()` 被频繁调用时刚初始化的高速口被误判为断开，反复横跳导致两个口都不识别
2. 实际键盘产品只会有一个 C 口，不需要切换
3. 双口同时工作最简单可靠，用户插任意一个口都能用

**影响**：
- `usb_config.h` 里 `USB_PORT_CONFIG = USB_PORT_DUAL_BOTH`
- 高速口和全速口都初始化，都有独立的描述符和中断处理
- 不保留任何双口切换相关代码

---

## 四、低功耗方案

### 决策：WFI + 阶梯式蓝牙连接间隔

**WFI（Wait For Interrupt）**：
- 主循环 `platform_run()` 后加 `__WFI()`
- 零睡死风险，任何中断（按键、蓝牙、定时器）都能唤醒

**阶梯式蓝牙连接间隔**：
| 档位 | 连接间隔 | 适用时段 |
|------|----------|----------|
| L0 | 100ms | 0~30 分钟 |
| L1 | 300ms | 30~90 分钟 |
| L2 | 500ms | 90~180 分钟 |
| L3 | 1000ms | 180 分钟以上 |

- 唤醒立即恢复 100ms
- 连接间隔上限 1000ms（再大首键延迟 >1 秒，且容易丢包断开）

**其他参数**：
- Supervision Timeout = 5 秒
- Slave Latency = 0
- 连接参数更新防抖：两次请求至少间隔 5 秒
- 无操作 10 分钟进睡眠，有线模式不进睡眠
- 断开状态阶梯式广播：100ms（0~30秒）→ 500ms（30秒~5分钟）→ 2000ms（5分钟以上）

**预期续航**：1000mAh 每天 5 小时，续航 12-18 天（待实际测试验证）

---

## 五、蓝牙 HID 报告去重方案

### 决策：发送失败不重试（A方案），QMK 矩阵扫描自动重发

**问题**：大连接间隔下，按一次键输出多个相同字符（连击）。

**根因**：待重试队列（pending_report）重复调用 `HidDev_Report`，导致同一个报告被多次加入协议栈发送队列。

**方案对比**：
- **A方案（采用）**：发送失败时不保存到待重试队列，直接放弃。QMK 矩阵扫描持续运行（10ms一次），下次扫描会重新发送最新报告。
- B方案（放弃）：限制重试次数（最多1次）。还是有可能重复入队。
- C方案（放弃）：发送失败也更新 `last_sent_report`。可能导致报告丢失。

**去重逻辑**：
- 在 `ble_send_report_internal()` 里比较当前报告和 `last_sent_report`，完全相同且 `last_sent_report_valid=true` 则跳过
- 发送成功才更新 `last_sent_report`，发送失败不更新
- 不影响快速连击（按下/松开报告内容不同都会发送）
- 不影响按住不放（操作系统 auto-repeat 处理连续输入）

---

## 六、指示灯方案

### 决策：LED0 = 连接状态灯，LED1 = 电源灯，独立状态机管理

**LED0（PB14）连接状态灯**：
| 状态 | 表现 |
|------|------|
| 正常使用 | 熄灭 |
| 有线插入 | 常亮 2 秒后熄灭 |
| 配对中 | 快闪 3Hz（亮100ms/灭233ms） |
| 回连中 | 慢闪 1Hz（亮200ms/灭800ms） |
| 连接成功 | 常亮 2 秒后熄灭 |

**LED1（PB15）电源灯**：
| 状态 | 表现 |
|------|------|
| 有线模式（USB连接） | 常亮 |
| 蓝牙模式（电池供电） | 熄灭 |
| 低电量（<20%，蓝牙模式） | 慢闪 1次/3秒（亮200ms/灭2800ms） |

**实现**：
- 独立驱动 `led_indicator.c/h`，状态机管理
- `led_indicator_task()` 在 `protocol_usb.c` 的 `platform_run()` 里调用（因为 `protocol.c` 的 `platform_run()` 是无限循环）
- 后续正式产品用 RGB 灯，此方案仅适用于评估板

---

## 七、代码架构

### 目录结构

```
ch585_v2/
├── build/                    # 编译产物（.gitignore 忽略）
├── docs/                     # 项目文档
│   ├── PORTING_DECISIONS.md # 本文档
│   └── FEATURES.md          # 功能详细说明
├── keyboards/                # 键盘定义
├── qmk_firmware/             # QMK 源码（Git Submodule）
├── qmk_porting/              # QMK 移植层
│   ├── ble/                  # 蓝牙相关
│   ├── keyboards/ch585_evb/ # 评估板相关
│   ├── protocol/             # 协议层
│   └── platforms/            # 平台相关
├── sdk/                      # CH585 SDK
├── user/                     # 用户代码
│   └── usb/                  # USB 驱动（沁恒官方库）
├── Dockerfile                # Docker 编译环境
├── build.sh                  # 编译脚本
├── BUILD.md                  # 编译指南
└── rules.cmake               # 编译规则
```

### 关键文件

| 文件 | 说明 |
|------|------|
| `qmk_porting/keyboards/ch585_evb/ch585_evb.c` | 主入口，模式切换 |
| `qmk_porting/keyboards/ch585_evb/led_indicator.c` | 指示灯驱动 |
| `qmk_porting/keyboards/ch585_evb/power_management.c` | 电源管理（低功耗、睡眠） |
| `qmk_porting/ble/ble_app.c` | 蓝牙应用（连接、HID、去重） |
| `qmk_porting/protocol/protocol_usb.c` | USB 协议层（发送报告、led_indicator_task 调用位置） |
| `qmk_porting/protocol/protocol.c` | 协议层（platform_run 无限循环） |
| `user/usb/usb_desc.c` | USB 描述符 |
| `user/usb/ch585_usbhs_device.c` | 高速口驱动 |
| `user/usb/ch585_usbfs_device.c` | 全速口驱动 |

---

## 八、未实现功能（待后续开发）

1. **2.4G 功能（ESB）**：当前 `ESB_ENABLE=OFF`，待 2.4G 开发板到货后开发
2. **电池电量真实检测**：当前是模拟值 100%，需要 ADC 电池检测电路
3. **充电管理芯片检测**：当前无法检测充电中/充满状态
4. **实际续航测试**：需要测量睡眠电流和实际使用天数
5. **切换设备闪 N 次指示槽位**：当前用常亮 2 秒代替（实际产品不用 LED，优先级低）
6. **量产测试和稳定性验证**：长时间使用测试

---

*文档版本：v1.0*
*创建日期：2026-09-11*

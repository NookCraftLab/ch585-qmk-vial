# CH585 QMK/Vial 三模键盘固件

基于沁恒 CH585 芯片的 QMK/Vial 三模键盘固件，支持有线 USB、蓝牙 BLE、2.4G（开发中），支持 Vial 全功能改键。

## 功能特性

### 已实现



* **USB 双口同时工作**：高速口（USB2）和全速口（USB1）同时初始化，插任意一个口都能识别使用

* **Vial 全功能改键**：支持 Raw HID 通信，可读取、修改、保存键位

* **蓝牙 BLE**：支持蓝牙连接、多设备切换

* **低功耗优化**：


  * WFI 休眠，零睡死风险

  * 阶梯式蓝牙连接间隔（100/300/500/1000ms）

  * 无操作 10 分钟进睡眠，按键唤醒

  * 有线模式不进睡眠

* **指示灯**：


  * LED0 连接状态灯（配对快闪、回连慢闪、连接成功常亮 2 秒）

  * LED1 电源灯（有线常亮、蓝牙熄灭、低电量慢闪）

* **蓝牙 HID 报告待重试队列**：发送失败时保存最新报告，主循环非阻塞重试，防止大连接间隔下连击和丢键

### 开发中



* **2.4G 无线**：ESB 协议，待开发

* **NCL-585 自研板**：基于评估板版本定制，待开发

## 硬件支持

### 评估板（CH585 EVB）

当前固件针对沁恒 CH585 官方评估板开发：



| 硬件     | 引脚        | 说明          |
| ------ | --------- | ----------- |
| 高速 USB | PB12/PB13 | 右边 USB2 口   |
| 全速 USB | PB10/PB11 | 左边 USB1 口   |
| LED0   | PB14      | 连接状态灯，低电平点亮 |
| LED1   | PB15      | 电源灯，低电平点亮   |

详细硬件说明见 [docs/HARDWARE.md](docs/HARDWARE.md)。

### 自研板（NCL-585）

`firmware/NCL-585/` 为自研板占位目录，后续基于评估板版本定制。

## 快速开始

### 环境要求



* Docker Desktop（Windows / macOS / Linux）

* 本地不需要安装任何编译工具链

### 编译



```
\# 克隆仓库（含 submodule）

git clone --recursive https://github.com/NookCraftLab/ch585-qmk-vial.git

cd ch585-qmk-vial

\# 编译评估板固件

docker run --rm -v "\$(pwd):/workspace" nookcraftlab/riscv:latest \\

&#x20; bash /workspace/firmware/ch585\_v2/build.sh
```

编译产物在 `firmware/ch585_v2/build/ch585_evb.hex`。

详细编译说明见 [firmware/ch585\_v2/BUILD.md](firmware/ch585_v2/BUILD.md)。

### 烧录

使用沁恒 WCHISPTool 或 MounRiver Studio 烧录 `ch585_evb.hex` 到 CH585 芯片。

### 使用 Vial



1. 用 USB 线连接键盘到电脑

2. 下载 [Vial](https://get.vial.today/)

3. 打开 Vial，自动识别键盘，即可改键

## 项目结构



```
ch585-qmk-vial/

├── firmware/

│   ├── ch585\_v2/              # 通用 CH585 QMK 移植（评估板版本，参考模板）

│   │   ├── qmk\_firmware/      # Git Submodule → NookCraftLab/qmk\_firmware

│   │   ├── qmk\_porting/       # QMK 移植层（蓝牙、USB、电源管理等）

│   │   ├── user/usb/           # USB 驱动（沁恒官方库适配）

│   │   ├── keyboards/          # 键盘定义（评估板）

│   │   ├── sdk/                # CH585 SDK

│   │   ├── docs/               # 项目文档

│   │   ├── Dockerfile           # Docker 编译环境

│   │   ├── build.sh             # 编译脚本

│   │   └── BUILD.md             # 编译指南

│   │

│   └── NCL-585/                # 自研板占位（后续基于 ch585\_v2 定制）

│

├── docker/

│   └── riscv/Dockerfile         # Docker 镜像定义

├── .github/

│   └── workflows/

│       └── build.yml            # GitHub Actions 自动编译

├── docs/

│   ├── FEATURES.md              # 功能详细说明

│   ├── PORTING\_DECISIONS.md     # 移植决策记录

│   ├── HARDWARE.md              # 硬件说明

│   └── LICENSE\_NOTICE.md        # 第三方许可证说明

├── README.md                     # 本文档

├── LICENSE                       # GPLv3（我们的代码）

├── .gitignore                    # Git 忽略规则

└── .gitmodules                   # Submodule 配置
```

## 低功耗参数



| 参数                  | 值       | 说明              |
| ------------------- | ------- | --------------- |
| 连接间隔 L0             | 100ms   | 0\~30 分钟        |
| 连接间隔 L1             | 300ms   | 30\~90 分钟       |
| 连接间隔 L2             | 500ms   | 90\~180 分钟      |
| 连接间隔 L3             | 1000ms  | 180 分钟以上        |
| Supervision Timeout | 5 秒     | 连接监控超时          |
| 睡眠超时                | 10 分钟   | 无操作进睡眠          |
| 预期续航                | 12-18 天 | 1000mAh，每天 5 小时 |

详细功能参数见 [docs/FEATURES.md](docs/FEATURES.md)。

## 许可证

### 我们的代码

本项目的原创代码（`qmk_porting/`、`user/usb/`、`keyboards/` 等）基于 **GPLv3** 许可证开源。

### 第三方组件



| 组件                   | 许可证         | 说明                                                                              |
| -------------------- | ----------- | ------------------------------------------------------------------------------- |
| QMK Firmware         | GPLv2/GPLv3 | [NookCraftLab/qmk\_firmware](https://github.com/NookCraftLab/qmk_firmware)，固化版本 |
| 沁恒 CH585 SDK         | Apache-2.0  | 沁恒官方开源部分，来源 [openwch/ch585](https://github.com/openwch/ch585)                   |
| 蓝牙协议栈（libCH58xBLE.a） | **闭源**      | 沁恒预编译二进制库，无源码，仅以二进制形式分发                                                         |

### 重要说明

> **蓝牙协议栈（libCH58xBLE.a）是沁恒微电子的闭源商业库**
>
> ，不属于本项目的开源部分。
> 使用本固件即表示您同意沁恒微电子蓝牙协议栈的使用条款。本项目仅以预编译二进制形式分发该库，不提供其源代码。
> 沁恒 CH585 SDK 的源码部分按 Apache-2.0 许可证分发，详见 SDK 目录内的相关说明。

详细许可证说明见 [docs/LICENSE\_NOTICE.md](docs/LICENSE_NOTICE.md)。

## 贡献

欢迎提交 Issue 和 Pull Request！

### 开发环境



* 本项目使用 Docker 编译，本地不需要安装工具链

* 提交代码前请确保本地编译通过

* 代码注释请使用中文，只写功能说明，不写历史背景和踩坑故事

### 提交规范



* feat: 新功能

* fix: 修复 bug

* docs: 文档更新

* refactor: 代码重构

* chore: 构建 / 工具链调整

## 相关链接



* [QMK 官方文档](https://docs.qmk.fm/)

* [Vial 官网](https://get.vial.today/)

* [沁恒微电子官网](https://www.wch.cn/)

* [CH585 芯片介绍](https://www.wch.cn/products/CH585.html)

* [NookCraftLab/qmk\_firmware](https://github.com/NookCraftLab/qmk_firmware) - 本项目使用的固化 QMK 版本

## 致谢



* 感谢 [QMK Firmware](https://qmk.fm/) 团队的开源工作

* 感谢 [沁恒微电子](https://www.wch.cn/) 提供 CH585 芯片和 SDK

* 感谢 [Vial](https://get.vial.today/) 项目提供改键工具



***

*本项目仅供学习和研究使用，使用本固件所产生的一切后果由使用者自行承担。*
# 第三方许可证说明

本文档说明本项目中使用的第三方组件及其许可证。

## 许可证汇总

| 组件 | 许可证 | 类型 | 说明 |
|------|--------|------|------|
| 本项目原创代码 | GPLv3 | 开源 | qmk_porting/、user/usb/、keyboards/ 等 |
| QMK Firmware | GPLv2/GPLv3 | 开源 | NookCraftLab/qmk_firmware，固化版本 |
| 沁恒 CH585 SDK | Apache-2.0 | 开源 | 沁恒官方 SDK 源码部分 |
| 蓝牙协议栈（libCH58xBLE.a） | 闭源 | 二进制 | 沁恒预编译库，无源码 |
| xPack riscv-none-elf-gcc | MIT/GPL | 开源 | 标准 RISC-V 工具链 |
| 沁恒 riscv-wch-elf-gcc | 沁恒许可 | 开源/二进制 | CH585 专用工具链 |
| printf 库 | 对应许可证 | 开源 | SDK 内第三方库 |

## 详细说明

### 1. 本项目原创代码

本项目的原创代码（包括但不限于 `qmk_porting/`、`user/usb/`、`keyboards/`、`docker/` 等目录下的代码）基于 **GNU General Public License v3.0 (GPLv3)** 许可证开源。

由于本项目衍生自 QMK Firmware（GPLv2/GPLv3），根据 GPL 许可证的传染性要求，本项目的衍生代码必须使用兼容的开源许可证。

### 2. QMK Firmware

QMK Firmware 是本项目的基础，使用 **GPLv2/GPLv3** 许可证。

本项目使用的 QMK 版本为 NookCraftLab 维护的固化版本：
- 仓库：https://github.com/NookCraftLab/qmk_firmware
- 基于 QMK 官方版本固化，用于保证编译环境一致性

QMK 官方仓库：https://github.com/qmk/qmk_firmware

### 3. 沁恒 CH585 SDK

沁恒 CH585 SDK 的源码部分基于 **Apache License 2.0** 许可证分发。

参考：
- 沁恒官方 GitHub：https://github.com/openwch/ch585
- Apache-2.0 许可证：https://www.apache.org/licenses/LICENSE-2.0

SDK 包含：
- 芯片寄存器定义（CH585SFR.h 等）
- 外设驱动库（GPIO、UART、SPI、I2C 等）
- USB 驱动库（高速/全速）
- 系统初始化代码
- 启动文件（startup_CH585.S）

### 4. 蓝牙协议栈（重要）

> **⚠️ 重要说明**
>
> **蓝牙协议栈（libCH58xBLE.a）是沁恒微电子的闭源商业库**，不属于本项目的开源部分。
>
> 本项目仅以预编译二进制形式（.a 静态库）分发该库，**不提供其源代码**。
>
> 使用本固件即表示您同意沁恒微电子蓝牙协议栈的使用条款。

蓝牙协议栈相关文件：
- `sdk/` 目录下的 `libCH58xBLE.a`（或类似命名的静态库）
- 蓝牙协议栈头文件（.h）

沁恒蓝牙协议栈的使用受沁恒微电子相关许可协议约束，详情请参考：
- 沁恒微电子官网：https://www.wch.cn/
- CH585 产品页面：https://www.wch.cn/products/CH585.html

### 5. 编译工具链

#### xPack riscv-none-elf-gcc

标准 RISC-V 工具链，版本 13.2.0，用于通用 RISC-V 芯片编译。

- 项目地址：https://github.com/xpack-dev-tools/riscv-none-elf-gcc-xpack
- 许可证：MIT / GPL（各组件不同）

#### 沁恒 riscv-wch-elf-gcc

沁恒官方 RISC-V 工具链，版本 12.2.0，CH585 专用，支持 mcpy 指令。

- 下载地址：https://www.wch.cn/downloads/
- 许可证：沁恒微电子许可协议

### 6. 其他第三方库

SDK 中可能包含其他第三方库（如 printf 库等），其许可证以各库目录内的 LICENSE 文件为准。

## 许可证文本

### GPLv3

本项目原创代码使用 GNU General Public License v3.0，完整文本见根目录 `LICENSE` 文件。

### Apache-2.0

沁恒 CH585 SDK 使用 Apache License 2.0，完整文本可参考：
https://www.apache.org/licenses/LICENSE-2.0

## 联系方式

如有许可证相关问题，请通过以下方式联系：
- GitHub Issues：https://github.com/NookCraftLab/ch585-qmk-vial/issues
- 组织主页：https://github.com/NookCraftLab

---

*本文档最后更新：2026-09-11*

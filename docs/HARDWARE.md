# 硬件说明

本文档说明 CH585 QMK/Vial 固件支持的硬件平台和引脚定义。

## 支持的硬件

### CH585 官方评估板（EVB）

当前固件针对沁恒 CH585 官方评估板开发。

#### 芯片信息

| 参数 | 值 |
|------|-----|
| 芯片型号 | CH585 |
| 内核 | RISC-V3A |
| 主频 | 62.4 MHz |
| FLASH | 448 KB |
| RAM | 128 KB |
| 蓝牙 | BLE 5.4 |
| USB | 高速 USB 2.0 (480Mbps) + 全速 USB 2.0 (12Mbps) |
| 2.4G | 支持（开发中） |

#### USB 接口

| 接口 | 引脚 | 位置 | 说明 |
|------|------|------|------|
| 高速 USB | PB12 (D+) / PB13 (D-) | 右边 USB2 口 | USB 2.0 High Speed |
| 全速 USB | PB10 (D+) / PB11 (D-) | 左边 USB1 口 | USB 2.0 Full Speed |

两个 USB 口同时工作，插任意一个口都能识别使用。

#### LED 指示灯

| LED | 引脚 | 功能 | 点亮方式 |
|-----|------|------|----------|
| LED0 | PB14 | 连接状态灯 | 低电平点亮 |
| LED1 | PB15 | 电源灯 | 低电平点亮 |

LED0 连接状态灯：
- 配对中：快闪 3Hz（亮100ms/灭233ms）
- 回连中：慢闪 1Hz（亮200ms/灭800ms）
- 连接成功/有线插入：常亮 2 秒后熄灭
- 正常使用：熄灭

LED1 电源灯：
- 有线模式（USB 连接）：常亮
- 蓝牙模式（电池供电）：熄灭
- 低电量（<20%，蓝牙模式）：慢闪 1次/3秒（亮200ms/灭2800ms）

#### 按键矩阵

评估板按键矩阵配置见 `firmware/ch585_v2/keyboards/ch585_evb/` 目录下的配置文件。

### NCL-585 自研板（开发中）

`firmware/NCL-585/` 为自研板占位目录，后续基于评估板版本定制开发。

## 编译环境

### Docker 镜像

本项目使用 Docker 编译，镜像包含：
- xPack riscv-none-elf-gcc 13.2.0（标准 RISC-V 工具链）
- 沁恒官方 riscv-wch-elf-gcc 12.2.0（CH585 专用，支持 mcpy 指令）

镜像名：`nookcraftlab/riscv:latest`

Dockerfile 位置：`docker/riscv/Dockerfile`

### 沁恒工具链获取

沁恒官方工具链（MRS Toolchain）需要从沁恒官网下载：
- 下载地址：https://www.wch.cn/downloads/
- 搜索：MounRiver Studio 或 MRS Toolchain
- Linux 版本：MRS_Toolchain_Linux_X64_V240.tar.xz

构建 Docker 镜像时，将工具链放在 `docker/riscv/` 目录下。

## 烧录工具

### WCHISPTool

沁恒官方烧录工具，支持 Windows：
- 下载地址：https://www.wch.cn/downloads/WCHISPTool_exe.html
- 支持串口、USB、蓝牙等多种烧录方式

### MounRiver Studio (MRS)

沁恒官方 IDE，集成编译和烧录功能：
- 下载地址：https://www.wch.cn/downloads/MRS_Toolchain_Linux_X64.html
- 支持 Windows、Linux、macOS

## 相关链接

- [沁恒微电子官网](https://www.wch.cn/)
- [CH585 芯片介绍](https://www.wch.cn/products/CH585.html)
- [CH585 数据手册](https://www.wch.cn/downloads/CH585DS1_PDF.html)
- [CH585 评估板资料](https://www.wch.cn/downloads/CH585EVT_ZIP.html)

---

*本文档最后更新：2026-09-11*

# CH585 QMK/Vial 三模键盘固件 - 编译指南

## 环境要求

- Docker Desktop（Windows / macOS / Linux）
- 本地不需要安装任何编译工具链

## 编译命令

### Windows（PowerShell）

```powershell
docker run --rm -v "E:\Works\AI work\NookCraftLab:/workspace" nookcraftlab/riscv:latest bash /workspace/firmware/qmk/ch585_v2/build.sh
```

### Linux / macOS

```bash
docker run --rm -v "/path/to/NookCraftLab:/workspace" nookcraftlab/riscv:latest bash /workspace/firmware/qmk/ch585_v2/build.sh
```

## 编译产物

编译完成后，产物在 `build/` 目录下：

| 文件 | 说明 | 大小 |
|------|------|------|
| `ch585_evb.hex` | HEX 格式固件（用于烧录） | ~587 KB |
| `ch585_evb.bin` | BIN 格式固件 | ~204 KB |
| `ch585_evb.elf` | ELF 格式（用于调试） | ~313 KB |

## 内存占用（参考）

| 区域 | 已用 | 总量 | 使用率 |
|------|------|------|--------|
| FLASH | 208776 B | 448 KB | 45.51% |
| RAM | 34772 B | 128 KB | 26.53% |

## Docker 镜像

- 镜像名：`nookcraftlab/riscv:latest`
- 包含工具链：
  - xPack riscv-none-elf-gcc 13.2.0
  - 沁恒官方 riscv-wch-elf-gcc 12.2.0
- Dockerfile 位置：`docker/riscv/Dockerfile`

## 常见问题

### 1. Docker 容器运行但没有输出

检查 Docker Desktop 是否正常运行，Engine 状态是否为 running。

### 2. 编译报错找不到文件

检查挂载路径是否正确，Windows 下路径用双引号包裹。

### 3. 烧录后不识别

- 检查 USB 线是否是数据线（不是充电线）
- 高速口=右边USB2（PB12/PB13），全速口=左边USB1（PB10/PB11）
- 两个口都支持，插任意一个都可以

## 相关文档

- 移植决策：`docs/PORTING_DECISIONS.md`
- 功能说明：`docs/FEATURES.md`

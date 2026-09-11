#!/bin/bash
# CH585 QMK编译脚本（使用分包镜像nookcraftlab/riscv，内含沁恒官方GCC12工具链）

set -e

# 工具链路径（分包镜像里已经预装，注意路径里有空格）
GCC_PATH="/opt/Toolchain/RISC-V Embedded GCC12/bin"

echo "=== 1. 工具链版本 ==="
"$GCC_PATH/riscv-wch-elf-gcc" --version | head -1

echo ""
echo "=== 2. 开始编译 ==="
cd /workspace/firmware/ch585_v2
rm -rf build
mkdir -p build
cd build
cmake .. -G Ninja \
    -DTOOLCHAIN_PREFIX=riscv-wch-elf \
    -DADDITIONAL_TOOLCHAIN_PATH="$GCC_PATH/" \
    -DCMAKE_BUILD_TYPE=Release
ninja

echo ""
echo "=== 3. 编译完成 ==="
ls -la *.hex *.bin *.elf 2>/dev/null || echo "未找到编译产物"

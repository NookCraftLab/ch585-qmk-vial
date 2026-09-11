FROM nookcraftlab/base:latest

ENV DEBIAN_FRONTEND=noninteractive

# 标准RISC-V工具链（xPack riscv-none-elf-gcc 13.2.0，通用RISC-V芯片用）
RUN cd /opt && \
    wget -q https://github.com/xpack-dev-tools/riscv-none-elf-gcc-xpack/releases/download/v13.2.0-2/xpack-riscv-none-elf-gcc-13.2.0-2-linux-x64.tar.gz && \
    tar -xzf xpack-riscv-none-elf-gcc-13.2.0-2-linux-x64.tar.gz && \
    rm xpack-riscv-none-elf-gcc-13.2.0-2-linux-x64.tar.gz && \
    mv xpack-riscv-none-elf-gcc-13.2.0-2 riscv-none-elf-gcc

# 沁恒官方工具链（riscv-wch-elf-gcc 12.2.0，CH585专用，支持mcpy）
COPY MRS_Toolchain_Linux_X64_V240.tar.xz /tmp/
RUN cd /opt && tar -xf /tmp/MRS_Toolchain_Linux_X64_V240.tar.xz && rm /tmp/MRS_Toolchain_Linux_X64_V240.tar.xz

# 标准工具链入PATH；沁恒工具链用绝对路径调用
ENV PATH="/opt/riscv-none-elf-gcc/bin:${PATH}"

# 验证两个工具链
RUN riscv-none-elf-gcc --version
RUN "/opt/Toolchain/RISC-V Embedded GCC12/bin/riscv-wch-elf-gcc" --version

WORKDIR /workspace

CMD ["/bin/bash"]

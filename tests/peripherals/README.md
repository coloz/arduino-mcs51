# MCS51 驱动行为测试

从仓库根目录运行 `node tests/peripherals/run.mjs`。默认使用已安装 STCXX 工具链的 Clang 和 Rust 工具链自带的 wasm-ld；也可通过 `--clang <path> --ld <path>` 指定。需要 Node.js 支持 WebAssembly。

测试实际生产 C 驱动，通过最小 Arduino/SFR/XFR/GPIO 边界替身执行。14 个型号分别覆盖 UART1、UART2–4、SPI、Wire、外部中断，共 70 组；包含参数拒绝、时钟重载、共享寄存器保留与恢复、全部已配置 UART 备用路由、兄弟定时器保留、RX 独立/回卷/溢出、TX 轮询、SPI 模式/位序/中断事务/低速回退、IIC ACK/NACK/重复 START/零长度 STOP/超时/从机回调。结果位于 `.build/peripheral-host-tests/results.json`。

这是寄存器行为模拟，不是完整 8051 CPU 模拟或实板测试。SFR 地址、复用位和时钟公式还须与官方手册核对；指令级竞态、真实总线时序、硅修订差异和栈峰值不由这些测试证明。完整目标编译矩阵见 [../compile/PERIPHERAL_RESULTS.md](../compile/PERIPHERAL_RESULTS.md)。

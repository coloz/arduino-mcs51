# MCS51 外设适配测试结果

记录日期（UTC）：2026-09-22。源码基线：`bfbe6c1cfda5662db49d54bf94cbf3124132ffba` 加当前未提交的适配；不代表已经发布。

完整矩阵 **924 项**：14 型号 × 33 个程序 × Arduino CLI / aily-builder。**754 项通过，162 项容量不足，8 项无 ADC/A0 而不适用，0 项其他失败**。容量不足和不适用均不计为通过。

70 组真实 C 驱动寄存器行为测试通过；构建适配器单元测试 6 项通过，编译器单元测试 25 项通过、2 项明确忽略的子进程 fixture。47 文件 SDK 同步、变体生成一致性、差异空白检查已通过。

每个 pass 都完成目标编译、链接、HEX 校验和/地址检查、型号容量验证、Flash/XDATA 统计及 `stc-cli validate`。行为测试使用模拟寄存器，并非真实 8051 指令仿真或上板测试。

另对 754 个通过映像的 3326 个 Timer0/UART/IIC 中断向量，逐项核对 HEX 中的跳转目标与链接映射中的 ISR 符号地址。

双构建器 462 对：462 对状态一致；377 对通过项 Flash 用量一致；194 对通过项的 HEX 地址/字节完全相同。其余映像的运行等价性未验证。

## 各型号结果

| 型号 | UART / IIC / SPI | 通过 / 容量不足 / 无 ADC / 其他 |
| --- | --- | --- |
| STC8H8K64U | 4 / 1 / 1 | 66 / 0 / 0 / 0 |
| STC8H1K08 | 2 / 1 / 1 | 18 / 48 / 0 / 0 |
| STC8G1K08A | 1 / 1 / 1 | 20 / 46 / 0 / 0 |
| STC8G1K08 | 2 / 1 / 1 | 18 / 48 / 0 / 0 |
| STC8G2K64S4 | 4 / 1 / 1 | 66 / 0 / 0 / 0 |
| STC8C2K64S4 | 4 / 1 / 1 | 62 / 0 / 4 / 0 |
| STC8H1K28 | 2 / 1 / 1 | 66 / 0 / 0 / 0 |
| STC8H3K64S4 | 4 / 1 / 1 | 66 / 0 / 0 / 0 |
| Ai8H2K12U | 2 / 1 / 1 | 46 / 20 / 0 / 0 |
| Ai8H2K32U | 2 / 1 / 1 | 66 / 0 / 0 / 0 |
| STC15F2K60S2 | 2 / 0 / 1 | 66 / 0 / 0 / 0 |
| STC12C5A60S2 | 2 / 0 / 1 | 66 / 0 / 0 / 0 |
| STC89C58RD+ | 1 / 0 / 0 | 62 / 0 / 4 / 0 |
| STC15W4K32S4 | 4 / 0 / 1 | 66 / 0 / 0 / 0 |

## 新增接口探针

| 型号 | 多串口 | SPI | Wire 主机 | Wire 从机能力分支 |
| --- | --- | --- | --- | --- |
| STC8H8K64U | pass | pass | pass | pass |
| STC8H1K08 | capacity | capacity | capacity | capacity |
| STC8G1K08A | pass | capacity | capacity | capacity |
| STC8G1K08 | capacity | capacity | capacity | capacity |
| STC8G2K64S4 | pass | pass | pass | pass |
| STC8C2K64S4 | pass | pass | pass | pass |
| STC8H1K28 | pass | pass | pass | pass |
| STC8H3K64S4 | pass | pass | pass | pass |
| Ai8H2K12U | capacity | pass | pass | pass |
| Ai8H2K32U | pass | pass | pass | pass |
| STC15F2K60S2 | pass | pass | pass | pass |
| STC12C5A60S2 | pass | pass | pass | pass |
| STC89C58RD+ | pass | pass | pass | pass |
| STC15W4K32S4 | pass | pass | pass | pass |

串口探针按 UART_COUNT 选择可用串口，单串口型号只验证 UART1。从机能力分支在 WIRE_HAS_SLAVE=0 的板型只编译软件主机；不能把这些 pass 解读为有硬件从机。STC89 的 SPI 探针验证软件回退。模式、路由、时钟和物理外设数量详见 [HARDWARE_INTERFACES.md](../../HARDWARE_INTERFACES.md)。

## 容量与边界

UART1 新增路由分发时曾使 SerialEcho 增大约 3 KiB；已将可选路由配置移入独立归档成员，恢复默认接线的小体积路径。所有最终容量仍按真实型号限制检查，不放宽 Flash/XRAM 上限。

| 型号 | SerialEcho Flash 字节 |
| --- | ---: |
| STC8H8K64U | 7514 |
| STC8H1K08 | 7345 |
| STC8G1K08A | 7264 |
| STC8G1K08 | 7345 |
| STC8G2K64S4 | 7507 |
| STC8C2K64S4 | 7507 |
| STC8H1K28 | 7345 |
| STC8H3K64S4 | 7513 |
| Ai8H2K12U | 7426 |
| Ai8H2K32U | 7426 |
| STC15F2K60S2 | 7140 |
| STC12C5A60S2 | 7544 |
| STC89C58RD+ | 7368 |
| STC15W4K32S4 | 7382 |

相对旧矩阵，原先通过而本次因完整驱动增大出现容量不足的组合（Arduino CLI）：

- stc8h1k08 / tests/compile/common/SPITransactions：8642 / 8192 字节
- stc8g1k08 / tests/compile/common/SPITransactions：8640 / 8192 字节
- stc8g1k08a / tests/compile/common/SPITransactions：8545 / 8192 字节
- ai8h2k12u / examples/PeripheralSmoke：14561 / 12288 字节
- ai8h2k12u / libraries/Wire/examples/MasterRegisterRead：14490 / 12288 字节
- ai8h2k12u / tests/compile/common/LcdCounter：12535 / 12288 字节
- ai8h2k12u / tests/compile/common/WireRegisterRead：14475 / 12288 字节
- ai8h2k12u / tests/compile/common/WireScanner：14139 / 12288 字节

24 项独立边界编译：EEPROM 和 Servo 缺头文件共 12 项；tone/noTone 缺 API 共 6 项；Wire 从机签名共 6 项编译通过。STC12/STC89 调用从机入口返回配置错误，由行为测试验证，编译通过并不改变 WIRE_HAS_SLAVE=0。`analogWrite` 仍是数字阈值输出；USB、RTC、PWM/PCA、通用定时器等缺口已列入接口文档。

## 证据与复现

工具：arduino-cli  Version: 1.5.1 Commit: 01f3d4f2b Date: 2026-06-05T10:22:12Z；aily-builder 1.2.17；STCXX 0.3.0；stc-cli 0.1.0；win32/x64。

主轮次初始并发 12；续跑并发记录：16。续跑校验工具和用例输入，已完成项保留且不重复计数。

主轮次：`.build/qualification/peripherals-qualified`；独立复测：`peripherals-recheck-g2`、`peripherals-recheck-h1`、`peripherals-recheck-c2`。原始失败尝试完整保留在各轮次 logs / results 中，报告 JSON 同时保留原始轮次和最终合并结果。

Arduino CLI 在少数首次构建中出现过无编译诊断的 exit 1；所有其他失败均须独立复查。若最终其他失败为 0，表示这些组合的独立复测通过，不代表已定位 CLI 偶发退出原因。

已校验 202 个源文件 SHA-256，确保最终矩阵期间核心、库、变体和测试输入未改变。原生驱动 SHA-256：`ac5ab6c67df48dcba42a1988faeb084536132e2c3a0979be75bbab97ac749428`。

```powershell
node tests/peripherals/run.mjs
node scripts/test-compile.mjs --aily .build/aily-builder/dist/main.js --workers 12 --jobs 2 --run <new-run>
node scripts/test-interrupt-vectors.mjs --run peripherals-qualified --overlays peripherals-recheck-g2,peripherals-recheck-h1,peripherals-recheck-c2
node scripts/report-peripherals.mjs --run peripherals-qualified --overlays peripherals-recheck-g2,peripherals-recheck-h1,peripherals-recheck-c2
```

[全部逐项结果](PERIPHERAL_RESULTS.json)；[编译运行说明](README.md)；[行为测试范围](../peripherals/README.md)。

**未烧录、未操作 COM 口；实板波形、硅修订差异、同时收发压力和中断栈峰值仍待上板验证。**

# MCS51 双构建器编译测试

记录日期：2026-09-22。

测试平台：win32/x64，Node v24.15.0；arduino-cli  Version: 1.5.1 Commit: 01f3d4f2b Date: 2026-06-05T10:22:12Z；aily-builder 1.2.17；stc-cli 0.1.0；STCXX 0.3.0。

完成 **812 个编译组合**：14 个板型 × 29 个程序 × 2 种构建器。**682 项通过，122 项容量超限，8 项因板型未提供 ADC/A0 而不适用，0 项其他失败**。容量超限和不适用均不计为通过；JSON 保留原始失败状态和诊断。

9 个 SDK 例程、20 个常见程序纳入矩阵。常见程序为本仓库编写的 Arduino 用法测试，包含多文件 C/C++ 与自动原型。所有成功项均检查 HEX 校验和、地址、型号容量、SDCC 内存统计，并通过 stc-cli validate。

两构建器共 406 组配对：406 组状态一致；双方通过的 341 组中，341 组 Flash 占用一致，103 组 HEX 映像按地址/字节完全一致。

抽查 SerialEcho 的跨构建器差异，生成 C 的区别在常量排列及符号编号，Flash 大小相同。本轮保留全部字节比较结果，未进行不同映像的运行时等价验证。

在同一构建目录重复执行原编译命令：10 项记录，10 项 HEX 与首次构建逐字节一致。原始命令、输出及哈希位于主轮次的 cache-replay 目录。

## 板型结果

计数顺序：通过 / 容量超限 / ADC 未提供 / 其他失败。每种构建器、每个板型各测试 29 个程序。

| 板型 | Flash / XDATA 字节 | Arduino CLI | aily-builder |
| --- | ---: | ---: | ---: |
| STC8H8K64U | 65536 / 8192 | 29 / 0 / 0 / 0 | 29 / 0 / 0 / 0 |
| STC8H1K08 | 8192 / 1024 | 10 / 19 / 0 / 0 | 10 / 19 / 0 / 0 |
| STC8G1K08A | 8192 / 1024 | 10 / 19 / 0 / 0 | 10 / 19 / 0 / 0 |
| STC8G1K08 | 8192 / 1024 | 10 / 19 / 0 / 0 | 10 / 19 / 0 / 0 |
| STC8G2K64S4 | 65536 / 2048 | 29 / 0 / 0 / 0 | 29 / 0 / 0 / 0 |
| STC8C2K64S4 | 65536 / 2048 | 27 / 0 / 2 / 0 | 27 / 0 / 2 / 0 |
| STC8H1K28 | 28672 / 1024 | 29 / 0 / 0 / 0 | 29 / 0 / 0 / 0 |
| STC8H3K64S4 | 65536 / 3072 | 29 / 0 / 0 / 0 | 29 / 0 / 0 / 0 |
| Ai8H2K12U | 12288 / 1024 | 25 / 4 / 0 / 0 | 25 / 4 / 0 / 0 |
| Ai8H2K32U | 32768 / 1024 | 29 / 0 / 0 / 0 | 29 / 0 / 0 / 0 |
| STC15F2K60S2 | 61433 / 1792 | 29 / 0 / 0 / 0 | 29 / 0 / 0 / 0 |
| STC12C5A60S2 | 61440 / 1024 | 29 / 0 / 0 / 0 | 29 / 0 / 0 / 0 |
| STC89C58RD+ | 32768 / 1024 | 27 / 0 / 2 / 0 | 27 / 0 / 2 / 0 |
| STC15W4K32S4 | 32768 / 3840 | 29 / 0 / 0 / 0 | 29 / 0 / 0 / 0 |

## 程序结果

下表分别统计每个程序在 14 个板型上的结果。Flash 范围取 Arduino CLI 成功项，包含运行库和实际链接后的代码。

| 程序 | 类别 | Arduino CLI | aily-builder | Flash 字节范围 |
| --- | --- | ---: | ---: | ---: |
| Blink | SDK 例程 | 14 / 0 / 0 / 0 | 14 / 0 / 0 / 0 | 3628–4642 |
| CppRuntime | SDK 例程 | 14 / 0 / 0 / 0 | 14 / 0 / 0 / 0 | 6260–6311 |
| PeripheralSmoke | SDK 例程 | 11 / 3 / 0 / 0 | 11 / 3 / 0 / 0 | 7782–9462 |
| SerialEcho | SDK 例程 | 14 / 0 / 0 / 0 | 14 / 0 / 0 / 0 | 6204–6621 |
| HelloWorld | SDK 例程 | 11 / 3 / 0 / 0 | 11 / 3 / 0 / 0 | 10927–12088 |
| PollingEcho | SDK 例程 | 11 / 3 / 0 / 0 | 11 / 3 / 0 / 0 | 9575–10872 |
| SoftwareLoopback | SDK 例程 | 11 / 3 / 0 / 0 | 11 / 3 / 0 / 0 | 8937–9828 |
| OneRevolution | SDK 例程 | 11 / 3 / 0 / 0 | 11 / 3 / 0 / 0 | 8487–9648 |
| MasterRegisterRead | SDK 例程 | 11 / 3 / 0 / 0 | 11 / 3 / 0 / 0 | 10001–10892 |
| AnalogReadSerial | 常见应用 | 9 / 3 / 2 / 0 | 9 / 3 / 2 / 0 | 8799–9281 |
| AnalogWriteFade | 常见应用 | 14 / 0 / 0 / 0 | 14 / 0 / 0 / 0 | 3744–4758 |
| BlinkWithoutDelay | 常见应用 | 14 / 0 / 0 / 0 | 14 / 0 / 0 / 0 | 3703–4717 |
| Debounce | 常见应用 | 14 / 0 / 0 / 0 | 14 / 0 / 0 / 0 | 3853–4867 |
| FlashStrings | 常见应用 | 14 / 0 / 0 / 0 | 14 / 0 / 0 / 0 | 6619–7036 |
| InterruptCounter | 常见应用 | 11 / 3 / 0 / 0 | 11 / 3 / 0 / 0 | 8058–8949 |
| IPAddressPrint | 常见应用 | 10 / 4 / 0 / 0 | 10 / 4 / 0 / 0 | 16241–16658 |
| LcdCounter | 常见应用 | 11 / 3 / 0 / 0 | 11 / 3 / 0 / 0 | 11297–12458 |
| MathFunctions | 常见应用 | 8 / 4 / 2 / 0 | 8 / 4 / 2 / 0 | 14786–15085 |
| MultiFile | 常见应用 | 14 / 0 / 0 / 0 | 14 / 0 / 0 / 0 | 6859–7276 |
| PrintFormats | 常见应用 | 11 / 3 / 0 / 0 | 11 / 3 / 0 / 0 | 8263–8680 |
| PulseDistance | 常见应用 | 11 / 3 / 0 / 0 | 11 / 3 / 0 / 0 | 8746–9637 |
| SerialCommand | 常见应用 | 11 / 3 / 0 / 0 | 11 / 3 / 0 / 0 | 8424–9315 |
| ShiftRegister | 常见应用 | 14 / 0 / 0 / 0 | 14 / 0 / 0 / 0 | 3930–4944 |
| SoftwareSerialBridge | 常见应用 | 10 / 4 / 0 / 0 | 10 / 4 / 0 / 0 | 12018–13009 |
| SPITransactions | 常见应用 | 14 / 0 / 0 / 0 | 14 / 0 / 0 / 0 | 6201–7362 |
| StepperSweep | 常见应用 | 11 / 3 / 0 / 0 | 11 / 3 / 0 / 0 | 8433–9594 |
| StringOperations | 常见应用 | 10 / 4 / 0 / 0 | 10 / 4 / 0 / 0 | 19173–19590 |
| WireRegisterRead | 常见应用 | 11 / 3 / 0 / 0 | 11 / 3 / 0 / 0 | 9995–10886 |
| WireScanner | 常见应用 | 11 / 3 / 0 / 0 | 11 / 3 / 0 / 0 | 9323–10214 |

## 容量与兼容边界

8 KiB Flash 型号并不能容纳所有 C++/总线/打印组合。容量拒绝来自实际链接器，未扩大型号内存配置或隐藏失败。详细 JSON 中保留每项诊断和日志路径。

Ai8H2K12U 的 12 KiB Flash 也无法容纳 4 个常见程序：Arduino CLI 链接报告中 IPAddressPrint 为 16,255 字节、MathFunctions 为 14,839 字节、SoftwareSerialBridge 为 12,679 字节、StringOperations 为 19,187 字节。该板型的全部 9 个 SDK 例程均通过两种构建器。

`analogWrite` 编译成功仍仅代表数字阈值输出；无 ADC 型号没有由编译获得 ADC 功能。引脚可用性、外设时序和运行时栈/堆峰值须实板验证。

STC8C2K64S4 和 STC89C58RD+ 的 SDK 配置没有 ADC/A0；AnalogReadSerial 和使用 ADC 输入的 MathFunctions 原样编译会失败。综合例程 PeripheralSmoke 则按 NUM_ANALOG_INPUTS 条件启用 ADC 部分。

| 独立边界探针 | Arduino CLI | aily-builder |
| --- | --- | --- |
| EEPROMReadWrite | missing-header | missing-header |
| ServoSweep | missing-header | missing-header |
| ToneMelody | compile-error | compile-error |
| WireSlave | compile-error | compile-error |

这些探针单独记录，不混入支持范围矩阵；SDK 未提供 tone/noTone、Servo、EEPROM 和 Wire 从机接口。

## 本次修复

- 将 5 个库例程的旧 C 函数表写法更新为当前 C++ API：`Serial.begin/write`、`SPISettings`、`Wire.endTransmission(false)`、LCD/Stepper/SoftwareSerial 实例。LCD 示例同时避开无效的 P1.2 默认引脚。
- 综合例程 PeripheralSmoke 按 NUM_ANALOG_INPUTS 条件编译 ADC 读取，修复无 ADC 板型上 A0 未定义的问题。
- 修复 Windows 长路径下原生适配器及编译器原子写入失败；新增长路径创建/覆盖回归测试。修复前测试可复现失败，修复后通过，PeripheralSmoke 的原长路径构建也通过。
- 添加可重跑的矩阵脚本、20 个常见应用、4 个边界探针和结构化结果；保留每次构建与型号校验的完整日志。

原生适配器测试 6 项通过；编译器测试 25 项通过，2 项为明确忽略的子进程 fixture。SDK 同步、variant 生成一致性和差异空白检查通过。

## 复现与证据

[运行说明](README.md)。主记录位于 `.build/qualification/matrix`，本报告合并轮次：`matrix`、`peripheral-fixed`、`cpp-recheck`。

Arduino CLI 使用板型隔离的本地核心缓存，部分先行用例采用完整构建；aily 使用本地对象/归档缓存，禁用远程下载。构建时间受并发与缓存状态影响，不作为性能比较。

STC8H3K64S4 / CppRuntime 的一次 Arduino CLI 调用无编译诊断并返回 1，独立复测通过；未定位这次退出的根因。原始记录未删除，最终矩阵采用单列复测结果。

源代码基线：`bfbe6c1cfda5662db49d54bf94cbf3124132ffba` 加本次工作区修复；原生驱动 SHA-256：`d51666fc967d60db3fa125c114ea68c5bc0305284237abdcaa4284da807eefa5`。工具命令、输入哈希、板型和逐项内存数据保存在相邻 JSON 记录中。

**本报告仅证明编译、链接、静态容量和固件格式检查结果，未进行烧录或实板运行测试。**

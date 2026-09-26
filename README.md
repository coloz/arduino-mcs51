# arduino-mcs51

面向 STC MCS51 芯片的独立 Arduino C++11 平台，版本 **0.0.1**。FQBN 为 `stc:mcs51:<板型 ID>`，可与 `stc:mcs251` 并存。当前完成 Windows x64 编译链适配；尚未进行本项目的实板验收。

芯片定义、引脚和 C HAL 来自工作区 `arduino-mcs251` 中保留的 MCS51 历史实现；构建适配器基于当前原生 Rust 驱动，使用现有 STCXX 0.3.0 的 Clang → LLVM-CBE → SDCC 工具链和 `stc-cli` 烧录器。共享文件及维护边界见 [来源清单](cores/STC/sdk-sources.json)。

## 支持型号

| 系列 | 板型 ID / 型号 |
| --- | --- |
| STC8H | `stc8h8k64u`、`stc8h3k64s4`、`stc8h1k28`、`stc8h1k08` |
| STC8G | `stc8g2k64s4`、`stc8g1k08`、`stc8g1k08a` |
| STC8C | `stc8c2k64s4` |
| Ai8 | `ai8h2k12u`、`ai8h2k32u` |
| STC15 | `stc15f2k60s2`、`stc15w4k32s4` |
| STC12 | `stc12c5a60s2` |
| STC89 | `stc89c58rd_plus`（STC89C58RD+） |

仅上述型号列入本版支持范围。同系列其他容量、封装不能直接视为同一板型。UART/IIC/SPI 数量、硬件路由、存储容量、端口掩码和 ADC 通道以 [devices.json](tools/variants/devices.json) 及生成的 variant 为准。引脚编码为 `(port << 4) | bit`，例如 `P3_2`；未假定任何板载 LED，Blink 需外接 LED 与限流电阻。

## 在当前工作区编译

需要 Windows x64、Arduino CLI 及其 builtin 工具、Node.js、Rust 和 STCXX 0.3.0 工具链。与 `arduino-mcs251` 共用工具链二进制，但使用本项目的 MCS51 驱动和锁文件。

```powershell
cd D:\Git\stc51\arduino-mcs51
node scripts/build-native-driver.mjs
node scripts/build-example.mjs
node scripts/build-example.mjs --fqbn stc:mcs51:stc8g1k08a --sketch examples/SerialEcho
```

驱动构建默认使用 Cargo 本地缓存；新机器可先在 `tools/stcxx-driver` 中执行 `cargo fetch --locked`。示例脚本优先复用 `.build/toolchain/stcxx-toolchain`，其次寻找 Arduino15 中已安装的 STCXX 0.3.0；也可显式传入 `--toolchain <解压目录>`。工作区已有 `../stcxx/dist/stcxx-toolchain-0.3.0-windows-x86_64.zip` 时，可执行：

```powershell
Expand-Archive ../stcxx/dist/stcxx-toolchain-0.3.0-windows-x86_64.zip .build/toolchain
```

脚本在 `.build` 中建立独立 sketchbook、配置和构建输出，不修改现有 Arduino 平台。HEX 路径在成功后输出。`--arduino-data <目录>` 可指定用于读取 Arduino builtin 工具的安装目录。

## Arduino IDE 手动安装

从 [v0.0.1 Release](https://github.com/coloz/arduino-mcs51/releases/tag/v0.0.1) 下载 `arduino-mcs51-0.0.1-windows-x86_64.zip`，将其中的 `arduino-mcs51-0.0.1` 目录作为 `<sketchbook>/hardware/stc/mcs51` 使用。此包包含 Windows x64 原生驱动；STCXX 0.3.0 工具链和 `stc-cli` 需另行安装。

将本项目复制到 `<sketchbook>/hardware/stc/mcs51`，保留 `cores`、`variants`、`libraries`、`examples`、`platform.txt`、`boards.txt` 和 `tools/stcxx-driver` 中的原生驱动、锁文件与许可证。不要复制 `.build` 或 Cargo `target`。已安装 `stcxx-toolchain` 与 `stc-cli` 时，Arduino 使用其工具路径；否则在平台目录创建 `platform.local.txt`，填入实际路径：

```ini
runtime.tools.stcxx-toolchain.path=D:/tools/stcxx-toolchain
runtime.tools.stc-cli.path=D:/Git/stc51/stc-cli/target/release
```

重新加载开发板数据，选择 `mcs51` 下的实际芯片。当前尚未发布开发板管理器索引，请使用上述方式手动安装。

## 时钟和烧录

本版 C++ 配置固定为 **12 MHz / large / stack-auto**；ISP 中的实际时钟必须设为 12 MHz。STC89 配置按 12T 定时器模式处理。时钟菜单不会修改芯片的 ISP 设置。

当前 14 个 variant 均有 `stc-cli` UART 烧录实现。STC89C58RD+ 和 STC12C5A60S2 的 IDE 上传速率为 19200 baud，其余为 115200；这是 ISP 传输速度，与 sketch 的 `Serial.begin()` 无关。请重新构建配套 `stc-cli`，使用包含旧系列 15 秒擦除等待修复的可执行文件。

所有型号通过 UART ISP 上传；默认 RX=P3.0、TX=P3.1。IDE 选择串口后上传，按烧录器提示重新上电。Ai8 无可靠型号 ID 的路径使用所选型号，需核对芯片丝印。也可离线检查及手动上传：

```powershell
../stc-cli/target/release/stc-cli.exe validate --expect STC8G1K08A --execution-mode mcs51 --file <固件.hex>
../stc-cli/target/release/stc-cli.exe flash --port COM5 --expect STC8G1K08A --execution-mode mcs51 --file <固件.hex> --reset manual
```

`SerialEcho` 使用 2400 baud，以兼容 12 MHz 下 STC89 的波特率分频；STC8 可按可实现的分频设置其他波特率。

## 接口与兼容性

提供 C++11 sketch、GPIO、Timer0 时间基准、按型号提供的 UART1–4、INT0/INT1、ADC，以及 String、Print、Stream、IPAddress。附带 Wire、SPI、SoftwareSerial、LiquidCrystal 和 Stepper。

- `Serial` 和 `Serial1` 对应 UART1，其余串口按 `STC_CORE_UART_COUNT` 提供。支持 8N1，每路默认 16 字节 RX 环形缓冲区，可保存 15 字节；发送同步等待完成。UART1 使用 Timer1，其他串口占用对应定时器或 BRT，避免资源冲突。
- Wire 默认软件主机 SDA=P3.2、SCL=P3.3，支持 ACK/NACK、重复 START 和超时。选择 variant 的有效硬件路由后可自动使用硬件主机；STC8/Ai8 提供硬件从机，以 `WIRE_HAS_SLAVE` 判断。STC12/STC15/STC89 不提供硬件从机。总线需外部上拉，软件时钟为近似值；缓冲区默认各 32 字节。
- SPI 支持主机和软件回退，使用 `usingHardware()` 查询路径。STC12/STC15 的模式 0/2 使用软件回退；STC89 无硬件 SPI。SS 由 sketch 控制，不提供 SPI 从机、DMA 或异步完成回调。
- `delayMicroseconds()` 要求 Timer0 已启动，支持 0–65535 微秒；屏蔽中断期间丢失的 `millis()`/`micros()` 节拍不会补回，不应在长期关中断时依赖 `delay()` 或计时超时。
- `analogWrite()` 当前仅按阈值输出高/低电平，不提供硬件 PWM。USB/CDC/HID、CAN、DMA、EEPROM API、`tone`/`noTone` 和 Servo 尚未接入。ADC 默认输出 10 位结果，按 variant 使用 A0 等别名。

MCS51 为小端，`int`/`size_t` 为 16 位，`long`/`ptrdiff_t` 为 32 位，通用指针为 24 位，函数指针为 16 位，`float`/`double` 为 32 位。不能混用 MCS251 对象或 ABI。支持全局构造和虚函数，不支持异常、RTTI、线程、完整 STL 和全局析构。DATA/IDATA 与调用栈共用 256 字节；链接成功不代表峰值栈安全。8 KiB Flash 型号可能无法容纳复杂库组合，链接器按真实容量拒绝超限。

UART1 默认将 5 字节高频状态放入 DATA；Wire 默认引脚使用位指令快速路径，占用额外 1 字节 DATA。内存优先时可在平台级 `build.extra_flags` 中加入 `-DSTC_SERIAL_STATE_IN_DATA=0 -DSTC_WIRE_FAST_DEFAULT=0`，使核心和库统一重编译；仅在 sketch 顶部定义宏不会改变其他翻译单元。

## 源码维护

Core 已按当前 MCS251 的结构整理：公共 Arduino 类在 `cores/STC`，硬件实现位于 `hal`，基础运行库位于 `runtime/include` 和 `runtime/src`。`cpp` 目录已移除。共享文件的唯一维护源与 MCS51 专属 ABI 文件见 [来源清单](cores/STC/sdk-sources.json)；安装后的平台可独立编译。

```powershell
node scripts/sync-core-sdk.mjs --check
node tools/variants/generate.mjs --check
node scripts/build-native-driver.mjs
```

上游文件修改后，审核变更并运行 `node scripts/sync-core-sdk.mjs` 更新副本。MCS251 的指针与栈优化必须单独核对 MCS51 ABI，不能直接覆盖。同步检查不通过时可保留已审核的本地副本，在 `tools/stcxx-driver` 目录执行 `cargo build --release --locked --offline`，再将 `target/release/stcxx.exe` 复制到该目录；不要从仓库根目录构建而遗漏子目录的静态 CRT 配置。

## 验证状态

2026-09-26 清理前的性能复查完成 126 组生产 C 行为测试，以及 116 个双构建器组合：112 通过、4 项既有容量不足。通过镜像的 460 个中断向量匹配链接符号；两次 Arduino CLI 无诊断退出经独立复测通过，原退出原因未确定。73 个可比通过组合的固件减少 53–658 字节。冷核心缓存处理阶段中位耗时从 4.78 秒降至 0.39 秒，不能据此推算整次编译或 MCU 运行速度。

上述为清理前记录；独立测试文件、报告和临时产物已移除。当前仅验证 Windows x64 构建与 HEX 合法性，没有本项目的实板周期、峰值栈或持续负载验收；Linux/macOS 支持也未完成。

## 来源与许可

本项目核心及附带库源于 `arduino-mcs251` 历史 MCS51 实现，基础运行库源于 `stcxx/sdk/runtime`；`tools/compiler` 是适配 MCS51 的独立 Rust 驱动副本，`tools/stcxx-driver` 是 Arduino 构建适配器。共享源清单记录于 `cores/STC/sdk-sources.json`，安装包保留副本，可独立使用。

项目和驱动的 MIT 许可证见 [LICENSE](LICENSE)；Arduino 衍生代码及第三方声明见 [LICENSES/README.md](LICENSES/README.md) 和源文件中的版权头，LGPL 全文保留于 [LICENSES/LGPL-2.1.txt](LICENSES/LGPL-2.1.txt)。Rust 依赖许可随原生驱动保留于 `tools/stcxx-driver/LICENSES`。

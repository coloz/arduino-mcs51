# arduino-mcs51

面向 STC MCS51 芯片的独立 Arduino C++11 平台，版本 **0.0.1**。FQBN 为 `stc:mcs51:<板型 ID>`，可与 `stc:mcs251` 并存。当前完成 Windows x64 编译链适配；尚未进行本项目的实板验收。

芯片定义、引脚和 C HAL 来自工作区 `arduino-mcs251` 中保留的 MCS51 历史实现；构建适配器基于当前原生 Rust 驱动，使用现有 STCXX 0.3.0 的 Clang → LLVM-CBE → SDCC 工具链和 `stc-cli` 烧录器。具体来源见 [SOURCES.md](SOURCES.md)。

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

仅上述型号列入本版支持范围。同系列其他容量、封装不能直接视为同一板型。存储容量、端口掩码和 ADC 通道以 [devices.json](tools/variants/devices.json) 为准。引脚编码为 `(port << 4) | bit`，例如 `P3_2`；未假定任何板载 LED，Blink 需外接 LED 与限流电阻。

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

所有型号通过 UART ISP 上传；默认 RX=P3.0、TX=P3.1。IDE 选择串口后上传，按烧录器提示重新上电。Ai8 无可靠型号 ID 的路径使用所选型号，需核对芯片丝印。也可离线检查及手动上传：

```powershell
../stc-cli/target/release/stc-cli.exe validate --expect STC8G1K08A --execution-mode mcs51 --file <固件.hex>
../stc-cli/target/release/stc-cli.exe flash --port COM5 --expect STC8G1K08A --execution-mode mcs51 --file <固件.hex> --reset manual
```

`SerialEcho` 使用 2400 baud，以兼容 12 MHz 下 STC89 的波特率分频；STC8 可按可实现的分频设置其他波特率。详见 [兼容性说明](COMPATIBILITY.md)。

## 源码维护

Core 已按当前 MCS251 的结构整理：公共 Arduino 类在 `cores/STC`，硬件实现位于 `hal`，基础运行库位于 `runtime/include` 和 `runtime/src`。`cpp` 目录已移除。共享文件的唯一维护源与 MCS51 专属 ABI 文件见 [来源清单](cores/STC/sdk-sources.json)；安装后的平台可独立编译。

```powershell
node scripts/sync-core-sdk.mjs --check
node tools/variants/generate.mjs --check
node scripts/build-native-driver.mjs
```

上游文件修改后，审核变更并运行 `node scripts/sync-core-sdk.mjs` 更新副本。平台支持范围见 [COMPATIBILITY.md](COMPATIBILITY.md)，发布验证状态见 [VALIDATION.md](VALIDATION.md)。

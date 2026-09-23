# 双构建器编译回归

在 Windows x64 上通过未经修改的 Arduino CLI 和 aily-builder 编译、链接，并使用 `stc-cli validate` 检查每个成功产出的 HEX。不会上传或操作串口。

从 SDK 根目录运行：

```powershell
node scripts/build-native-driver.mjs
node scripts/test-compile.mjs --aily D:/Git/aily-project/aily-builder/dist/main.js --workers 8 --jobs 2
```

默认自动发现 `examples`、`libraries/*/examples` 的全部主 sketch，并加入 `tests/compile/common` 的 24 个常见应用及硬件接口探针；在 `devices.json` 的全部 14 个板型上分别使用两种构建器。附属 `.ino` 不单独计数，C/C++ 附属源文件由构建器正常处理。测试程序为本仓库编写的典型 Arduino 用法，数字引脚按 SDK 的 `Pn_m` 编码适配，不代表对第三方库生态的全面认证。

可限制范围：

```powershell
node scripts/test-compile.mjs --aily D:/Git/aily-project/aily-builder/dist/main.js --boards stc8h8k64u --suites examples --run examples-only
node scripts/test-compile.mjs --aily D:/Git/aily-project/aily-builder/dist/main.js --boards stc8g1k08a --suites common --cases BlinkWithoutDelay,WireScanner
node scripts/test-compile.mjs --aily D:/Git/aily-project/aily-builder/dist/main.js --boards stc8h8k64u --suites boundaries --run boundaries
```

`--builders arduino` / `--builders aily` 可单独运行；`--toolchain`、`--arduino-data` 沿用工作区配置，`--cli`、`--validator` 可指定工具。aily-builder 入口也可以通过 `AILY_BUILDER` 环境变量指定。使用仓库构建版本时，其根目录应有 `ninja/ninja.exe`。

每轮使用新的 `.build/qualification/<run>` 目录，保留以下文件，不覆盖已有轮次：

- `metadata.json`：工具版本、源代码状态、驱动与输入哈希、实际矩阵。
- `logs/*.log`：完整参数（JSON 参数数组，无 shell 拼接）、诊断及 HEX 校验结果。
- `results.jsonl`：逐条即时结果，便于在长任务期间查看进度。
- `results.json`、`summary.json`：完整结果、两构建器状态与固件映像比较。
- `b/*`：编译产物、HEX、SDCC `.mem` 以及构建器缓存元数据；短目录名避免测试框架本身耗尽 Windows 路径长度。

每轮使用独立结果目录；构建器可以正常复用本机对象缓存。Arduino CLI 默认使用 `.build/qc` 下按轮次和板型隔离的标准核心缓存，HEX/ELF/MEM 另外复制到各用例目录；`--arduino-cache false` 可强制每个用例使用新的完整构建目录。CLI 显式传入 `--build-path` 会关闭全局核心缓存，见 [官方升级说明](https://docs.arduino.cc/arduino-cli/UPGRADING)。aily 生成和复用 `.build/qualification/archive-cache` 中的本地归档缓存，远程下载被关闭。`pass` 要求编译退出成功、HEX 校验和/地址/容量合法、`.mem` 内存统计可读取且未超出型号容量，以及 `stc-cli validate` 成功。两个构建器的 HEX 按地址和字节归一化后比较，不依赖记录排版。

如需中止长任务，在该轮目录创建 `STOP` 文件；正在编译的项目结束后停止调度，并写出已完成结果。未完成全部计划项时返回非零。原进程退出后，以相同选项加 `--resume true` 继续同一 `--run`，只运行尚未记录的项；主 `.ino`、驱动、`platform.txt`、`boards.txt` 或测试矩阵变化时会拒绝恢复。更换工具链、构建器或其他依赖源文件后，应使用新的轮次名完整重跑。

`capacity` 是容量拒绝，**不是通过**。其他失败分为 `missing-header`、`compile-error`、`artifact-error` 和 `timeout`。任意失败都会令脚本返回非零，包含明确要求执行的 `boundaries` 探针；不能用缺少输出的成功退出掩盖问题。超时为每次编译 5 分钟。

`common` 覆盖非阻塞定时、按键消抖、ADC、analogWrite、Stream 串口解析、String、动态数学、PROGMEM/F、Print/Printable、IPAddress、外部中断、移位、测脉宽、I²C 扫描和寄存器读取、SPISettings/缓冲区传输、LCD、Stepper、SoftwareSerial，以及多文件 C/C++ 和自动函数原型。`boundaries` 单独探查 tone、EEPROM、Servo 和 Wire 从机边界；Wire 从机现已按型号支持，另有默认矩阵中的 WireSlaveSupported 探针。

STC8C2K64S4 和 STC89C58RD+ 的 SDK 配置未提供 ADC/A0；AnalogReadSerial、MathFunctions 保留对 `A0` 的调用，用来暴露这一适用范围。在原始结果中仍记为编译失败，汇总报告单列为 ADC 未提供。

早期基线结果见 [RESULTS.md](RESULTS.md)，逐项数据见 [RESULTS.json](RESULTS.json)。保留原始轮次目录时，可重新生成本次报告：

```powershell
node scripts/report-compile.mjs --run matrix --overlays peripheral-fixed,cpp-recheck --boundaries boundaries-final
```

编译成功仅说明语法、ABI、链接及静态资源检查通过。`analogWrite` 当前是数字阈值输出；无 ADC 的板型不会因此得到 ADC 能力。LCD 接线、串口波特率、轮询串口时序、栈峰值和实际硬件行为仍需实板验证，见 [COMPATIBILITY.md](../../COMPATIBILITY.md)。

当前接口适配矩阵见 [PERIPHERAL_RESULTS.md](PERIPHERAL_RESULTS.md)，主机行为测试说明见 [../peripherals/README.md](../peripherals/README.md)。

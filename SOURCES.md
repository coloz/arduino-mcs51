# 来源与维护边界

2026-09-22 按当前 MCS251 core 结构整理：公共 Arduino 类位于 core 根目录，硬件 C 实现位于 `hal`，基础运行库位于 `runtime/include` 与 `runtime/src`。共享文件和目标专属文件逐项登记在 `cores/STC/sdk-sources.json`：33 个基础运行库文件来自 `stcxx/sdk/runtime`，14 个公共 API 文件来自当前 `arduino-mcs251/cores/STC`。MCS51 配置、运行时 ABI 声明、堆和 6 个 libc 桥接文件独立维护；同步脚本不会覆盖它们。安装包中的同步副本用于独立构建，开发时通过 `sync-core-sdk.mjs --check` 检查漂移。

同步包含 String 数字进制格式、Stream 浮点解析、Arduino 命名空间与 MultiTarget 兼容修复；两套编译桥均保留 CBE 请求的原生 math.h，运行库增加 copysignf。原始来源如下，不能将它们的历史测试结果视为本次重构验证。

- `cores/STC`、五个附带库、STC8/Ai8 型号数据库和 variant 生成代码：工作区 `arduino-mcs251` 的 `b30afd6`，该版本仍保留 MCS51 支持。STC12/STC15/STC89 型号条目取自 `be55db6`。本项目重新生成板卡定义，未复用历史验证结论。
- `tools/compiler`：工作区 `stcxx/compiler` 0.3.0 原生 Rust 实现的独立副本，适配为 MCS51。当前上游明确拒绝 MCS51，因此此处保留独立源码，避免改变现有 MCS251 平台的行为。后续上游修复需要单独审核合入。
- `tools/stcxx-driver`：当前 `arduino-mcs251` 的 `a5af558` 构建适配器，Cargo 依赖改为本项目的 `../compiler`。MCS51 前端 ABI 来自历史双目标锁；Windows 工具和运行库 SHA-256 来自现有 0.3.0 工具包锁文件。
- 烧录能力与容量校验使用工作区 `stc-cli`；本次只运行离线 validate，没有连接芯片或执行擦除、烧录。

产品资料按数据库中的 `official_url` 逐项保留。创建时复核了宏晶的 [STC8H8K64U](https://www.stcmicro.com/cn/stc/stc8h8k64u.html)、[STC8G1K08](https://www.stcmicro.com/cn/stc/stc8g1k08.html)、[STC8H1K08](https://www.stcmicro.com/cn/stc/stc8h1k08.html) 产品页。端口掩码仍需按实际封装核对，不能由整个系列的最大引脚数推断某块板的引脚。

项目与驱动采用原有 MIT 许可证；附带 Arduino 衍生代码及其许可证保留于源文件和 `LICENSES`，Rust 依赖许可证保留于 `tools/stcxx-driver/LICENSES`。编译工具本身仍由独立 STCXX 工具包提供。

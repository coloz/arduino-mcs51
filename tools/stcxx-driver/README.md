# MCS51 原生 Arduino 构建适配器

此适配器使用 `../compiler` 中独立维护的 MCS51 驱动，支持 Arduino CLI 的预处理、C/C++ 编译、缓存归档、链接、HEX 导出和容量显示。构建时不依赖脚本解释器；Node.js 仅用于维护脚本。

从项目根目录运行 `node scripts/build-native-driver.mjs`。Windows x64 工具锁固定匹配 STCXX 0.3.0；SDCC 使用 `-mmcs51`、`sdas8051` 和 `lib/large-stack-auto`，C++ 使用 `msp430-stc51-none-eabi`。运行时尺寸和 IR 目标均进行校验。

`tools/compiler` 只暴露本平台的 Arduino 配方和 `package-platform` 命令；独立 SDK 子命令已移除。MCS251 工具链与 Arduino 平台继续由相邻项目维护。

# 0.0.1

首次发布独立 `stc:mcs51` 平台，提供 14 个 MCS51 板型，以 STC8C/G/H 为重点。恢复 MCS51 小端 C++ ABI、16 位程序指针、SDCC large-stack-auto 运行库和低地址 HEX 上传配置，提供可复现的本地构建脚本。

本版为 Windows x64 实验开发版，提供手动安装包 `arduino-mcs51-0.0.1-windows-x86_64.zip` 及 SHA-256 元数据文件。安装包包含原生驱动，STCXX 0.3.0 工具链和 `stc-cli` 需另行安装；尚未提供开发板管理器索引。安装方法见 [README.md](README.md)。

编译与 API 验证记录见 [VALIDATION.md](VALIDATION.md)；未完成实板验收，支持范围与限制见 [COMPATIBILITY.md](COMPATIBILITY.md)。


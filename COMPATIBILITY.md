# 支持范围

公共类行为以 SHA-256 锁定的官方 ArduinoCore-API 1.5.2 为对照，123 项主机检查覆盖 String、Print/Printable、Stream、IPAddress 和 Client。已提供 `arduino::` 类型别名、`pin_size_t`/`PinStatus`/`PinMode`、`bitToggle`，保留既有全局类名和扩展接口。目标构建还验证 C/C++ 头文件隔离、动态数学调用和全局构造函数。此范围不等于所有官方库或硬件功能都已实现。

本版提供 C++11 sketch、C 驱动、GPIO、Timer0 时间基准、按型号提供的 UART1–4、INT0/INT1、按型号声明的 ADC，以及 String/Print/Stream/HardwareSerial。附带 Wire（硬件/软件 I²C 主机，部分型号支持硬件从机）、SPI（硬件主机/软件回退）、SoftwareSerial、LiquidCrystal、Stepper。SPI 和 Wire 的默认引脚见各 variant；引脚可配置，需避免与中断或其他外设同时占用。

`analogWrite` 当前按 Arduino 无 PWM 引脚的规则输出高/低电平，**不提供硬件 PWM**。USB、USB CDC/HID、CAN、DMA、EEPROM API 尚未接入。UART、IIC、SPI 数量、路由、模式限制与其他未适配接口见 [硬件接口说明](HARDWARE_INTERFACES.md)。未提供 `tone`/`noTone`，SDK 也不附带 Servo、EEPROM 库；这些调用的编译边界探针位于 `tests/compile/boundaries`。STC8H8K64U/Ai8 的芯片硬件带 USB 不意味着本平台提供 USB 接口或 USB 上传。

MCS51 ABI 为小端：`int` / `size_t` 16 位，`long` / `ptrdiff_t` 32 位，通用指针 24 位，函数指针 16 位，成员数据指针 16 位，成员函数指针 32 位，`float` / `double` 32 位。禁止混用 MCS251 的目标文件、头文件 ABI 或高地址 HEX。Clang 使用定制 `msp430-stc51-none-eabi` 前端，实际机器代码由 SDCC `-mmcs51` 生成。

支持全局构造、虚函数和成员函数指针的编译适配；异常、RTTI、线程、完整 STL、全局析构、可变长栈分配不受支持。MCS51 的 DATA/IDATA 与调用栈共用 256 字节，链接成功不能证明深层 C++ 调用不会耗尽栈。

每个型号显式保留 512–4096 字节 XDATA 堆。8 KiB Flash 型号适合小程序；String、浮点打印、总线库叠加可能超出 Flash/XRAM，链接器按型号限制拒绝溢出。STC89C51RC/STC89C52RC 等较小 XRAM 型号未加入此 C++ 配置。

ADC 默认按 Arduino 10 位结果输出，可通过 `analogReadResolution` 调整。不同型号的通道编号与引脚映射不同，使用 A0 等 variant 别名。STC8G1K08A 的 6 路 ADC 映射到 P3.0–P3.3、P5.4–P5.5，不要套用 STC8G1K08 的引脚配置。

全部板型均为实验性移植，当前仅验证 Windows x64 构建和 HEX 合法性。Linux/macOS 原生驱动和锁文件、实板时钟/串口/总线/中断/ADC 验收尚未完成。历史上其他项目的实板或模拟器结果不作为本项目的验证结果。

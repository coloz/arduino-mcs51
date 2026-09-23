# MCS51 硬件接口核对与适配

本轮按官方型号资料和寄存器手册补齐接口。下表是物理控制器数量，不是可同时使用且互不冲突的引脚组数。芯片最大逻辑引脚与具体封装仍须分别核对。

外设数量不代表完整 Arduino 驱动一定能装入该型号。8 KiB 板型的部分新接口探针会超出 Flash，需按测试报告选择程序和型号；本轮没有放宽容量限制。

| 型号 | UART | IIC | SPI |
| --- | ---: | ---: | ---: |
| STC8H8K64U | 4 | 1 | 1 |
| STC8H1K08 | 2 | 1 | 1 |
| STC8G1K08A | 1 | 1 | 1 |
| STC8G1K08 | 2 | 1 | 1 |
| STC8G2K64S4 | 4 | 1 | 1 |
| STC8C2K64S4 | 4 | 1 | 1 |
| STC8H1K28 | 2 | 1 | 1 |
| STC8H3K64S4 | 4 | 1 | 1 |
| Ai8H2K12U | 2 | 1 | 1 |
| Ai8H2K32U | 2 | 1 | 1 |
| STC15F2K60S2 | 2 | 0 | 1 |
| STC12C5A60S2 | 2 | 0 | 1 |
| STC89C58RD+ | 1 | 0 | 0 |
| STC15W4K32S4 | 4 | 0 | 1 |

数据源及完整路由位于 [devices.json](tools/variants/devices.json)，由生成器同步到 `boards.txt`、各型号的 `pins_arduino.h` 和 `variant.json`。官方依据：[STC8H 手册](https://www.stcmicro.com/datasheet/STC8H-cn.pdf)、[STC8G 手册](https://www.stcmicro.com/datasheet/STC8G-en.pdf)、[STC8C 手册](https://www.stcmicro.com/datasheet/STC8C-cn.pdf)、[Ai8 手册](https://www.stcmicro.com/datasheet/Ai8-cn.pdf)、[STC15F2K60S2 手册](https://www.stcmicro.com/datasheet/STC15F2K60S2-en.pdf)、[STC15W4K32S4 手册](https://www.stcmicro.com/datasheet/STC15W4K32S4-en.pdf)、[STC12C5A60S2 手册](https://www.stcmicro.com/datasheet/STC12C5A60S2-en.pdf)、[STC89C5x 型号资料](https://www.stcmicro.com/cn/stc/stc89c51rc.html)。

## 串口

`Serial` 和 `Serial1` 对应 UART1；根据 `STC_CORE_UART_COUNT` 提供 `Serial2`、`Serial3`、`Serial4`。每路独立接收缓冲区默认 16 字节，环形队列可保存 15 字节；发送同步等待完成。支持 `begin/end/available/peek/read/write/flush/overflow`、8N1 和 `setPinsChecked(rx, tx)`；配置失败通过 `configurationError()` 返回。仅能在停止状态下切换到该串口完整的硬件路由。

```cpp
#if STC_CORE_UART_COUNT >= 2
Serial2.setPinsChecked(PIN_SERIAL2_RX, PIN_SERIAL2_TX);
Serial2.begin(9600);
if (!Serial2.configurationError()) Serial2.write((uint8_t)0x55);
#endif
```

UART1 使用 Timer1，现代型号 UART2/3/4 分别占用 Timer2/3/4；STC12 UART2 使用独立 8 位 BRT。额外串口会拒绝已运行的定时器或已开启的串口，并在 `end()` 恢复借用的定时器和路由字段。不要让其他库同时操作这些资源。UART1 保留原来的 Timer1 保存/恢复行为。

STC8 的 TM2PS/TM3PS/TM4PS 是 FEA0/FEA1/FEA2；本项目两个 Ai8 型号为 FEA2/FEA3/FEA4，且有独立 TM1PS=FEA1。配置时保存并清除相关预分频，结束时恢复。MCS51 UART4 为 S4CON=84H、S4BUF=85H，T4T3M=D1H，不能套用 MCS251 地址。

波特率误差超过 3% 时拒绝配置；12 MHz / 12T 的 STC89 不能准确产生 9600 baud，已有示例使用 2400。拒绝无效波特率会报告失败；若原来已有有效会话，该会话保留。非 8N1 配置仍按原 API 行为停止该串口。

## Wire / IIC

`Wire` 保留原软件主机默认 SDA=P3.2、SCL=P3.3。选择生成的硬件路由后，主机在可实现时钟范围内自动使用硬件；低于控制器最低速率时回退软件。`usingHardware()` 可查询当前路径。

```cpp
#if STC_CORE_I2C_COUNT
Wire.setPinsChecked(PIN_I2C1_SDA, PIN_I2C1_SCL);
#endif
Wire.begin();
Wire.setClock(100000);
Wire.setWireTimeout(25000, true);
```

STC8/Ai8 增加硬件从机：`begin(address)`、`onReceive`、`onRequest`，由 `WIRE_HAS_SLAVE` 表示能力。全新对象直接 `begin(address)` 会选择首个可用硬件引脚组；已有主机或自定义引脚时，应先 `end()` 并明确选择硬件引脚。地址为 1–127；回调在中断中运行，保持简短，避免阻塞或再次发起主机事务。一次只使用主机或从机模式，不支持多主机仲裁。

STC12/STC15/STC89 使用软件 IIC 主机，`WIRE_HAS_SLAVE=0`；从机入口保留统一类签名，但调用返回配置错误，不会伪装成硬件从机。所有总线均需适当外部上拉，软件时钟为近似值。不要在长期屏蔽中断时依赖基于 `micros()` 的等待超时。

收发缓冲区默认各 32 字节；支持重复 START、ACK/NACK、超时状态及恢复。零长度读取在 `sendStop=true` 时会释放前一次保留的总线。本次配置的每个型号最多只有一个 IIC 控制器，因此没有 `Wire1`。

## SPI

保留原 `PIN_SPI_*` 软件默认接线。`PIN_SPI1_*` 是该型号首个可用硬件路由；可使用其他经生成器确认的完整 MOSI/MISO/SCK 组合。SS 由 sketch 控制，可选任意不重叠的有效 GPIO。`SPI.usingHardware()` 区分硬件和软件路径；`setPinsChecked`、`beginTransactionChecked`、`configurationError` 报告引脚、设置、忙和超时错误。嵌套事务会被拒绝且不会提前恢复中断。

```cpp
#if STC_CORE_SPI_COUNT
SPI.setPinsChecked(PIN_SPI1_MOSI, PIN_SPI1_MISO, PIN_SPI1_SCK, PIN_SPI1_SS);
#endif
SPI.begin();
SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE1));
// 拉低实际使用的片选 GPIO，transfer，再拉高片选。
SPI.endTransaction();
```

- STC8G/STC8C、STC8H1K08/1K28 使用 /4、/8、/16、/32；STC15F 和 STC12 为 /4、/16、/64、/128；STC15W4K32S4 为 /4、/8、/32、/64。
- STC8H8K64U、STC8H3K64S4 的 SPI 选择器 3 在不同芯片修订版为 /32 或 /2。本轮这些型号及 Ai8 只使用共同可核实的 /4、/8、/16，避免意外超速。选出的硬件 SCLK 不高于请求值；太低的速率回退软件。
- STC12/STC15 手册明确将 CPHA=0 与 SSIG=1 组合标为未定义，因此模式 0/2 保留软件回退；模式 1/3 启用硬件。STC8/Ai8 支持四种主机模式。软件支持四种模式与两种位序。
- STC12 英文手册的备用 SPI 引脚表与 SPI 章节前后矛盾，只启用一致的默认 MOSI=P1.5、MISO=P1.6、SCK=P1.7 路由。Ai8 会保存并清除 MOSI/MISO 交换位，结束时恢复。
- STC89 无独立 SPI，仍提供软件 `SPI`。无第二控制器，不提供 `SPI1` 对象；`PIN_SPI1_*` 中的数字是物理控制器编号。

不提供 SPI 从机、DMA 或异步完成回调。GPIO 回退的实际速度受软件执行开销影响。

## 其余接口核查

| 接口 | 当前适配范围 |
| --- | --- |
| GPIO、ADC | 按型号引脚掩码和 ADC 表提供；无 ADC 的板型不提供 A0 |
| Timer0 | `millis/micros/delay` 时间基准，无通用定时器用户类 |
| INT0/INT1 | FALLING；STC8/Ai8/STC15 支持 CHANGE，STC12/STC89 支持 LOW；其余模式明确拒绝 |
| INT2–INT4、端口中断 | 未提供 Arduino 驱动 |
| PWM/PCA | `analogWrite` 仍为高低电平阈值输出，未提供硬件 PWM/PCA |
| EEPROM/IAP | 部分芯片具有 Flash IAP，未提供 EEPROM 库 |
| USB CDC/HID、RTC、看门狗、比较器 | 芯片能力依型号而异，尚未提供对应 Arduino 库 |
| tone、Servo | 尚未提供 |
| SoftwareSerial、LiquidCrystal、Stepper | 已有软件库，纳入完整编译回归；SoftwareSerial 为轮询实现 |

`interruptModeSupported()` 可查询模式；`attachInterruptChecked()` 返回 0 成功、1 无效参数、2 不支持的触发模式。普通 `attachInterrupt()` 保留 Arduino 的 void 签名。现代系列的 IT0/IT1=0 是双边沿，不能当作 LOW 电平触发。

## 验证与上板验收

执行 `node tests/peripherals/run.mjs` 运行 14 型号 × 5 类驱动的 70 组真实 C 源码寄存器行为测试。完整编译包含 9 个 SDK 示例和 24 个应用/接口探针、14 个板型、两种构建器，共 924 个组合。最终结果及容量限制见 [PERIPHERAL_RESULTS.md](tests/compile/PERIPHERAL_RESULTS.md)。

这些测试不会操作串口或烧录。尚未完成实板时序、封装接线、各修订版、接收压力和中断栈峰值验证。上板时应分别测试各 UART 自环及同时收发、IIC 主机连接已知地址设备及两板从机回调、SPI MOSI/MISO 自环并用逻辑分析仪检查四模式和实际 SCLK；再检查结束/重新初始化和总线超时恢复。容量拒绝不计为通过。

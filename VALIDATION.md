# Validation status

The MCS51 platform is an experimental Windows x64 port using the locked STCXX 0.3.0 toolchain.

Historical qualification before the UART/IIC/SPI adaptation, completed on 2026-09-22, using Arduino CLI 1.5.1 and aily-builder 1.2.17:

- All 9 SDK examples and 20 common Arduino application fixtures were compiled for all 14 board profiles with both builders: 812 combinations.
- The final matrix contains 682 passes, 122 Flash-capacity rejections and 8 ADC/A0 cases that are not supported by the selected SDK board profile. Capacity rejections and unsupported cases are not passes. No other failures remain in the final matrix after the separately recorded fixes and retests.
- Every passing image passed HEX checksum/address checks, model-specific `stc-cli validate` and static Flash/XDATA accounting.
- All 406 builder pairs agree on status. All 341 passing pairs agree on Flash usage; 103 also have identical addressed HEX bytes. Runtime equivalence of differing images has not been established.
- Repeating 10 selected builds in their original build directories produced identical HEX files.
- Native adapter unit tests: 6 passed. Compiler unit tests: 25 passed, 2 explicitly ignored subprocess fixtures. The Windows long-path regression and the originally failing long-path sketch build passed after the fix.
- The 47-file SDK synchronization check and generated variant consistency check passed.
- Eight separate boundary checks at that time confirmed that the SDK did not provide EEPROM, Servo, tone/noTone or Wire slave APIs.

The matrix includes retests of the corrected library examples and the ADC guard in PeripheralSmoke. One Arduino CLI invocation of STC8H3K64S4/CppRuntime exited without compiler diagnostics; an independent retest passed. Both records are retained, and the cause of that isolated exit is undetermined.

See the [full report](tests/compile/RESULTS.md), [per-case records](tests/compile/RESULTS.json) and [rerun instructions](tests/compile/README.md). Source fixtures and scripts are retained; raw logs and build artifacts are under `.build/qualification`.

Earlier qualification recorded 123 portable API behavior comparisons matching ArduinoCore-API 1.5.2. Those historical host comparisons were not rerun in this session and are not included in the counts above.

No physical-board or peak stack/heap qualification is claimed. Hardware/API limitations remain in [COMPATIBILITY.md](COMPATIBILITY.md).

## UART upload qualification

On 2026-09-23, all 14 current variants passed 121 offline CLI checks against the sibling `stc-cli`, including model mapping, physical/linker address boundaries and upload rejection before port access. The programmer passes 91 Rust tests, including full-capacity packets and delayed/failed erase handling. STC89/STC12 board upload speeds were corrected to 19200 baud for 12 MHz operation. See [the model matrix and rerun instructions](UPLOAD_SUPPORT.md). This is host validation, not physical-board qualification.

## Peripheral adaptation qualification

The current UART/IIC/SPI changes supersede the hardware-interface limitations in the historical baseline above. See [hardware coverage](HARDWARE_INTERFACES.md) and the [current full matrix](tests/compile/PERIPHERAL_RESULTS.md). The current checks include 70 register-behavior suites using the production C drivers, the 924-case dual-builder matrix, 6 adapter tests and 25 compiler tests (2 subprocess fixtures explicitly ignored). Capacity failures and unsupported board features are reported separately. Physical-board behavior remains unverified.

The completed current matrix has **754 passes, 162 capacity rejections, 8 no-ADC/A0 cases, and 0 other failures** after four independently logged Arduino CLI retests. All 462 builder pairs agree on status, all 377 passing pairs agree on Flash usage, and 194 have identical addressed HEX bytes. All 3,326 checked Timer0/UART/IIC vectors in the 754 passing images match the linked ISR symbols.

Eight board/sketch pairs that passed the historical matrix now exceed Flash because of the larger drivers (16 builder combinations): three 8 KiB SPITransactions profiles and five Ai8H2K12U applications. Their exact required sizes and limits are listed in the current report. These are recorded regressions, not passing or physically verified programs.

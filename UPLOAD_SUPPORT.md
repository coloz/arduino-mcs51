# UART upload support

Audited on 2026-09-23 against the actual `variants/*/variant.json`, generated
`boards.txt`, `platform.txt`, and the sibling `stc-cli` source tree.

All 14 current variants already have a UART programming implementation:
7 database records are `stable` and 7 are `experimental`. No model is missing
or limited to `probe-only`. This audit corrected erase handling and board upload
speeds; it did not invent new device IDs or promote hardware qualification.

## Model matrix

All program windows start at `0x0000`. Capacities below are physical program
Flash in bytes. E means `--allow-experimental`; U means
`--force-unverified-target`. The Arduino recipe supplies E for all boards and U
only for the two Ai8H boards.

| Variant/model | ID | Flash | Protocol | Database level | Required flags | Upload baud |
| --- | --- | ---: | --- | --- | --- | ---: |
| Ai8H2K12U | unpublished | 12288 | official/ai8h | experimental | E+U | 115200 |
| Ai8H2K32U | unpublished | 32768 | official/ai8h | experimental | E+U | 115200 |
| STC12C5A60S2 | D17E | 61440 | legacy/stc12 | stable | none | 19200 |
| STC15F2K60S2 | F408 | 61440 | legacy/stc15 | experimental | E | 115200 |
| STC15W4K32S4 | F525 | 32768 | legacy/stc15 | experimental | E | 115200 |
| STC89C58RD_PLUS | F108 | 32768 | legacy/stc89 | experimental | E | 19200 |
| STC8C2K64S4 | F7D4 | 65536 | legacy/stc8 | experimental | E | 115200 |
| STC8G1K08 | F754 | 8192 | legacy/stc8g | stable | none | 115200 |
| STC8G1K08A | F794 | 8192 | legacy/stc8g | stable | none | 115200 |
| STC8G2K64S4 | F764 | 65536 | legacy/stc8g | stable | none | 115200 |
| STC8H1K08 | F734 | 8192 | legacy/stc8g | stable | none | 115200 |
| STC8H1K28 | F724 | 28672 | legacy/stc8g | experimental | E | 115200 |
| STC8H3K64S4 | F744 | 65536 | legacy/stc8d | stable | none | 115200 |
| STC8H8K64U | F784 | 65536 | legacy/stc8d | stable | none | 115200 |

`STC89C58RD_PLUS` is a directory name: Arduino passes `STC89C58RD+`, which
resolves to `STC89C58RD+/LE58RD+`. STC8G1K08 resolves to
`STC8G1K08-20/16PIN`; STC8G1K08A resolves to `STC8G1K08A-8PIN`.
The other eight-pin STC8G1K08 package has a different ID and is not silently
substituted for the 20/16-pin variant.

STC15F2K60S2 deliberately has a smaller Arduino linker budget of 61433 bytes;
its programmer capacity remains 61440 bytes. Rounding the final transfer to a
512-byte erase boundary is valid. Four 64 KiB STC8 models expose the complete
`0000..FFFF` program window; their 512-byte default IAP allocation shares that
Flash and must not be counted as additional storage.

## Corrections

The programmer previously allowed 15 seconds for STC8D/STC32 erase responses,
but only the normal 500 ms serial timeout for STC89, STC12, STC15, STC8 and
STC8G. Ten current variants use those latter protocols. All legacy erase
commands now allow 15 seconds and restore the normal timeout afterward.
A missing, corrupt or wrong ACK, or a timeout-restoration failure, leaves the
session in `EraseFailed`: it cannot retry erase, write code/options or start
the MCU through that session. The erase packet and shared-IAP selection stay
unchanged.

The generated board upload speed for STC89C58RD+ and STC12C5A60S2 is now 19200
baud. At the platform's fixed 12 MHz clock, the old 115200 setting gives about
8.51% divider error for STC89 in 12T mode and 6.99% for STC12. At 19200 these
errors fall to about 2.34% and 0.16%. The independent programmer still honors
an explicit `--baud`; other clock configurations can require another speed.

The timing, divider and erase-packet reference is the project's pinned
[stcgal protocols.py](https://github.com/grigorig/stcgal/blob/fdf5fdd60515a260bb303dcf7b251c2b2671f91c/stcgal/protocols.py).
The source uses a 15-second post-identification timeout and reports divider
errors above 5%. Ai8H uses the separately documented
[Ai8 manual](https://www.stcmicro.com/datasheet/Ai8-cn.pdf), chapter 7,
printed pages 660-670; implementation details and source limits are retained
in the programmer's [protocol support document](../stc-cli/docs/PROTOCOL_SUPPORT.md).

## Verification

`stc-cli` passes 81 library tests, 8 binary unit tests and 2 CLI integration
tests (91 total), formatting and Clippy with warnings denied. New coverage
includes all 14 model/alias/ID/protocol/capacity mappings, last-byte HEX/BIN
acceptance, high-address and overflow rejection, 12 legacy variants' full-size
program packets, delayed and failed erase responses, and both Ai8H variants'
full-capacity parameter/erase/program sequences without an options write.

The platform's cross-repository check reads current variant metadata and board
properties and invokes the actual executable. It passed 121 offline CLI
checks across all 14 variants, including both linker and physical boundaries,
wrong execution mode, required experimental/unverified flags and rejection of
invalid uploads before port access. No physical serial or HID I/O is performed.

Run from `arduino-mcs51` after rebuilding `stc-cli`:

```powershell
cargo test --manifest-path ../stc-cli/Cargo.toml --locked
cargo build --manifest-path ../stc-cli/Cargo.toml --release --locked
node tools/variants/generate.mjs --check
node scripts/test-upload.mjs
# Or check a specific executable:
node scripts/test-upload.mjs --cli ../stc-cli/target/debug/stc-cli.exe
```

Each CLI check run records its matrix and small firmware fixtures under
`.build/upload-validation-*/results.json`.

## Hardware limits

These results establish host implementation and integration coverage, not
physical qualification of all 14 MCUs. Database `stable` means the exact model
has an upstream stcgal hardware-test record, not that this Rust binary or this
Arduino core has been hardware-qualified for it.

The platform uses UART ISP, including on U-suffix devices. It does not provide
STC8H8K64U/Ai8H native USB upload. Ai8H's published UART flow cannot verify the
physical model: manually match the chip and selected board before using U.
It preserves existing hardware options and requires a reset/power cycle to
run after programming. Other protocols also require the actual application
clock to match the 12 MHz build configuration. The Ai8H loader's fixed 24 MHz
clock is separate from the application's clock.

Programming can erase shared EEPROM/IAP data. Protocol ACK checking is not
an independent flash readback. Hardware acceptance still needs each concrete
package and bootloader revision, power-cycle entry, repeated uploads and
observable execution of the resulting firmware.

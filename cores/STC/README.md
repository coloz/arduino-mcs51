# STC MCS51 core

Public Arduino headers and C++ classes live at this directory's root. `hal/`
owns native C hardware services. `runtime/include` contains the Clang-only
freestanding headers; `runtime/src` contains language/runtime support.
Native C always uses SDCC's standard headers. There is no `cpp/` facade layer.

`sdk-sources.json` lists the shared files from `stcxx/sdk/runtime` and the public
classes from `arduino-mcs251`, plus the MCS51-owned ABI/runtime files. Run
`node scripts/sync-core-sdk.mjs` from the platform root after reviewing upstream
changes; `--check` detects drift without writing. Installed Arduino packages
contain regular copies and build without either sibling repository.

MCS51 owns its serial/backend, GPIO, timers, interrupts, heap and libc ABI
adapters. Its generic pointers are 24-bit, XDATA/function pointers 16-bit,
size_t 16-bit, and byte order little endian. Do not substitute MCS251 ABI files.

Board capabilities are generated from `tools/variants/devices.json`.
See the [platform README](../../README.md) for capabilities and validation limits.

# Validation status

The MCS51 platform is an experimental Windows x64 port using the locked STCXX 0.3.0 toolchain.

Qualification performed on 2026-09-22:

- 25 target compile/link cases passed across all 14 board profiles; generated HEX files passed model-specific address and capacity checks.
- 123 portable API behavior comparisons matched ArduinoCore-API 1.5.2.
- API and dynamic math clean/cached builds produced identical firmware.
- Compiler and Arduino adapter unit checks, target rejection and Flash overflow checks passed.
- Dynamic sqrt declarations, the copysign runtime fallback and MCS51 P1 global constructor handling were verified through target compilation.

This records the previously completed qualification. Local test scripts, fixtures, caches and detailed temporary reports have been removed from the source workspace.

No physical-board or peak stack/heap qualification is claimed. Hardware/API limitations remain in COMPATIBILITY.md. The platform package retains this status document as part of its release metadata.

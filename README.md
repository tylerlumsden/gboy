# gboy

A Game Boy (DMG) emulator written in modern C++20, built around compile-time
instruction dispatch and zero-cost instrumentation.

The CPU core is complete and validated against reference implementations. The
graphics pipeline is still under construction — see [Status](#status).

## Design highlights

**Compile-time instruction dispatch.** The SM83 instruction set is implemented
with template specialization and `if constexpr` rather than a 256-entry jump
table, so opcode behaviour is resolved at compile time. Flag computation is
handled by variadic templates constrained on `std::convertible_to<Byte>`, so a
single `carry_add` / `half_carry_add` implementation covers any operand count.

**Zero-cost logging.** Log statements below the build's `LOG_LEVEL` compile away
entirely — no runtime branch, no string formatting. A fold expression over
`std::index_sequence` fans each message out to every logger at or above its
severity, and `LOG_LEVEL` is a CMake cache variable, so a debug build gets full
per-instruction tracing while a release build pays nothing for it.

**Cycle-driven memory bus.** Memory access goes through a `Proxy` template with
an implicit `operator Byte()` and an `operator=`, so reads and writes read like
plain assignments while an injected access hook fires cycle accounting on every
one. Timing stays correct without scattering tick calls through the instruction
handlers.

**Differential testing against a reference implementation.** A dedicated
`Doctor` log level emits [Gameboy Doctor](https://github.com/robertheaton/gameboy-doctor)
trace format, so CPU state can be diffed instruction-by-instruction against a
known-good implementation. The core also passes Blargg's `cpu_instrs` suite in
full.

**Data-oriented architecture.** Plain structs holding state, free functions
operating over them, minimal class hierarchy. The `core` library has no
dependency on the frontend — the emulator hands out frame buffers through a
callback, so the CPU and memory system are testable headless.

## Status

| Component | State |
|---|---|
| SM83 CPU core | Complete — full base and CB-prefixed instruction set |
| Validation | Passes Blargg's `cpu_instrs`; Gameboy Doctor trace diffing |
| Interrupts | Working |
| Timer | Working |
| Cartridge / MBC1 banking | Working |
| **PPU / graphics** | **Work in progress — does not render yet** |
| Audio (APU) | Not started |
| Joypad input | Not started |
| MBCs beyond MBC1 | Not started |

This does not play games yet. The CPU is the finished part.

## Build

Requires CMake 3.16+ and a C++20 compiler. Everything builds under `-Wall -Werror`.

```sh
cmake --preset release
cmake --build build/release
./build/release/main path/to/rom.gb
```

Presets: `debug` (`LOG_LEVEL=0`, full tracing), `release` (`LOG_LEVEL=2`),
and `profile` (`RelWithDebInfo` with `-fno-omit-frame-pointer
-fno-optimize-sibling-calls` for accurate `perf` stacks).

## Profiling

```sh
script/profile.sh   # perf record against cpu_instrs.gb
script/flame.sh     # render a flamegraph via inferno
```

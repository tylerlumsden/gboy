# gboy 

Game Boy emulator written in modern C++20 with reasonably clean code in mind
- SM83 emulated core that passes all Blargg's reference cpu_instrs tests
- Working interrupts, ROM banking, and timer. (Display/PPU are WIP)
- Experimental conveniences like zero-cost logging, and compile-time instruction specialization

## Build and Run

### Requirements:

- C++20-compatible compiler
- CMake 3.16 minimum

```sh
# From the root directory:
cmake --preset release
cmake --build build/release

./build/release/main [Path to ROM]
```
## Acknowledgement

Implemented with the Pan Docs [https://gbdev.io/pandocs/](https://gbdev.io/pandocs/) document as the key technical reference. Much thanks to the authors of this project and the work done to compile the Game Boy's obscure behaviours.

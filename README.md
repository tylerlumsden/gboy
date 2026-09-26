# gboy 

Game Boy emulator written in modern C++20 with reasonably clean code in mind
- SM83 emulated core that passes all Blargg's reference cpu_instrs tests
- Working interrupts, ROM banking, and timer. PPU passes dmg-acid2 test.
- Both Super Mario Land and Tetris playable.
- Experimental conveniences like zero-cost logging, and compile-time instruction specialization

<img width="168" height="178" alt="image" src="https://github.com/user-attachments/assets/e3bf91c0-35f0-4245-bf7b-8f2fb4d8e66c" />


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

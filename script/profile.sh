cmake --build build/profile

perf record -F 999 -o perf.data ./build/profile/main resources/cpu_instrs.gb

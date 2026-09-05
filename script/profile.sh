cmake --build build/profile

perf record -F 999 -o perf.data -- timeout 10 ./build/profile/main resources/cpu_instrs.gb

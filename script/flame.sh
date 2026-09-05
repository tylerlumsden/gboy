perf script -i perf.data | inferno-collapse-perf | inferno-flamegraph > flame.svg
perf script -i perf.data | inferno-collapse-perf | flamelens
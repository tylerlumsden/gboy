# DEBUGINFOD_URLS= stops perf blocking for minutes on network symbol fetches.
DEBUGINFOD_URLS= perf script -i perf.data | inferno-collapse-perf | inferno-flamegraph --minwidth 0.1 > flame.svg
DEBUGINFOD_URLS= perf script -i perf.data | inferno-collapse-perf | flamelens

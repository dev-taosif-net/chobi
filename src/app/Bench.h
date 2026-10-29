// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

namespace chobi::bench {

// Startup/latency instrumentation for bench/run_bench.py. Enabled with CHOBI_BENCH=1; prints
// "[bench] <ms since start> <event>" to stderr. Zero cost when disabled.
void start();
bool enabled();
void mark(const char* event);

// CHOBI_BENCH_QUIT=1: exit right after the first video frame is painted.
bool quitAfterFirstFrame();

}  // namespace chobi::bench

// SPDX-License-Identifier: GPL-3.0-or-later
#include "app/Bench.h"

#include <cstdio>

#include <QElapsedTimer>
#include <QtGlobal>

namespace chobi::bench {
namespace {

QElapsedTimer& timer() {
  static QElapsedTimer t;
  return t;
}

bool g_enabled = false;
bool g_quit = false;

}  // namespace

void start() {
  g_enabled = qEnvironmentVariableIntValue("CHOBI_BENCH") == 1;
  g_quit = qEnvironmentVariableIntValue("CHOBI_BENCH_QUIT") == 1;
  timer().start();
}

bool enabled() { return g_enabled; }

bool quitAfterFirstFrame() { return g_quit; }

void mark(const char* event) {
  if (!g_enabled) return;
  std::fprintf(stderr, "[bench] %.3f %s\n", timer().nsecsElapsed() / 1e6, event);
  std::fflush(stderr);
}

}  // namespace chobi::bench

// SPDX-License-Identifier: GPL-3.0-or-later
#include "core/TimeFormat.h"

#include <cmath>
#include <cstdio>

namespace chobi::core {

std::string formatTime(double seconds, bool forceHours) {
  if (!std::isfinite(seconds) || seconds < 0) seconds = 0;
  const long long total = static_cast<long long>(std::floor(seconds));
  const long long h = total / 3600;
  const long long m = (total / 60) % 60;
  const long long s = total % 60;

  char buf[32];
  if (h > 0 || forceHours) {
    std::snprintf(buf, sizeof buf, "%lld:%02lld:%02lld", h, m, s);
  } else {
    std::snprintf(buf, sizeof buf, "%lld:%02lld", m, s);
  }
  return buf;
}

}  // namespace chobi::core

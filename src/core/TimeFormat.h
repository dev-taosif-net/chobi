// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <string>

namespace chobi::core {

// "1:02:03" when hours are needed (or forced), otherwise "2:03". Negative or non-finite
// values render as zero.
std::string formatTime(double seconds, bool forceHours = false);

}  // namespace chobi::core

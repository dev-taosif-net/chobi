// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <string>

namespace chobi::core {

// "1:02:03" when hours are needed (or forced), otherwise "2:03". Negative or non-finite
// values render as zero.
std::string formatTime(double seconds, bool forceHours = false);

// Time left, with a leading minus sign (U+2212): "−1:19:38". Whole seconds are subtracted, so
// formatTime(position) plus this always adds up to formatTime(duration).
std::string formatRemaining(double position, double duration, bool forceHours = false);

}  // namespace chobi::core

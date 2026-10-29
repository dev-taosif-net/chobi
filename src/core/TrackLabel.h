// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <string>
#include <string_view>

#include "core/Track.h"

namespace chobi::core {

// Menu label for a track, e.g. "Bangla · AC-3 · 5.1" or "English — Commentary · AAC · stereo".
// Missing language metadata is rendered as "Unknown language".
std::string trackLabel(const Track& track);

std::string prettyCodecName(std::string_view codec);
std::string channelLayoutName(int channels);

}  // namespace chobi::core

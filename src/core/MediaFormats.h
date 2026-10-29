// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <string_view>
#include <vector>

namespace chobi::core {

// Single source of truth for file-type lists: the open dialog filter, drag-and-drop routing,
// the Linux .desktop MimeType list and Windows file associations are derived from these.
// They only steer the UI; playback itself accepts anything FFmpeg can demux.

// Video container extensions (lowercase, no dot).
const std::vector<std::string_view>& videoExtensions();

// Subtitle formats that are auto-discovered next to a video.
const std::vector<std::string_view>& autoSubtitleExtensions();

// Subtitle formats accepted for manual loading (superset of the auto list).
const std::vector<std::string_view>& manualSubtitleExtensions();

bool isAutoSubtitleExtension(std::string_view lowerExtension);
bool isManualSubtitleExtension(std::string_view lowerExtension);

}  // namespace chobi::core

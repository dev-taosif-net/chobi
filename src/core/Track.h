// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint>
#include <string>

namespace chobi::core {

enum class TrackType { Video, Audio, Subtitle };

// Engine-neutral description of one stream. No engine property names leak past this struct.
struct Track {
  std::int64_t id = 0;  // engine track id, unique per type, > 0
  TrackType type = TrackType::Video;
  std::string title;
  std::string language;  // as reported by the container/engine, may be empty
  std::string codec;     // engine codec name ("aac", "ac3", "subrip")
  int channels = 0;      // audio only, 0 when unknown
  std::string externalFile;  // set for subtitle files loaded from disk
  bool isDefault = false;
  bool forced = false;
  bool external = false;
  bool selected = false;
  bool hearingImpaired = false;
};

}  // namespace chobi::core

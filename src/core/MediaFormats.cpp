// SPDX-License-Identifier: GPL-3.0-or-later
#include "core/MediaFormats.h"

#include <algorithm>

namespace chobi::core {

const std::vector<std::string_view>& videoExtensions() {
  static const std::vector<std::string_view> list = {
      "3g2", "3gp", "asf", "avi", "divx", "dv",  "f4v",  "flv",  "h264", "hevc", "ivf",
      "m2t", "m2ts", "m2v", "m4v", "mkv", "mov", "mp4", "mpe", "mpeg", "mpg", "mts",
      "mxf", "nut", "ogm", "ogv", "qt",  "rm",  "rmvb", "ts",  "vob",  "webm", "wmv",
      "wtv", "y4m"};
  return list;
}

const std::vector<std::string_view>& autoSubtitleExtensions() {
  static const std::vector<std::string_view> list = {"ass", "srt", "ssa", "vtt"};
  return list;
}

const std::vector<std::string_view>& manualSubtitleExtensions() {
  static const std::vector<std::string_view> list = {"ass", "idx", "lrc", "smi", "srt",
                                                     "ssa", "sub", "sup", "vtt"};
  return list;
}

bool isAutoSubtitleExtension(std::string_view ext) {
  const auto& l = autoSubtitleExtensions();
  return std::find(l.begin(), l.end(), ext) != l.end();
}

bool isManualSubtitleExtension(std::string_view ext) {
  const auto& l = manualSubtitleExtensions();
  return std::find(l.begin(), l.end(), ext) != l.end();
}

}  // namespace chobi::core

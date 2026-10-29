// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace chobi::core {

struct SubtitleCandidate {
  std::string fileName;     // directory entry name (UTF-8)
  std::string language;     // ISO 639-1 primary code, "" when the file has no language tag
  std::string languageTag;  // normalized full tag ("en-US"), "" when untagged
  std::string format;       // "ass", "ssa", "srt" or "vtt"
  bool forced = false;
  bool hearingImpaired = false;  // ".sdh" / ".cc"
  bool exactCase = true;         // stem matched without case folding
};

struct SubtitleMatchOptions {
  // ISO 639-1 codes, most preferred first (e.g. {"bn", "en"}).
  std::vector<std::string> preferredLanguages;
};

// Picks the external subtitle files in `directoryEntries` that belong to `videoFileName`
// and returns them best-first. A file matches only when its name is the video's stem
// followed by optional dot-separated language/flag tokens and a subtitle extension:
//   Movie.mkv -> Movie.srt, Movie.en.srt, Movie.en-US.srt, Movie.bn.forced.ass
// Anything else in the directory (Movie2.srt, Movie.Part2.srt, Other.srt) is rejected.
//
// Ordering is deterministic: preferred languages (in order), then untagged files, then other
// languages alphabetically; within a language, non-forced before forced, regular before SDH,
// exact-case before case-folded, ASS > SSA > SRT > VTT, and finally by file name.
std::vector<SubtitleCandidate> findSubtitles(std::string_view videoFileName,
                                             const std::vector<std::string>& directoryEntries,
                                             const SubtitleMatchOptions& options);

// File name without its last extension ("a.b.mkv" -> "a.b").
std::string_view fileStem(std::string_view fileName);

}  // namespace chobi::core

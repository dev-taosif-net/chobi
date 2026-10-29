// SPDX-License-Identifier: GPL-3.0-or-later
#include "core/SubtitleFinder.h"

#include <algorithm>
#include <optional>
#include <tuple>

#include "core/Language.h"
#include "core/MediaFormats.h"

namespace chobi::core {
namespace {

constexpr std::size_t kMaxTagTokens = 3;

bool equalsIgnoreAsciiCase(std::string_view a, std::string_view b) {
  return a.size() == b.size() && toLowerAscii(a) == toLowerAscii(b);
}

int formatRank(std::string_view format) {
  if (format == "ass") return 0;
  if (format == "ssa") return 1;
  if (format == "srt") return 2;
  return 3;  // vtt
}

// Parses "en.forced" / "sdh.en-US" style tag lists. Rejects unknown tokens.
bool parseTags(std::string_view tags, SubtitleCandidate* c) {
  std::size_t count = 0;
  bool haveLanguage = false;
  while (true) {
    const std::size_t dot = tags.find('.');
    const std::string_view token = tags.substr(0, dot);
    if (token.empty() || ++count > kMaxTagTokens) return false;

    const std::string lower = toLowerAscii(token);
    if (lower == "forced") {
      c->forced = true;
    } else if (lower == "sdh" || lower == "cc") {
      c->hearingImpaired = true;
    } else if (lower == "default" || lower == "full") {
      // Informational flags; accepted but not ranked.
    } else {
      LanguageTag tag;
      if (haveLanguage || !parseLanguageTag(token, &tag)) return false;
      haveLanguage = true;
      c->language = tag.primary;
      c->languageTag = tag.full;
    }

    if (dot == std::string_view::npos) return true;
    tags.remove_prefix(dot + 1);
  }
}

std::optional<SubtitleCandidate> matchEntry(std::string_view videoStem, std::string_view entry) {
  const std::size_t dot = entry.rfind('.');
  if (dot == std::string_view::npos || dot == 0) return std::nullopt;
  const std::string format = toLowerAscii(entry.substr(dot + 1));
  if (!isAutoSubtitleExtension(format)) return std::nullopt;

  SubtitleCandidate c;
  c.fileName = std::string(entry);
  c.format = format;

  const std::string_view stem = entry.substr(0, dot);
  if (stem.size() < videoStem.size()) return std::nullopt;

  const std::string_view head = stem.substr(0, videoStem.size());
  if (head == videoStem) {
    c.exactCase = true;
  } else if (equalsIgnoreAsciiCase(head, videoStem)) {
    c.exactCase = false;
  } else {
    return std::nullopt;
  }

  const std::string_view rest = stem.substr(videoStem.size());
  if (rest.empty()) return c;
  if (rest.front() != '.') return std::nullopt;  // "Movie2.srt", "Movie Part2.srt"
  if (!parseTags(rest.substr(1), &c)) return std::nullopt;
  return c;
}

}  // namespace

std::string_view fileStem(std::string_view fileName) {
  const std::size_t dot = fileName.rfind('.');
  if (dot == std::string_view::npos || dot == 0) return fileName;
  return fileName.substr(0, dot);
}

std::vector<SubtitleCandidate> findSubtitles(std::string_view videoFileName,
                                             const std::vector<std::string>& directoryEntries,
                                             const SubtitleMatchOptions& options) {
  const std::string_view videoStem = fileStem(videoFileName);
  std::vector<SubtitleCandidate> found;
  if (videoStem.empty()) return found;

  for (const std::string& entry : directoryEntries) {
    if (entry == videoFileName) continue;
    auto c = matchEntry(videoStem, entry);
    if (!c && videoStem != videoFileName) c = matchEntry(videoFileName, entry);  // Movie.mkv.srt
    if (c) found.push_back(std::move(*c));
  }

  const auto& prefs = options.preferredLanguages;
  auto languageRank = [&prefs](const SubtitleCandidate& c) -> std::size_t {
    if (c.language.empty()) return prefs.size();  // untagged
    const auto it = std::find(prefs.begin(), prefs.end(), c.language);
    return it != prefs.end() ? static_cast<std::size_t>(it - prefs.begin()) : prefs.size() + 1;
  };
  auto key = [&](const SubtitleCandidate& c) {
    return std::make_tuple(languageRank(c), c.languageTag, c.forced, c.hearingImpaired,
                           !c.exactCase, formatRank(c.format), c.fileName);
  };
  std::sort(found.begin(), found.end(),
            [&](const SubtitleCandidate& a, const SubtitleCandidate& b) { return key(a) < key(b); });
  return found;
}

}  // namespace chobi::core

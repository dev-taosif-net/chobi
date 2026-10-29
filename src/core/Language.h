// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <string>
#include <string_view>

namespace chobi::core {

struct LanguageTag {
  std::string primary;  // ISO 639-1 code, lowercase ("en", "bn")
  std::string full;     // normalized tag with region/script ("en-US", "zh-Hans")
};

// Parses a filename token or metadata value as a language. Accepts ISO 639-1 ("en"),
// ISO 639-2 ("eng", "ben"), common English names ("english", "bangla") and region/script
// subtags ("en-US", "pt_BR", "zh-Hans"). Returns false for anything else, including "und".
bool parseLanguageTag(std::string_view token, LanguageTag* out);

// Convenience wrapper: normalized primary code, or "" when not a recognized language.
std::string normalizeLanguage(std::string_view token);

// Human-readable English name ("bn" -> "Bangla"). Unknown codes are returned uppercased.
std::string languageDisplayName(std::string_view code);

std::string toLowerAscii(std::string_view s);

}  // namespace chobi::core

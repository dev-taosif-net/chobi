// SPDX-License-Identifier: GPL-3.0-or-later
#include "core/Language.h"

#include <algorithm>
#include <iterator>
#include <cctype>
#include <utility>

namespace chobi::core {
namespace {

// ISO 639-1 (two-letter) codes.
constexpr std::string_view kIso6391[] = {
    "aa", "ab", "ae", "af", "ak", "am", "an", "ar", "as", "av", "ay", "az", "ba", "be", "bg",
    "bi", "bm", "bn", "bo", "br", "bs", "ca", "ce", "ch", "co", "cr", "cs", "cu", "cv", "cy",
    "da", "de", "dv", "dz", "ee", "el", "en", "eo", "es", "et", "eu", "fa", "ff", "fi", "fj",
    "fo", "fr", "fy", "ga", "gd", "gl", "gn", "gu", "gv", "ha", "he", "hi", "ho", "hr", "ht",
    "hu", "hy", "hz", "ia", "id", "ie", "ig", "ii", "ik", "io", "is", "it", "iu", "ja", "jv",
    "ka", "kg", "ki", "kj", "kk", "kl", "km", "kn", "ko", "kr", "ks", "ku", "kv", "kw", "ky",
    "la", "lb", "lg", "li", "ln", "lo", "lt", "lu", "lv", "mg", "mh", "mi", "mk", "ml", "mn",
    "mr", "ms", "mt", "my", "na", "nb", "nd", "ne", "ng", "nl", "nn", "no", "nr", "nv", "ny",
    "oc", "oj", "om", "or", "os", "pa", "pi", "pl", "ps", "pt", "qu", "rm", "rn", "ro", "ru",
    "rw", "sa", "sc", "sd", "se", "sg", "si", "sk", "sl", "sm", "sn", "so", "sq", "sr", "ss",
    "st", "su", "sv", "sw", "ta", "te", "tg", "th", "ti", "tk", "tl", "tn", "to", "tr", "ts",
    "tt", "tw", "ty", "ug", "uk", "ur", "uz", "ve", "vi", "vo", "wa", "wo", "xh", "yi", "yo",
    "za", "zh", "zu"};

using Pair = std::pair<std::string_view, std::string_view>;

// ISO 639-2 (bibliographic and terminology) codes for common languages.
constexpr Pair kIso6392[] = {
    {"afr", "af"}, {"alb", "sq"}, {"amh", "am"}, {"ara", "ar"}, {"arm", "hy"}, {"asm", "as"},
    {"aze", "az"}, {"baq", "eu"}, {"bel", "be"}, {"ben", "bn"}, {"bod", "bo"}, {"bos", "bs"},
    {"bul", "bg"}, {"bur", "my"}, {"cat", "ca"}, {"ces", "cs"}, {"chi", "zh"}, {"cym", "cy"},
    {"cze", "cs"}, {"dan", "da"}, {"deu", "de"}, {"dut", "nl"}, {"ell", "el"}, {"eng", "en"},
    {"epo", "eo"}, {"est", "et"}, {"eus", "eu"}, {"fas", "fa"}, {"fil", "tl"}, {"fin", "fi"},
    {"fra", "fr"}, {"fre", "fr"}, {"geo", "ka"}, {"ger", "de"}, {"gle", "ga"}, {"glg", "gl"},
    {"gre", "el"}, {"guj", "gu"}, {"hau", "ha"}, {"heb", "he"}, {"hin", "hi"}, {"hrv", "hr"},
    {"hun", "hu"}, {"hye", "hy"}, {"ibo", "ig"}, {"ice", "is"}, {"ind", "id"}, {"isl", "is"},
    {"ita", "it"}, {"jpn", "ja"}, {"kan", "kn"}, {"kat", "ka"}, {"kaz", "kk"}, {"khm", "km"},
    {"kor", "ko"}, {"kur", "ku"}, {"lao", "lo"}, {"lat", "la"}, {"lav", "lv"}, {"lit", "lt"},
    {"mac", "mk"}, {"mal", "ml"}, {"mar", "mr"}, {"may", "ms"}, {"mkd", "mk"}, {"mon", "mn"},
    {"msa", "ms"}, {"mya", "my"}, {"nep", "ne"}, {"nld", "nl"}, {"nob", "nb"}, {"nor", "no"},
    {"ori", "or"}, {"pan", "pa"}, {"per", "fa"}, {"pol", "pl"}, {"por", "pt"}, {"pus", "ps"},
    {"ron", "ro"}, {"rum", "ro"}, {"rus", "ru"}, {"sin", "si"}, {"slk", "sk"}, {"slo", "sk"},
    {"slv", "sl"}, {"som", "so"}, {"spa", "es"}, {"sqi", "sq"}, {"srp", "sr"}, {"swa", "sw"},
    {"swe", "sv"}, {"tam", "ta"}, {"tel", "te"}, {"tgl", "tl"}, {"tha", "th"}, {"tib", "bo"},
    {"tur", "tr"}, {"ukr", "uk"}, {"urd", "ur"}, {"uzb", "uz"}, {"vie", "vi"}, {"wel", "cy"},
    {"xho", "xh"}, {"yor", "yo"}, {"zho", "zh"}, {"zul", "zu"}};

// English names commonly used in subtitle filenames ("Movie.English.srt").
constexpr Pair kNames[] = {
    {"arabic", "ar"},     {"bangla", "bn"},     {"bengali", "bn"},    {"chinese", "zh"},
    {"czech", "cs"},      {"danish", "da"},     {"dutch", "nl"},      {"english", "en"},
    {"farsi", "fa"},      {"finnish", "fi"},    {"french", "fr"},     {"german", "de"},
    {"greek", "el"},      {"gujarati", "gu"},   {"hebrew", "he"},     {"hindi", "hi"},
    {"hungarian", "hu"},  {"indonesian", "id"}, {"italian", "it"},    {"japanese", "ja"},
    {"kannada", "kn"},    {"korean", "ko"},     {"malay", "ms"},      {"malayalam", "ml"},
    {"marathi", "mr"},    {"nepali", "ne"},     {"norwegian", "no"},  {"persian", "fa"},
    {"polish", "pl"},     {"portuguese", "pt"}, {"punjabi", "pa"},    {"romanian", "ro"},
    {"russian", "ru"},    {"sinhala", "si"},    {"spanish", "es"},    {"swedish", "sv"},
    {"tamil", "ta"},      {"telugu", "te"},     {"thai", "th"},       {"turkish", "tr"},
    {"ukrainian", "uk"},  {"urdu", "ur"},       {"vietnamese", "vi"}};

constexpr Pair kDisplayNames[] = {
    {"ar", "Arabic"},     {"as", "Assamese"},   {"bg", "Bulgarian"}, {"bn", "Bangla"},
    {"ca", "Catalan"},    {"cs", "Czech"},      {"da", "Danish"},    {"de", "German"},
    {"el", "Greek"},      {"en", "English"},    {"es", "Spanish"},   {"et", "Estonian"},
    {"eu", "Basque"},     {"fa", "Persian"},    {"fi", "Finnish"},   {"fr", "French"},
    {"gl", "Galician"},   {"gu", "Gujarati"},   {"he", "Hebrew"},    {"hi", "Hindi"},
    {"hr", "Croatian"},   {"hu", "Hungarian"},  {"id", "Indonesian"}, {"is", "Icelandic"},
    {"it", "Italian"},    {"ja", "Japanese"},   {"km", "Khmer"},     {"kn", "Kannada"},
    {"ko", "Korean"},     {"lt", "Lithuanian"}, {"lv", "Latvian"},   {"ml", "Malayalam"},
    {"mr", "Marathi"},    {"ms", "Malay"},      {"my", "Burmese"},   {"nb", "Norwegian Bokmål"},
    {"ne", "Nepali"},     {"nl", "Dutch"},      {"no", "Norwegian"}, {"or", "Odia"},
    {"pa", "Punjabi"},    {"pl", "Polish"},     {"pt", "Portuguese"}, {"ro", "Romanian"},
    {"ru", "Russian"},    {"si", "Sinhala"},    {"sk", "Slovak"},    {"sl", "Slovenian"},
    {"sr", "Serbian"},    {"sv", "Swedish"},    {"ta", "Tamil"},     {"te", "Telugu"},
    {"th", "Thai"},       {"tl", "Tagalog"},    {"tr", "Turkish"},   {"uk", "Ukrainian"},
    {"ur", "Urdu"},       {"vi", "Vietnamese"}, {"zh", "Chinese"}};

template <std::size_t N>
std::string_view lookup(const Pair (&table)[N], std::string_view key) {
  for (const auto& [k, v] : table) {
    if (k == key) return v;
  }
  return {};
}

bool isAlpha(std::string_view s) {
  return !s.empty() && std::all_of(s.begin(), s.end(), [](unsigned char c) {
    return std::isalpha(c) != 0;
  });
}

bool isDigits(std::string_view s) {
  return !s.empty() && std::all_of(s.begin(), s.end(), [](unsigned char c) {
    return std::isdigit(c) != 0;
  });
}

std::string primaryCode(std::string_view lower) {
  if (lower.size() == 2) {
    if (std::find(std::begin(kIso6391), std::end(kIso6391), lower) != std::end(kIso6391)) {
      return std::string(lower);
    }
    return {};
  }
  if (lower.size() == 3) return std::string(lookup(kIso6392, lower));
  return {};
}

}  // namespace

std::string toLowerAscii(std::string_view s) {
  std::string out(s);
  for (char& c : out) {
    if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
  }
  return out;
}

bool parseLanguageTag(std::string_view token, LanguageTag* out) {
  const std::string lower = toLowerAscii(token);
  if (lower.empty() || lower.size() > 16) return false;

  // Whole-word English names never carry subtags.
  if (std::string_view name = lookup(kNames, lower); !name.empty()) {
    if (out) *out = {std::string(name), std::string(name)};
    return true;
  }

  // Split "en-US" / "pt_BR" / "zh-Hans-CN" into subtags.
  std::string_view rest = lower;
  std::string_view parts[3];
  std::size_t count = 0;
  while (true) {
    const std::size_t sep = rest.find_first_of("-_");
    if (count == 3) return false;
    parts[count++] = rest.substr(0, sep);
    if (sep == std::string_view::npos) break;
    rest.remove_prefix(sep + 1);
  }

  if (!isAlpha(parts[0])) return false;
  const std::string primary = primaryCode(parts[0]);
  if (primary.empty()) return false;

  std::string full = primary;
  for (std::size_t i = 1; i < count; ++i) {
    const std::string_view sub = parts[i];
    if (sub.size() == 2 && isAlpha(sub)) {  // region: US
      full += '-';
      full += static_cast<char>(sub[0] - 'a' + 'A');
      full += static_cast<char>(sub[1] - 'a' + 'A');
    } else if (sub.size() == 4 && isAlpha(sub)) {  // script: Hans
      full += '-';
      full += static_cast<char>(sub[0] - 'a' + 'A');
      full += sub.substr(1);
    } else if (sub.size() == 3 && isDigits(sub)) {  // UN M.49 region: 419
      full += '-';
      full += sub;
    } else {
      return false;
    }
  }

  if (out) *out = {primary, full};
  return true;
}

std::string normalizeLanguage(std::string_view token) {
  LanguageTag tag;
  return parseLanguageTag(token, &tag) ? tag.primary : std::string();
}

std::string languageDisplayName(std::string_view code) {
  const std::string lower = toLowerAscii(code);
  LanguageTag tag;
  const std::string primary = parseLanguageTag(lower, &tag) ? tag.primary : lower;
  if (std::string_view name = lookup(kDisplayNames, primary); !name.empty()) {
    return std::string(name);
  }
  std::string upper(code);
  for (char& c : upper) {
    if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
  }
  return upper;
}

}  // namespace chobi::core

// SPDX-License-Identifier: GPL-3.0-or-later
#include "core/Version.h"

#include <charconv>
#include <vector>

namespace chobi::core {
namespace {

bool parseNumber(std::string_view s, int* out) {
  if (s.empty() || s.size() > 9) return false;
  if (s.size() > 1 && s.front() == '0') return false;  // no leading zeros (SemVer)
  const auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), *out);
  return ec == std::errc() && ptr == s.data() + s.size();
}

std::vector<std::string_view> split(std::string_view s, char sep) {
  std::vector<std::string_view> out;
  while (true) {
    const std::size_t p = s.find(sep);
    out.push_back(s.substr(0, p));
    if (p == std::string_view::npos) return out;
    s.remove_prefix(p + 1);
  }
}

bool isNumeric(std::string_view s) {
  return !s.empty() && s.find_first_not_of("0123456789") == std::string_view::npos;
}

int comparePrerelease(std::string_view a, std::string_view b) {
  if (a == b) return 0;
  if (a.empty()) return 1;  // release > prerelease
  if (b.empty()) return -1;
  const auto pa = split(a, '.');
  const auto pb = split(b, '.');
  for (std::size_t i = 0; i < pa.size() && i < pb.size(); ++i) {
    const bool na = isNumeric(pa[i]);
    const bool nb = isNumeric(pb[i]);
    if (na && nb) {
      if (pa[i].size() != pb[i].size()) return pa[i].size() < pb[i].size() ? -1 : 1;
      if (const int c = pa[i].compare(pb[i])) return c < 0 ? -1 : 1;
    } else if (na != nb) {
      return na ? -1 : 1;  // numeric identifiers sort before alphanumeric ones
    } else if (const int c = pa[i].compare(pb[i])) {
      return c < 0 ? -1 : 1;
    }
  }
  if (pa.size() == pb.size()) return 0;
  return pa.size() < pb.size() ? -1 : 1;
}

}  // namespace

std::optional<Version> parseVersion(std::string_view text) {
  if (!text.empty() && (text.front() == 'v' || text.front() == 'V')) text.remove_prefix(1);
  text = text.substr(0, text.find('+'));  // drop build metadata
  Version v;
  const std::size_t dash = text.find('-');
  if (dash != std::string_view::npos) {
    v.prerelease = std::string(text.substr(dash + 1));
    if (v.prerelease.empty()) return std::nullopt;
    text = text.substr(0, dash);
  }
  const auto parts = split(text, '.');
  if (parts.size() != 3) return std::nullopt;
  if (!parseNumber(parts[0], &v.major) || !parseNumber(parts[1], &v.minor) ||
      !parseNumber(parts[2], &v.patch)) {
    return std::nullopt;
  }
  return v;
}

int compareVersions(const Version& a, const Version& b) {
  if (a.major != b.major) return a.major < b.major ? -1 : 1;
  if (a.minor != b.minor) return a.minor < b.minor ? -1 : 1;
  if (a.patch != b.patch) return a.patch < b.patch ? -1 : 1;
  return comparePrerelease(a.prerelease, b.prerelease);
}

bool isNewerVersion(std::string_view candidate, std::string_view current) {
  const auto c = parseVersion(candidate);
  const auto cur = parseVersion(current);
  return c && cur && compareVersions(*c, *cur) > 0;
}

}  // namespace chobi::core

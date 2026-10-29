// SPDX-License-Identifier: GPL-3.0-or-later
#include "core/ResumeStore.h"

#include <algorithm>
#include <charconv>
#include <cmath>

namespace chobi::core {
namespace {

constexpr std::string_view kHeaderV1 = "chobi-resume 1";
constexpr std::string_view kHeaderV2 = "chobi-resume 2";

// Keys and paths come from file names, which may contain almost any character. Escape the
// three that would break the line format.
std::string escapeField(std::string_view field) {
  std::string out;
  out.reserve(field.size());
  for (char c : field) {
    switch (c) {
      case '\\': out += "\\\\"; break;
      case '\t': out += "\\t"; break;
      case '\n': out += "\\n"; break;
      default: out += c;
    }
  }
  return out;
}

std::string unescapeField(std::string_view field) {
  std::string out;
  out.reserve(field.size());
  for (std::size_t i = 0; i < field.size(); ++i) {
    if (field[i] == '\\' && i + 1 < field.size()) {
      const char n = field[++i];
      out += n == 't' ? '\t' : n == 'n' ? '\n' : n;
    } else {
      out += field[i];
    }
  }
  return out;
}

// Splits on unescaped tabs (escaped ones are written as "\t", never as a raw tab).
std::vector<std::string_view> splitTabs(std::string_view line) {
  std::vector<std::string_view> out;
  while (true) {
    const std::size_t tab = line.find('\t');
    out.push_back(line.substr(0, tab));
    if (tab == std::string_view::npos) return out;
    line.remove_prefix(tab + 1);
  }
}

template <typename T>
bool parseNumber(std::string_view s, T* out) {
  if (s.empty()) return false;
  const auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), *out);
  return ec == std::errc() && ptr == s.data() + s.size();
}

}  // namespace

ResumeStore::ResumeStore(std::size_t capacity) : capacity_(std::max<std::size_t>(capacity, 1)) {}

std::string ResumeStore::makeKey(std::string_view fileName, std::uint64_t sizeBytes) {
  return std::to_string(sizeBytes) + ":" + std::string(fileName);
}

bool ResumeStore::shouldRemember(double position, double duration) {
  if (!std::isfinite(position) || position < kMinPosition) return false;
  if (duration > 0 && position >= duration * kMaxWatchedRatio) return false;
  return true;
}

bool ResumeStore::expired(const Entry& e, std::int64_t now) {
  return now - e.updatedAt > kRetentionSeconds;
}

void ResumeStore::parse(std::string_view text, std::int64_t now) {
  entries_.clear();
  int version = 0;
  bool first = true;
  while (!text.empty()) {
    const std::size_t nl = text.find('\n');
    std::string_view line = text.substr(0, nl);
    text = nl == std::string_view::npos ? std::string_view() : text.substr(nl + 1);
    if (!line.empty() && line.back() == '\r') line.remove_suffix(1);

    if (first) {
      first = false;
      version = line == kHeaderV2 ? 2 : line == kHeaderV1 ? 1 : 0;
      if (version == 0) return;  // unknown format: start fresh rather than misread
      continue;
    }
    if (line.empty()) continue;

    const std::vector<std::string_view> f = splitTabs(line);
    Entry e;
    if (!parseNumber(f[0], &e.position) || !std::isfinite(e.position)) continue;
    if (version == 1) {  // "<seconds>\t<key>"
      if (f.size() != 2 || f[1].empty()) continue;
      e.key = unescapeField(f[1]);
      e.updatedAt = now;  // unknown age: give it a full retention period
    } else {  // "<seconds>\t<updatedAt>\t<key>\t<path>"
      if (f.size() != 4 || f[2].empty() || !parseNumber(f[1], &e.updatedAt)) continue;
      e.key = unescapeField(f[2]);
      e.path = unescapeField(f[3]);
    }
    put(std::move(e));
  }
}

std::string ResumeStore::serialize() const {
  std::string out(kHeaderV2);
  out += '\n';
  char buf[64];
  for (const Entry& e : entries_) {
    const auto [ptr, ec] =
        std::to_chars(buf, buf + sizeof buf, e.position, std::chars_format::fixed, 1);
    if (ec != std::errc()) continue;
    out.append(buf, ptr);
    out += '\t';
    out += std::to_string(e.updatedAt);
    out += '\t';
    out += escapeField(e.key);
    out += '\t';
    out += escapeField(e.path);
    out += '\n';
  }
  return out;
}

std::optional<double> ResumeStore::lookup(std::string_view key, std::int64_t now) const {
  for (const Entry& e : entries_) {
    if (e.key == key) return expired(e, now) ? std::nullopt : std::optional<double>(e.position);
  }
  return std::nullopt;
}

void ResumeStore::record(std::string_view key, std::string_view path, double position,
                         double duration, std::int64_t now) {
  if (key.empty()) return;
  if (shouldRemember(position, duration)) {
    put({std::string(key), std::string(path), position, now});
  } else {
    remove(key);
  }
}

void ResumeStore::remove(std::string_view key) {
  entries_.erase(std::remove_if(entries_.begin(), entries_.end(),
                                [&](const Entry& e) { return e.key == key; }),
                 entries_.end());
}

bool ResumeStore::pruneExpired(std::int64_t now) {
  const std::size_t before = entries_.size();
  entries_.erase(std::remove_if(entries_.begin(), entries_.end(),
                                [now](const Entry& e) { return expired(e, now); }),
                 entries_.end());
  return entries_.size() != before;
}

void ResumeStore::put(Entry entry) {
  remove(entry.key);
  entries_.push_back(std::move(entry));
  if (entries_.size() > capacity_) {
    entries_.erase(entries_.begin(), entries_.begin() + (entries_.size() - capacity_));
  }
}

}  // namespace chobi::core

// SPDX-License-Identifier: GPL-3.0-or-later
#include "core/ResumeStore.h"

#include <algorithm>
#include <charconv>
#include <cmath>

namespace chobi::core {
namespace {

constexpr std::string_view kHeader = "chobi-resume 1";

// Keys come from file names, which may contain any character except '/' (and on Windows a
// few more). Escape the three that would break the line format.
std::string escapeKey(std::string_view key) {
  std::string out;
  out.reserve(key.size());
  for (char c : key) {
    switch (c) {
      case '\\': out += "\\\\"; break;
      case '\t': out += "\\t"; break;
      case '\n': out += "\\n"; break;
      default: out += c;
    }
  }
  return out;
}

std::string unescapeKey(std::string_view key) {
  std::string out;
  out.reserve(key.size());
  for (std::size_t i = 0; i < key.size(); ++i) {
    if (key[i] == '\\' && i + 1 < key.size()) {
      const char n = key[++i];
      out += n == 't' ? '\t' : n == 'n' ? '\n' : n;
    } else {
      out += key[i];
    }
  }
  return out;
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

void ResumeStore::parse(std::string_view text) {
  entries_.clear();
  bool first = true;
  while (!text.empty()) {
    const std::size_t nl = text.find('\n');
    std::string_view line = text.substr(0, nl);
    text = nl == std::string_view::npos ? std::string_view() : text.substr(nl + 1);
    if (!line.empty() && line.back() == '\r') line.remove_suffix(1);

    if (first) {
      first = false;
      if (line != kHeader) return;  // unknown format: start fresh rather than misread
      continue;
    }
    const std::size_t tab = line.find('\t');
    if (tab == std::string_view::npos || tab == 0 || tab + 1 >= line.size()) continue;

    double position = 0;
    const auto [ptr, ec] = std::from_chars(line.data(), line.data() + tab, position);
    if (ec != std::errc() || ptr != line.data() + tab || !std::isfinite(position)) continue;
    put(unescapeKey(line.substr(tab + 1)), position);
  }
}

std::string ResumeStore::serialize() const {
  std::string out(kHeader);
  out += '\n';
  char buf[64];
  for (const Entry& e : entries_) {
    const auto [ptr, ec] =
        std::to_chars(buf, buf + sizeof buf, e.position, std::chars_format::fixed, 1);
    if (ec != std::errc()) continue;
    out.append(buf, ptr);
    out += '\t';
    out += escapeKey(e.key);
    out += '\n';
  }
  return out;
}

std::optional<double> ResumeStore::lookup(std::string_view key) const {
  for (const Entry& e : entries_) {
    if (e.key == key) return e.position;
  }
  return std::nullopt;
}

void ResumeStore::record(std::string_view key, double position, double duration) {
  if (key.empty()) return;
  if (shouldRemember(position, duration)) {
    put(key, position);
  } else {
    remove(key);
  }
}

void ResumeStore::remove(std::string_view key) {
  entries_.erase(std::remove_if(entries_.begin(), entries_.end(),
                                [&](const Entry& e) { return e.key == key; }),
                 entries_.end());
}

void ResumeStore::put(std::string_view key, double position) {
  remove(key);
  entries_.push_back({std::string(key), position});
  if (entries_.size() > capacity_) {
    entries_.erase(entries_.begin(), entries_.begin() + (entries_.size() - capacity_));
  }
}

}  // namespace chobi::core

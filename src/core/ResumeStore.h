// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace chobi::core {

// Remembers playback positions. Entries are keyed by file identity (name + size) rather than
// full path, so they survive moved files and sandbox document-portal paths. Pure in-memory
// logic; the app layer does the file I/O.
class ResumeStore {
 public:
  static constexpr double kMinPosition = 30.0;      // don't bother below 30 s
  static constexpr double kMaxWatchedRatio = 0.95;  // treat >= 95% as finished
  static constexpr std::size_t kDefaultCapacity = 500;

  explicit ResumeStore(std::size_t capacity = kDefaultCapacity);

  static std::string makeKey(std::string_view fileName, std::uint64_t sizeBytes);
  static bool shouldRemember(double position, double duration);

  // Text format, one entry per line, oldest first: "<seconds>\t<key>".
  void parse(std::string_view text);
  std::string serialize() const;

  std::optional<double> lookup(std::string_view key) const;
  // Stores the position when worth remembering, otherwise forgets the entry.
  void record(std::string_view key, double position, double duration);
  void remove(std::string_view key);
  std::size_t size() const { return entries_.size(); }

 private:
  struct Entry {
    std::string key;
    double position;
  };
  void put(std::string_view key, double position);

  std::size_t capacity_;
  std::vector<Entry> entries_;  // least recently used first
};

}  // namespace chobi::core

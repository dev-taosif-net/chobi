// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace chobi::core {

// Remembers playback positions. Entries are keyed by file identity (name + size), so a file
// replaced by a different one under the same name never resumes at a stale position. Each
// entry also keeps the file's last known path, so positions of deleted videos can be dropped,
// and the time it was last watched, so entries expire after kRetentionSeconds.
// Pure in-memory logic with an injected clock (Unix seconds); the app layer does the I/O.
class ResumeStore {
 public:
  static constexpr double kMinPosition = 30.0;      // don't bother below 30 s
  static constexpr double kMaxWatchedRatio = 0.95;  // treat >= 95% as finished
  static constexpr std::size_t kDefaultCapacity = 500;
  static constexpr std::int64_t kRetentionSeconds = 7 * 24 * 60 * 60;  // 7 days

  struct Entry {
    std::string key;
    std::string path;        // UTF-8, last place the file was played from ("" if unknown)
    double position = 0;     // seconds
    std::int64_t updatedAt = 0;  // Unix seconds of the last save
  };

  explicit ResumeStore(std::size_t capacity = kDefaultCapacity);

  static std::string makeKey(std::string_view fileName, std::uint64_t sizeBytes);
  static bool shouldRemember(double position, double duration);

  // Text format, one entry per line, oldest first:
  //   "chobi-resume 2" header, then "<seconds>\t<updatedAt>\t<key>\t<path>".
  // Version 1 files (no time, no path) are migrated with updatedAt = now.
  void parse(std::string_view text, std::int64_t now);
  std::string serialize() const;

  // Expired entries are never returned, even before pruneExpired() runs.
  std::optional<double> lookup(std::string_view key, std::int64_t now) const;
  // Stores the position (refreshing its 7-day clock) when worth remembering, otherwise
  // forgets the entry.
  void record(std::string_view key, std::string_view path, double position, double duration,
              std::int64_t now);
  void remove(std::string_view key);
  bool pruneExpired(std::int64_t now);  // true if anything was removed

  const std::vector<Entry>& entries() const { return entries_; }
  std::size_t size() const { return entries_.size(); }

 private:
  static bool expired(const Entry& e, std::int64_t now);
  void put(Entry entry);

  std::size_t capacity_;
  std::vector<Entry> entries_;  // least recently used first
};

}  // namespace chobi::core

// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace chobi::core {

// Semantic version: MAJOR.MINOR.PATCH with an optional "-prerelease" part (build metadata
// after '+' is ignored). A leading 'v' is accepted ("v0.2.0").
struct Version {
  int major = 0;
  int minor = 0;
  int patch = 0;
  std::string prerelease;  // "beta.1", "" for a release
};

std::optional<Version> parseVersion(std::string_view text);

// SemVer 2.0 precedence: <0 if a < b, 0 if equal, >0 if a > b. "1.0.0-beta" < "1.0.0".
int compareVersions(const Version& a, const Version& b);

// True only when both parse and `candidate` is strictly newer than `current`.
bool isNewerVersion(std::string_view candidate, std::string_view current);

}  // namespace chobi::core

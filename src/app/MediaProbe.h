// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <optional>
#include <string>
#include <vector>

#include <QString>

#include "core/SubtitleFinder.h"

namespace chobi {

class ResumeService;

struct ExternalSubtitle {
  QString path;
  core::SubtitleCandidate info;
};

struct ProbeResult {
  QString path;
  bool exists = false;
  QString error;
  std::string resumeKey;
  std::optional<double> resumePosition;
  std::vector<ExternalSubtitle> subtitles;  // best first
};

// Everything that touches the filesystem before playback: stat, one non-recursive listing of
// the video's directory (filtered to subtitle extensions) and the resume lookup.
// Runs on a worker thread.
ProbeResult probeMedia(const QString& path, const std::vector<std::string>& preferredLanguages,
                       ResumeService* resume);

}  // namespace chobi

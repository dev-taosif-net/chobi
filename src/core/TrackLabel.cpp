// SPDX-License-Identifier: GPL-3.0-or-later
#include "core/TrackLabel.h"

#include <utility>
#include <vector>

#include "core/Language.h"

namespace chobi::core {
namespace {

constexpr std::pair<std::string_view, std::string_view> kCodecNames[] = {
    {"aac", "AAC"},       {"ac3", "AC-3"},       {"alac", "ALAC"},    {"dts", "DTS"},
    {"eac3", "E-AC-3"},   {"flac", "FLAC"},      {"mp2", "MP2"},      {"mp3", "MP3"},
    {"opus", "Opus"},     {"truehd", "TrueHD"},  {"vorbis", "Vorbis"}, {"wmav2", "WMA"},
    {"wmapro", "WMA Pro"}, {"ass", "ASS"},       {"ssa", "SSA"},      {"subrip", "SRT"},
    {"srt", "SRT"},       {"webvtt", "WebVTT"},  {"mov_text", "MP4 text"},
    {"hdmv_pgs_subtitle", "PGS"}, {"dvd_subtitle", "VobSub"}, {"dvb_subtitle", "DVB"},
    {"h264", "H.264"},    {"hevc", "HEVC"},      {"av1", "AV1"},      {"vp9", "VP9"},
    {"vp8", "VP8"},       {"mpeg2video", "MPEG-2"}, {"mpeg4", "MPEG-4"}, {"vc1", "VC-1"},
    {"prores", "ProRes"}, {"vvc", "VVC"}};

// ISO 639-2 placeholders that carry no language information.
bool isUnknownLanguage(std::string_view code) {
  const std::string lower = toLowerAscii(code);
  return lower.empty() || lower == "und" || lower == "mis" || lower == "mul" || lower == "zxx";
}

}  // namespace

std::string prettyCodecName(std::string_view codec) {
  if (codec.empty()) return {};
  for (const auto& [raw, pretty] : kCodecNames) {
    if (raw == codec) return std::string(pretty);
  }
  if (codec.substr(0, 4) == "pcm_") return "PCM";
  std::string upper(codec);
  for (char& c : upper) {
    if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
  }
  return upper;
}

std::string channelLayoutName(int channels) {
  switch (channels) {
    case 0: return {};
    case 1: return "mono";
    case 2: return "stereo";
    case 6: return "5.1";
    case 8: return "7.1";
    default: return std::to_string(channels) + " ch";
  }
}

std::string trackLabel(const Track& track) {
  std::string name = isUnknownLanguage(track.language) ? std::string("Unknown language")
                                                      : languageDisplayName(track.language);
  if (!track.title.empty() && track.title != name) name += " — " + track.title;

  std::vector<std::string> parts{name};
  if (std::string codec = prettyCodecName(track.codec); !codec.empty()) parts.push_back(codec);
  if (track.type == TrackType::Audio) {
    if (std::string ch = channelLayoutName(track.channels); !ch.empty()) parts.push_back(ch);
  }
  if (track.forced) parts.emplace_back("forced");
  if (track.hearingImpaired) parts.emplace_back("SDH");
  if (track.external) parts.emplace_back("external");

  std::string out;
  for (std::size_t i = 0; i < parts.size(); ++i) {
    if (i) out += " · ";
    out += parts[i];
  }
  return out;
}

}  // namespace chobi::core

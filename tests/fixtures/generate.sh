#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Generates the test-media corpus with ffmpeg (nothing binary is committed).
# Usage: generate.sh OUTDIR
#
# Every clip is tiny (160x90 or 320x180, a few seconds) so the corpus builds in seconds.
# A clip whose encoder is missing from the local ffmpeg is skipped, and the test that needs
# it reports SKIP instead of failing.
set -uo pipefail

out="${1:?usage: generate.sh OUTDIR}"
mkdir -p "$out"
cd "$out" || exit 1

stamp=".generated-v3"
[[ -f "$stamp" ]] && exit 0

ff() { ffmpeg -nostdin -hide_banner -loglevel error -y "$@"; }
try() {  # try NAME ffmpeg-args...: keep going if this encoder/muxer isn't available
  local name="$1"; shift
  if ! ff "$@" 2>"$name.log"; then
    echo "skip $name: $(head -c 200 "$name.log")" >&2
    rm -f "$name"
  fi
  rm -f "$name.log"
}

SRC_V=(-f lavfi -i "testsrc2=size=320x180:rate=25")
SMALL_V=(-f lavfi -i "testsrc2=size=160x90:rate=25")
SRC_A=(-f lavfi -i "sine=frequency=440:sample_rate=48000")

# --- Tier 1: main playback clip (keyframe every second for seek tests) ---------------------
try h264_aac.mp4 "${SRC_V[@]}" "${SRC_A[@]}" -t 12 -c:v libx264 -preset veryfast \
  -pix_fmt yuv420p -g 25 -c:a aac -b:a 96k -shortest h264_aac.mp4

# --- Keyframes only at 0, 10 and 20 s, like a downloaded movie (arrow-key seek tests) -------
try long_gop.mp4 "${SRC_V[@]}" "${SRC_A[@]}" -t 30 -c:v libx264 -preset veryfast \
  -pix_fmt yuv420p -x264-params keyint=250:scenecut=0 -c:a aac -b:a 96k -shortest long_gop.mp4

# --- Multiple audio tracks: eng stereo AAC, ben 5.1 AC-3 (titled), untagged mono Opus ------
try multi_audio.mkv "${SRC_V[@]}" \
  -f lavfi -i "sine=frequency=440:sample_rate=48000" \
  -f lavfi -i "aevalsrc=sin(330*2*PI*t)|sin(440*2*PI*t)|sin(550*2*PI*t)|0.1*sin(60*2*PI*t)|sin(660*2*PI*t)|sin(770*2*PI*t):channel_layout=5.1:sample_rate=48000" \
  -f lavfi -i "sine=frequency=880:sample_rate=48000" \
  -t 12 -map 0:v -map 1:a -map 2:a -map 3:a \
  -c:v libx264 -preset veryfast -pix_fmt yuv420p -g 25 \
  -c:a:0 aac -ac:a:0 2 -c:a:1 ac3 -c:a:2 libopus -ac:a:2 1 \
  -metadata:s:a:0 language=eng -metadata:s:a:1 language=ben -metadata:s:a:1 "title=Bangla dub" \
  -shortest multi_audio.mkv

# --- Format coverage (one small clip per container/codec family) ---------------------------
try hevc.mkv "${SMALL_V[@]}" -t 2 -c:v libx265 -x265-params log-level=error -pix_fmt yuv420p hevc.mkv
try hevc10.mkv "${SMALL_V[@]}" -t 2 -c:v libx265 -x265-params log-level=error -pix_fmt yuv420p10le hevc10.mkv
try vp9.webm "${SMALL_V[@]}" "${SRC_A[@]}" -t 2 -c:v libvpx-vp9 -deadline realtime -cpu-used 8 -c:a libopus -shortest vp9.webm
try av1.mkv "${SMALL_V[@]}" -t 2 -c:v libsvtav1 -preset 12 av1.mkv
try mpeg2.ts "${SMALL_V[@]}" "${SRC_A[@]}" -t 2 -c:v mpeg2video -c:a ac3 -shortest mpeg2.ts
try mpeg2.vob "${SMALL_V[@]}" "${SRC_A[@]}" -t 2 -c:v mpeg2video -c:a ac3 -shortest -f vob mpeg2.vob
try mpeg4.avi "${SMALL_V[@]}" "${SRC_A[@]}" -t 2 -c:v mpeg4 -c:a libmp3lame -shortest mpeg4.avi
try wmv2.wmv "${SMALL_V[@]}" "${SRC_A[@]}" -t 2 -c:v wmv2 -c:a wmav2 -shortest wmv2.wmv
try flv1.flv "${SMALL_V[@]}" "${SRC_A[@]}" -t 2 -c:v flv -c:a libmp3lame -ar 44100 -shortest flv1.flv
try theora.ogv "${SMALL_V[@]}" "${SRC_A[@]}" -t 2 -c:v libtheora -c:a libvorbis -shortest theora.ogv
try prores.mov "${SMALL_V[@]}" "${SRC_A[@]}" -t 2 -c:v prores_ks -c:a pcm_s16le -shortest prores.mov
try h263.3gp -f lavfi -i "testsrc2=size=176x144:rate=15" "${SRC_A[@]}" -t 2 -c:v h263 -c:a aac -ar 16000 -ac 1 -shortest h263.3gp
try mjpeg.avi "${SMALL_V[@]}" -t 2 -c:v mjpeg mjpeg.avi
try ffv1.nut "${SMALL_V[@]}" -t 2 -c:v ffv1 ffv1.nut
try dnxhd.mxf -f lavfi -i "testsrc2=size=1920x1080:rate=25" -t 1 -c:v dnxhd -b:v 36M -pix_fmt yuv422p dnxhd.mxf

# --- Subtitles next to a video with a Unicode + spaces name ---------------------------------
base="ছবি টেস্ট ✓ Movie"
if [[ -f h264_aac.mp4 ]]; then
  cp h264_aac.mp4 "$base.mp4"
fi
printf '1\n00:00:00,500 --> 00:00:04,000\nআমার সোনার বাংলা\n\n2\n00:00:05,000 --> 00:00:09,000\nআমি তোমায় ভালোবাসি\n' > "$base.bn.srt"
printf '1\n00:00:00,500 --> 00:00:04,000\nMy golden Bengal\n' > "$base.en.srt"
cat > "$base.ass" <<'EOF'
[Script Info]
ScriptType: v4.00+
PlayResX: 320
PlayResY: 180

[V4+ Styles]
Format: Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, OutlineColour, BackColour, Bold, Italic, Underline, StrikeOut, ScaleX, ScaleY, Spacing, Angle, BorderStyle, Outline, Shadow, Alignment, MarginL, MarginR, MarginV, Encoding
Style: Default,Noto Sans Bengali,20,&H00FFFFFF,&H000000FF,&H00000000,&H00000000,0,0,0,0,100,100,0,0,1,1,0,2,10,10,10,1

[Events]
Format: Layer, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text
Dialogue: 0,0:00:00.50,0:00:04.00,Default,,0,0,0,,{\b1}বাংলা{\b0} and English
EOF
printf 'WEBVTT\n\n00:00.500 --> 00:04.000\nHello\n' > "$base.vtt"
printf '1\n00:00:00,500 --> 00:00:04,000\nunrelated\n' > "Unrelated.srt"
printf '1\n00:00:00,500 --> 00:00:04,000\nunrelated\n' > "$base Part2.srt"
# Legacy encoding: Windows-1252 "Café" (0xE9).
printf '1\n00:00:00,500 --> 00:00:04,000\nCaf\xe9 cr\xe8me\n' > cp1252.srt

# --- Broken / unusual inputs ----------------------------------------------------------------
head -c 65536 /dev/urandom > garbage.mkv
: > empty.mp4
if [[ -f h264_aac.mp4 ]]; then
  size=$(wc -c < h264_aac.mp4)
  head -c $((size * 40 / 100)) h264_aac.mp4 > truncated.mp4  # moov atom is at the end: unplayable
fi
if [[ -f multi_audio.mkv ]]; then
  size=$(wc -c < multi_audio.mkv)
  head -c $((size * 60 / 100)) multi_audio.mkv > truncated.mkv  # partial download: plays what exists
fi

touch "$stamp"
echo "fixtures ready in $out"

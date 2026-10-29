# Supported formats: the "all types of video" contract

No player can honestly promise *every* video. Chobi's promise is: **everything the FFmpeg build
behind libmpv can demux and decode, except DRM-protected media.** This page sorts that into
tiers we can test.

## Tier 1: tested in CI on every commit

Each row is a generated clip in `tests/fixtures/generate.sh`, played headless by
`tests/integration/test_mpv_engine.cpp` (`formatCoverage`), which checks that it actually
decodes frames, not just that it opens.

| Container | Video codec(s) in the test corpus |
|---|---|
| MP4 | H.264 + AAC |
| MKV | H.264, HEVC 8-bit, HEVC 10-bit, AV1 |
| WebM | VP9 + Opus |
| MPEG-TS | MPEG-2 + AC-3 |
| VOB / MPEG-PS | MPEG-2 + AC-3 |
| AVI | MPEG-4 Part 2 (Xvid/DivX class) + MP3, MJPEG |
| ASF / WMV | WMV2 + WMA |
| FLV | Sorenson H.263 (FLV1) + MP3 |
| OGV | Theora + Vorbis |
| MOV | ProRes + PCM |
| 3GP | H.263 + AAC |
| NUT | FFV1 (lossless) |
| MXF | DNxHD |

Also covered by tests: three audio tracks (AAC stereo, AC-3 5.1, Opus mono), external SRT/ASS/
VTT subtitles in Bangla and English, a Windows-1252 subtitle, a partially downloaded MKV (plays),
and broken inputs (random bytes, empty file, MP4 without its index, missing file), all of which
must fail with a clear message and leave the player usable.

## Tier 2: expected to work, smoke-tested manually

Plays through FFmpeg, often with software decoding:

- Codecs: VC-1/WMV3, VVC/H.266, CineForm, HAP, RealVideo/RMVB, Indeo, Cinepak, H.264 Hi10P,
  4:2:2/4:4:4 and 12-bit
- Audio: E-AC-3, TrueHD, DTS/DTS-HD MA (core), FLAC, ALAC, PCM; Atmos/DTS:X play their core
- Embedded subtitles: PGS, VobSub, DVB, mov_text, CEA-608/708
- Interlaced material (deinterlacing via mpv), variable frame rate, high frame rate
- HDR10/HLG, tone-mapped to SDR (see limitations)
- Network: http(s), HLS, RTSP; DASH depends on the FFmpeg build
- Discs: unencrypted DVD/Blu-ray *titles* when libmpv is built with libdvdnav/libbluray

## Not supported

- DRM: Widevine, PlayReady, FairPlay (protected iTunes M4V), AACS/BD+ Blu-ray
- DVD and Blu-ray menus
- Dolby Vision profile 5 colour reshaping and HDR passthrough (embedded renderer limit)
- YouTube and similar page URLs (no youtube-dl/yt-dlp helper is spawned, by design)
- Bijoy/SutonnyMJ "ASCII" Bangla subtitles: they are not Unicode text, so no encoding detection
  can fix them

## Hardware decoding

Hardware decoding depends on the GPU, not on Chobi. On the reference machine (AMD Renoir, VCN 2)
H.264, HEVC 8/10-bit, VP9 and MPEG-2 decode on the GPU; **AV1 and VVC fall back to software**, so
4K AV1 costs noticeably more CPU there. The decoder in use is reported in benchmark output
(`hwdec`).

#!/usr/bin/env bash
# Converts every .wav under a directory to Ogg Vorbis (same name, .ogg), keeping each file's
# channel count and sample rate. Vorbis stores the exact sample count, so the loops and the
# sample-aligned stems stay gapless/aligned (unlike MP3). Existing .ogg files are left alone.
#
# Usage: tools/wav_to_ogg.sh <dir> [out-dir] [quality]
#   out-dir  where the .ogg files are written, mirroring <dir> (default: next to each .wav)
#   quality  libvorbis -q:a value, -1..10 (default 4, about 128 kbit/s)
# Needs ffmpeg built with libvorbis.
set -euo pipefail

src=${1:?usage: wav_to_ogg.sh <dir> [out-dir] [quality]}
out=${2:-}
quality=${3:-4}

command -v ffmpeg >/dev/null || { echo "ffmpeg not found in PATH" >&2; exit 1; }

count=0
while IFS= read -r -d '' wav; do
    rel=${wav#"$src"/}
    dest=${out:+$out/}${out:+$rel}
    [[ -z "$out" ]] && dest=$wav
    dest=${dest%.*}.ogg
    mkdir -p "$(dirname "$dest")"
    ffmpeg -nostdin -hide_banner -loglevel error -y -i "$wav" -vn -c:a libvorbis -q:a "$quality" "$dest"
    count=$((count + 1))
done < <(find "$src" -type f -iname '*.wav' -print0 | sort -z)

echo "converted $count file(s) at -q:a $quality"

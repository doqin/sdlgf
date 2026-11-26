#!/usr/bin/env bash
# Assembles the Linux release tarball of the demo: the demo binary plus the exported assets only.
#
# The tests link into the same output directory as the demo (build/bin), so the files are
# staged by whitelist rather than copied wholesale; Aseprite sources are dropped for the same
# reason they are not copied into the build tree.
#
# Usage: scripts/package-linux.sh [output-dir] [build-bin-dir]
#   defaults: dist, the first of build/bin/Release, build/bin/Debug, build/bin that has a demo
set -euo pipefail

out=${1:-dist}
build_dir=${2:-}
name=sdlgf-demo-linux-x86_64
stage="$out/$name"

if [[ -z "$build_dir" ]]; then
    for candidate in build/bin/Release build/bin/Debug build/bin; do
        if [[ -x "$candidate/demo" ]]; then build_dir=$candidate; break; fi
    done
    build_dir=${build_dir:-build/bin}
fi

if [[ ! -x "$build_dir/demo" ]]; then
    echo "demo binary not found or not executable in $build_dir - build Release first." >&2
    exit 1
fi
if [[ ! -d "$build_dir/assets" ]]; then
    echo "assets not found in $build_dir - configure with SDLGF_COPY_ASSETS=ON (the default)." >&2
    exit 1
fi

rm -rf "$stage"
mkdir -p "$stage"
cp "$build_dir/demo" "$stage/"
cp -r "$build_dir/assets" "$stage/assets"

# artist sources, never runtime assets
find "$stage" -type f \( -name '*.ase' -o -name '*.aseprite' \) -delete

stray=$(find "$stage" -name '*_test*' -print -quit)
if [[ -n "$stray" ]]; then
    echo "refusing to package test files: $stray" >&2
    exit 1
fi

tar -C "$out" -czf "$out/$name.tar.gz" "$name"

if tar -tzf "$out/$name.tar.gz" | grep -Eq '/[^/]*(_test[^/]*|\.ase(prite)?)$'; then
    echo "packaged tarball still contains test or Aseprite files:" >&2
    tar -tzf "$out/$name.tar.gz" | grep -E '/[^/]*(_test[^/]*|\.ase(prite)?)$' >&2
    exit 1
fi

echo "Linux package: $out/$name.tar.gz"

#!/bin/sh
# Build the portable `misty` binary with Cosmopolitan (one file for Linux, Windows, macOS,
# BSD; x86-64 and ARM64). Usage: scripts/build-cosmo.sh [output-dir]
#
# Uses $COSMOCC (the unpacked cosmocc folder) when set, else downloads cosmocc into
# .cosmocc/ next to this repo. cosmoc++ does not read source from stdin, so files are passed.
set -eu

ROOT=$(cd "$(dirname "$0")/.." && pwd)
OUT=${1:-"$ROOT/dist"}
COSMOCC=${COSMOCC:-"$ROOT/.cosmocc"}

if [ ! -x "$COSMOCC/bin/cosmoc++" ]; then
  echo "fetching cosmocc into $COSMOCC" >&2
  mkdir -p "$COSMOCC"
  curl -fsSL -o "$COSMOCC/cosmocc.zip" https://cosmo.zip/pub/cosmocc/cosmocc.zip
  (cd "$COSMOCC" && unzip -qo cosmocc.zip)
fi

mkdir -p "$OUT"
"$COSMOCC/bin/cosmoc++" -std=c++17 -O2 -Wall -Wextra \
  -I "$ROOT/include" \
  "$ROOT/src/text.cpp" "$ROOT/src/color.cpp" "$ROOT/src/table.cpp" "$ROOT/src/chart.cpp" \
  "$ROOT/cli/main.cpp" \
  -o "$OUT/misty"

# Cosmopolitan writes misty (APE, runs everywhere) plus per-arch debug ELFs; keep a .exe copy
# for Windows users, who need an extension to run it from cmd.exe.
cp "$OUT/misty" "$OUT/misty.exe"
ls -l "$OUT/misty" "$OUT/misty.exe" >&2

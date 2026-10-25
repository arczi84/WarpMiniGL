#!/usr/bin/env bash
set -euo pipefail
cd -- "$(dirname -- "$0")/.."
tmp=$(mktemp -d /tmp/warpminigl-test.XXXXXX)
trap 'rm -rf "$tmp"' EXIT
cc -std=c99 -Wall -Wextra -Werror tests/pixels.c -o "$tmp/pixels"
"$tmp/pixels"
python3 tests/bridge.py
python3 tests/perspective_arrays.py
python3 tests/perspective_state.py
python3 tests/readback.py
python3 tests/legacy_readback.py
python3 tests/direct_present.py
python3 tests/window_backend.py
python3 tests/bitmap_backend.py
python3 tests/bitmap_backend.py --copy

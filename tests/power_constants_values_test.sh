#!/bin/zsh
set -euo pipefail

repo_root="${0:A:h:h}"
src_root="${POWER_CONSTANTS_SRC_ROOT:-$repo_root}"
binary="$TMPDIR/power_constants_values_test"

clang++ -std=c++17 -Wall -Wextra -pedantic \
  -I"$src_root/src" -I"$src_root/tests/stubs" \
  "$repo_root/tests/power_constants_values_test.cpp" \
  -o "$binary"

"$binary"

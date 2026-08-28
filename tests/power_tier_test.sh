#!/bin/zsh
set -euo pipefail

repo_root="${0:A:h:h}"
binary="$TMPDIR/power_tier_test"

clang++ -std=c++17 -Wall -Wextra -pedantic \
  -I"$repo_root/src" \
  "$repo_root/src/power/BatteryHealth.cpp" \
  "$repo_root/src/power/PowerTier.cpp" \
  "$repo_root/tests/power_tier_test.cpp" \
  -o "$binary"

"$binary"

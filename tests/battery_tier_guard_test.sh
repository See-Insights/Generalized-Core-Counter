#!/bin/zsh
set -euo pipefail

repo_root="${0:A:h:h}"
binary="$TMPDIR/battery_tier_guard_test"

clang++ -std=c++17 -Wall -Wextra -pedantic \
  -I"$repo_root/src" \
  "$repo_root/src/power/BatteryHealth.cpp" \
  "$repo_root/src/power/PowerTier.cpp" \
  "$repo_root/src/reporting/BatteryTierGuard.cpp" \
  "$repo_root/tests/battery_tier_guard_test.cpp" \
  -o "$binary"

"$binary"

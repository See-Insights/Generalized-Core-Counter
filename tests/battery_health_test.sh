#!/bin/zsh
set -euo pipefail

repo_root="${0:A:h:h}"
binary="$TMPDIR/battery_health_test"
fixture="$repo_root/tests/fixtures/battery-soc-vcell-samples-2026-08.csv"

clang++ -std=c++17 -Wall -Wextra -pedantic \
  -I"$repo_root/src" \
  "$repo_root/src/power/BatteryHealth.cpp" \
  "$repo_root/tests/battery_health_test.cpp" \
  -o "$binary"

"$binary" "$fixture"

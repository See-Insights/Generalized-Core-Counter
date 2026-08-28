#!/bin/zsh
set -euo pipefail

repo_root="${0:A:h:h}"
binary="$TMPDIR/power_composition_test"

clang++ -std=c++17 -Wall -Wextra -pedantic \
  -I"$repo_root/tests/stubs/power_composition_overrides" -I"$repo_root/src" \
  "$repo_root/tests/power_composition_test.cpp" \
  "$repo_root/src/power/PowerPlatform.cpp" \
  "$repo_root/src/power/ChargeInhibit.cpp" \
  -o "$binary"

"$binary"

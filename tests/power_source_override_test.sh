#!/bin/zsh
set -euo pipefail

repo_root="${0:A:h:h}"
binary="$TMPDIR/power_source_override_test"

clang++ -std=c++17 -Wall -Wextra -pedantic \
  -I"$repo_root/tests/stubs/power_source_override_overrides" -I"$repo_root/src" \
  "$repo_root/tests/power_source_override_test.cpp" \
  "$repo_root/src/power/PowerManager.cpp" \
  "$repo_root/src/cloud/DeviceStatusPublisher.cpp" \
  "$repo_root/tests/stubs/power_source_override_overrides/cloud/CloudTestShim.cpp" \
  -o "$binary"

"$binary"

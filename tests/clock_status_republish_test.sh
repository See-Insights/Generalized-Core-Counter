#!/bin/zsh
set -euo pipefail

repo_root="${0:A:h:h}"
binary="$TMPDIR/clock_status_republish_test"

clang++ -std=c++17 -Wall -Wextra -pedantic \
  -I"$repo_root/tests/stubs/clock_status_republish_overrides" \
  -I"$repo_root/tests/stubs/power_source_override_overrides" \
  -I"$repo_root/src" \
  "$repo_root/tests/clock_status_republish_test.cpp" \
  "$repo_root/src/power/PowerManager.cpp" \
  "$repo_root/src/cloud/DeviceStatusPublisher.cpp" \
  "$repo_root/src/cloud/Cloud.cpp" \
  "$repo_root/tests/stubs/clock_status_republish_overrides/CloudLinkStubs.cpp" \
  -o "$binary"

"$binary"

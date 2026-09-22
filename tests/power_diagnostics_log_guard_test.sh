#!/bin/zsh
set -euo pipefail

repo_root="${0:A:h:h}"
binary="$TMPDIR/power_diagnostics_log_guard_test"

clang++ -std=c++17 -Wall -Wextra -pedantic \
  -DENABLE_DIAGNOSTICS_PUBLISH_MODE=1 \
  -I"$repo_root/tests/stubs/diag_overrides" -I"$repo_root/src" \
  "$repo_root/tests/power_diagnostics_log_guard_test.cpp" \
  "$repo_root/src/power/PowerDiagnostics.cpp" \
  "$repo_root/src/power/PowerManager.cpp" \
  -o "$binary"

"$binary"

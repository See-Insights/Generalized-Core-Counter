#!/bin/zsh
set -euo pipefail

repo_root="${0:A:h:h}"
binary="$TMPDIR/reporting_policy_adapter_test"

clang++ -std=c++17 -Wall -Wextra -pedantic \
  -I"$repo_root/tests/stubs/reporting_policy_adapter_overrides" -I"$repo_root/src" \
  "$repo_root/tests/reporting_policy_adapter_test.cpp" \
  "$repo_root/src/reporting/RuntimeReportingPolicy.cpp" \
  "$repo_root/src/reporting/ReportingPolicy.cpp" \
  "$repo_root/src/reporting/BatteryTierGuard.cpp" \
  "$repo_root/src/power/PowerTier.cpp" \
  "$repo_root/src/power/BatteryHealth.cpp" \
  -o "$binary"

"$binary"

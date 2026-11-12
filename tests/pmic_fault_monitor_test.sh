#!/bin/zsh
set -euo pipefail

repo_root="${0:A:h:h}"
binary="$TMPDIR/pmic_fault_monitor_test"

clang++ -std=c++17 -Wall -Wextra -pedantic \
  -I"$repo_root/tests/stubs/pmic_fault_monitor_overrides" -I"$repo_root/src" \
  "$repo_root/tests/pmic_fault_monitor_test.cpp" \
  "$repo_root/src/power/PmicFaultMonitor.cpp" \
  "$repo_root/src/power/PowerManager.cpp" \
  "$repo_root/src/power/BatteryAuthorityPolicy.cpp" \
  -o "$binary"

# Each scenario runs as its own process - pollAndRemediate()'s remediation
# state is function-local `static` and persists across calls WITHIN one
# process (by design, modeling escalation across uptime), so scenarios must
# not share a process or they would observe each other's accumulated state.
"$binary" 1
"$binary" 2
"$binary" 3
"$binary" 4
"$binary" 5
"$binary" 6
"$binary" 7
"$binary" 8
"$binary" 9
"$binary" 10
"$binary" 11
"$binary" 12
"$binary" 13
"$binary" 14
"$binary" 15
"$binary" 16

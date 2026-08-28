#!/bin/zsh
set -euo pipefail

repo_root="${0:A:h:h}"
binary="$TMPDIR/pmic_fault_monitor_test"

clang++ -std=c++17 -Wall -Wextra -pedantic \
  -I"$repo_root/tests/stubs/pmic_fault_monitor_overrides" -I"$repo_root/src" \
  "$repo_root/tests/pmic_fault_monitor_test.cpp" \
  "$repo_root/src/power/PmicFaultMonitor.cpp" \
  "$repo_root/src/power/PowerManager.cpp" \
  "$repo_root/src/sensors/BatteryAuthorityPolicy.cpp" \
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

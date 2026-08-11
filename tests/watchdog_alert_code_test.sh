#!/bin/zsh
set -euo pipefail

repo_root="${0:A:h:h}"
binary="$TMPDIR/watchdog_alert_code_test"

# --- Part 1: compile and run the host-side logic-mirror test ---
clang++ -std=c++17 -Wall -Wextra -pedantic \
  "$repo_root/tests/watchdog_alert_code_test.cpp" \
  -o "$binary"
"$binary"

# --- Part 2: fidelity checks against the real source ---
persistent_src="$repo_root/src/MyPersistentData.cpp"
app_src="$repo_root/src/Generalized-Core-Counter.cpp"

check_file() {
  local file="$1"
  local desc="$2"
  local pattern="$3"
  if ! grep -q -- "$pattern" "$file"; then
    echo "FIDELITY CHECK FAILED: $desc (pattern not found in $file: $pattern)" >&2
    exit 1
  fi
}

# Code 19 must be present in getAlertSeverity(), in its OWN distinct tier
# (case 19: return 4;) - NOT folded back into the same fallthrough group/
# "return 3" as codes 14-18/20-21 (the Stage 7 severity-collision bug).
check_file "$persistent_src" "alert code 19 present in getAlertSeverity()" "case 19: // watchdog reset"

# Confirm 19 has its own "return 4" distinct from the tier-3 group's
# "return 3", and is NOT part of the same case-fallthrough block as 14/18/21.
awk '
  /case 19:/ { in19=1 }
  in19 && /return [0-9]+;/ { print; exit }
' "$persistent_src" | grep -q "return 4;" || {
  echo "FIDELITY CHECK FAILED: alert code 19 must return a distinct severity tier (4), strictly above the tier-3 group, in $persistent_src" >&2
  exit 1
}

# Confirm 14/18/21 remain in the shared tier-3 group and 19 is NOT among their
# case labels (i.e. it was moved out, not just numerically reordered within
# the same fallthrough block).
tier3_block=$(awk '/case 14:/,/return 3;/' "$persistent_src")
if echo "$tier3_block" | grep -q "case 19:"; then
  echo "FIDELITY CHECK FAILED: alert code 19 must NOT share the tier-3 fallthrough block with 14/18/21 in $persistent_src" >&2
  exit 1
fi
if ! echo "$tier3_block" | grep -q "case 18:"; then
  echo "FIDELITY CHECK FAILED: alert code 18 expected to remain in the tier-3 block in $persistent_src" >&2
  exit 1
fi

# Code 19 must NOT appear in isAutoClearAfterReportAlert()'s case list.
autoclear_body=$(awk '/static bool isAutoClearAfterReportAlert/,/^}/' "$app_src")
if echo "$autoclear_body" | grep -q "case 19:"; then
  echo "FIDELITY CHECK FAILED: alert code 19 must NOT be in isAutoClearAfterReportAlert()" >&2
  exit 1
fi

# current.raiseAlert(19) must actually be called somewhere (the classification
# call site added in setup()).
check_file "$app_src" "current.raiseAlert(19) call site present" "current.raiseAlert(19);"

# The webhook payload's "alerts" field must be fed directly from
# current.get_alertCode() (confirms no new plumbing needed, per the WO).
check_file "$app_src" "reportedAlertCode sourced from current.get_alertCode()" "const int8_t reportedAlertCode = current.get_alertCode();"
check_file "$app_src" "\"alerts\" field present in OCCUPANCY payload format" "\\\\\"alerts\\\\\":%i"

echo "Fidelity checks passed: alert-code 19 severity/auto-clear mirror matches real source"
echo "Watchdog alert-code test suite passed"

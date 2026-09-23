#!/bin/zsh
set -euo pipefail

repo_root="${0:A:h:h}"
binary="$TMPDIR/occupancy_start_time_clamp_test"

# --- Part 1: compile and run the host-side logic-mirror test ---
clang++ -std=c++17 -Wall -Wextra -pedantic \
  "$repo_root/tests/occupancy_start_time_clamp_test.cpp" \
  -o "$binary"
"$binary"

# --- Part 2: fidelity checks against the real source (WO-2026-09-22-001) ---
header="$repo_root/src/MyPersistentData.h"
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

check_file "$header" "revalidateOccupancyStartTimeIfTimeAvailable() declared" \
  "void revalidateOccupancyStartTimeIfTimeAvailable();"

check_file "$persistent_src" "revalidateOccupancyStartTimeIfTimeAvailable() defined" \
  "void currentStatusData::revalidateOccupancyStartTimeIfTimeAvailable() {"

# The relocated function's body must actually gate on Clock::isTimeValid()
# and occupied, and use the same +5s clamp tolerance the original branch did.
# The full guard-line literal is required (not the two identifiers checked
# piecemeal) so a mutation that keeps both substrings present but neuters
# the condition itself (e.g. wrapping it in `if (false && (...))`) is still
# caught.
relocated_body=$(awk '/void currentStatusData::revalidateOccupancyStartTimeIfTimeAvailable/,/^}/' "$persistent_src")
if ! echo "$relocated_body" | grep -q "if (!Clock::isTimeValid() || !current.get_occupied())"; then
  echo "FIDELITY CHECK FAILED: revalidateOccupancyStartTimeIfTimeAvailable() does not gate on the exact (!Clock::isTimeValid() || !current.get_occupied()) condition" >&2
  exit 1
fi
if ! echo "$relocated_body" | grep -q "start > now + 5)"; then
  echo "FIDELITY CHECK FAILED: revalidateOccupancyStartTimeIfTimeAvailable() does not use the original +5s clamp tolerance" >&2
  exit 1
fi
if ! echo "$relocated_body" | grep -q "current.set_occupancyStartTime(now);"; then
  echo "FIDELITY CHECK FAILED: revalidateOccupancyStartTimeIfTimeAvailable() does not clamp to set_occupancyStartTime(now)" >&2
  exit 1
fi

# validate()'s own occupancy block must no longer contain the dead-branch
# clamp (moved out, not duplicated back in) - it must not itself call
# set_occupancyStartTime a second time inside validate().
validate_body=$(awk '/^bool currentStatusData::validate/,/^}/' "$persistent_src")
if echo "$validate_body" | grep -q "set_occupancyStartTime(now)"; then
  echo "FIDELITY CHECK FAILED: validate() still contains the clamp inline - it should have been relocated, not duplicated" >&2
  exit 1
fi

# The relocated function must actually be called once from global setup(),
# and that call site must come after ab1805.withFOUT(WKP).setup() seeds
# Time - otherwise this is the same structurally-unreachable bug moved
# to a new location.
check_file "$app_src" "revalidateOccupancyStartTimeIfTimeAvailable() called from setup()" \
  "CurrentReadings::revalidateOccupancyStartTimeIfTimeAvailable();"

ab1805_line=$(grep -n "ab1805.withFOUT(WKP).setup();" "$app_src" | head -1 | cut -d: -f1)
call_line=$(grep -n "CurrentReadings::revalidateOccupancyStartTimeIfTimeAvailable();" "$app_src" | head -1 | cut -d: -f1)
if [[ -z "$ab1805_line" || -z "$call_line" ]]; then
  echo "FIDELITY CHECK FAILED: could not locate both ab1805.setup() and the revalidate call site in $app_src" >&2
  exit 1
fi
if (( call_line <= ab1805_line )); then
  echo "FIDELITY CHECK FAILED: revalidateOccupancyStartTimeIfTimeAvailable() is called at line $call_line, before ab1805.withFOUT(WKP).setup() at line $ab1805_line seeds Time - it would still be structurally unreachable" >&2
  exit 1
fi

echo "Fidelity checks passed: occupancyStartTime clamp relocated to a reachable post-RTC-seed call site, logic unchanged"
echo "occupancy_start_time_clamp_test: full suite passed"

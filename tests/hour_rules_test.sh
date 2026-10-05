#!/bin/zsh
set -euo pipefail

# WO-2026-09-24-004 (v36-HourRules): open and close hours follow three rules.
#
#   1. Always-open is exactly openHour = 0, closeHour = 24.
#   2. closeHour > openHour.
#   3. 0 <= openHour <= 12.
#
# Part 1 (hour_rules_test.cpp) compiles the REAL src/Config.h shared check and
# the REAL window functions extracted verbatim from src/time/Clock.cpp, and
# compares them against hand-written copies of the v35 versions.
#
# Part 2 is structural. Cloud::applyTimingConfig() cannot be compiled on the
# host - it is built on Device OS LedgerData/Variant, and no existing harness
# stubs those - so the pair-apply behaviour it must have (merge, one check,
# both-or-neither, keep the last valid hours, success = false) is traced in the
# real function body instead, together with the src-wide absence of the
# openHour == closeHour sentinel and of the overnight branch.
#
# Part 3 re-runs Part 1 against broken COPIES of Config.h (production sources
# are never modified): weakening any one of the three rules must make Part 1
# fail.

repo_root="${0:A:h:h}"
work="$TMPDIR/hour_rules_test_$$"
mkdir -p "$work"
trap 'rm -rf "$work"' EXIT

clock_src="$repo_root/src/time/Clock.cpp"
boundary_src="$repo_root/src/time/DailyBoundary.cpp"
config_apply_src="$repo_root/src/cloud/ConfigApply.cpp"
config_src="$repo_root/src/Config.cpp"
config_hdr="$repo_root/src/Config.h"
persist_src="$repo_root/src/MyPersistentData.cpp"

extract_function() {
  # $1 = file, $2 = a literal that starts the function's signature line
  python3 - "$1" "$2" <<'PY'
import sys

path, needle = sys.argv[1], sys.argv[2]
lines = open(path).readlines()

start = None
for i, line in enumerate(lines):
    if needle in line:
        start = i
        break
if start is None:
    sys.exit(f"EXTRACT_FAILED: {needle!r} not found in {path}")

depth = 0
opened = False
for i in range(start, len(lines)):
    for ch in lines[i]:
        if ch == '{':
            depth += 1
            opened = True
        elif ch == '}':
            depth -= 1
    if opened and depth == 0:
        sys.stdout.write("".join(lines[start:i + 1]))
        break
else:
    sys.exit(f"EXTRACT_FAILED: no closing brace for {needle!r} in {path}")
PY
}

# --- The real Clock.cpp window functions, verbatim -------------------------
{
  echo "// Extracted verbatim from src/time/Clock.cpp by tests/hour_rules_test.sh."
  extract_function "$clock_src" "static bool isWithinOpenHoursForHour("
  echo ""
  extract_function "$clock_src" "static int secondsUntilNextOpenForSeconds("
} > "$work/real_clock_fns.inc"

if ! grep -q "isWithinOpenHoursForHour" "$work/real_clock_fns.inc" || \
   ! grep -q "secondsUntilNextOpenForSeconds" "$work/real_clock_fns.inc"; then
  echo "FAILED: could not extract the real Clock.cpp window functions" >&2
  exit 1
fi

build() {
  local config_include="$1" out="$2"
  clang++ -std=c++17 -Wall -Wextra -pedantic \
    -I"$config_include" \
    -I"$repo_root/tests/stubs/hour_rules_overrides" \
    -I"$repo_root/src" \
    -I"$work" \
    "$repo_root/tests/hour_rules_test.cpp" \
    -o "$out"
}

# --- Part 1: clean run against the real shared check -----------------------
build "$repo_root/src" "$work/hour_rules_test"
"$work/hour_rules_test"

echo ""
echo "--- Part 2: structural checks against the real sources ---"

apply_fn=$(extract_function "$config_apply_src" "bool Cloud::applyTimingConfig(")
if [[ -z "$apply_fn" ]]; then
  echo "FAILED: could not extract Cloud::applyTimingConfig()" >&2
  exit 1
fi
apply_code=$(print -r -- "$apply_fn" | sed 's://.*$::')

fail_if_missing() {
  local haystack="$1" needle="$2" message="$3"
  if ! print -r -- "$haystack" | grep -qF -- "$needle"; then
    echo "FAILED: $message (missing: $needle)" >&2
    exit 1
  fi
}

# The pair is merged, with an absent value falling back to the current one.
fail_if_missing "$apply_code" 'getMergedIntValue(defaultTiming, deviceTiming, "openHour"' \
  "applyTimingConfig() must still read a merged openHour"
fail_if_missing "$apply_code" 'getMergedIntValue(defaultTiming, deviceTiming, "closeHour"' \
  "applyTimingConfig() must still read a merged closeHour"
fail_if_missing "$apply_code" 'SystemConfig::get_openTime()' \
  "an absent openHour must fall back to the current open hour"
fail_if_missing "$apply_code" 'SystemConfig::get_closeTime()' \
  "an absent closeHour must fall back to the current close hour"

# Exactly one rules check, and it is the shared one.
rule_calls=$(print -r -- "$apply_code" | grep -c "Config::hoursRuleFailure(\|Config::hoursFollowRules(" || true)
if (( rule_calls != 1 )); then
  echo "FAILED: applyTimingConfig() must check the hours exactly once, through the shared Config check (found $rule_calls)" >&2
  exit 1
fi

# The old independent 0-23 range checks must be gone.
if print -r -- "$apply_code" | grep -q "validateRange(openHour\|validateRange(closeHour"; then
  echo "FAILED: applyTimingConfig() must no longer range-check openHour/closeHour independently" >&2
  exit 1
fi

# Both or neither: the two setters are written once each, in the same branch.
open_sets=$(print -r -- "$apply_code" | grep -c "SystemConfig::set_openTime(" || true)
close_sets=$(print -r -- "$apply_code" | grep -c "SystemConfig::set_closeTime(" || true)
if (( open_sets != 1 || close_sets != 1 )); then
  echo "FAILED: applyTimingConfig() must write each hour exactly once (open=$open_sets close=$close_sets)" >&2
  exit 1
fi

rule_line=$(print -r -- "$apply_code" | grep -n "Config::hoursRuleFailure(\|Config::hoursFollowRules(" | head -1 | cut -d: -f1)
open_set_line=$(print -r -- "$apply_code" | grep -n "SystemConfig::set_openTime(" | head -1 | cut -d: -f1)
close_set_line=$(print -r -- "$apply_code" | grep -n "SystemConfig::set_closeTime(" | head -1 | cut -d: -f1)
if (( rule_line >= open_set_line || rule_line >= close_set_line )); then
  echo "FAILED: applyTimingConfig() must check the pair BEFORE applying either hour" >&2
  exit 1
fi
if (( close_set_line - open_set_line != 1 )); then
  echo "FAILED: applyTimingConfig() must apply both hours together (set_openTime and set_closeTime adjacent)" >&2
  exit 1
fi

# A rejected pair is logged with the offending values and the rule, and fails.
reject_block=$(print -r -- "$apply_code" | sed -n "${close_set_line},\$p")
fail_if_missing "$reject_block" "Log.warn" "a rejected pair must be logged"
fail_if_missing "$reject_block" "success = false" "a rejected pair must make the apply fail"
if ! print -r -- "$reject_block" | grep -q "hoursRule\|rule"; then
  echo "FAILED: the rejection log must name the rule that was broken" >&2
  exit 1
fi
echo "OK: applyTimingConfig() merges the pair, checks it once through Config, and applies both or neither"

# The shared check is the only place the rules are written.
rule_defs=$(grep -rn "hoursRuleFailure" "$repo_root/src" | grep -c "inline const char \*hoursRuleFailure" || true)
if (( rule_defs != 1 )); then
  echo "FAILED: Config::hoursRuleFailure() must be defined exactly once in src/ (found $rule_defs)" >&2
  exit 1
fi
for src_file in "$config_src" "$persist_src" "$config_apply_src"; do
  if ! grep -q "hoursRuleFailure(\|hoursFollowRules(" "$src_file"; then
    echo "FAILED: $src_file must use the shared hour-rules check" >&2
    exit 1
  fi
done
for src_file in "$config_src" "$persist_src"; do
  if grep -q "get_openTime() > 23\|get_closeTime() > 23\|openHour > 23\|closeHour > 23" "$src_file"; then
    echo "FAILED: $src_file must not keep its own 0-23 hour range check" >&2
    exit 1
  fi
done
echo "OK: the three validators share one check, and no validator keeps its own hour range test"

# No sentinel and no overnight branch anywhere in src/.
if grep -rn "openHour == closeHour\|closeHour == openHour" "$repo_root/src" | grep -v "^\s*//" | grep -q .; then
  echo "FAILED: the openHour == closeHour always-open sentinel must be gone from src/" >&2
  grep -rn "openHour == closeHour\|closeHour == openHour" "$repo_root/src" >&2
  exit 1
fi
if grep -rn "get_openTime() == .*get_closeTime()\|get_closeTime() == .*get_openTime()" "$repo_root/src" | grep -q .; then
  echo "FAILED: an open == close sentinel must not be re-derived from the stored hours" >&2
  exit 1
fi
if grep -rn "openHour > closeHour\|closeHour < openHour" "$repo_root/src" | grep -q .; then
  echo "FAILED: the overnight-window branch must be gone from src/" >&2
  grep -rn "openHour > closeHour\|closeHour < openHour" "$repo_root/src" >&2
  exit 1
fi
boundary_check=$(extract_function "$boundary_src" "Result check(time_t now) {")
fail_if_missing "$boundary_check" "close = SystemConfig::get_closeTime();" \
  "DailyBoundary::check() must use the configured close hour directly"
echo "OK: no always-open sentinel and no overnight branch remain in src/"

echo ""
echo "--- Part 3: rule mutations (each must make Part 1 fail) ---"

mutate_and_expect_failure() {
  local name="$1" sed_expr="$2" witness="$3"
  local mutant_dir="$work/mutant"
  rm -rf "$mutant_dir"
  mkdir -p "$mutant_dir"
  sed "$sed_expr" "$config_hdr" > "$mutant_dir/Config.h"
  if ! grep -qF -- "$witness" "$mutant_dir/Config.h"; then
    echo "FAILED (mutation harness): could not plant '$name' - Config.h's shared check has been rewritten, update this script" >&2
    exit 1
  fi
  if ! build "$mutant_dir" "$mutant_dir/mutant" 2>"$work/build.log"; then
    echo "FAILED (mutation harness): mutant '$name' did not compile - the mutation is malformed, not the code" >&2
    cat "$work/build.log" >&2
    exit 1
  fi
  if "$mutant_dir/mutant" >/dev/null 2>&1; then
    echo "FAILED (mutation): '$name' was NOT detected - the rules test has a blind spot" >&2
    exit 1
  fi
  echo "OK (mutation): '$name' correctly detected"
}

# Rule 1: always-open is 0/24, so 24 must be an accepted close hour.
mutate_and_expect_failure "rule 1 weakened - close hour capped at 23 again" \
  's/if (closeHour > 24) {/if (closeHour > 23) {/' \
  'if (closeHour > 23) {'

# Rule 2: closeHour must be strictly greater than openHour.
mutate_and_expect_failure "rule 2 weakened - open == close accepted again" \
  's/if (closeHour <= openHour) {/if (closeHour < openHour) {/' \
  'if (closeHour < openHour) {'

# Rule 3: openHour must be 0-12.
mutate_and_expect_failure "rule 3 removed - openHour upper bound dropped" \
  's/if (openHour < 0 || openHour > 12) {/if (openHour < 0) {/' \
  'if (openHour < 0) {'

echo ""
echo "hour_rules_test: clean run passed, structure verified, and all three rule mutations detected"

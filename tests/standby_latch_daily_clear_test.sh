#!/bin/zsh
set -euo pipefail

# WO-2026-10-09-003: the modem standby latch clears once a day.
#
# The real daily-close block (State_Report.cpp), the real latch owner
# (power/ModemHealth.{h,cpp}: thresholds, enum, latch functions and
# clearAtDailyClose) and the real teardown trip check (State_Sleep.cpp) are
# extracted byte-for-byte
# and compiled into standby_latch_daily_clear_test.cpp. A separate step re-reads
# src/ and fails with COPY_MISMATCH if any extracted piece differs from it.
# Mutants are built from private copies of the sources (in the temp dir,
# never in the worktree) and each must make its own targeted check FAIL at run
# time; a mutant that does not compile is a broken mutation, not a caught one.
# Set STANDBY_LATCH_SRC_ROOT to test another src/ root.

tests_dir="${0:A:h}"
repo_root="${tests_dir:h}"
src_root="${STANDBY_LATCH_SRC_ROOT:-$repo_root/src}"
work="$(mktemp -d "${TMPDIR:-/tmp}/standby_latch_daily_clear.XXXXXX")"
trap 'rm -rf "$work"' EXIT

extract_and_build() {
  # $1 = src root, $2 = work dir, $3 = binary
  local root="$1" dir="$2" binary="$3"
  mkdir -p "$dir"
  python3 - "$root" "$dir" <<'PY'
import sys

root, out = sys.argv[1], sys.argv[2]
report = open(f"{root}/state/State_Report.cpp").read()
sleep = open(f"{root}/state/State_Sleep.cpp").read()
mh_h = open(f"{root}/power/ModemHealth.h").read()
mh_cpp = open(f"{root}/power/ModemHealth.cpp").read()


def die(msg):
    sys.exit("EXTRACT_FAILED: " + msg)


def between(text, start, end, what):
    if text.count(start) != 1:
        die(f"{what}: start marker count {text.count(start)}")
    i = text.index(start)
    j = text.find(end, i)
    if j < 0:
        die(f"{what}: end marker missing")
    return text[i:j]


pieces = {
    "gen_report_block.inc": between(
        report, "  if (due) {\n    dailyCleanup();", "\n\n  // This timestamp is the authoritative", "report block") + "\n",
    "gen_trip_check.inc": between(
        sleep, "    if (teardownElapsedMs > MODEM_UNSTABLE_SLOW_TEARDOWN_MS) {",
        "    Observability::cycleStats().markTeardownEnd(millis());", "trip check"),
}
defs = (between(mh_h, "constexpr unsigned long MODEM_UNSTABLE_SLOW_TEARDOWN_MS",
                "void markModemUnstable(", "constants and enum")
        + mh_cpp[mh_cpp.index("constexpr unsigned long MODEM_UNSTABLE_RECOVERY_TEARDOWN_MS"):])
pieces["gen_sleep_defs.inc"] = defs

for name, body in pieces.items():
    open(f"{out}/{name}", "w").write(body)
PY
  c++ -std=c++14 -Wall -Wextra -Werror -I"$dir" -I"$tests_dir" \
    "$tests_dir/standby_latch_daily_clear_test.cpp" -o "$binary"
}

# Byte check: each extracted piece must appear verbatim in the real sources.
verify_copies() {
  python3 - "$src_root" "$1" <<'PY'
import sys

root, d = sys.argv[1], sys.argv[2]
report = open(f"{root}/state/State_Report.cpp").read()
sleep = open(f"{root}/state/State_Sleep.cpp").read()
owner = open(f"{root}/power/ModemHealth.h").read() + open(f"{root}/power/ModemHealth.cpp").read()
for name, src in (("gen_report_block.inc", report), ("gen_trip_check.inc", sleep)):
    if open(f"{d}/{name}").read().rstrip("\n") not in src:
        sys.exit(f"COPY_MISMATCH: {name}")
defs = open(f"{d}/gen_sleep_defs.inc").read()
for part in ("constexpr unsigned long MODEM_UNSTABLE_SLOW_TEARDOWN_MS",
             "void markModemUnstable(", "void maybeClearModemUnstable(", "void clearAtDailyClose("):
    if part not in defs or part not in owner:
        sys.exit("COPY_MISMATCH: gen_sleep_defs.inc " + part)
for line in defs.splitlines():
    if line.strip() and line not in owner:
        sys.exit("COPY_MISMATCH: gen_sleep_defs.inc line: " + line)
PY
}

clean_dir="$work/clean"
extract_and_build "$src_root" "$clean_dir" "$clean_dir/test"
verify_copies "$clean_dir"
"$clean_dir/test"

mutate_and_expect_failure() {
  # $1 = label, $2 = expected failing check prefix, $3 = file, $4 = old, $5 = new
  local label="$1" expect="$2" file="$3" old="$4" new="$5"
  local m="$work/mut_${label// /_}"
  mkdir -p "$m/src/state" "$m/src/power"
  cp "$src_root/state/State_Report.cpp" "$src_root/state/State_Sleep.cpp" "$m/src/state/"
  cp "$src_root/power/ModemHealth.cpp" "$src_root/power/ModemHealth.h" "$m/src/power/"
  python3 - "$m/src/$file" "$old" "$new" <<'PY'
import sys

path, old, new = sys.argv[1:4]
text = open(path).read()
if text.count(old) != 1:
    sys.exit(f"MUTATION_NOT_APPLIED: {old!r} x{text.count(old)}")
open(path, "w").write(text.replace(old, new))
PY
  extract_and_build "$m/src" "$m" "$m/test" > "$m/build.log" 2>&1 || {
    echo "MUTANT_DID_NOT_BUILD: $label"; cat "$m/build.log"; exit 1; }
  if "$m/test" > "$m/run.log" 2>&1; then
    echo "MUTANT_SURVIVED: $label"; exit 1
  fi
  if ! grep -q "^FAIL $expect" "$m/run.log"; then
    echo "MUTANT_WRONG_CHECK: $label (wanted $expect)"; cat "$m/run.log"; exit 1
  fi
  echo "caught: $label ($(grep -c '^FAIL' "$m/run.log") check(s), incl. $expect)"
}

mutate_and_expect_failure "clear outside if due" "notdue:" "state/State_Report.cpp" \
  "    SystemConfig::set_lastDailyCleanup(now);
" "    SystemConfig::set_lastDailyCleanup(now);
  }
  {
"
mutate_and_expect_failure "clear call dropped" "latched:modemUnstable" "state/State_Report.cpp" \
  "    clearAtDailyClose();
" ""
mutate_and_expect_failure "modemUnstable not cleared" "latched:modemUnstable" "power/ModemHealth.cpp" \
  "    session.modemUnstable = false;
" ""
mutate_and_expect_failure "standbySuppressed not cleared" "latched:standbySuppressed" "power/ModemHealth.cpp" \
  "    session.modemStandbySuppressed = false;
    session.modemUnstableReason = 0;
" "    session.modemUnstableReason = 0;
"
mutate_and_expect_failure "reason not cleared" "latched:reason" "power/ModemHealth.cpp" \
  "    session.modemUnstableReason = 0;
" ""
mutate_and_expect_failure "timeout count not cleared" "latched:timeoutCount" "power/ModemHealth.cpp" \
  "    session.modemUnstableReason = 0;
    session.modemConnectTimeoutCount = 0;
" "    session.modemUnstableReason = 0;
"
mutate_and_expect_failure "clear unconditional" "unlatched:" "power/ModemHealth.cpp" \
  "  if (session.modemUnstable) {" "  {"

echo "standby_latch_daily_clear_test: clean run passed and all mutations detected"

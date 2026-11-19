#!/bin/zsh
set -euo pipefail

# WO-2026-11-19-001: the wiring of the one connect decision and its hold.
#
# Part 1 extracts the real "build Inputs, call decide, act" block of
# handleReportingState() byte-for-byte, checks the copy against src/ (COPY_MISMATCH)
# and runs it in connect_hold_wiring_test.cpp with the real ConnectDecision.cpp.
# Part 2 reads the real sources: the block sits where the chain sat (after the webhook
# stage-2 exit, the config-invalid exit, the battery commit and returnToSleepAfterReport,
# before the already-connected exit); Report has no other connect decision; the failure
# stamp is written once, beside noteConnectTimeout() on the timeout path; the 30 s defer
# and the four session fields it was the last user of are gone.
# Part 3 plants mutations on private copies of src/ (in the temp dir, never in the
# worktree); each must make the check aimed at it FAIL at run time, not at compile time.
# Set CONNECT_HOLD_SRC_ROOT to test another src/ root.

tests_dir="${0:A:h}"
repo_root="${tests_dir:h}"
src_root="${CONNECT_HOLD_SRC_ROOT:-$repo_root/src}"
work="$(mktemp -d "${TMPDIR:-/tmp}/connect_hold_wiring.XXXXXX")"
trap 'rm -rf "$work"' EXIT

run_all() {
  # $1 = src root, $2 = work dir. Prints FAIL lines; returns non-zero when any check fails.
  local root="$1" dir="$2"
  mkdir -p "$dir"
  python3 - "$root" "$dir" <<'PY'
import glob
import re
import sys

root, out = sys.argv[1], sys.argv[2]
report = open(f"{root}/state/State_Report.cpp").read()
connect = open(f"{root}/state/State_Connect.cpp").read()


def code_only(text):
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    return re.sub(r"//[^\n]*", "", text)


start = "    ConnectDecision::Inputs in;\n"
if report.count(start) != 1:
    sys.exit(f"EXTRACT_FAILED: decision block start x{report.count(start)}")
i = report.index(start)
j = report.find("transitionTo(decision.action", i)
if j < 0:
    sys.exit("EXTRACT_FAILED: the decision is not acted on by one transitionTo(decision.action ...)")
end = report.index(";\n", j) + 2
block = report[i:end]
open(f"{out}/gen_decision_block.inc", "w").write(block)
if open(f"{out}/gen_decision_block.inc").read() != report[i:end] or block not in report:
    sys.exit("COPY_MISMATCH: gen_decision_block.inc")

fails = []


def check(label, cond):
    if not cond:
        fails.append(label)


# Today's order: R0, R1, the connected test, the battery commit, returnToSleepAfterReport, the decision, R1b.
marks = [report.find(t) for t in (
    'transitionTo(ERROR_STATE, "webhook long-term failure");',
    'transitionTo(CONNECTING_STATE, "config invalid");',
    "if (!Particle.connected()) {",
    "BatteryAuthority::commit(",
    "session.returnToSleepAfterReport = true;",
    start,
    'transitionTo(IDLE_STATE, "already connected");')]
check("order: R0, R1, connected test, battery commit, returnToSleepAfterReport, decision, R1b", -1 not in marks and marks == sorted(marks))

# The decision is the only connect decision left in Report.
rcode = code_only(report)
check("single: decide() is called once", len(re.findall(r"\bConnectDecision::decide\(", rcode)) == 1)
check("single: Report's transitions are R0, R1, the decision and R1b",
      sorted(re.findall(r"transitionTo\(\s*([^,]+),", rcode)) == sorted([
          "ERROR_STATE", "CONNECTING_STATE", "IDLE_STATE",
          "decision.action == ConnectDecision::Action::CONNECT ? CONNECTING_STATE : IDLE_STATE"]))
check("single: the hold line is logged in one place", rcode.count('"CONNECT_HOLD: remaining=%lu failures=%u trigger=%s"') == 1)

# The failure stamp: one write in src/, beside noteConnectTimeout() inside the timeout branch.
writes = []
reads = []
for path in sorted(glob.glob(f"{root}/**/*", recursive=True)):
    if not path.endswith((".cpp", ".h")):
        continue
    text = code_only(open(path).read())
    rel = path[len(root) + 1:]
    writes += [rel for _ in re.finditer(r"\blastConnectFailMs\s*(?:=(?!=)|[-+*/|&^%]=|\+\+|--)", text) if rel != "state/StateMachine.h"]
    reads += [rel for _ in re.finditer(r"\blastConnectFailMs\b(?!\s*(?:=(?!=)|[-+*/|&^%]=|\+\+|--))", text) if rel != "state/StateMachine.h"]
    for gone in ("MODEM_UNSTABLE_RECONNECT_DEFER_MS", "lastTeardownEndMs", "lastCloudDisconnectElapsedMs",
                 "lastModemOffElapsedMs", "lastTotalTeardownElapsedMs", "reconnect deferred", '"modem unstable'):
        check(f"gone: {gone} is no longer in {rel}", gone not in text)
check("stamp: lastConnectFailMs is written once, in State_Connect.cpp", writes == ["state/State_Connect.cpp"])
check("stamp: lastConnectFailMs is read once, in State_Report.cpp", reads == ["state/State_Report.cpp"])
timeout_open = connect.find("if (elapsedMs > budgetMs) {")
beside = re.search(r"\n    noteConnectTimeout\(\);\n    session\.lastConnectFailMs = millis\(\);", connect)
check("stamp: set beside noteConnectTimeout() on the timeout path",
      timeout_open >= 0 and beside is not None and beside.start() > timeout_open
      and connect.find("noteConnectSuccess();") < timeout_open)
machine = code_only(open(f"{root}/state/StateMachine.h").read())
check("stamp: the field is RAM-only session state", len(re.findall(r"\buint32_t lastConnectFailMs = 0;", machine)) == 1)

for label in fails:
    print(f"FAIL {label}")
open(f"{out}/structure_failed", "w").write("1" if fails else "0")
PY
  c++ -std=c++17 -Wall -Wextra -Wno-unused-parameter -Werror -I"$dir" -I"$tests_dir/stubs/connect_decision" -I"$root" \
    "$tests_dir/connect_hold_wiring_test.cpp" "$root/power/ConnectDecision.cpp" -o "$dir/test"
  local rc=0
  "$dir/test" || rc=1
  [[ "$(<"$dir/structure_failed")" == 0 ]] || rc=1
  return $rc
}

run_all "$src_root" "$work/clean"
echo "structure: order kept, one decision, one stamp on the timeout path, the 30 s defer and its fields are gone"

mutate_and_expect_failure() {
  # $1 = label, $2 = prefix of the check aimed at it, $3 = rel file, $4 = old, $5 = new
  local label="$1" expect="$2" rel="$3" old="$4" new="$5"
  local m="$work/mut_${label// /_}"
  mkdir -p "$m"
  cp -R "$src_root" "$m/src"
  python3 - "$m/src/$rel" "$old" "$new" <<'PY'
import sys

path, old, new = sys.argv[1:4]
text = open(path).read()
if text.count(old) != 1:
    sys.exit(f"MUTATION_NOT_APPLIED: {old!r} x{text.count(old)}")
open(path, "w").write(text.replace(old, new))
PY
  if run_all "$m/src" "$m/w" > "$m/run.log" 2>&1; then
    echo "MUTANT_SURVIVED: $label"; exit 1
  fi
  if grep -q "error:\|EXTRACT_FAILED\|COPY_MISMATCH" "$m/run.log"; then
    echo "MUTANT_DID_NOT_BUILD: $label"; cat "$m/run.log"; exit 1
  fi
  if ! grep -q "^FAIL $expect" "$m/run.log"; then
    echo "MUTANT_WRONG_CHECK: $label (wanted $expect)"; head -5 "$m/run.log"; exit 1
  fi
  echo "caught: $label ($(grep -c '^FAIL' "$m/run.log") check(s), incl. $(grep -m1 "^FAIL $expect" "$m/run.log" | cut -c6-90))"
  rm -rf "$m"
}

mutate_and_expect_failure "failure count not wired" "wire: hold: a failure 10 min ago" "state/State_Report.cpp" \
  "in.failureCount = session.modemConnectTimeoutCount;" "in.failureCount = 0;"
mutate_and_expect_failure "hold clock not wired" "wire: hold: a failure 10 min ago" "state/State_Report.cpp" \
  "in.msSinceLastFailure = millis() - session.lastConnectFailMs;" "in.msSinceLastFailure = millis();"
mutate_and_expect_failure "hold line dropped" "wire: log: one CONNECT_HOLD line" "state/State_Report.cpp" \
  "if (decision.holdRemainingMs != 0) {" "if (false) {"
mutate_and_expect_failure "a wait connects" "wire: R10 nothing due goes to Idle" "state/State_Report.cpp" \
  "? CONNECTING_STATE : IDLE_STATE" "? CONNECTING_STATE : CONNECTING_STATE"
mutate_and_expect_failure "daily close not wired" "wire: R4 the daily close connects" "state/State_Report.cpp" \
  "in.closeDue = due;" "in.closeDue = false;"
mutate_and_expect_failure "failure stamp dropped" "stamp: lastConnectFailMs is written once" "state/State_Connect.cpp" \
  "    noteConnectTimeout();
    session.lastConnectFailMs = millis();" "    noteConnectTimeout();"
mutate_and_expect_failure "failure stamp on the success path" "stamp: lastConnectFailMs is written once" "state/State_Connect.cpp" \
  "    noteConnectSuccess();" "    noteConnectSuccess();
    session.lastConnectFailMs = millis();"
mutate_and_expect_failure "defer field restored" "gone: lastTeardownEndMs" "state/SleepTeardown.cpp" \
  "    if (teardownElapsedMs > MODEM_UNSTABLE_SLOW_TEARDOWN_MS) {" "    session.lastTeardownEndMs = millis();
    if (teardownElapsedMs > MODEM_UNSTABLE_SLOW_TEARDOWN_MS) {"

echo "connect_hold_wiring_test: clean run passed and all mutations detected"

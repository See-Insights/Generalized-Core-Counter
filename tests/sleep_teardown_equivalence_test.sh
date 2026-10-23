#!/bin/zsh
set -euo pipefail

# WO-2026-10-20-003 (Stage 6): moving the sleep cloud-disconnect and modem
# teardown from handleSleepingState() into state/SleepTeardown changes no
# behaviour. tests/sleep_teardown_equivalence_test.cpp runs one scenario table
# through (a) the teardown block extracted by marker text from the base
# State_Sleep.cpp and (b) the real src/state/SleepTeardown.cpp, against the same
# fakes, for cellular and Wi-Fi. Both transcripts must equal, byte for byte, the
# frozen fixtures/sleep_teardown_transcript.txt (its '# ' header lines are
# stripped first). Step (a) needs the base commit named in the fixture header;
# when this tree has no such commit the (a) check is skipped and said so.
# Mutants of a private copy of SleepTeardown.cpp must each change the transcript.
# Set SLEEP_TEARDOWN_SRC_ROOT to test another src/ root.
# '--regenerate' rewrites the fixture from the base block (a).

tests_dir="${0:A:h}"
repo_root="${tests_dir:h}"
src_root="${SLEEP_TEARDOWN_SRC_ROOT:-$repo_root/src}"
fixture="$tests_dir/fixtures/sleep_teardown_transcript.txt"
stubs="$tests_dir/stubs/sleep_teardown"
work="$(mktemp -d "${TMPDIR:-/tmp}/sleep_teardown_equivalence.XXXXXX")"
trap 'rm -rf "$work"' EXIT

base="$(sed -n 's/^# Base commit: //p' "$fixture" 2>/dev/null || true)"
base="${base:-0c565fa}"
base_src="$work/base_State_Sleep.cpp"
have_base=0
if git -C "$repo_root" show "${base}:src/state/State_Sleep.cpp" > "$base_src" 2>/dev/null; then have_base=1; fi

python3 - "$have_base" "$base_src" "$work" <<'PY'
import sys

have, path, work = sys.argv[1:4]
out = {"labels": "", "safety": "", "block": ""}
if have == "1":
    text = open(path).read()

    def span(start, end, inclusive_line=False):
        i = text.find(start)
        if i < 0 or text.count(start) != 1:
            sys.exit(f"BASE_MARKER_NOT_UNIQUE: {start!r} x{text.count(start)}")
        j = text.find(end, i)
        if j < 0:
            sys.exit(f"BASE_MARKER_MISSING: {end!r}")
        if inclusive_line:
            j = text.find("\n", j) + 1
        return text[i:j]

    out["labels"] = (span("const char *connectionModeLabel(", "void logWakeSummary(") +
                     span("void logTeardownContext(", "// Drain USB CDC"))
    out["safety"] = span("  // Safety: if disconnectRequested", "  // Particle's reference pattern")
    out["block"] = span("  // If we're going to sleep with cellular network standby", "  setAppBreadcrumb(23);", True)
for k, v in out.items():
    open(f"{work}/base_{k}.inc", "w").write(v)
PY

build_run() {
  # $1 = platform (cellular|wifi), $2 = mode (a|b), $3 = output, rest = extra include roots
  local platform="$1" mode="$2" out="$3"; shift 3
  local defs=()
  [[ "$platform" == cellular ]] && defs=(-DWiring_Cellular=1 -DHAL_PLATFORM_CELLULAR=1) || defs=(-DWiring_WiFi=1)
  local incs=(-I"$stubs")
  for r in "$@"; do incs+=(-I"$r"); done
  c++ -std=c++17 -Wall -Wextra -Wno-unused-parameter -Werror "${defs[@]}" "${incs[@]}" -I"$src_root" \
    -DBASE_LABELS_INC="\"$work/base_labels.inc\"" -DBASE_SAFETY_INC="\"$work/base_safety.inc\"" \
    -DBASE_BLOCK_INC="\"$work/base_block.inc\"" \
    "$tests_dir/sleep_teardown_equivalence_test.cpp" -o "$out.bin"
  "$out.bin" "$mode" > "$out"
}

run_all() {
  # $1 = mode, $2 = output prefix, rest = extra include roots
  local mode="$1" prefix="$2"; shift 2
  build_run cellular "$mode" "$prefix.cellular" "$@"
  build_run wifi "$mode" "$prefix.wifi" "$@"
  cat "$prefix.cellular" "$prefix.wifi" > "$prefix"
}

if [[ "${1:-}" == "--regenerate" ]]; then
  (( have_base )) || { echo "NO_BASE_COMMIT: $base"; exit 1; }
  run_all a "$work/regen.txt"
  python3 - "$base" "$work/regen.txt" "$fixture" <<'PY'
import hashlib
import re
import sys

base, body_path, fixture = sys.argv[1:4]
body = open(body_path).read()
counts = re.findall(r"^SCENARIOS (\d+) PASSES (\d+) WAIT (\d+) READY (\d+) ERROR (\d+) DISTINCT_EVENTS (\d+)$", body, re.M)
scen = sum(int(c[0]) for c in counts)
passes = sum(int(c[1]) for c in counts)
head = f"""# Frozen output of the pre-change sleep teardown block: WO-2026-10-20-003 (Stage 6).
# Base commit: {base}
# Scenarios: {scen} (cellular {counts[0][0]} + wifi {counts[1][0]}); passes: {passes}
# Per-platform summary (scenarios passes WAIT READY ERROR distinct-events): cellular {' '.join(counts[0])}; wifi {' '.join(counts[1])}
# Body sha256: {hashlib.sha256(body.encode()).hexdigest()}
# Generated once by extracting the block by marker text from {base}:src/state/State_Sleep.cpp (git show),
# wrapping it in tests/sleep_teardown_equivalence_test.cpp against tests/stubs/sleep_teardown, and running mode a.
"""
open(fixture, "w").write(head + body)
PY
  echo "fixture written: $fixture"
  exit 0
fi

grep -v '^# ' "$fixture" > "$work/expected.txt"
python3 - "$fixture" "$work/expected.txt" <<'PY'
import hashlib
import re
import sys

fixture, body_path = sys.argv[1:3]
head = open(fixture).read()
body = open(body_path).read()
want = re.search(r"^# Body sha256: (\w+)$", head, re.M).group(1)
if hashlib.sha256(body.encode()).hexdigest() != want:
    sys.exit("FIXTURE_HEADER_SHA_MISMATCH")
scen = int(re.search(r"^# Scenarios: (\d+)", head, re.M).group(1))
if sum(int(n) for n in re.findall(r"^SCENARIOS (\d+) ", body, re.M)) != scen:
    sys.exit("FIXTURE_HEADER_SCENARIO_COUNT_MISMATCH")
PY

run_all b "$work/new.txt"
diff -u "$work/expected.txt" "$work/new.txt" > "$work/new.diff" || { head -60 "$work/new.diff"; echo "EQUIVALENCE_FAILED: SleepTeardown.cpp differs from the frozen transcript"; exit 1; }

if (( have_base )); then
  run_all a "$work/old.txt"
  diff -u "$work/expected.txt" "$work/old.txt" > "$work/old.diff" || { head -60 "$work/old.diff"; echo "FIXTURE_NOT_FROM_BASE: the base block differs from the fixture ($base)"; exit 1; }
else
  echo "BASE_CHECK_SKIPPED: commit $base is not in this tree; only the fixture comparison ran"
fi

for line in "ALERT 15" "TRANSITION 1 sleep-disconnect-timeout" \
            "CALL Cellular.off" "CALL Particle.disconnect" "CALL WiFi.off" "PROGRESS DISCONNECT_WAIT" \
            "MODEM markUnstable reason=1" "MODEM maybeClear" "CRUMB 23" "CYCLE teardownEnd" \
            "LOG WARN SLEEP: disconnectRequested latched without timestamp - resetting" \
            "LOG WARN SLEEP: standby sleep failed; forcing modem-off reason=sleep_error err=-170" \
            "LOG WARN SLEEP: pre-sleep gate blocked (cloud=0 radioOn=1 standby=0) - requesting modem off" \
            "LOG WARN SLEEP: disconnect/modem-off exceeded budget" \
            "LOG INFO SLEEP: modem-off complete elapsed" "LOG INFO Sleep: td="; do
  grep -qF -- "$line" "$work/new.txt" || { echo "LOG_LINE_NEVER_EXERCISED: $line"; exit 1; }
done
for outcome in WAIT READY ERROR; do
  grep -q " $outcome f=" "$work/new.txt" || { echo "OUTCOME_NEVER_SEEN: $outcome"; exit 1; }
done

mutate_and_expect_diff() {
  # $1 = label, $2 = old, $3 = new
  local m="$work/mut_${1// /_}"
  mkdir -p "$m/state"
  cp "$src_root/state/SleepTeardown.cpp" "$src_root/state/SleepTeardown.h" "$m/state/"
  python3 - "$m/state/SleepTeardown.cpp" "$2" "$3" <<'PY'
import sys

path, old, new = sys.argv[1:4]
text = open(path).read()
if text.count(old) != 1:
    sys.exit(f"MUTATION_NOT_APPLIED: {old!r} x{text.count(old)}")
open(path, "w").write(text.replace(old, new))
PY
  run_all b "$m/new.txt" "$m" > "$m/build.log" 2>&1 || {
    echo "MUTANT_DID_NOT_BUILD: $1"; cat "$m/build.log"; exit 1; }
  if diff -q "$work/expected.txt" "$m/new.txt" > /dev/null; then
    # $4 = "unreachable": the branch is dead (see below), so surviving is expected and only reported
    [[ "${4:-}" == unreachable ]] && { echo "unpinned (dead branch): $1"; return; }
    echo "MUTANT_SURVIVED: $1"; exit 1
  fi
  echo "caught: $1"
}

mutate_and_expect_diff "budget plus one second" "cloudBudgetSec) * 1000UL;

    // When using" "cloudBudgetSec) * 1000UL + 1000UL;

    // When using"
mutate_and_expect_diff "standby cap 6000" "DISCONNECT_STANDBY_MAX_MS) {
        budgetMs = ConnectivityPolicy::DISCONNECT_STANDBY_MAX_MS;" "DISCONNECT_STANDBY_MAX_MS) {
        budgetMs = ConnectivityPolicy::DISCONNECT_STANDBY_MAX_MS + 1000UL;"
mutate_and_expect_diff "no alert 15 on primary timeout" "(unsigned long)(millis() - disconnectRequestStartMs));
      RecoveryState::raiseAlert(15);" "(unsigned long)(millis() - disconnectRequestStartMs));"
# The final precondition's timeout branch cannot run: by then the block above has either returned (requested
# and still on) or cleared disconnectRequested (requested and off), so the gate always takes the request path.
mutate_and_expect_diff "no alert 15 on precondition timeout" "markModemUnstable(MODEM_UNSTABLE_REASON_SLOW_TEARDOWN, elapsedMs);
      RecoveryState::raiseAlert(15);" "markModemUnstable(MODEM_UNSTABLE_REASON_SLOW_TEARDOWN, elapsedMs);" unreachable
mutate_and_expect_diff "no isOff precondition" "return !Particle.connected() && Cellular.isOff();" "return !Particle.connected();"
mutate_and_expect_diff "slow teardown threshold inclusive" "teardownElapsedMs > MODEM_UNSTABLE_SLOW_TEARDOWN_MS" "teardownElapsedMs >= MODEM_UNSTABLE_SLOW_TEARDOWN_MS"
mutate_and_expect_diff "progress throttle 1001" "(nowMs - lastDisconnectProgressMs) > 1000UL" "(nowMs - lastDisconnectProgressMs) > 1001UL"
mutate_and_expect_diff "force flag not cleared" "      forceModemOffAfterStandbyFailure = false;
" "
"
mutate_and_expect_diff "lost timestamp guard dropped" "if (disconnectRequested && disconnectRequestStartMs == 0) {" "if (false) {"

echo "sleep_teardown_equivalence_test: both transcripts match the frozen pre-move output, all mutations detected"

#!/bin/zsh
set -euo pipefail

# WO-2026-10-19-004: moving the modem-health latch writes into power/ModemHealth
# changes no decision and no log line. The real src/power/ModemHealth.cpp runs
# the cases of modem_health_equivalence_test.cpp; the transcript (the four latch
# fields and every log line with its level) must equal, byte for byte, the
# frozen output of the pre-move code in fixtures/modem_health_transcript.txt
# (its '# ' header lines are stripped first). No old source is needed here.
# Mutants of a private copy of ModemHealth.cpp must each make the transcript
# differ from the fixture. Set MODEM_HEALTH_SRC_ROOT to test another src/ root.

tests_dir="${0:A:h}"
repo_root="${tests_dir:h}"
src_root="${MODEM_HEALTH_SRC_ROOT:-$repo_root/src}"
fixture="$tests_dir/fixtures/modem_health_transcript.txt"
work="$(mktemp -d "${TMPDIR:-/tmp}/modem_health_equivalence.XXXXXX")"
trap 'rm -rf "$work"' EXIT

grep -v '^# ' "$fixture" > "$work/expected.txt"

build_run() {
  # $1 = src root, $2 = transcript
  c++ -std=c++14 -Wall -Wextra -Werror -I"$tests_dir/stubs/modem_health" -I"$1" \
    "$tests_dir/modem_health_equivalence_test.cpp" -o "$2.bin"
  "$2.bin" > "$2"
}

build_run "$src_root" "$work/new.txt"
diff -u "$work/expected.txt" "$work/new.txt" || { echo "EQUIVALENCE_FAILED"; exit 1; }
tail -1 "$work/new.txt" | grep -qx "CASES 176" || { echo "UNEXPECTED_CASE_COUNT"; exit 1; }
for line in "LOG WARN MODEM_HEALTH: unstable reason=connect_timeout count=2" \
            "LOG WARN MODEM_HEALTH: unstable reason=slow_teardown elapsed=12345" \
            "LOG WARN MODEM_POLICY: standby temporarily disabled reason=unstable_modem" \
            "LOG INFO MODEM_HEALTH: recovered" "LOG INFO MODEM_POLICY: standby restored" \
            "LOG INFO MODEM_POLICY: standby re-allowed at daily close; night modem-off decides"; do
  grep -q -- "$line" "$work/new.txt" || { echo "LOG_LINE_NEVER_EXERCISED: $line"; exit 1; }
done

mutate_and_expect_diff() {
  # $1 = label, $2 = old, $3 = new
  local m="$work/mut_${1// /_}"
  mkdir -p "$m/power"
  cp "$src_root/power/ModemHealth.cpp" "$src_root/power/ModemHealth.h" "$m/power/"
  python3 - "$m/power/ModemHealth.cpp" "$2" "$3" <<'PY'
import sys

path, old, new = sys.argv[1:4]
text = open(path).read()
if text.count(old) != 1:
    sys.exit(f"MUTATION_NOT_APPLIED: {old!r} x{text.count(old)}")
open(path, "w").write(text.replace(old, new))
PY
  build_run "$m" "$m/new.txt" > "$m/build.log" 2>&1 || {
    echo "MUTANT_DID_NOT_BUILD: $1"; cat "$m/build.log"; exit 1; }
  if diff -q "$work/expected.txt" "$m/new.txt" > /dev/null; then echo "MUTANT_SURVIVED: $1"; exit 1; fi
  echo "caught: $1"
}

mutate_and_expect_diff "connect timeout threshold 3" "THRESHOLD = 2;" "THRESHOLD = 3;"
mutate_and_expect_diff "recovery limit 6000" "RECOVERY_TEARDOWN_MS = 5000UL" "RECOVERY_TEARDOWN_MS = 6000UL"
mutate_and_expect_diff "count not saturated" "if (session.modemConnectTimeoutCount < 0xFF) {" "if (true) {"
mutate_and_expect_diff "daily close skips count" "    session.modemConnectTimeoutCount = 0;
    Log.info(\"MODEM_POLICY: standby re-allowed" "    Log.info(\"MODEM_POLICY: standby re-allowed"

echo "modem_health_equivalence_test: transcript matches the frozen pre-move output (176 cases), all mutations detected"

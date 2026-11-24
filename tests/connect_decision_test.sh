#!/bin/zsh
set -euo pipefail

# WO-2026-11-19-001: one connect decision, with a hold after failure.
#
# connect_decision_test.cpp is compiled against the real src/power/ConnectDecision.cpp
# and run: the characterization of the REPORTING chain at eb398b8 (failureCount = 0),
# the hold sequence (1, 2, 4, 6, 6 h), the hold rows, the service-request exemption and
# the reset. The module is built against tests/stubs/connect_decision/Particle.h, which is
# empty, and must not log or name a global.
# Mutants are built from private copies of ConnectDecision.cpp (in the temp dir, never
# in the worktree); each must make the check aimed at it FAIL at run time. A mutant
# that does not compile is a broken mutation, not a caught one.
# Set CONNECT_DECISION_SRC_ROOT to test another src/ root.

tests_dir="${0:A:h}"
repo_root="${tests_dir:h}"
src_root="${CONNECT_DECISION_SRC_ROOT:-$repo_root/src}"
work="$(mktemp -d "${TMPDIR:-/tmp}/connect_decision.XXXXXX")"
trap 'rm -rf "$work"' EXIT

build() {
  # $1 = ConnectDecision.cpp to link, $2 = binary
  c++ -std=c++17 -Wall -Wextra -Werror -I"$tests_dir/stubs/connect_decision" -I"$src_root" \
    "$tests_dir/connect_decision_test.cpp" "$1" -o "$2"
}

build "$src_root/power/ConnectDecision.cpp" "$work/clean_test"
"$work/clean_test"

# Pure: no Particle or Device OS call, no global, no logging.
python3 - "$src_root" <<'PY'
import re
import sys

root = sys.argv[1]
for rel in ("power/ConnectDecision.h", "power/ConnectDecision.cpp"):
    code = open(f"{root}/{rel}").read()
    code = re.sub(r"/\*.*?\*/", "", code, flags=re.S)
    code = re.sub(r"//[^\n]*", "", code)
    code = re.sub(r'"(?:\\.|[^"\\\n])*"', '""', code)
    hit = re.search(r'\b(?:Log|Particle|Time|System|Cellular|WiFi|Serial|session)\b|\bmillis\s*\(|\bextern\b', code)
    if hit:
        sys.exit(f"NOT_PURE: {rel} uses {hit.group(0)!r}")
    # A file-scope (column 0) object that is not const: "type name;" or "type name = ...;".
    var = re.search(r"^(?!const\b|constexpr\b|using\b|typedef\b)(?:static\s+)?(?:[\w:<>]+[\s*&]+)+\w+\s*(?:=[^(){}]*)?;", code, re.M)
    if var:
        sys.exit(f"NOT_PURE: {rel} defines a file-scope variable: {var.group(0)!r}")
PY
echo "pure: ConnectDecision builds against the empty Particle stub and names no Device OS object, global or log"

mutate_and_expect_failure() {
  # $1 = label, $2 = prefix of the check aimed at it, $3 = old, $4 = new
  local label="$1" expect="$2" old="$3" new="$4"
  local m="$work/mut_${label// /_}"
  mkdir -p "$m"
  python3 - "$src_root/power/ConnectDecision.cpp" "$m/ConnectDecision.cpp" "$old" "$new" <<'PY'
import sys

src, dst, old, new = sys.argv[1:5]
text = open(src).read()
if text.count(old) != 1:
    sys.exit(f"MUTATION_NOT_APPLIED: {old!r} x{text.count(old)}")
open(dst, "w").write(text.replace(old, new))
PY
  build "$m/ConnectDecision.cpp" "$m/test" > "$m/build.log" 2>&1 || {
    echo "MUTANT_DID_NOT_BUILD: $label"; cat "$m/build.log"; exit 1; }
  if "$m/test" > "$m/run.log" 2>&1; then
    echo "MUTANT_SURVIVED: $label"; exit 1
  fi
  if ! grep -q "^FAIL $expect" "$m/run.log"; then
    echo "MUTANT_WRONG_CHECK: $label (wanted $expect)"; head -5 "$m/run.log"; exit 1
  fi
  echo "caught: $label ($(grep -c "^FAIL $expect" "$m/run.log") '$expect' check(s) of $(grep -c '^FAIL' "$m/run.log") failed, e.g. $(grep -m1 "^FAIL $expect" "$m/run.log" | cut -c1-96))"
}

hold_rule="if (!in.serviceRequest && in.failureCount >= 1 && in.msSinceLastFailure < holdMs(in.failureCount)) {"
mutate_and_expect_failure "hold rule dropped" "hold: occupancy change waits" "$hold_rule" "if (false) {"
mutate_and_expect_failure "doubling removed" "seq: failure 2 holds 2 h" "n++) ms <<= 1;" "n++) {}"
mutate_and_expect_failure "cap removed" "seq: failure 4 holds 6 h" \
  "return ms < ConnectivityPolicy::HOLD_MAX_MS ? ms : ConnectivityPolicy::HOLD_MAX_MS;" "return ms;"
mutate_and_expect_failure "occupancy change exempt from the hold" "hold: occupancy change waits" \
  "if (!in.serviceRequest && in.failureCount >= 1" "if (!in.serviceRequest && !in.occupancyChange && in.failureCount >= 1"
mutate_and_expect_failure "service request held" "service: connects during a hold" \
  "if (!in.serviceRequest && in.failureCount >= 1" "if (in.failureCount >= 1"
mutate_and_expect_failure "daily close after keep alive" "char: R4 daily close, KEEP_ALIVE" \
  "  if (in.closeDue) return \"daily close\";
  if (in.webhookHealthDue) return \"webhook health check\";
  if (in.effectiveMode == SystemConfig::INTERMITTENT_KEEP_ALIVE) return \"keep alive mode\";" \
  "  if (in.webhookHealthDue) return \"webhook health check\";
  if (in.effectiveMode == SystemConfig::INTERMITTENT_KEEP_ALIVE) return \"keep alive mode\";
  if (in.closeDue) return \"daily close\";"

echo "connect_decision_test: clean run passed and all mutations detected"

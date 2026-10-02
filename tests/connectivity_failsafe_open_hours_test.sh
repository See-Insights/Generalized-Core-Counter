#!/bin/zsh
set -euo pipefail

# WO-2026-10-02-001 item A: the connectivity failsafe recovers within about
# three OPEN hours instead of twelve wall-clock hours.
#
# Part 1 (connectivity_failsafe_open_hours_test.cpp) runs a host mirror of
# connectivityFailsafeSupervisor()'s gating. The three timings and the three
# structural facts it depends on are parsed out of the real sources here and
# injected as -D flags, so a mutation in src/ changes what Part 1 asserts.
#
# Part 2 traces the real supervisor body to confirm the open-hours rule is
# actually implemented the way the work order specifies.

repo_root="${0:A:h:h}"
binary="$TMPDIR/connectivity_failsafe_open_hours_test"
policy_src="$repo_root/src/power/ConnectivityPolicy.h"
app_src="$repo_root/src/Generalized-Core-Counter.cpp"

extract_supervisor() {
  python3 - "$app_src" <<'PY'
import sys

path = sys.argv[1]
lines = open(path).readlines()

start = None
for i, line in enumerate(lines):
    if line.startswith("void connectivityFailsafeSupervisor() {"):
        start = i
        break
if start is None:
    sys.exit("EXTRACT_FAILED: connectivityFailsafeSupervisor() not found")

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
    sys.exit("EXTRACT_FAILED: closing brace not found")
PY
}

supervisor=$(extract_supervisor)
if [[ -z "$supervisor" ]]; then
  echo "FAILED: could not extract connectivityFailsafeSupervisor() from $app_src" >&2
  exit 1
fi

# --- Timings, from the PRODUCTION branch of the policy header -------------
read_policy() {
  python3 - "$policy_src" "$1" <<'PY'
import re, sys

path, name = sys.argv[1], sys.argv[2]
text = open(path).read()
# The production values are the ones in the #else branch of the
# CONNECTIVITY_FAILSAFE_TEST_MODE block.
block = re.search(r"#if CONNECTIVITY_FAILSAFE_TEST_MODE(.*?)#else(.*?)#endif", text, re.S)
if not block:
    sys.exit("EXTRACT_FAILED: CONNECTIVITY_FAILSAFE_TEST_MODE block not found")
m = re.search(r"constexpr\s+time_t\s+%s\s*=\s*([^;]+);" % re.escape(name), block.group(2))
if not m:
    sys.exit("EXTRACT_FAILED: %s not found in the production branch" % name)
expr = m.group(1).replace("L", "").strip()
if not re.fullmatch(r"[0-9*+\s]+", expr):
    sys.exit("EXTRACT_FAILED: %s is not a plain arithmetic literal (%s)" % (name, expr))
print(int(eval(expr)))
PY
}

stale_sec=$(read_policy CONNECTIVITY_FAILSAFE_STALE_SEC)
cooldown_sec=$(read_policy CONNECTIVITY_FAILSAFE_COOLDOWN_SEC)
jitter_sec=$(read_policy CONNECTIVITY_FAILSAFE_JITTER_MAX_SEC)

echo "Production failsafe timings from $policy_src:"
echo "  stale=${stale_sec}s cooldown=${cooldown_sec}s jitterMax=${jitter_sec}s"

# --- Structural facts, from the real supervisor body ----------------------
open_gate=0
if print -r -- "$supervisor" | grep -q "Clock::openness() != Clock::Openness::Open"; then
  open_gate=1
fi

open_base=0
if print -r -- "$supervisor" | grep -q "DailyBoundary::todayAt(SystemConfig::get_openTime())"; then
  open_base=1
fi

first_stage=$(print -r -- "$supervisor" |
  grep -E "const uint8_t nextStage" |
  grep -oE "\? *[0-9]+" | grep -oE "[0-9]+" | head -1)
if [[ -z "$first_stage" ]]; then
  # No conditional form at all: the first action is currentStage + 1 = 1.
  first_stage=1
fi

clang++ -std=c++17 -Wall -Wextra -pedantic \
  -DREAL_STALE_SEC="$stale_sec" \
  -DREAL_COOLDOWN_SEC="$cooldown_sec" \
  -DREAL_JITTER_MAX_SEC="$jitter_sec" \
  -DREAL_OPEN_GATE="$open_gate" \
  -DREAL_USES_OPEN_BASE="$open_base" \
  -DREAL_FIRST_STAGE="$first_stage" \
  "$repo_root/tests/connectivity_failsafe_open_hours_test.cpp" \
  -o "$binary"
"$binary"

echo ""
echo "--- Part 2: the real supervisor's open-hours rule ---"

fail() {
  echo "FAILED: $1" >&2
  exit 1
}

if (( stale_sec != 3 * 3600 )); then
  fail "CONNECTIVITY_FAILSAFE_STALE_SEC must be 3 h (10800s) in production, found ${stale_sec}s"
fi
if (( cooldown_sec != 6 * 3600 )); then
  fail "CONNECTIVITY_FAILSAFE_COOLDOWN_SEC must stay 6 h, found ${cooldown_sec}s"
fi
if (( jitter_sec != 30 * 60 )); then
  fail "CONNECTIVITY_FAILSAFE_JITTER_MAX_SEC must stay 30 min, found ${jitter_sec}s"
fi
echo "OK: production timings are 3 h stale / 6 h cooldown / 30 min jitter"

(( open_gate == 1 )) || fail "the supervisor must act only while Clock::openness() == Open"
(( open_base == 1 )) || \
  fail "the supervisor must base the age on DailyBoundary::todayAt(SystemConfig::get_openTime())"
echo "OK: the supervisor gates on Clock::openness() and bases the age on today's opening"

# The age must be measured from the LATER of lastConnection and the opening.
if ! print -r -- "$supervisor" | grep -q "(openedAt > lastConnection) ? openedAt : lastConnection"; then
  fail "the age base must be the later of today's opening and lastConnection"
fi
if ! print -r -- "$supervisor" | grep -q "connectionAgeSec = now - ageBase"; then
  fail "connectionAgeSec must be measured from the open-hours age base"
fi
echo "OK: the age is now - max(lastConnection, today's opening)"

# The clear-on-success path must stay where it is.
if ! grep -q 'clearConnectivityFailsafeRecovery("cloud-ok")' "$repo_root/src/state/State_Connect.cpp"; then
  fail "State_Connect.cpp must still clear the failsafe on a successful cloud connection"
fi
echo "OK: a successful cloud connection still clears the failsafe"

# Test mode keeps its short timings, and the open-hours rule applies there too.
test_stale=$(python3 - "$policy_src" <<'PY'
import re, sys
text = open(sys.argv[1]).read()
block = re.search(r"#if CONNECTIVITY_FAILSAFE_TEST_MODE(.*?)#else", text, re.S).group(1)
m = re.search(r"CONNECTIVITY_FAILSAFE_STALE_SEC\s*=\s*([^;]+);", block)
print(int(eval(m.group(1).replace("L", ""))))
PY
)
(( test_stale == 300 )) || fail "test-mode stale must stay 5 min, found ${test_stale}s"
echo "OK: test mode keeps its 5 min stale threshold"

failsafe_test_src="$repo_root/src/diagnostics/ConnectivityFailsafeTest.cpp"
for needle in "Clock::openness() == Clock::Openness::Open" \
              "DailyBoundary::todayAt(SystemConfig::get_openTime())" \
              "now - ageBase"; do
  grep -q "$needle" "$failsafe_test_src" || \
    fail "the test-mode prediction in ConnectivityFailsafeTest.cpp must also use: $needle"
done
echo "OK: the test-mode prediction follows the same open-hours rule"

echo ""
echo "connectivity_failsafe_open_hours_test (host mirror + real-source fidelity) passed"

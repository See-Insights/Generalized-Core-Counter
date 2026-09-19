#!/bin/zsh
set -euo pipefail

# WO-2026-09-19 Step 3b: Clock::openness()'s truth table.
#
# Part 1 (clock_openness_test.cpp) runs a hand-written mirror of the
# three-input decision on the host. Part 2 here traces the REAL
# src/time/Clock.cpp to confirm the real openness() actually checks trust
# FIRST, then config validity, then falls through to the same open/closed
# computation isWithinOpenHours() itself uses - so the host-side mirror
# cannot silently drift from what production actually does.

repo_root="${0:A:h:h}"
binary="$TMPDIR/clock_openness_test"
clock_src="$repo_root/src/time/Clock.cpp"

clang++ -std=c++17 -Wall -Wextra -pedantic \
  -I"$repo_root/src" \
  "$repo_root/tests/clock_openness_test.cpp" \
  -o "$binary"
"$binary"

echo ""
echo "--- Part 2: Clock::openness() fidelity checks against the real source ---"

openness_fn=$(python3 - "$clock_src" <<'PY'
import sys

path = sys.argv[1]
with open(path) as f:
    lines = f.readlines()

start_idx = None
for i, line in enumerate(lines):
    if "Openness openness() {" in line:
        start_idx = i
        break

if start_idx is None:
    sys.exit("EXTRACT_FAILED: signature not found")

depth = 0
opened = False
end_idx = None
for i in range(start_idx, len(lines)):
    for ch in lines[i]:
        if ch == '{':
            depth += 1
            opened = True
        elif ch == '}':
            depth -= 1
    if opened and depth == 0:
        end_idx = i
        break

if end_idx is None:
    sys.exit("EXTRACT_FAILED: closing brace not found")

sys.stdout.write("".join(lines[start_idx:end_idx + 1]))
PY
)

if [[ -z "$openness_fn" ]]; then
  echo "FAILED: could not extract Clock::openness() verbatim from $clock_src" >&2
  exit 1
fi
echo "Extracted the real Clock::openness() body verbatim from $clock_src"

if ! print -r -- "$openness_fn" | grep -q "!isTrusted()"; then
  echo "FAILED: Clock::openness() must check !isTrusted() (not !isTimeValid() or a re-derived trust check)" >&2
  exit 1
fi

if ! print -r -- "$openness_fn" | grep -q "!Config::isValid(false)"; then
  echo "FAILED: Clock::openness() must check !Config::isValid(false)" >&2
  exit 1
fi

if ! print -r -- "$openness_fn" | grep -q "Openness::Unknown"; then
  echo "FAILED: Clock::openness() must be able to return Openness::Unknown" >&2
  exit 1
fi

# Ordering: the trust check must appear BEFORE the config check, which must
# appear before the open/closed computation - so an untrusted clock never
# even reaches the config or hour-window logic (must never fail open).
trust_idx=$(print -r -- "$openness_fn" | grep -n "!isTrusted()" | head -1 | cut -d: -f1)
config_idx=$(print -r -- "$openness_fn" | grep -n "!Config::isValid(false)" | head -1 | cut -d: -f1)
hour_idx=$(print -r -- "$openness_fn" | grep -n "isWithinOpenHoursForHour(" | head -1 | cut -d: -f1)

if [[ -z "$trust_idx" || -z "$config_idx" || -z "$hour_idx" ]]; then
  echo "FAILED: could not locate all three checks (trust/config/hour) inside Clock::openness()" >&2
  exit 1
fi

if (( trust_idx >= config_idx || config_idx >= hour_idx )); then
  echo "FAILED: Clock::openness() must check trust, then config validity, then the hour window, in that order (found lines trust=$trust_idx config=$config_idx hour=$hour_idx)" >&2
  exit 1
fi

echo "OK: Clock::openness() checks isTrusted(), then Config::isValid(false), then the hour window, in that order - and can return Unknown"
echo ""
echo "clock_openness_test (host mirror + real-source fidelity) passed"

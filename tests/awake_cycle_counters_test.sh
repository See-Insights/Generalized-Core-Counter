#!/bin/zsh
set -euo pipefail

# WO-2026-10-02-001 item E: "cyc" and "slp".
#
# Part 1 is BEHAVIOURAL against the real header: src/observability/
# AwakeCycleCounters.h is compiled on the host and driven directly, so the
# invariants (cyc starts at 1, cyc >= slp always, a FAILED sleep increments
# cyc but not slp) are proven on production code.
#
# Part 2 is structural: every System.sleep() call in State_Sleep.cpp must be
# followed immediately by recordSleepReturn(), with the success of that very
# call - a new sleep site that forgets the counter fails here.

repo_root="${0:A:h:h}"
work="$TMPDIR/awake_cycle_counters_test"
sleep_src="$repo_root/src/state/State_Sleep.cpp"
header="$repo_root/src/observability/AwakeCycleCounters.h"

rm -rf "$work"
mkdir -p "$work"

cat > "$work/main.cpp" <<'EOF'
#include <cstdint>
#include <cstdio>

#include "observability/AwakeCycleCounters.h"

static int failures = 0;

static void check(const char *name, unsigned long got, unsigned long want) {
  if (got == want) {
    std::printf("  ok   %-58s %lu\n", name, got);
  } else {
    std::printf("  FAIL %-58s got %lu, want %lu\n", name, got, want);
    failures++;
  }
}

static void invariant() {
  if (AwakeCycles::cycles < AwakeCycles::sleeps) {
    std::printf("  FAIL cyc >= slp violated (cyc=%lu slp=%lu)\n",
                (unsigned long)AwakeCycles::cycles,
                (unsigned long)AwakeCycles::sleeps);
    failures++;
  }
}

int main() {
  std::printf("--- Part 1: AwakeCycles, driven directly ---\n");

  check("cyc starts at 1 at boot", AwakeCycles::cycles, 1);
  check("slp starts at 0 at boot", AwakeCycles::sleeps, 0);
  invariant();

  // A successful sleep: a new awake period, and the previous one ended in a
  // sleep.
  AwakeCycles::recordSleepReturn(true);
  check("after one successful sleep, cyc", AwakeCycles::cycles, 2);
  check("after one successful sleep, slp", AwakeCycles::sleeps, 1);
  invariant();

  // A failed sleep: still a return from System.sleep(), so cyc moves; the
  // period did not end in a sleep, so slp does not.
  AwakeCycles::recordSleepReturn(false);
  check("a failed sleep increments cyc", AwakeCycles::cycles, 3);
  check("a failed sleep does NOT increment slp", AwakeCycles::sleeps, 1);
  invariant();

  // The documented arithmetic: cyc - slp - 1 is the number of failed calls.
  check("cyc - slp - 1 counts the failed sleep calls",
        AwakeCycles::cycles - AwakeCycles::sleeps - 1, 1);

  // A long run of mixed results keeps the invariant.
  for (int i = 0; i < 1000; i++) {
    AwakeCycles::recordSleepReturn((i % 3) != 0);
    invariant();
  }
  check("after 1000 mixed sleeps, cyc", AwakeCycles::cycles, 1003);
  check("after 1000 mixed sleeps, slp", AwakeCycles::sleeps, 1 + 666);
  invariant();

  // "The same cyc in two reports means the device did not sleep between
  // them": nothing but a return from System.sleep() can move cyc.
  const uint32_t before = AwakeCycles::cycles;
  check("cyc is unchanged without a sleep return", AwakeCycles::cycles, before);

  if (failures != 0) {
    std::printf("\nFAILED: %d check(s)\n", failures);
    return 1;
  }
  std::printf("\nPart 1 passed\n");
  return 0;
}
EOF

clang++ -std=c++17 -Wall -Wextra -pedantic -I"$repo_root/src" "$work/main.cpp" -o "$work/bin"
"$work/bin"

echo ""
echo "--- Part 2: every System.sleep() site records its return ---"

fail() {
  echo "FAILED: $1" >&2
  exit 1
}

# The counters must be RAM only - a retained/persisted counter would survive
# the night HIBERNATE and break "reset at every boot".
code_lines=$(grep -vE '^\s*(\*|//|/\*)' "$header" || true)
if print -r -- "$code_lines" | grep -qE "\bretained\b|SystemConfig|RecoveryState|static"; then
  fail "AwakeCycleCounters.h must hold plain RAM counters, not retained or persisted state"
fi
if ! print -r -- "$code_lines" | grep -q "inline uint32_t cycles = 1"; then
  fail "cyc must start at 1 at boot"
fi
if ! print -r -- "$code_lines" | grep -q "sleeps = 0"; then
  fail "slp must start at 0 at boot"
fi
echo "OK: cyc and slp are plain RAM counters, 1 and 0 at boot (reset at every boot)"

python3 - "$sleep_src" <<'PY' || exit 1
import re
import sys

path = sys.argv[1]
lines = open(path).readlines()

sites = [i for i, line in enumerate(lines) if re.search(r"=\s*System\.sleep\(", line)]
if len(sites) != 4:
    print("FAILED: expected 4 System.sleep() call sites in %s, found %d" % (path, len(sites)))
    sys.exit(1)

ok = True
for i in sites:
    call = lines[i].strip()
    nxt = lines[i + 1].strip() if i + 1 < len(lines) else ""
    # The result variable the call assigned to.
    var = re.match(r"(?:const\s+)?(?:SystemSleepResult\s+)?(\w+)\s*=", call).group(1)
    want = "AwakeCycles::recordSleepReturn(%s.error() == SYSTEM_ERROR_NONE);" % var
    if nxt != want:
        print("FAILED: %s:%d - %s" % (path, i + 1, call))
        print("        next line must be: %s" % want)
        print("        found:             %s" % (nxt or "<end of file>"))
        ok = False
    else:
        print("  ok   line %4d: %-46s -> records %s" % (i + 1, call, var))

if not ok:
    sys.exit(1)
print("OK: all 4 System.sleep() sites increment cyc, and slp only on success")
PY

grep -q '#include "observability/AwakeCycleCounters.h"' "$sleep_src" || \
  fail "State_Sleep.cpp must include observability/AwakeCycleCounters.h"

rm -rf "$work"
echo ""
echo "awake_cycle_counters_test (real-header behaviour + call-site structure) passed"

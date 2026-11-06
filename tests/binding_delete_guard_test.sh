#!/bin/zsh
# WO-2026-11-04-002 - v49 binding-delete guard.
#
# Behavioral test: the REAL guard block is extracted verbatim from setup() in
# src/Generalized-Core-Counter.cpp and compiled against a fake ExternalTime /
# System / Log, together with the REAL boot-storm qualification, count and trip
# blocks, so the decisions are executed rather than pattern-matched.
#
# The fake models the Device OS 6.6.0 contract: disable() returns 0 even when the
# delete failed (hal/shared/exrtc_hal.cpp hal_exrtc_unbind), so only a re-read of
# status().bound() shows whether the binding is gone.
# Pins: bound -> disable() once, one warning, reset with data 8 only when the
# re-read shows it unbound; disable() ok but binding survives -> no reset; second
# boot (reason 140 / data 8) still bound -> no reset, survived message; unbound
# -> no call, no reset; disable() error -> logged, no reset; the next
# boot after reason 140 / data 8 does not count toward the boot storm (even with
# bootInProgress left set) and cannot trip the guard or raise its alert.
# Mutations (applied to copies in TMPDIR) must each fail: guard after
# ab1805.setup(), disable() unconditional, guard removed, reset when r != 0, a
# different reset cause, warning removed, an error path that returns from setup(),
# the re-check dropped, the "caused by this guard" condition dropped, the survived
# message removed.
set -euo pipefail

repo_root="${0:A:h:h}"
work="${TMPDIR:-/tmp}/binding_delete_guard_test"
rm -rf "$work"
mkdir -p "$work"

extract_braced_block() {
  local file="$1" marker="$2" nth="${3:-1}"
  awk -v marker="$marker" -v nth="$nth" '
    !active && index($0, marker) { if (++hits == nth) active = 1 }
    active {
      print
      opens = gsub(/\{/, "{")
      closes = gsub(/\}/, "}")
      depth += opens - closes
      if (seen_open && depth == 0) exit
      if (opens > 0) seen_open = 1
    }
  ' "$file"
}

build_and_run() {
  local app_src="$1" tag="$2"
  local generated="$work/${tag}.cpp" binary="$work/${tag}"

  python3 "$repo_root/tests/binding_delete_guard_order_test.py" "$app_src" >/dev/null || return 1

  local guard qual count trip
  guard=$(extract_braced_block "$app_src" "if (ExternalTime.status().bound()")
  qual=$(extract_braced_block "$app_src" "switch (reason) {" 1)
  count=$(extract_braced_block "$app_src" "if (previousBootFailedEarly && qualifiesForStormCount) {" 2)
  trip=$(extract_braced_block "$app_src" "if (bootStormCount >= 6 && previousBootFailedEarly && qualifiesForStormCount) {")
  if [[ -z "$guard" || -z "$qual" || -z "$count" || -z "$trip" ]]; then
    echo "EXTRACTION FAILED in $app_src" >&2
    return 1
  fi
  if [[ "$qual" != *qualifiesForStormCount* ]]; then
    echo "EXTRACTION FAILED: first switch (reason) is not the boot-storm qualification" >&2
    return 1
  fi

  {
    cat <<'CPP'
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <vector>

#include "ResetCause.h"   // the REAL cause codes

#define RESET_REASON_NONE 0
#define RESET_REASON_PANIC 130
#define RESET_REASON_WATCHDOG 10
#define RESET_REASON_UNKNOWN 20
#define RESET_REASON_USER 140
#define RESET_REASON_USER_APPLICATION 141
#define RESET_REASON_POWER_MANAGEMENT 142

std::vector<uint32_t> resets;
int warnings = 0;
int disableCalls = 0;
int disableResult = 0;
bool boundState = false;
bool deleteWorks = true;
int fakeResetReason = 0;
uint32_t fakeResetData = 0;

enum class SystemSleepMode { ULTRA_LOW_POWER };
struct SystemSleepConfiguration {
  SystemSleepConfiguration &mode(SystemSleepMode) { return *this; }
  SystemSleepConfiguration &duration(unsigned long) { return *this; }
};
struct FakeSystem { void reset(uint32_t data) { resets.push_back(data); } int resetReason() { return fakeResetReason; } uint32_t resetReasonData() { return fakeResetData; } template <typename T> void sleep(const T &) {} };
FakeSystem System;

struct FakeLog {
  void info(const char *, ...) {}
  void error(const char *, ...) {}
  void warn(const char *, ...) { warnings++; }
};
FakeLog Log;

struct FakeStatus { bool b; bool bound() const { return b; } };
struct FakeExternalTime {
  FakeStatus status() const { return FakeStatus{boundState}; }
  int disable() { disableCalls++; if (deleteWorks) boundState = false; return disableResult; }
};
FakeExternalTime ExternalTime;

// Boot-storm trip surface
struct FakeParticle { void process() {} };
FakeParticle Particle;
namespace Connectivity { inline void requestFullDisconnectAndRadioOff() {} }
inline void drainSerialBeforeSleep() {}

uint8_t bootStormCount = 0, bootStormTripCount = 0;
bool bootStormAlertPending = false, bootInProgress = false;

// Returns true when control reaches the end of the guard (setup() would continue).
bool reachedEnd = false;
void guardAsSetupVoid() {
CPP
    echo "// ===== Extracted verbatim from src/Generalized-Core-Counter.cpp ====="
    echo "$guard"
    cat <<'CPP'
  reachedEnd = true;
}
bool guardAsSetup() { reachedEnd = false; guardAsSetupVoid(); return reachedEnd; }

void stormBoot(int reason, uint32_t reasonData) {
  (void)reasonData;
  const bool previousBootFailedEarly = bootInProgress;
  bootInProgress = true;
  bool qualifiesForStormCount = false;
CPP
    echo "$qual"
    echo "$count"
    echo "$trip"
    cat <<'CPP'
}

static void fail(const char *msg) { std::cerr << "FAIL: " << msg << std::endl; std::exit(1); }

static void world(bool bound, int r, bool works = true, int reason = RESET_REASON_NONE, uint32_t data = 0) {
  resets.clear(); warnings = 0; disableCalls = 0; boundState = bound; disableResult = r;
  deleteWorks = works; fakeResetReason = reason; fakeResetData = data;
}

int main() {
  // bound, disable() succeeds: one delete, one warning, reset with data 8, once.
  world(true, 0);
  guardAsSetup();
  if (disableCalls != 1) fail("bound: disable() not called exactly once");
  if (warnings != 1) fail("bound: expected exactly one warning");
  if (resets.size() != 1 || resets[0] != 8 || resets[0] != (uint32_t)RESET_CAUSE_EXRTC_BINDING_DELETE)
    fail("bound: expected one reset with RESET_CAUSE_EXRTC_BINDING_DELETE (8)");

  // disable() returns 0 but the binding survives: no reset, boot continues.
  world(true, 0, false);
  if (!guardAsSetup()) fail("survive: setup() did not continue");
  if (!resets.empty()) fail("survive: reset although the binding is still bound");
  if (warnings != 1) fail("survive: expected only the delete warning on a first boot");

  // second boot caused by this guard, still bound: no reset, survived message logged.
  world(true, 0, false, RESET_REASON_USER, (uint32_t)RESET_CAUSE_EXRTC_BINDING_DELETE);
  if (!guardAsSetup()) fail("second boot: setup() did not continue");
  if (!resets.empty()) fail("second boot: reset again (unbounded loop)");
  if (warnings != 2) fail("second boot: the survived message was not logged");

  // second boot caused by this guard, delete works this time: never a second reset.
  world(true, 0, true, RESET_REASON_USER, (uint32_t)RESET_CAUSE_EXRTC_BINDING_DELETE);
  if (!guardAsSetup() || !resets.empty()) fail("second boot, delete worked: reset again");

  // reason 140 with other data is not this guard: a worked delete still resets once.
  world(true, 0, true, RESET_REASON_USER, 1);
  guardAsSetup();
  if (resets.size() != 1) fail("other user reset: expected one reset");

  // disable() error and the binding survives: no reset either.
  world(true, -1, false);
  if (!guardAsSetup() || !resets.empty()) fail("error+survive: reset or setup() did not continue");

  // unbound: nothing.
  world(false, 0);
  if (!guardAsSetup()) fail("unbound: setup() did not continue");
  if (disableCalls != 0 || warnings != 0 || !resets.empty()) fail("unbound: the guard acted");

  // disable() error: logged, boot continues, never a reset.
  world(true, -1);
  if (!guardAsSetup()) fail("error: setup() did not continue");
  if (disableCalls != 1) fail("error: disable() not called once");
  if (warnings != 1) fail("error: the failure was not logged");
  if (!resets.empty()) fail("error: reset on a failed delete");

  // Boot storm: the next boot after reason 140 / data 8 does not count, even
  // with bootInProgress left set and the count one below the trip threshold.
  bootStormCount = 5; bootStormTripCount = 0; bootStormAlertPending = false; bootInProgress = true;
  stormBoot(RESET_REASON_USER, (uint32_t)RESET_CAUSE_EXRTC_BINDING_DELETE);
  if (bootStormCount != 5) fail("reason 140 / data 8 incremented bootStormCount");
  if (bootStormTripCount != 0 || bootStormAlertPending) fail("reason 140 / data 8 tripped the boot-storm guard");

  // Control: the same state with a qualifying reason does count and trip.
  bootStormCount = 5; bootStormTripCount = 0; bootStormAlertPending = false; bootInProgress = true;
  stormBoot(RESET_REASON_WATCHDOG, 0);
  if (bootStormCount != 6 || bootStormTripCount != 1 || !bootStormAlertPending)
    fail("control: a watchdog reset no longer counts/trips (the pin is not live)");

  std::cout << "binding_delete_guard_test: guard behavior and boot-storm exclusion pass" << std::endl;
  return 0;
}
CPP
  } > "$generated"

  if ! clang++ -std=c++17 -Wall -Wextra -I "$repo_root/src" "$generated" -o "$binary" 2>"$work/${tag}.build.log"; then
    cat "$work/${tag}.build.log" >&2
    return 1
  fi
  "$binary"
}

if ! build_and_run "$repo_root/src/Generalized-Core-Counter.cpp" shipped; then
  echo "FAILED: the shipped binding-delete guard does not pass" >&2
  exit 1
fi

make_mutant() {
  python3 - "$repo_root/src/Generalized-Core-Counter.cpp" "$work/mutant.cpp" "$1" <<'PY'
import re, sys
src, dst, kind = sys.argv[1:4]
t = open(src).read()
m = re.search(r"  if \(ExternalTime\.status\(\)\.bound\(\)\) \{\n.*?\n  \}\n", t, re.S)
guard = m.group(0)
def sub(old, new, text=t):
    assert old in text, "mutation setup failed: " + kind
    return text.replace(old, new, 1)
if kind == "after-ab1805-setup":
    u = t.replace(guard, "", 1)
    u = sub("ab1805.withFOUT(WKP).setup();", "ab1805.withFOUT(WKP).setup();\n" + guard, u)
elif kind == "unconditional":
    u = sub("if (ExternalTime.status().bound()) {", "if (ExternalTime.status().bound() || true) {")
elif kind == "removed":
    u = t.replace(guard, "", 1)
elif kind == "reset-on-error":
    u = sub("if (r == 0 && ", "if (r != 0 && ")
elif kind == "other-cause":
    u = sub("System.reset(RESET_CAUSE_EXRTC_BINDING_DELETE)", "System.reset(RESET_CAUSE_OUT_OF_MEMORY)")
elif kind == "no-warning":
    u = sub('    Log.warn("AB1805: stored Device OS binding deleted (r=%d)", r);\n', "")
elif kind == "error-returns":
    u = sub('binding survived the delete");\n', 'binding survived the delete");\n    if (r != 0) return;\n')
elif kind == "no-recheck":
    u = sub("r == 0 && !ExternalTime.status().bound() && !ours", "r == 0 && !ours")
elif kind == "no-ours":
    u = sub("&& !ours) System.reset", ") System.reset")
elif kind == "no-survived":
    u = sub('    else if (ours && ExternalTime.status().bound()) Log.warn("AB1805: binding survived the delete");\n', "")
open(dst, "w").write(u)
PY
}

for kind in after-ab1805-setup unconditional removed reset-on-error other-cause no-warning error-returns no-recheck no-ours no-survived; do
  make_mutant "$kind"
  if build_and_run "$work/mutant.cpp" "mutant-$kind" >/dev/null 2>&1; then
    echo "MUTATION NOT CAUGHT: $kind" >&2
    exit 1
  fi
  echo "Mutation caught: $kind"
done

rm -rf "$work"
echo "binding_delete_guard_test: full suite passed"

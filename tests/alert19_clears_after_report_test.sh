#!/bin/zsh
# WO-2026-10-06-001 - event alerts are reported once and then cleared, so none
# can hide other alerts: 19 (watchdog reset) and, from round 2, 42 (data
# ledger publish failure / OTA updates pending). 18 (state-machine thrash) is
# deliberately left sticky; see testThrashStaysSetAfterReport.
#
# Behavioural test: the REAL getAlertSeverity() and raiseAlert()
# (src/MyPersistentData.cpp), isAutoClearAfterReportAlert() and publishData()'s
# clear-after-report block (src/Generalized-Core-Counter.cpp) are extracted
# verbatim from the sources and driven against fakes.
set -euo pipefail

repo_root="${0:A:h:h}"
persistent_src="$repo_root/src/MyPersistentData.cpp"
app_src="$repo_root/src/Generalized-Core-Counter.cpp"
generated="${TMPDIR:-/tmp}/alert19_clears_after_report_test.cpp"
binary="${TMPDIR:-/tmp}/alert19_clears_after_report_test"

extract_braced_block() {
  local file="$1"
  local marker="$2"
  local out
  out=$(awk -v marker="$marker" '
    !active && index($0, marker) { active = 1 }
    active {
      print
      opens = gsub(/\{/, "{")
      closes = gsub(/\}/, "}")
      depth += opens - closes
      if (seen_open && depth == 0) exit
      if (opens > 0) seen_open = 1
    }
  ' "$file")
  if [[ -z "$out" ]]; then
    print -u2 "EXTRACT_FAILED: $file has no block starting with: $marker"
    exit 1
  fi
  print -r -- "$out"
}

{
  cat <<'CPP'
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <ctime>

// ----- Fakes ------------------------------------------------------------------
struct FakeTime { time_t now() const { return 1791200000; } };
FakeTime Time;
struct FakeLog { void info(const char *, ...) {} };
FakeLog Log;

static int8_t g_alertCode = 0;
static time_t g_lastAlertTime = 0;
int8_t get_alertCode() { return g_alertCode; }
void set_alertCode(int8_t v) { g_alertCode = v; }
void set_lastAlertTime(time_t v) { g_lastAlertTime = v; }

namespace RecoveryState {
int8_t get_alertCode() { return g_alertCode; }
void set_alertCode(int8_t v) { g_alertCode = v; }
void set_lastAlertTime(time_t v) { g_lastAlertTime = v; }
}  // namespace RecoveryState

namespace SystemConfig {
bool get_verboseMode() { return false; }
}  // namespace SystemConfig
CPP

  echo ""
  echo "// ===== Extracted verbatim from src/MyPersistentData.cpp ====="
  extract_braced_block "$persistent_src" "static int getAlertSeverity(int8_t code) {"
  echo ""
  # A member function in the source; renamed to a free function here only.
  extract_braced_block "$persistent_src" "void currentStatusData::raiseAlert(int8_t value) {" \
    | sed '1s/currentStatusData::raiseAlert/raiseAlert/'

  echo ""
  echo "// ===== Extracted verbatim from src/Generalized-Core-Counter.cpp ====="
  extract_braced_block "$app_src" "static bool isAutoClearAfterReportAlert(int alertCode) {"
  echo ""
  echo "// publishData()'s alert handling, around its extracted clear block."
  echo "int publishReport(bool queued) {"
  echo "  const int8_t reportedAlertCode = RecoveryState::get_alertCode();  // the payload's \"alerts\""
  extract_braced_block "$app_src" "  if (queued &&"
  echo "  return reportedAlertCode;"
  echo "}"

  cat <<'CPP'

// ----- Tests --------------------------------------------------------------------
void reset() {
  g_alertCode = 0;
  g_lastAlertTime = 0;
}

// The WO's scenario: a watchdog reboot raises 19; the first report carries it
// and clears it; a later alert 40 is then visible (before, 19 masked it).
void testWatchdogReportedOnceThenLaterAlertVisible() {
  reset();
  raiseAlert(19);                       // setup() after a watchdog reboot
  assert(g_alertCode == 19);
  assert(publishReport(true) == 19);    // first report carries it
  assert(g_alertCode == 0);             // ...and clears it
  assert(g_lastAlertTime == 0);
  raiseAlert(40);                       // repeated webhook failures, later
  assert(g_alertCode == 40);
  assert(publishReport(true) == 40);    // now visible
  assert(publishReport(true) == 40);    // 40 isn't auto-clear: stays until its own clear
  printf("PASS: testWatchdogReportedOnceThenLaterAlertVisible\n");
}

// A report the queue rejects must not clear 19; the next report still carries it.
void testUnqueuedReportDoesNotClear() {
  reset();
  raiseAlert(19);
  assert(publishReport(false) == 19);
  assert(g_alertCode == 19);
  assert(publishReport(true) == 19);
  assert(g_alertCode == 0);
  printf("PASS: testUnqueuedReportDoesNotClear\n");
}

// While 19 is set, it still outranks other alerts (severity is unchanged).
void testNineteenStillOutranksWhileSet() {
  reset();
  raiseAlert(41);
  raiseAlert(19);
  assert(g_alertCode == 19);
  raiseAlert(15);
  assert(g_alertCode == 19);
  printf("PASS: testNineteenStillOutranksWhileSet\n");
}

// Alert 18 (state-machine thrash) stays set after a report: it is NOT in the
// clear list. Round 2's Stage 7 found that clearing it can lose a tier-3 18:
// ThrashGuard raises 18 and resets at once (ThrashGuard.cpp:150-152), before
// the 250 ms deferred save (MyPersistentData.cpp:681). Chip dropped 18 from
// v38 (2026-10-06); this guard fails if 18 is re-added without that fix.
void testThrashStaysSetAfterReport() {
  reset();
  raiseAlert(18);
  assert(publishReport(true) == 18);
  assert(g_alertCode == 18);            // still set
  assert(publishReport(true) == 18);
  printf("PASS: testThrashStaysSetAfterReport\n");
}

// Round 2: alert 42 (data ledger publish failure, or OTA updates pending at the
// sleep gate; severity 2) is an event. Reported once, then cleared, and a later
// lower alert (44, severity 1) is visible.
void testLedgerFailureReportedOnceThenLaterAlertVisible() {
  reset();
  raiseAlert(42);
  raiseAlert(44);                       // masked while 42 is set
  assert(g_alertCode == 42);
  assert(publishReport(true) == 42);
  assert(g_alertCode == 0);
  raiseAlert(44);
  assert(publishReport(true) == 44);
  printf("PASS: testLedgerFailureReportedOnceThenLaterAlertVisible\n");
}

int main() {
  testWatchdogReportedOnceThenLaterAlertVisible();
  testUnqueuedReportDoesNotClear();
  testNineteenStillOutranksWhileSet();
  testThrashStaysSetAfterReport();
  testLedgerFailureReportedOnceThenLaterAlertVisible();
  printf("Event alerts clear after report test passed\n");
  return 0;
}
CPP
} > "$generated"

clang++ -std=c++17 -Wall -Wextra -pedantic -Wno-unused-function \
  "$generated" \
  -o "$binary"

"$binary"

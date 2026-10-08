// WO-2026-10-08-001 item B: Report's "already connected" branch refreshes
// lastConnection. Part of tests/connected_online_last_connection_test.sh.
//
// The branch body is extracted verbatim from src/state/State_Report.cpp and
// compiled here against a stub Time / SystemConfig / transitionTo, so a change
// to the real branch changes what this file runs.

#include <cstdint>
#include <cstdio>
#include <ctime>

namespace {

int failures = 0;

time_t clockNow = 0;
time_t storedLastConnection = 0;
int idleTransitions = 0;

struct TimeStub {
  time_t now() const { return clockNow; }
} Time;

namespace SystemConfig {
void set_lastConnection(time_t value) { storedLastConnection = value; }
} // namespace SystemConfig

enum State { IDLE_STATE };
void transitionTo(State, const char *reason) {
  (void)reason;
  idleTransitions++;
}

// The real branch: the lines from "// An online device ..." to the transition.
void alreadyConnectedBranch() {
#include "already_connected_branch.inc"
}

// Mirrors the supervisor's age: from the later of lastConnection and opening.
constexpr time_t STALE = REAL_STALE_SEC;
bool failsafeWouldAct(time_t now, time_t lastConnection, time_t openedAt) {
  const time_t ageBase = (openedAt > lastConnection) ? openedAt : lastConnection;
  return now > ageBase && (now - ageBase) >= STALE;
}

void check(const char *name, bool ok) {
  printf("  %s %s\n", ok ? "ok  " : "FAIL", name);
  if (!ok) {
    failures++;
  }
}

} // namespace

int main() {
  constexpr time_t H = 3600;
  const time_t openedAt = 1000000;

  clockNow = openedAt + 5;
  storedLastConnection = openedAt - 20 * H;
  alreadyConnectedBranch();
  check("online device through 'already connected' gets lastConnection == now",
        storedLastConnection == clockNow);
  check("the branch still transitions to IDLE", idleTransitions == 1);

  // An open day of hourly reports, device online throughout (CONNECTED mode).
  storedLastConnection = openedAt - 20 * H;
  bool everActed = false;
  for (int hour = 0; hour <= 16; ++hour) {
    clockNow = openedAt + hour * H + 5;
    // The supervisor runs every loop pass; sample just before each report.
    for (time_t t = clockNow - H + 10; t < clockNow && hour > 0; t += 600) {
      if (failsafeWouldAct(t, storedLastConnection, openedAt)) {
        everActed = true;
      }
    }
    alreadyConnectedBranch();
  }
  check("an open day of hourly reports never reaches the failsafe threshold (no stage-2 reset)",
        !everActed);

  // Control: the same day without the refresh (the pre-fix behaviour) acts.
  storedLastConnection = openedAt - 20 * H;
  check("control: without the refresh the same device crosses the threshold",
        failsafeWouldAct(openedAt + 3 * H, storedLastConnection, openedAt));

  if (failures != 0) {
    printf("\nFAILED: %d check(s)\n", failures);
    return 1;
  }
  printf("\nconnected-online host test passed\n");
  return 0;
}

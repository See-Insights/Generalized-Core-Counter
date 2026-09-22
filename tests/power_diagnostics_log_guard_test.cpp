// WO-2026-09-15-001 Amendment C: host regression test for
// PowerDiagnostics::logPowerState()'s change-detection guard.
//
// Compiles the real src/power/PowerDiagnostics.cpp (and PowerManager.cpp,
// for the PowerManager::instance() calls inside logPowerState()) against
// the lightweight host stubs under tests/stubs/diag_overrides/. The stub
// PowerPlatform::readPowerSource() always returns a fixed snapshot, so
// powerSource/profile are constant across calls in this harness - soc
// (test-controlled via testCurrent.socValue) and `reason` are the two
// fields this test varies, which is sufficient to exercise the guard:
// unchanged (reason, soc) must suppress, a change in either must not, and
// forceLog must bypass suppression regardless.
//
// TestLog::infoCallCount (tests/stubs/diag_overrides/Particle.h) is the
// real signal checked here - not the "pdiag" batch, which
// power_diagnostics_batch_test.cpp already covers and which this fix
// deliberately leaves unconditional (see logPowerState()'s own comment).

#include "power/PowerDiagnostics.h"

#include <cassert>
#include <cstdio>

#include "MyPersistentData.h"
#include "Particle.h"

TestCurrentStatus testCurrent;
TestSystemStatus testSysStatus;

namespace {

void resetLog() {
  Log.infoCallCount = 0;
}

} // namespace

int main() {
  // First call ever: nothing to compare against, must log.
  testCurrent.socValue = 80.0f;
  resetLog();
  PowerDiagnostics::logPowerState("setup");
  assert(Log.infoCallCount == 1);

  // Repeated identical (reason, soc): must suppress.
  resetLog();
  PowerDiagnostics::logPowerState("setup");
  assert(Log.infoCallCount == 0);

  // Different reason, same soc: must log once - a different lifecycle
  // checkpoint is worth its own record even if values happen to match.
  resetLog();
  PowerDiagnostics::logPowerState("post-refreshInputProfile");
  assert(Log.infoCallCount == 1);

  // That same reason, repeated several times with soc still unchanged:
  // this is the exact WO-2026-09-15-001 shape (the real serial evidence
  // this WO documents is several consecutive "post-refreshInputProfile"
  // calls with unchanged values) - all suppressed after the first.
  resetLog();
  PowerDiagnostics::logPowerState("post-refreshInputProfile");
  PowerDiagnostics::logPowerState("post-refreshInputProfile");
  PowerDiagnostics::logPowerState("post-refreshInputProfile");
  assert(Log.infoCallCount == 0);

  // A genuine value change (soc) with the SAME reason as last logged must
  // still surface - no diagnostic value lost.
  testCurrent.socValue = 79.9f; // crosses the %.1f-rounded display value
  resetLog();
  PowerDiagnostics::logPowerState("post-refreshInputProfile");
  assert(Log.infoCallCount == 1);

  // Sub-0.05% float noise that would print identically must NOT count as a
  // change (rounds to the same displayed 79.9%), same reason.
  testCurrent.socValue = 79.904f;
  resetLog();
  PowerDiagnostics::logPowerState("post-refreshInputProfile");
  assert(Log.infoCallCount == 0);

  // forceLog bypasses suppression even with (reason, soc) unchanged from
  // the last logged call - this is what gives the 7 existing call sites
  // that pass forceLog=true (setup, both profile-change sites) real teeth.
  resetLog();
  PowerDiagnostics::logPowerState("post-refreshInputProfile", /*forceLog=*/true);
  assert(Log.infoCallCount == 1);

  // Immediately after a forced log with unchanged (reason, soc), a normal
  // (non-forced) call with the same pair is suppressed again - forceLog
  // does not permanently disable the guard, it only bypasses it once.
  resetLog();
  PowerDiagnostics::logPowerState("post-refreshInputProfile");
  assert(Log.infoCallCount == 0);

  printf("power_diagnostics_log_guard_test: all assertions passed\n");
  return 0;
}

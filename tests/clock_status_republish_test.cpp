// Host test for the Round-5 cleanup task 3 / Stage 7 finding 3 deferred
// status-republish fix, extended per Round 6 follow-up (Stage 7 finding 2).
//
// Stage 7 finding 2 (verbatim): "The production implementation is correct
// ... However, the test only checks that checkClockResync() calls
// requestStatusPublish(). Turning Cloud::requestStatusPublish() into a
// no-op would leave all new tests and the firmware build passing while
// reintroducing the late-sync telemetry defect."
//
// The prior round's tests/clock_resync_wiring_test.py check (still present,
// unmodified) only proves checkClockResync() CALLS
// Cloud::instance().requestStatusPublish(...) - a call-site/shape check.
// This file instead proves the BEHAVIOUR end to end using the REAL
// production functions, not a mirror of them:
//
//     confirmed sync -> Cloud::requestStatusPublish() sets the pending flag
//       -> Cloud::loop() drains it while Particle.connected()
//       -> Cloud::writeDeviceStatusToCloud() is actually invoked
//       -> the JSON payload it writes carries clock.trusted == true
//
// It does this by compiling and linking the REAL, unmodified
// src/cloud/Cloud.cpp (for Cloud::requestStatusPublish()/Cloud::loop()'s
// pendingStatusPublish drain - NEITHER of which the power-source-override
// harness links; that harness stubs the entire Cloud class out via
// cloud/CloudTestShim.cpp) together with the REAL, unmodified
// src/cloud/DeviceStatusPublisher.cpp (for writeDeviceStatusToCloud()'s
// actual JSON-writing code, exactly as the power-source-override harness
// already established is host-testable) and the REAL src/power/
// PowerManager.cpp (a dependency of DeviceStatusPublisher.cpp). See
// tests/stubs/clock_status_republish_overrides/ for the supporting
// (non-production-logic) scaffolding this requires, and
// tests/clock_status_republish_test.sh for the exact build.
//
// A no-op Cloud::requestStatusPublish() (the exact mutation Stage 7 asked
// to be provable) breaks the THIRD link above: pendingStatusPublish never
// gets set, so Cloud::loop() never calls writeDeviceStatusToCloud(), so
// the JSON observer below never records a "trusted" field at all - see
// testConfirmedSyncPublishesTrustedTrueEndToEnd()'s assertions, and the
// mutation-test evidence in this round's Implementation Report.

#include "cloud/Cloud.h"
#include "MyPersistentData.h"
#include "Particle.h"
#include "power/PowerPlatform.h"

#include <cassert>
#include <cstdio>
#include <string>

// --- Globals required by the real Cloud.cpp/DeviceStatusPublisher.cpp/ ---
// --- PowerManager.cpp translation units, following the exact pattern   ---
// --- already established by tests/power_source_override_test.cpp.     ---
TestCurrentStatus testCurrent;
TestSystemStatus testSysStatus;
TestSensorConfig testSensorConfig;

TestNrfUsbdRegs testNrfUsbdRegs;
TestNrfPowerRegs testNrfPowerRegs;

PowerPlatform::TestPowerPlatformState PowerPlatform::testPowerPlatformState;

// Defined in CloudLinkStubs.cpp; test-controlled input to that file's
// isClockTrusted()/observedTimeSyncedLastMs()/reportedSyncAgeMs() stubs.
extern bool testClockTrusted;

// Populated by the JSONBufferWriter stand-in in Particle.h so this test can
// observe which value the real, unmodified src/cloud/DeviceStatusPublisher.cpp
// actually wrote for a given JSON field name - here, clock.trusted.
JSONFieldObserver g_statusJsonObserver;

// Round 6 follow-up (Stage 7 finding 2, second pass): records what the
// real writeDeviceStatusToCloud() actually handed to
// deviceStatusLedger.set() - i.e. proof the ledger write itself happened,
// not just that a JSON payload was composed. See Particle.h's Ledger::set().
LedgerSetObserver g_ledgerSetObserver;

// Test-controlled connectivity flag (see Particle.h in this override dir).
bool testParticleConnected = false;

namespace {

void resetHarness() {
  g_statusJsonObserver.reset();
  g_ledgerSetObserver.reset();
  testParticleConnected = false;
  testClockTrusted = false; // declared extern in cloud/Cloud.h via CloudLinkStubs.cpp
}

// Negative control: without ever calling Cloud::requestStatusPublish(), a
// fresh Cloud singleton's pendingStatusPublish starts false (see the real
// Cloud::Cloud() constructor), so Cloud::loop() must drain nothing even
// once connected - proving the assertion in the positive test below isn't
// trivially true regardless of what loop() does.
void testNoRequestMeansLoopPublishesNothing() {
  resetHarness();
  testParticleConnected = true;

  Cloud::instance().loop();

  bool trusted = false;
  assert(g_statusJsonObserver.findBool("trusted", &trusted) == false);
  assert(g_ledgerSetObserver.callCount == 0);
}

// The behaviour Stage 7 finding 2 requires be provable: a confirmed sync
// (modeled here as isClockTrusted() becoming true - see CloudLinkStubs.cpp)
// drives requestStatusPublish(), which Cloud::loop() must drain into an
// ACTUAL call to the real writeDeviceStatusToCloud(), whose real JSON
// writer code (in DeviceStatusPublisher.cpp) must observe isClockTrusted(),
// write clock.trusted == true, and - the specific gap Stage 7's second
// pass flagged - the resulting payload must actually reach
// deviceStatusLedger.set() (recorded by g_ledgerSetObserver, via
// Ledger::set() in Particle.h), not merely be composed in memory.
//
// A no-op Cloud::requestStatusPublish() breaks this: pendingStatusPublish
// never becomes true, so loop() never calls writeDeviceStatusToCloud() at
// all (M1). Forcing the payload to trusted=false breaks the boolValue
// assertion (M2). Removing deviceStatusLedger.set(data) from
// DeviceStatusPublisher.cpp breaks the g_ledgerSetObserver assertions below
// (M3) - this is the specific case the previous round's version of this
// test could not catch, because dataSize() always returned 0 and
// bufferBase == "" == the initial lastPublishedStatus, so duplicate
// suppression returned early before set() was ever reached, REGARDLESS of
// whether set() had been deleted from production. See this round's
// Implementation Report for the mutation-test transcript proving all
// three now fail correctly.
void testConfirmedSyncPublishesTrustedTrueEndToEnd() {
  resetHarness();
  testClockTrusted = true; // "confirmed sync advance -> successful RTC write"

  Cloud::instance().requestStatusPublish("ClockResync");

  // Not connected yet: loop() must not drain (mirrors the real
  // `pendingStatusPublish && Particle.connected()` gate) - proves the drain
  // genuinely depends on connectivity, not just the pending flag.
  testParticleConnected = false;
  Cloud::instance().loop();
  bool trustedBeforeConnect = false;
  assert(g_statusJsonObserver.findBool("trusted", &trustedBeforeConnect) == false);
  assert(g_ledgerSetObserver.callCount == 0);

  // Now connected: the deferred drain must fire and the real publisher
  // code must have observed isClockTrusted() == true.
  testParticleConnected = true;
  Cloud::instance().loop();

  bool trusted = false;
  const bool found = g_statusJsonObserver.findBool("trusted", &trusted);
  assert(found);
  assert(trusted == true);

  // Assert on what the ledger actually RECEIVED, not merely on what was
  // composed: the real deviceStatusLedger.set(data) call must have run
  // exactly once, and the exact payload it received must carry the
  // trusted=true field this scenario set up - i.e. the same value the
  // JSON-field observer above saw must independently show up in the
  // literal bytes handed to Ledger::set().
  assert(g_ledgerSetObserver.callCount == 1);
  assert(g_ledgerSetObserver.lastPayload.find("trusted=true;") != std::string::npos);

  // Exactly-once semantics, continued directly from the publish above (not
  // a fresh resetHarness() scenario): Cloud is a process-wide singleton
  // (Cloud::instance()), so its private lastPublishedStatus persists
  // across calls within this test binary. Testing "does loop() repeat
  // itself" as a standalone scenario after resetHarness() would only reset
  // this test's OWN observers, not that persisted singleton state, and
  // would therefore spuriously depend on whatever the previous scenario
  // last published rather than on this scenario's own drain - so it is
  // deliberately kept as a direct continuation here: nothing new was
  // requested and nothing in the composed payload changed, so a second
  // loop() call must not reach deviceStatusLedger.set() again. This proves
  // writeDeviceStatusToCloud()'s success path actually clears
  // pendingStatusPublish (Cloud.cpp:694), not just that SOME publish
  // happened once.
  g_statusJsonObserver.reset();
  Cloud::instance().loop(); // nothing newly requested
  bool trustedAfterRepeat = false;
  assert(g_statusJsonObserver.findBool("trusted", &trustedAfterRepeat) == false);
  assert(g_ledgerSetObserver.callCount == 1); // still 1 - no repeat set()
}

} // namespace

int main() {
  testNoRequestMeansLoopPublishesNothing();
  testConfirmedSyncPublishesTrustedTrueEndToEnd();

  printf("clock_status_republish_test: all assertions passed\n");
  return 0;
}

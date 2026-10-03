// WO-2026-10-03-001 (v35-LedgerNoRetry) behavioral host test.
//
// Goal under test, in plain language: a ledger write that's already waiting
// is never retried; the latest content is written once, after the previous
// write completes.
//
// Before this WO, Cloud::loop() called writeDeviceStatusToCloud() on EVERY
// pass while pendingStatusPublish was set. While an earlier write was in
// flight, noteLedgerSyncRequest() refused each one, so every pass rebuilt
// the whole status JSON, heap-allocated a LedgerData, logged, and threw it
// all away - about 700 LedgerDuplicateStillInflight warnings in 16 s on
// Dev-09. Refused DATA writes, and refused STATUS writes made directly by
// ConnectState, were dropped outright.
//
// This test links the REAL, unmodified src/cloud/Cloud.cpp and
// src/cloud/DeviceStatusPublisher.cpp (same harness shape as
// tests/clock_status_republish_test.cpp, whose stub directory it reuses for
// everything except Particle.h - see tests/stubs/ledger_no_retry_overrides/
// and tests/ledger_no_retry_test.sh) and proves, against that real code:
//
//   A. The wait is silent and free. With a STATUS write in flight and a
//      republish requested, ~13,600 Cloud::loop() passes across a simulated
//      300 s produce ZERO Ledger::set() calls, ZERO JSON payload builds,
//      ZERO log lines and ZERO heap allocations (counting global
//      operator new). On the first pass after the sync completes there is
//      EXACTLY ONE write, carrying the latest content (the payload is
//      rebuilt at write time, so a clock-trust change made during the wait
//      is in it).
//   B. Nothing is dropped. A refused DATA write (ReportState's shape) is
//      written after completion rather than silently discarded, and so is a
//      refused direct STATUS write (ConnectState's shape).
//   C. One deferred operation per pass. With both a STATUS and a DATA write
//      deferred and neither ledger in flight, a single loop() pass issues
//      exactly one of them; the other waits for the next pass.
//
// Mutations this is designed to catch (transcripts in the Implementation
// Report):
//   M1 removing the in-flight gate from Cloud::loop()'s status drain;
//   M2 removing the one-operation-per-pass `return;` after that drain.

#include "cloud/Cloud.h"
#include "MyPersistentData.h"
#include "Particle.h"
#include "power/PowerPlatform.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <new>
#include <string>

// --- Counting allocator -------------------------------------------------
// Global operator new/delete replacements so the test can assert that a
// loop() pass during the wait allocates nothing at all. This is the direct
// check on the per-retry `LedgerData data = LedgerData::fromJSON(...)` heap
// churn the WO is about.
namespace {
unsigned long g_allocCount = 0;
} // namespace

void *operator new(size_t size) {
  ++g_allocCount;
  void *ptr = std::malloc(size ? size : 1);
  if (!ptr) {
    throw std::bad_alloc();
  }
  return ptr;
}

void *operator new[](size_t size) { return ::operator new(size); }
void operator delete(void *ptr) noexcept { std::free(ptr); }
void operator delete[](void *ptr) noexcept { std::free(ptr); }
void operator delete(void *ptr, size_t) noexcept { std::free(ptr); }
void operator delete[](void *ptr, size_t) noexcept { std::free(ptr); }

// --- Globals required by the real translation units ---------------------
// Same pattern as tests/clock_status_republish_test.cpp.
TestCurrentStatus testCurrent;
TestSystemStatus testSysStatus;
TestSensorConfig testSensorConfig;

TestNrfUsbdRegs testNrfUsbdRegs;
TestNrfPowerRegs testNrfPowerRegs;

PowerPlatform::TestPowerPlatformState PowerPlatform::testPowerPlatformState;

extern bool testClockTrusted; // CloudLinkStubs.cpp

JSONFieldObserver g_statusJsonObserver;
LedgerSetObserver g_ledgerSetObserver;

bool testParticleConnected = false;

// Declared in this harness's Particle.h.
bool g_simulateLedgerSyncLatency = false;
Ledger *g_ledgerRegistry[8] = {};
size_t g_ledgerRegistryCount = 0;
unsigned long g_testMillis = 1000UL;

namespace {

constexpr unsigned long kLoopIntervalMs = 22UL;   // Dev-09's observed retry cadence
constexpr unsigned long kInflightWaitMs = 300000UL; // the observed ~300 s callback latency

bool payloadIsDeviceData(const std::string &payload) {
  // Only the device-data payload carries the occupancy object.
  return payload.find("totalOccupiedSec=") != std::string::npos;
}

bool payloadIsDeviceStatus(const std::string &payload) {
  // Only the device-status payload carries the clock-trust field.
  return payload.find("trusted=") != std::string::npos;
}

// Mirrors what the real sync callbacks in LedgerClient.cpp do when the
// cloud acknowledges a write: the ledger's lastSynced() catches up with its
// lastUpdated(), and Cloud's request tracker retires the entry. The test
// performs this from normal (non-callback) context on purpose - production
// must never WRITE from the callback, and nothing here does.
void completeLedgerSync(void *ledger, Cloud::LedgerRequestKind kind) {
  assert(ledger != nullptr);
  static_cast<Ledger *>(ledger)->testCompleteSync();
  const Cloud::LedgerSyncDiagnostics before = Cloud::instance().ledgerSyncDiagnostics();
  const Cloud::LedgerSyncDiagnostics after = before;
  Cloud::instance().noteLedgerSyncComplete(kind, 0, 0, before, after);
}

void advanceAndLoop() {
  g_testMillis += kLoopIntervalMs;
  Cloud::instance().loop();
}

// --- A: the wait is silent and free, then exactly one write -------------
//
// Returns the device-status Ledger object, so the later scenarios can
// complete its sync.
void *testInflightWaitIsSilentThenWritesLatestContentOnce() {
  g_simulateLedgerSyncLatency = true;
  testParticleConnected = true;
  testClockTrusted = false;

  // The in-flight write: ConnectState's STATUS publish (State_Connect.cpp).
  g_ledgerSetObserver.reset();
  g_statusJsonObserver.reset();
  const bool firstWrite = Cloud::instance().writeDeviceStatusToCloud("ConnectState");
  assert(firstWrite);
  assert(g_ledgerSetObserver.callCount == 1);
  assert(payloadIsDeviceStatus(g_ledgerSetObserver.lastPayload));
  void *statusLedger = g_ledgerSetObserver.lastLedger;
  assert(static_cast<Ledger *>(statusLedger)->testIsInFlight());

  // The republish request that produced the flood: one call, from the
  // clock's confirmed resync (Clock.cpp).
  Cloud::instance().requestStatusPublish("ClockResync");
  assert(Cloud::instance().ledgerSyncDiagnostics().pendingStatusPublish);

  // The content changes DURING the wait. Because the payload is rebuilt at
  // write time (never cached), the single deferred write below must carry
  // this value, not the one that was current when the request was made.
  testClockTrusted = true;

  // The wait: ~13,600 passes over a simulated 300 s.
  g_statusJsonObserver.reset();
  g_logCounter.reset();
  const int setCallsBeforeWait = g_ledgerSetObserver.callCount;
  unsigned long passes = 0;
  const unsigned long waitEndMs = g_testMillis + kInflightWaitMs;
  while (g_testMillis < waitEndMs) {
    const unsigned long allocBefore = g_allocCount;
    advanceAndLoop();
    ++passes;
    // No heap allocation on ANY pass during the wait.
    assert(g_allocCount == allocBefore);
  }
  assert(passes > 10000UL);
  assert(g_ledgerSetObserver.callCount == setCallsBeforeWait); // zero writes
  assert(g_statusJsonObserver.count == 0);                     // zero payload builds
  assert(g_logCounter.lines == 0UL);                           // zero log lines
  assert(Cloud::instance().ledgerSyncDiagnostics().pendingStatusPublish);

  // The cloud finally acknowledges the original write.
  completeLedgerSync(statusLedger, Cloud::LEDGER_REQUEST_KIND_STATUS);
  assert(!static_cast<Ledger *>(statusLedger)->testIsInFlight());

  // First pass after completion: exactly one write, with the LATEST content.
  advanceAndLoop();
  assert(g_ledgerSetObserver.callCount == setCallsBeforeWait + 1);
  assert(payloadIsDeviceStatus(g_ledgerSetObserver.lastPayload));
  assert(g_ledgerSetObserver.lastPayload.find("trusted=true;") != std::string::npos);
  assert(!Cloud::instance().ledgerSyncDiagnostics().pendingStatusPublish);

  // ...and only one: the deferred flag is cleared, so further passes are
  // quiet even once this new write's sync lands.
  const int setCallsAfterDrain = g_ledgerSetObserver.callCount;
  completeLedgerSync(statusLedger, Cloud::LEDGER_REQUEST_KIND_STATUS);
  for (int i = 0; i < 50; ++i) {
    advanceAndLoop();
  }
  assert(g_ledgerSetObserver.callCount == setCallsAfterDrain);

  return statusLedger;
}

// --- B1: a refused DATA write is deferred, not dropped ------------------
//
// Returns the device-data Ledger object.
void *testRefusedDataWriteIsWrittenAfterCompletion() {
  // The in-flight DATA write (ConnectState's publishDataToLedger()).
  Cloud::instance().publishDataToLedger("ConnectState");
  assert(payloadIsDeviceData(g_ledgerSetObserver.lastPayload));
  void *dataLedger = g_ledgerSetObserver.lastLedger;
  assert(static_cast<Ledger *>(dataLedger)->testIsInFlight());

  const int setCallsBefore = g_ledgerSetObserver.callCount;

  // ReportState's write arrives while that one is still in flight. The
  // caller must still see success (ReportState and alert 42 unchanged),
  // but the write must NOT be dropped.
  const bool reportStateWrite = Cloud::instance().publishDataToLedger("ReportState");
  assert(reportStateWrite);
  assert(g_ledgerSetObserver.callCount == setCallsBefore); // refused, not written

  // It must not be retried while the earlier write is in flight, either.
  for (int i = 0; i < 100; ++i) {
    advanceAndLoop();
  }
  assert(g_ledgerSetObserver.callCount == setCallsBefore);

  // Once the earlier write completes, the deferred one is issued - once.
  completeLedgerSync(dataLedger, Cloud::LEDGER_REQUEST_KIND_DATA);
  advanceAndLoop();
  assert(g_ledgerSetObserver.callCount == setCallsBefore + 1);
  assert(payloadIsDeviceData(g_ledgerSetObserver.lastPayload));

  const int setCallsAfterDrain = g_ledgerSetObserver.callCount;
  completeLedgerSync(dataLedger, Cloud::LEDGER_REQUEST_KIND_DATA);
  for (int i = 0; i < 50; ++i) {
    advanceAndLoop();
  }
  assert(g_ledgerSetObserver.callCount == setCallsAfterDrain);

  return dataLedger;
}

// --- B2: a refused direct ConnectState STATUS write is deferred ---------
void testRefusedConnectStateStatusWriteIsWrittenAfterCompletion(void *statusLedger) {
  testClockTrusted = false; // make the next status payload differ

  // Put a STATUS write in flight (ClockResync's deferred republish path).
  Cloud::instance().requestStatusPublish("ClockResync");
  const int setCallsBeforeInflight = g_ledgerSetObserver.callCount;
  advanceAndLoop();
  assert(g_ledgerSetObserver.callCount == setCallsBeforeInflight + 1);
  assert(static_cast<Ledger *>(statusLedger)->testIsInFlight());

  // ConnectState now writes STATUS directly, as State_Connect.cpp does.
  // Before this WO that write returned false and was lost entirely: no
  // flag was set, so nothing ever republished it.
  testClockTrusted = true; // the content ConnectState wanted published
  const int setCallsBeforeRefusal = g_ledgerSetObserver.callCount;
  const bool connectWrite = Cloud::instance().writeDeviceStatusToCloud("ConnectState");
  assert(!connectWrite);
  assert(g_ledgerSetObserver.callCount == setCallsBeforeRefusal);
  assert(Cloud::instance().ledgerSyncDiagnostics().pendingStatusPublish);

  completeLedgerSync(statusLedger, Cloud::LEDGER_REQUEST_KIND_STATUS);
  advanceAndLoop();
  assert(g_ledgerSetObserver.callCount == setCallsBeforeRefusal + 1);
  assert(g_ledgerSetObserver.lastPayload.find("trusted=true;") != std::string::npos);

  completeLedgerSync(statusLedger, Cloud::LEDGER_REQUEST_KIND_STATUS);
}

// --- C: at most one deferred operation per loop() pass ------------------
void testOneDeferredOperationPerPass(void *statusLedger, void *dataLedger) {
  // Arrange a deferred DATA write: one in flight, one refused, then the
  // in-flight one completes - so pendingDataPublish is set with the data
  // ledger NOT in flight.
  Cloud::instance().publishDataToLedger("ConnectState");
  assert(static_cast<Ledger *>(dataLedger)->testIsInFlight());
  Cloud::instance().publishDataToLedger("ReportState"); // refused -> deferred
  completeLedgerSync(dataLedger, Cloud::LEDGER_REQUEST_KIND_DATA);
  assert(!static_cast<Ledger *>(dataLedger)->testIsInFlight());

  // ...and a deferred STATUS write, with the status ledger not in flight.
  testClockTrusted = false; // make the next status payload differ
  assert(!static_cast<Ledger *>(statusLedger)->testIsInFlight());
  Cloud::instance().requestStatusPublish("ClockResync");

  // Both deferred, neither ledger in flight: one pass, exactly one write,
  // and it is the STATUS one (the status drain comes first).
  const int setCallsBefore = g_ledgerSetObserver.callCount;
  advanceAndLoop();
  assert(g_ledgerSetObserver.callCount == setCallsBefore + 1);
  assert(payloadIsDeviceStatus(g_ledgerSetObserver.lastPayload));

  // The DATA write waits for a later pass - here, for the STATUS write it
  // yielded to to finish, since both drains share the same in-flight gate.
  completeLedgerSync(statusLedger, Cloud::LEDGER_REQUEST_KIND_STATUS);
  advanceAndLoop();
  assert(g_ledgerSetObserver.callCount == setCallsBefore + 2);
  assert(payloadIsDeviceData(g_ledgerSetObserver.lastPayload));

  completeLedgerSync(dataLedger, Cloud::LEDGER_REQUEST_KIND_DATA);
}

// A successful direct DATA write supersedes an earlier deferred write.
void testDirectDataWriteClearsDeferral(void *dataLedger) {
  assert(Cloud::instance().publishDataToLedger("ConnectState"));
  const int beforeRefusal = g_ledgerSetObserver.callCount;
  assert(Cloud::instance().publishDataToLedger("ReportState"));
  assert(g_ledgerSetObserver.callCount == beforeRefusal);
  completeLedgerSync(dataLedger, Cloud::LEDGER_REQUEST_KIND_DATA);

  // A direct caller wins before loop() has drained the deferral.
  testCurrent.totalOccupiedSecondsValue = 7654;
  assert(Cloud::instance().publishDataToLedger("ReportState"));
  assert(g_ledgerSetObserver.callCount == beforeRefusal + 1);
  assert(g_ledgerSetObserver.lastPayload.find("totalOccupiedSec=7654;") != std::string::npos);
  completeLedgerSync(dataLedger, Cloud::LEDGER_REQUEST_KIND_DATA);

  for (int i = 0; i < 50; ++i) {
    advanceAndLoop();
    assert(g_ledgerSetObserver.callCount == beforeRefusal + 1);
  }
}

} // namespace

int main() {
  void *statusLedger = testInflightWaitIsSilentThenWritesLatestContentOnce();
  void *dataLedger = testRefusedDataWriteIsWrittenAfterCompletion();
  testRefusedConnectStateStatusWriteIsWrittenAfterCompletion(statusLedger);
  testOneDeferredOperationPerPass(statusLedger, dataLedger);
  testDirectDataWriteClearsDeferral(dataLedger);

  printf("ledger_no_retry_test: all assertions passed\n");
  return 0;
}

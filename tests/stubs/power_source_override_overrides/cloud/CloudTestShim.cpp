// Test-only scaffolding for the `Cloud` singleton class used by
// src/cloud/DeviceStatusPublisher.cpp.
//
// This file does NOT reimplement any of the ledger-sync business logic from
// the real src/cloud/Cloud.cpp / src/cloud/CloudLedgerSync.cpp (that logic is
// unrelated to the power-source telemetry wiring this harness targets, and
// pulling in the real Cloud.cpp would require the full Particle Ledger SDK).
// Instead it provides the minimum construction/bookkeeping Cloud::instance()
// needs to exist, and makes Cloud::noteLedgerSyncRequest() always report "no
// request issued" (return 0). Both
// Cloud::writeDeviceStatusToCloud()/publishDataToLedger() in the REAL,
// unmodified DeviceStatusPublisher.cpp already handle that return value by
// taking their existing early-return path *after* building the JSON status
// payload (the code under test) -- so the JSON writer calls this harness
// verifies (including the `overrideActive` field write at
// DeviceStatusPublisher.cpp:163) still execute for real, but the harness never
// needs to reach actual Ledger::set()/cloud sync machinery.
#include "cloud/Cloud.h"

Cloud *Cloud::_instance = nullptr;

Cloud &Cloud::instance() {
  if (!_instance) {
    _instance = new Cloud();
  }
  return *_instance;
}

Cloud::Cloud()
    : ledgersSynced(false),
      lastApplySuccess(false),
      pendingStatusPublish(false),
      pendingStatusPublishSource(nullptr),
      pendingConfigApply(false),
      pendingDeviceStatusSync(false),
      pendingDeviceDataSync(false) {
  lastPublishedStatus[0] = '\0';
}

Cloud::~Cloud() {}

Cloud::LedgerSyncDiagnostics Cloud::ledgerSyncDiagnostics() const {
  return LedgerSyncDiagnostics{0, false, false, false, false, false, false, false};
}

bool Cloud::isLedgerPointerTracked(const void * /*ptr*/) const { return false; }

uint32_t Cloud::noteLedgerSyncRequest(LedgerRequestKind /*kind*/,
                                       const char * /*source*/,
                                       const void * /*ptr*/) {
  // Always report "not issued" so the real publisher functions take their
  // existing early-return path immediately after building/observing the JSON
  // status payload, without this harness needing a real Ledger backend.
  return 0;
}

// Never actually reached at runtime in this harness (noteLedgerSyncRequest()
// always returns 0 above, so the real publisher functions never reach the
// deviceStatusLedger.set()/deviceDataLedger.set() failure branch that would
// call this) -- provided only to satisfy the linker, since
// DeviceStatusPublisher.cpp references the symbol unconditionally.
void Cloud::noteLedgerSyncFail(uint32_t /*seq*/, int /*error*/) {}

// WO-2026-08-29-002 item 8: DeviceStatusPublisher.cpp calls the real
// isClockTrusted() defined in Generalized-Core-Counter.cpp, which this
// power-source-focused harness does not compile/link. Provided only to
// satisfy the linker; always reports "trusted" so this harness's existing
// assertions about the rest of the status payload are unaffected.
bool isClockTrusted() { return true; }

// Finding 2 (Round 4 review): DeviceStatusPublisher.cpp now also calls the
// real observedTimeSyncedLastMs() defined in Generalized-Core-Counter.cpp,
// which this harness does not compile/link either. Provided only to
// satisfy the linker; a fixed nonzero value keeps this harness's existing
// syncAgeSec/clock-payload assertions stable.
uint32_t observedTimeSyncedLastMs() { return 1000; }

// Round 6 (Stage 7 finding 1): DeviceStatusPublisher.cpp now derives
// syncAgeSec from reportedSyncAgeMs() (the wrap-aware, sentinel-bearing
// telemetry value) rather than calling observedTimeSyncedLastMs() itself.
// The real reportedSyncAgeMs() is defined in Generalized-Core-Counter.cpp,
// which this harness does not compile/link. Provided only to satisfy the
// linker; a fixed, non-sentinel value keeps this harness's status-payload
// assertions unaffected (this harness does not itself assert on
// syncAgeSec's numeric value - see power_source_override_test.cpp).
uint32_t reportedSyncAgeMs() { return 1000; }

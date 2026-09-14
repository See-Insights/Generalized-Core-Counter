// Round 6 follow-up (Stage 7 finding 2): the handful of external symbols
// src/cloud/Cloud.cpp itself needs beyond what tests/clock_status_
// republish_test.cpp defines directly, so the REAL, unmodified Cloud.cpp
// (not a mirror/shim of it, unlike prior rounds' harnesses) can be compiled
// and linked on the host.
//
// Cloud.cpp's `#include "../Config.h"` is a relative include, so it always
// resolves to the REAL src/Config.h (declaring real functions), regardless
// of this override directory's -I search path - that header is reached
// only from the small, rarely-exercised loop()/loadConfigurationFromCloud()
// pendingConfigApply branch (never taken at runtime here, since
// pendingConfigApply stays false for this test's whole lifetime), but the
// linker still needs these symbols resolved because that code is compiled
// into Cloud.cpp's object file regardless of runtime reachability. These
// stubs are never actually invoked; they exist only to satisfy the linker,
// exactly like the same pattern already established in
// tests/stubs/power_source_override_overrides/cloud/CloudTestShim.cpp for a
// different, non-overlapping set of symbols.
#include "cloud/Cloud.h"
#include "../../../src/Config.h" // real header - declares Config::Source/CONFIG_SOURCE_DEFAULT

namespace Config {

const char *sourceToString(Source) { return "unknown"; }
Source getSource() { return CONFIG_SOURCE_DEFAULT; }
bool isValid(bool, const char **) { return true; }
bool validateConfigFields(bool, const char **) { return true; }
void markLedgerConfigurationValid() {}
void logDiagnostics(const char *) {}

} // namespace Config

// Cloud::applyConfigurationFromLedger() is a private member implemented in
// the separate src/cloud/ConfigApply.cpp translation unit in production
// (pulling it in would drag in the full sensor/messaging/modes/reporting/
// power/timing config-application logic, which is unrelated to what this
// test targets - the deferred status-publish drain). Never actually
// invoked here (pendingConfigApply stays false for this test's whole
// lifetime), so a stub that always reports failure is sufficient to
// satisfy the linker without misrepresenting real behavior if it were ever
// reached.
bool Cloud::applyConfigurationFromLedger(const LedgerData &, const LedgerData &) {
    return false;
}

// FIRMWARE_VERSION: referenced unconditionally by
// DeviceStatusPublisher.cpp's writeDeviceStatusToCloud().
const char *FIRMWARE_VERSION = "test-fw";

// Round 6 follow-up (Stage 7 finding 2): test-controlled clock-trust
// telemetry inputs. The REAL implementations of these three functions live
// in Generalized-Core-Counter.cpp, which this harness deliberately does not
// compile/link (it has its own heavy Particle/AB1805/state-machine
// dependencies unrelated to what this test targets). Provided here, in the
// SAME spirit as tests/stubs/power_source_override_overrides/cloud/
// CloudTestShim.cpp's isClockTrusted()/observedTimeSyncedLastMs() stubs,
// but made test-controllable (via a global the test flips) rather than
// hardcoded, because THIS test's entire purpose is to prove that a
// confirmed sync (isClockTrusted() == true) actually reaches the published
// JSON payload as clock.trusted == true through the real
// Cloud::requestStatusPublish() -> Cloud::loop() -> writeDeviceStatusToCloud()
// chain.
bool testClockTrusted = false;


bool isClockTrusted() { return testClockTrusted; }
uint32_t observedTimeSyncedLastMs() { return testClockTrusted ? 12345u : 0u; }
uint32_t reportedSyncAgeMs() { return testClockTrusted ? 5000u : 0xFFFFFFFFu; }

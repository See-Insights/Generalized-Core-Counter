// Host-side regression test for the Boron USB source override's
// reporting/telemetry wiring (WO: Power source-reporting order and
// overrideActive telemetry wiring).
//
// Compiles the real src/power/PowerManager.cpp against lightweight host
// stubs under tests/stubs/power_source_override_overrides/ that make
// PowerPlatform::detectCapabilities()/readPowerSource() mutable per-scenario
// (unlike tests/stubs/power/PowerPlatform.h, which always reports
// hasPmicPowerConfiguration=false and therefore never reaches the override
// block at all), plus a Particle.h stub that forces the
// `PLATFORM_ID == PLATFORM_BORON` guard on and provides mutable stand-ins
// for the NRF_USBD/NRF_POWER registers the override reads directly.
//
// Covers all four scenarios called out in the work order:
//   1. Override fires -> nextReport.reading.powerSource reflects the
//      corrected (post-override) value.
//   2. Override does not fire -> source stays at the raw value.
//   3. overrideActive == true when the override fired.
//   4. overrideActive == false when it did not.
// Plus regression coverage that fallbackUsed's existing (unrelated,
// currently always-false-in-this-path) behavior is unaffected.
//
// Finding-1 remediation (Stage 7 mutation-testing gap): this file also
// compiles and links the REAL, unmodified src/cloud/DeviceStatusPublisher.cpp
// (via tests/power_source_override_test.sh) so the telemetry-writing path at
// DeviceStatusPublisher.cpp:163 (`overrideActive`) is exercised by an actual
// call to Cloud::writeDeviceStatusToCloud(), not just by reading
// PowerManager's in-memory report. See the
// testPublisherReportsOverride*() tests below, and
// tests/stubs/power_source_override_overrides/Particle.h /
// cloud/CloudTestShim.cpp for the supporting (non-production-logic) scaffolding.

#include "power/PowerManager.h"
#include "power/PowerPlatform.h"
#include "cloud/Cloud.h"
#include "MyPersistentData.h"
#include "Particle.h"

#include <cassert>
#include <iostream>

TestCurrentStatus testCurrent;
TestSystemStatus testSysStatus;
TestSensorConfig testSensorConfig;

TestNrfUsbdRegs testNrfUsbdRegs;
TestNrfPowerRegs testNrfPowerRegs;

// Populated by the JSONBufferWriter stand-in in Particle.h so this test can
// observe which value the real, unmodified src/cloud/DeviceStatusPublisher.cpp
// actually wrote for a given JSON field name (see Finding 1 test below).
JSONFieldObserver g_statusJsonObserver;

// Required by DeviceStatusPublisher.cpp (`extern const char* FIRMWARE_VERSION;`).
const char *FIRMWARE_VERSION = "test-fw";

PowerPlatform::TestPowerPlatformState PowerPlatform::testPowerPlatformState;

namespace {

// Mirrors the anonymous-namespace power-source constants in
// src/power/PowerManager.cpp (not directly reachable from this test file).
constexpr int kPowerSourceUnknown = 0;
constexpr int kPowerSourceVin = 1;
constexpr int kPowerSourceUsbHost = 2;

// Bit values matching PowerManager.cpp's override condition:
// usbEnumerated = (USBADDR & 0x7F) != 0
// usbVbusPresent = (USBREGSTATUS & 0x01)
// usbRegReady   = (USBREGSTATUS & 0x02)
constexpr uint32_t kUsbAddrEnumerated = 0x05;
constexpr uint32_t kUsbRegStatusVbusAndReady = 0x03;

void resetHarness() {
  PowerPlatform::testPowerPlatformState = {};
  PowerPlatform::testPowerPlatformState.capabilities.hasPmicPowerConfiguration = true;
  testNrfUsbdRegs = {};
  testNrfPowerRegs = {};
}

// Scenario 1 + 3: override fires (raw VIN + USB hardware enumerated).
// nextReport.reading.powerSource must reflect the corrected USB_HOST value,
// and overrideActive telemetry must be true.
void testOverrideFiresReportsCorrectedSourceAndSetsOverrideActive() {
  resetHarness();
  PowerPlatform::testPowerPlatformState.snapshot.source = kPowerSourceVin;
  PowerPlatform::testPowerPlatformState.snapshot.status = PowerAvailability::Valid;
  testNrfUsbdRegs.USBADDR = kUsbAddrEnumerated;
  testNrfPowerRegs.USBREGSTATUS = kUsbRegStatusVbusAndReady;

  const bool result = PowerManager::instance().refreshInputProfile();
  assert(result);

  const PowerReading &reading = PowerManager::instance().latestReport().reading;
  assert(reading.powerSource == kPowerSourceUsbHost);
  assert(reading.overrideActive);
  // fallbackUsed is unrelated to the override and must remain untouched.
  assert(!reading.fallbackUsed);

  const PowerReport &report = PowerManager::instance().latestReport();
  assert(report.activeInputProfile == PowerInputProfile::UsbBench);
}

// Scenario 2 + 4: override does not fire because USB hardware is not
// enumerated (raw VIN source persists, no regression). overrideActive must
// be false.
void testOverrideDoesNotFireKeepsRawSourceAndClearsOverrideActive() {
  resetHarness();
  PowerPlatform::testPowerPlatformState.snapshot.source = kPowerSourceVin;
  PowerPlatform::testPowerPlatformState.snapshot.status = PowerAvailability::Valid;
  testNrfUsbdRegs.USBADDR = 0; // not enumerated
  testNrfPowerRegs.USBREGSTATUS = 0;

  const bool result = PowerManager::instance().refreshInputProfile();
  assert(result);

  const PowerReading &reading = PowerManager::instance().latestReport().reading;
  assert(reading.powerSource == kPowerSourceVin);
  assert(!reading.overrideActive);
  assert(!reading.fallbackUsed);

  const PowerReport &report = PowerManager::instance().latestReport();
  assert(report.activeInputProfile == PowerInputProfile::Solar35W);
}

// Scenario 4 (second case): override's trigger source condition isn't met at
// all (raw source is already USB_HOST, not VIN/UNKNOWN) even though the USB
// hardware signals look "override-eligible". Confirms overrideActive stays
// false and the raw source is reported unchanged whenever the override's
// condition simply doesn't apply.
void testNonEligibleSourceNeverSetsOverrideActive() {
  resetHarness();
  PowerPlatform::testPowerPlatformState.snapshot.source = kPowerSourceUsbHost;
  PowerPlatform::testPowerPlatformState.snapshot.status = PowerAvailability::Valid;
  testNrfUsbdRegs.USBADDR = kUsbAddrEnumerated;
  testNrfPowerRegs.USBREGSTATUS = kUsbRegStatusVbusAndReady;

  const bool result = PowerManager::instance().refreshInputProfile();
  assert(result);

  const PowerReading &reading = PowerManager::instance().latestReport().reading;
  assert(reading.powerSource == kPowerSourceUsbHost);
  assert(!reading.overrideActive);
  assert(!reading.fallbackUsed);
}

// Sanity check that raw UNKNOWN source (the override's other eligible raw
// value) also gets corrected and reported when USB hardware is enumerated.
void testOverrideFiresFromUnknownSource() {
  resetHarness();
  PowerPlatform::testPowerPlatformState.snapshot.source = kPowerSourceUnknown;
  PowerPlatform::testPowerPlatformState.snapshot.status = PowerAvailability::Unknown;
  testNrfUsbdRegs.USBADDR = kUsbAddrEnumerated;
  testNrfPowerRegs.USBREGSTATUS = kUsbRegStatusVbusAndReady;

  const bool result = PowerManager::instance().refreshInputProfile();
  assert(result);

  const PowerReading &reading = PowerManager::instance().latestReport().reading;
  assert(reading.powerSource == kPowerSourceUsbHost);
  assert(reading.overrideActive);
  assert(!reading.fallbackUsed);
}

// --- Finding-1 remediation: extend coverage to the REAL
// src/cloud/DeviceStatusPublisher.cpp telemetry-writing path -------------
//
// Stage 7 mutation testing found that the original harness above only linked
// src/power/PowerManager.cpp, so a regression that rewired
// DeviceStatusPublisher.cpp's `overrideActive` JSON field back to
// `fallbackUsed` (DeviceStatusPublisher.cpp:163) went uncaught -- nothing in
// the suite actually called the publisher's JSON-writing code. These two
// tests call the REAL Cloud::writeDeviceStatusToCloud() (now linked by
// tests/power_source_override_test.sh) and inspect, via the JSONBufferWriter
// stand-in's g_statusJsonObserver (see Particle.h), what value it actually
// wrote for the "overrideActive" field.

// Mutation (c) target: DeviceStatusPublisher.cpp:163 must write
// `powerReport.reading.overrideActive` (true here) into the "overrideActive"
// JSON field, not `fallbackUsed` (which this code path always leaves false).
// If that line is rewired back to fallbackUsed, the captured JSON field would
// read `false` instead of the expected `true`, and this test fails.
void testPublisherReportsOverrideActiveWhenOverrideFires() {
  resetHarness();
  g_statusJsonObserver.reset();
  PowerPlatform::testPowerPlatformState.snapshot.source = kPowerSourceVin;
  PowerPlatform::testPowerPlatformState.snapshot.status = PowerAvailability::Valid;
  testNrfUsbdRegs.USBADDR = kUsbAddrEnumerated;
  testNrfPowerRegs.USBREGSTATUS = kUsbRegStatusVbusAndReady;

  const bool refreshed = PowerManager::instance().refreshInputProfile();
  assert(refreshed);

  const PowerReading &reading = PowerManager::instance().latestReport().reading;
  assert(reading.overrideActive);
  assert(!reading.fallbackUsed);

  // Call the REAL Cloud::writeDeviceStatusToCloud(). It always takes its
  // early-return path right after building the status JSON (this harness's
  // Cloud::noteLedgerSyncRequest() stub, in
  // tests/stubs/power_source_override_overrides/cloud/CloudTestShim.cpp,
  // always reports "not issued"), so no real ledger/cloud sync ever happens
  // -- but the JSON writer calls, including the overrideActive field write,
  // execute for real.
  Cloud::instance().writeDeviceStatusToCloud("mutation-test-override-fires");

  bool capturedOverrideActive = false;
  const bool found = g_statusJsonObserver.findBool("overrideActive", &capturedOverrideActive);
  assert(found);
  assert(capturedOverrideActive == true);
}

// Companion scenario: override does not fire. Confirms the publisher writes
// `false` here too (parity check; mutation (c) alone would not be caught by
// this scenario since fallbackUsed is also false here, which is exactly why
// the "fires" scenario above is the one that catches the rewiring mutation).
void testPublisherReportsOverrideInactiveWhenOverrideDoesNotFire() {
  resetHarness();
  g_statusJsonObserver.reset();
  PowerPlatform::testPowerPlatformState.snapshot.source = kPowerSourceVin;
  PowerPlatform::testPowerPlatformState.snapshot.status = PowerAvailability::Valid;
  testNrfUsbdRegs.USBADDR = 0;
  testNrfPowerRegs.USBREGSTATUS = 0;

  const bool refreshed = PowerManager::instance().refreshInputProfile();
  assert(refreshed);

  const PowerReading &reading = PowerManager::instance().latestReport().reading;
  assert(!reading.overrideActive);
  assert(!reading.fallbackUsed);

  Cloud::instance().writeDeviceStatusToCloud("mutation-test-override-inactive");

  bool capturedOverrideActive = true;
  const bool found = g_statusJsonObserver.findBool("overrideActive", &capturedOverrideActive);
  assert(found);
  assert(capturedOverrideActive == false);
}

// --- WO-2026-08-24-001: firmware-object "flags" build-flag witness --------
//
// DeviceStatusPublisher.cpp's firmware object now writes a "flags" bitmask
// field derived directly from the same #if conditions BuildProfile.h uses to
// gate each feature (see BuildProfile.h and the "compiledBuildFlags" block in
// both Generalized-Core-Counter.cpp's Boot: line and here). This exercises
// the REAL, unmodified DeviceStatusPublisher.cpp writer path (not a
// source-level reimplementation) so a future rewiring of that field back to
// something else -- or a bit layout that silently drifts from the Boot: line
// -- would be caught here, the same way the overrideActive tests above catch
// drift in that field.
//
// This test's expected value (0x6008 / 24584) is computed from
// src/BuildProfile.h's actual DEFAULT flag values as of this WO, plus the
// Addendum A 0x4000 bit:
//   ENABLE_PMIC_FORENSICS=1        -> bit3  (0x0008)
//   ENABLE_DIAGNOSTICS_PUBLISH_MODE=1 -> bit13 (0x2000)
//   PLATFORM_ID == PLATFORM_BORON (forced by the test stub Particle.h, the
//     same guard that gates PowerManager.cpp's USB source override) -> bit14
//     (0x4000)
//   all other witnessed flags default to 0.
// tests/build_flags_witness_test.sh separately proves (by compiling the
// REAL, extracted witness expression under a second, deliberately-flipped
// build-flag configuration, and again under matching/non-matching
// PLATFORM_ID/PLATFORM_BORON configurations) that this value actually
// changes when the compiled flags change -- i.e. it isn't a
// hardcoded/inert field.
void testPublisherFirmwareObjectReportsCompiledBuildFlags() {
  resetHarness();
  g_statusJsonObserver.reset();
  PowerPlatform::testPowerPlatformState.snapshot.source = kPowerSourceVin;
  PowerPlatform::testPowerPlatformState.snapshot.status = PowerAvailability::Valid;
  testNrfUsbdRegs.USBADDR = 0;
  testNrfPowerRegs.USBREGSTATUS = 0;

  const bool refreshed = PowerManager::instance().refreshInputProfile();
  assert(refreshed);

  Cloud::instance().writeDeviceStatusToCloud("mutation-test-build-flags-witness");

  int capturedFlags = -1;
  const bool found = g_statusJsonObserver.findInt("flags", &capturedFlags);
  assert(found);
  assert(capturedFlags == 0x6008);
}

} // namespace

int main() {
  testOverrideFiresReportsCorrectedSourceAndSetsOverrideActive();
  testOverrideDoesNotFireKeepsRawSourceAndClearsOverrideActive();
  testNonEligibleSourceNeverSetsOverrideActive();
  testOverrideFiresFromUnknownSource();
  testPublisherReportsOverrideActiveWhenOverrideFires();
  testPublisherReportsOverrideInactiveWhenOverrideDoesNotFire();
  testPublisherFirmwareObjectReportsCompiledBuildFlags();
  std::cout << "Power source override reporting/telemetry tests passed\n";
  return 0;
}

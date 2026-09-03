#pragma once

// Round 6 follow-up (Stage 7 finding 2): this file is a fork of
// tests/stubs/power_source_override_overrides/Particle.h, extended to also
// support linking the REAL src/cloud/Cloud.cpp (not just
// DeviceStatusPublisher.cpp/PowerManager.cpp as the power-source override
// harness does) - see tests/clock_status_republish_test.cpp/.sh. The two
// additions over the original are:
//   1. TestParticleClass::connected() is now test-controlled (a mutable
//      global flag), not hardcoded false - Cloud::loop()'s deferred-publish
//      drain is gated on Particle.connected(), so the test needs to flip it.
//   2. Ledger::get() is added (the real Cloud::loadConfigurationFromCloud()/
//      loop()'s pendingConfigApply branch call it) - trivially returns an
//      empty LedgerData, same spirit as the existing Ledger::set() stub.
// Kept as a separate file (not a shared edit to the original) so the
// power-source override harness's Particle.h is completely unaffected by
// this test's needs.

#include <cmath>
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cstdio>
#include <string>

constexpr int SYSTEM_ERROR_NONE = 0;

struct TestLog {
  template <typename... Args>
  void info(const char *, Args...) {}

  template <typename... Args>
  void warn(const char *, Args...) {}
};

inline TestLog Log;

// Host-side stand-ins for the Device OS platform identifiers so
// PowerManager.cpp's `#if PLATFORM_ID == PLATFORM_BORON` guard can be forced
// on for this test build. (WO-2026-08-24-001 removed
// ENABLE_BORON_USB_SOURCE_OVERRIDE -- the override now compiles
// unconditionally on Boron, gated only by this platform check.)
#define PLATFORM_BORON 88
#define PLATFORM_ID PLATFORM_BORON

// Host-side stand-ins for the Nordic nRF52840 USB/POWER peripheral registers
// that the Boron USB source override reads directly
// (NRF_USBD->USBADDR / NRF_POWER->USBREGSTATUS). Tests control these via the
// mutable global instances below.
struct TestNrfUsbdRegs {
  uint32_t USBADDR = 0;
};

struct TestNrfPowerRegs {
  uint32_t USBREGSTATUS = 0;
};

extern TestNrfUsbdRegs testNrfUsbdRegs;
extern TestNrfPowerRegs testNrfPowerRegs;

#define NRF_USBD (&testNrfUsbdRegs)
#define NRF_POWER (&testNrfPowerRegs)

// --- Extended for the Finding-1 remediation round: scaffolding needed to
// additionally link the real src/cloud/DeviceStatusPublisher.cpp -----------
//
// DeviceStatusPublisher.cpp guards Cellular-specific code with this macro;
// the host build has no cellular modem, so keep it off.
#define Wiring_Cellular 0

// Minimal JSONBufferWriter stand-in.
//
// DeviceStatusPublisher.cpp only needs a writer that (a) accepts the same
// call sequence (name()/value()/beginObject()/endObject()/buffer()/dataSize())
// the real Particle JSONBufferWriter supports, and (b) lets the test observe
// which value was written for a given field name. It does not need to
// produce byte-for-byte spec-compliant JSON: nothing downstream in the
// harness parses the buffer as JSON (the real function's duplicate-suppression
// strcmp() and LedgerData::fromJSON() call are both stubbed out below), so
// reproducing exact JSON syntax here would add complexity without adding
// coverage. What this class DOES do faithfully is record, for every
// name()->value() pair, the field name and value actually passed by the real
// production code -- that observation is what mutation (c) must be caught by.
struct JSONFieldObserver {
  static constexpr size_t kMaxFields = 64;
  struct Field {
    char name[64];
    bool isBool = false;
    bool boolValue = false;
    bool isInt = false;
    int intValue = 0;
  };
  Field fields[kMaxFields];
  size_t count = 0;

  void record(const char *n, bool value) {
    if (count >= kMaxFields) return;
    std::snprintf(fields[count].name, sizeof(fields[count].name), "%s", n ? n : "");
    fields[count].isBool = true;
    fields[count].boolValue = value;
    ++count;
  }

  // WO-2026-08-24-001: records int-valued fields (e.g. the "flags" build-flag
  // witness in DeviceStatusPublisher.cpp's firmware object) the same way
  // record(bool) already does for overrideActive.
  void recordInt(const char *n, int value) {
    if (count >= kMaxFields) return;
    std::snprintf(fields[count].name, sizeof(fields[count].name), "%s", n ? n : "");
    fields[count].isInt = true;
    fields[count].intValue = value;
    ++count;
  }

  bool findBool(const char *n, bool *out) const {
    for (size_t i = 0; i < count; ++i) {
      if (fields[i].isBool && std::strcmp(fields[i].name, n) == 0) {
        *out = fields[i].boolValue;
        return true;
      }
    }
    return false;
  }

  bool findInt(const char *n, int *out) const {
    for (size_t i = 0; i < count; ++i) {
      if (fields[i].isInt && std::strcmp(fields[i].name, n) == 0) {
        *out = fields[i].intValue;
        return true;
      }
    }
    return false;
  }

  void reset() { count = 0; }
};

// One observer instance shared by every JSONBufferWriter the real code
// constructs on the host build; the test resets it before each scenario.
extern JSONFieldObserver g_statusJsonObserver;

// Round 6 follow-up (Stage 7 finding 2, second pass): dataSize() used to
// unconditionally return 0, which left bufferBase == "" for every call.
// Since Cloud's lastPublishedStatus member also starts as "" (see
// Cloud::Cloud()), the real writeDeviceStatusToCloud()'s duplicate-
// suppression check at DeviceStatusPublisher.cpp
// (`strcmp(lastPublishedStatus, bufferBase) == 0`) always matched and
// returned true BEFORE reaching deviceStatusLedger.set(data) - so the test
// exercised deferred invocation and JSON-field composition, but never the
// actual ledger write, which is the specific link Stage 7 flagged (mutation
// M3: removing deviceStatusLedger.set(data) from production still passed).
//
// This class now genuinely serialises each name()/value() call into
// `buffer_` and tracks a real write position in `pos_`, so:
//   - dataSize() reports the ACTUAL bytes written (never 0 for a non-empty
//     object), so bufferBase is non-empty and duplicate suppression must
//     do a REAL byte-for-byte comparison instead of "" == "".
//   - Two calls that compose different values (e.g. clock.trusted true vs.
//     false) serialise to different byte sequences, so duplicate
//     suppression correctly treats them as distinct publishes - it is not
//     defeated, only exercised honestly.
//   - Two calls that compose IDENTICAL values still serialise identically,
//     so the real duplicate-suppression logic still correctly treats a
//     genuine repeat as a no-op, which is what
//     testDrainedPublishDoesNotRepeatOnSubsequentLoopCalls() depends on.
// This is still not spec-compliant JSON (nothing downstream parses it as
// JSON - LedgerData::fromJSON below just stores the raw text verbatim for
// the test to inspect), but it is a faithful, deterministic serialisation
// of exactly what the real production code composed, which is what the
// duplicate-suppression comparison and the ledger-write assertion both
// need to be honest.
class JSONBufferWriter {
 public:
  JSONBufferWriter(char *buffer, size_t capacity) : buffer_(buffer), capacity_(capacity) {
    if (buffer_ && capacity_ > 0) {
      buffer_[0] = '\0';
    }
  }

  JSONBufferWriter &beginObject() {
    append("{");
    return *this;
  }
  JSONBufferWriter &endObject() {
    append("}");
    return *this;
  }

  JSONBufferWriter &name(const char *n) {
    lastName_ = n ? n : "";
    append(lastName_.c_str());
    append("=");
    return *this;
  }

  JSONBufferWriter &value(bool v) {
    g_statusJsonObserver.record(lastName_.c_str(), v);
    append(v ? "true" : "false");
    append(";");
    return *this;
  }
  JSONBufferWriter &value(int v) {
    g_statusJsonObserver.recordInt(lastName_.c_str(), v);
    char numBuf[16];
    std::snprintf(numBuf, sizeof(numBuf), "%d", v);
    append(numBuf);
    append(";");
    return *this;
  }
  JSONBufferWriter &value(long v) {
    char numBuf[24];
    std::snprintf(numBuf, sizeof(numBuf), "%ld", v);
    append(numBuf);
    append(";");
    return *this;
  }
  JSONBufferWriter &value(unsigned long v) {
    char numBuf[24];
    std::snprintf(numBuf, sizeof(numBuf), "%lu", v);
    append(numBuf);
    append(";");
    return *this;
  }
  JSONBufferWriter &value(float v, int decimals) {
    char numBuf[32];
    std::snprintf(numBuf, sizeof(numBuf), "%.*f", decimals, (double)v);
    append(numBuf);
    append(";");
    return *this;
  }
  JSONBufferWriter &value(double v, int decimals) {
    char numBuf[32];
    std::snprintf(numBuf, sizeof(numBuf), "%.*f", decimals, v);
    append(numBuf);
    append(";");
    return *this;
  }
  JSONBufferWriter &value(const char *v) {
    append(v ? v : "");
    append(";");
    return *this;
  }

  char *buffer() const { return buffer_; }
  size_t dataSize() const { return pos_; }

 private:
  void append(const char *text) {
    if (!buffer_ || !text) return;
    const size_t len = std::strlen(text);
    // Leave room for the trailing NUL the real caller adds itself via
    // bufferBase[writerBase.dataSize()] = '\0' (mirrors the real
    // JSONBufferWriter's own bounds behaviour: silently truncate rather
    // than overrun).
    const size_t room = (pos_ < capacity_) ? (capacity_ - pos_ - 1) : 0;
    const size_t toCopy = (len < room) ? len : room;
    if (toCopy > 0) {
      std::memcpy(buffer_ + pos_, text, toCopy);
      pos_ += toCopy;
      buffer_[pos_] = '\0';
    }
  }

  char *buffer_;
  size_t capacity_;
  size_t pos_ = 0;
  std::string lastName_;
};

// Minimal Ledger / LedgerData stand-ins.
//
// Real Cloud.h declares `Ledger` member fields. This test links the REAL
// src/cloud/Cloud.cpp (unlike the power-source override harness, which
// stubs the whole Cloud class out), so unlike that harness's Particle.h,
// Cloud::noteLedgerSyncRequest() here is the REAL implementation, and this
// stand-in's lastUpdated()/lastSynced() always returning 0 (never
// "unsynced") is what keeps its "already inflight" duplicate-detection
// branch from firing, so a first-time status publish request is issued a
// real nonzero sequence number and DeviceStatusPublisher.cpp's
// deviceStatusLedger.set(data) call (via Ledger::set(), below) is reached
// for real - this is what makes writeDeviceStatusToCloud() actually return
// true end to end, not just take an early "could not issue" return. get()
// is added over the power-source-override Particle.h's Ledger stand-in
// because the real Cloud::loadConfigurationFromCloud()/loop()'s
// pendingConfigApply branch call it (never reached at runtime here, since
// pendingConfigApply stays false, but the symbol must still resolve at
// link time since Cloud.cpp's compiled code references it).
//
// Round 6 follow-up (Stage 7 finding 2, second pass): LedgerData now
// carries the exact text passed to LedgerData::fromJSON() (i.e. exactly
// what writeDeviceStatusToCloud() serialised for THIS publish), and
// Ledger::set() records both a call count and the last received payload
// into g_ledgerSetObserver, below. This lets the test assert on what the
// ledger actually RECEIVED (per Stage 7's explicit ask), not merely on
// what JSONBufferWriter composed - i.e. it distinguishes "the payload was
// built" from "the payload actually reached deviceStatusLedger.set()",
// which is exactly the gap mutation M3 (removing that call from
// production) is meant to expose.
class LedgerData {
 public:
  LedgerData() = default;
  explicit LedgerData(std::string json) : json_(std::move(json)) {}
  static LedgerData fromJSON(const char *json) { return LedgerData(json ? std::string(json) : std::string()); }
  const std::string &json() const { return json_; }

 private:
  std::string json_;
};

struct LedgerSetObserver {
  int callCount = 0;
  std::string lastPayload;

  void reset() {
    callCount = 0;
    lastPayload.clear();
  }
};

extern LedgerSetObserver g_ledgerSetObserver;

class Ledger {
 public:
  int64_t lastUpdated() const { return 0; }
  int64_t lastSynced() const { return 0; }
  int set(const LedgerData &data) {
    g_ledgerSetObserver.callCount++;
    g_ledgerSetObserver.lastPayload = data.json();
    return SYSTEM_ERROR_NONE;
  }
  LedgerData get() const { return LedgerData(); }
};

// Minimal Time / System / Particle / Cellular globals.
struct TestTimeClass {
  long now() const { return 1700000000L; }
  bool isValid() const { return true; }
};
inline TestTimeClass Time;

struct TestSystemClass {
  unsigned long freeMemory() const { return 65536UL; }
  int resetReason() const { return 0; }
  unsigned long resetReasonData() const { return 0UL; }
};
inline TestSystemClass System;

// Test-controlled connectivity flag. Round 6 follow-up (Stage 7 finding 2):
// Cloud::loop()'s deferred status-publish drain is gated on
// `pendingStatusPublish && Particle.connected()`, so the test needs to
// flip this to exercise (and to deliberately NOT exercise) that drain -
// the original power-source-override Particle.h hardcodes this false,
// which would never let that drain run.
extern bool testParticleConnected;

struct TestParticleClass {
  bool connected() const { return testParticleConnected; }
  unsigned long timeSyncedLast() const { return 0UL; }
  bool syncTimeDone() const { return true; }
  void syncTime() {}
};
inline TestParticleClass Particle;

struct TestCellularClass {
  bool ready() const { return false; }
};
inline TestCellularClass Cellular;

inline unsigned long millis() { return 1000UL; }

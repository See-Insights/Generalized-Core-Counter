#pragma once

#include <cmath>
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cstdio>
#include <string>

constexpr int SYSTEM_ERROR_NONE = 0;

struct TestLog {
  // WO-2026-09-15-001 Amendment C: real call counter, additive to the
  // existing no-op behavior - existing assertions in this test suite don't
  // read it, so this is inert for them. Used by the LedgerPayloadStatus
  // log-guard tests in power_source_override_test.cpp.
  int infoCallCount = 0;

  template <typename... Args>
  void info(const char *, Args...) {
    infoCallCount++;
  }

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

class JSONBufferWriter {
 public:
  JSONBufferWriter(char *buffer, size_t capacity) : buffer_(buffer), capacity_(capacity) {
    if (buffer_ && capacity_ > 0) {
      buffer_[0] = '\0';
    }
  }

  JSONBufferWriter &beginObject() { return *this; }
  JSONBufferWriter &endObject() { return *this; }

  JSONBufferWriter &name(const char *n) {
    lastName_ = n ? n : "";
    appendRaw(lastName_.c_str());
    appendRaw(":");
    return *this;
  }

  JSONBufferWriter &value(bool v) {
    g_statusJsonObserver.record(lastName_.c_str(), v);
    appendRaw(v ? "true," : "false,");
    return *this;
  }
  JSONBufferWriter &value(int v) {
    g_statusJsonObserver.recordInt(lastName_.c_str(), v);
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%d,", v);
    appendRaw(buf);
    return *this;
  }
  JSONBufferWriter &value(long v) {
    char buf[24];
    std::snprintf(buf, sizeof(buf), "%ld,", v);
    appendRaw(buf);
    return *this;
  }
  JSONBufferWriter &value(unsigned long v) {
    char buf[24];
    std::snprintf(buf, sizeof(buf), "%lu,", v);
    appendRaw(buf);
    return *this;
  }
  JSONBufferWriter &value(float f, int) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%f,", (double)f);
    appendRaw(buf);
    return *this;
  }
  JSONBufferWriter &value(double d, int) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%f,", d);
    appendRaw(buf);
    return *this;
  }
  JSONBufferWriter &value(const char *s) {
    appendRaw(s ? s : "");
    appendRaw(",");
    return *this;
  }

  char *buffer() const { return buffer_; }
  size_t dataSize() const { return pos_; }

 private:
  // WO-2026-09-15-001 Amendment C: earlier this stub never wrote into
  // buffer_ at all (dataSize() was hardcoded to 0), which made the
  // production strcmp() duplicate-suppression gate at
  // DeviceStatusPublisher.cpp always compare two empty strings and made the
  // per-call byte count constant regardless of payload content - neither
  // condition holds on real hardware, and both defeated the
  // LedgerPayloadStatus log-guard tests below. This does not need to
  // produce byte-for-byte spec-compliant JSON (see the class comment
  // above); it only needs the written length to actually track what was
  // named and valued, so downstream code that reads dataSize()/buffer()
  // observes a size that moves when a real payload's size would move.
  void appendRaw(const char *s) {
    if (!buffer_ || capacity_ == 0) return;
    const size_t len = std::strlen(s);
    // Leave room for the production code's own bufferBase[dataSize()]='\0'
    // write, which lands one byte past what we report here.
    const size_t maxPos = capacity_ - 1;
    const size_t avail = maxPos > pos_ ? maxPos - pos_ : 0;
    const size_t toCopy = len < avail ? len : avail;
    std::memcpy(buffer_ + pos_, s, toCopy);
    pos_ += toCopy;
  }

  char *buffer_;
  size_t capacity_;
  size_t pos_ = 0;
  std::string lastName_;
};

// Minimal Ledger / LedgerData stand-ins.
//
// Real Cloud.h declares `Ledger` member fields and DeviceStatusPublisher.cpp
// calls LedgerData::fromJSON()/Ledger::set(). This harness never reaches
// actual cloud sync (Cloud::noteLedgerSyncRequest(), stubbed in
// cloud/CloudTestShim.cpp, always returns 0, so the real function takes its
// existing "could not issue ledger request" early-return before calling
// Ledger::set()), so these stand-ins only need to be layout-valid and satisfy
// the calls that DO execute unconditionally (construction, lastUpdated()/
// lastSynced() inside the ENABLE_LEDGER_TRACE-guarded diagnostics lambda,
// which is defined but not invoked when ENABLE_LEDGER_TRACE is off).
class LedgerData {
 public:
  static LedgerData fromJSON(const char *) { return LedgerData(); }
};

class Ledger {
 public:
  int64_t lastUpdated() const { return 0; }
  int64_t lastSynced() const { return 0; }
  int set(const LedgerData &) { return SYSTEM_ERROR_NONE; }
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

struct TestParticleClass {
  bool connected() const { return false; }
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

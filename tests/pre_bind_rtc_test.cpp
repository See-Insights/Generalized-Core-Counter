// v50 S1b: src/time/PreBindRtc.h compiled as-is; Clock::isPlausibleEpoch is the real body extracted from Clock.cpp
// (included as CLOCK_PLAUSIBLE_INC by tests/pre_bind_rtc_test.py). Not a bench result: the chip itself is untested here.
#include <cstdio>
#include <cstdint>
#include <ctime>
#include <mutex>
#include <vector>
#include <string>

static int failures = 0;
#define CHECK(c) do { if (!(c)) { std::printf("FAIL line %d: %s\n", __LINE__, #c); failures++; } } while (0)

static std::vector<std::string> events;

struct FakeTime {
  bool valid = false;
  time_t now_ = 0;
  bool isValid() { return valid; }
  void setTime(time_t t) { events.push_back("setTime"); now_ = t; valid = true; }
};
static FakeTime fakeTime;
#define Time fakeTime

namespace Clock {
#include CLOCK_PLAUSIBLE_INC
}

struct FakeWire {
  bool enabled = false;
  void lock() { events.push_back("lock"); }
  void unlock() { events.push_back("unlock"); }
  bool isEnabled() { return enabled; }
  void begin() { events.push_back("begin"); enabled = true; }
};

struct FakeAb {
  uint8_t ctrl1 = 0x12;
  time_t chip = 1750000000;
  bool timeOk = true, readOk = true, writeOk = true;
  int writes = 0, writeCalls = 0, failWriteCall = 0;  // failWriteCall: 1-based write call that fails
  bool wrtcBlocksRead = true;  // vendored getRtcAsTime fails while WRTC is set
  uint8_t lastReg = 0x10, readReg = 0x10;
  bool getRtcAsTime(time_t &t) { events.push_back("time"); if (!timeOk || (wrtcBlocksRead && (ctrl1 & 0x01))) return false; t = chip; return true; }
  bool readRegister(uint8_t r, uint8_t &v) { events.push_back("read"); readReg = r; if (!readOk) return false; v = ctrl1; return true; }
  bool writeRegister(uint8_t r, uint8_t v) { events.push_back("write"); lastReg = r; writeCalls++; if (!writeOk || writeCalls == failWriteCall) return false; if (r != 0x10) return true; ctrl1 = v; writes++; return true; }
};

#include "time/PreBindRtc.h"

static void reset() { events.clear(); fakeTime = FakeTime(); }

int main() {
  FakeWire w;
  w.enabled = true;

  // WRTC property over all 256 CONTROL1 values: set bit 0, clear bit 6, keep the rest.
  for (int v = 0; v < 256; v++) {
    FakeAb ab; ab.ctrl1 = (uint8_t)v; reset();
    CHECK(PreBindRtc::run(ab, w) && ab.ctrl1 == (uint8_t)((v | 0x01) & ~0x40) && ab.lastReg == 0x10 && ab.readReg == 0x10);
  }
  { FakeAb ab; ab.ctrl1 = 0x12; reset(); PreBindRtc::run(ab, w); CHECK(ab.ctrl1 == 0x13); }
  { FakeAb ab; ab.ctrl1 = 0x53; reset(); PreBindRtc::run(ab, w); CHECK(ab.ctrl1 == 0x13); }

  // Seed rule.
  { FakeAb ab; reset(); CHECK(PreBindRtc::run(ab, w) && fakeTime.valid && fakeTime.now_ == 1750000000); }
  { FakeAb ab; reset(); fakeTime.valid = true; fakeTime.now_ = 1760000000; PreBindRtc::run(ab, w); CHECK(fakeTime.now_ == 1760000000); }
  { FakeAb ab; ab.chip = 946684800; reset(); PreBindRtc::run(ab, w); CHECK(!fakeTime.valid); }  // year 2000
  { FakeAb ab; ab.chip = 0; reset(); PreBindRtc::run(ab, w); CHECK(!fakeTime.valid); }
  { FakeAb ab; ab.timeOk = false; reset(); CHECK(!PreBindRtc::run(ab, w) && !fakeTime.valid && ab.writes == 1 && ab.ctrl1 == 0x13); }  // time read failed: false, CONTROL1 still written

  // WRTC=1 with a valid chip time: WRTC cleared for the read, time seeded, CONTROL1 ends normalised.
  {
    FakeAb ab; ab.ctrl1 = 0x53; reset();
    CHECK(PreBindRtc::run(ab, w) && fakeTime.valid && fakeTime.now_ == 1750000000 && ab.ctrl1 == 0x13 && ab.writes == 2);
    const std::vector<std::string> want = {"lock", "read", "write", "time", "setTime", "write", "unlock"};
    CHECK(events == want);
  }
  // Retry: a failed bind leaves WRTC=1; the next unbound boot still seeds.
  { FakeAb ab; reset(); PreBindRtc::run(ab, w); fakeTime = FakeTime(); events.clear(); CHECK(PreBindRtc::run(ab, w) && fakeTime.valid && ab.ctrl1 == 0x13); }
  // WRTC=1 with an implausible chip time does not seed.
  { FakeAb ab; ab.ctrl1 = 0x13; ab.chip = 946684800; reset(); PreBindRtc::run(ab, w); CHECK(!fakeTime.valid && ab.ctrl1 == 0x13); }

  // Failures return false (the caller logs and continues); none throws or exits.
  { FakeAb ab; ab.readOk = false; reset(); CHECK(!PreBindRtc::run(ab, w) && ab.writes == 0); }
  { FakeAb ab; ab.writeOk = false; reset(); CHECK(!PreBindRtc::run(ab, w)); }
  { FakeAb ab; ab.ctrl1 = 0x13; ab.failWriteCall = 1; ab.wrtcBlocksRead = false; reset(); CHECK(!PreBindRtc::run(ab, w) && ab.ctrl1 == 0x13 && ab.writeCalls == 2); }  // clear fails: still restores
  { FakeAb ab; ab.ctrl1 = 0x13; ab.failWriteCall = 2; reset(); CHECK(!PreBindRtc::run(ab, w) && fakeTime.valid); }  // final write fails
  { FakeAb ab; ab.ctrl1 = 0x12; ab.failWriteCall = 1; reset(); CHECK(!PreBindRtc::run(ab, w) && ab.writeCalls == 1); }  // final write fails, no clear needed

  // Wire: lock held across the time read and the CONTROL1 read-modify-write; begin() only when not enabled.
  {
    FakeAb ab; FakeWire w2; reset();
    PreBindRtc::run(ab, w2);
    const std::vector<std::string> want = {"lock", "begin", "read", "time", "setTime", "write", "unlock"};
    CHECK(events == want);
  }
  {
    FakeAb ab; reset(); PreBindRtc::run(ab, w);
    CHECK(events.front() == "lock" && events.back() == "unlock" && events[1] == "read");
    int begins = 0; for (auto &e : events) begins += e == "begin";
    CHECK(begins == 0);
  }

  if (failures) { std::printf("%d failure(s)\n", failures); return 1; }
  std::printf("pre_bind_rtc_test: all passed\n");
  return 0;
}

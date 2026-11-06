// v50 S1: ExternalRtc helpers and the bind-once logic, compiled from the real src/time/ExternalRtc.h
// against tests/stubs/external_rtc/Particle.h. Not a bench result: the chip itself is untested here.
#include "time/ExternalRtc.h"
#include <cstdlib>

static int failures = 0;
#define CHECK(c) do { if (!(c)) { std::printf("FAIL line %d: %s\n", __LINE__, #c); failures++; } } while (0)

struct FakeRtc {
  bool bound_ = false;
  int enableResult = 0, enables = 0;
  particle::Am18x5Configuration stored, lastEnabled;
  particle::Status status() const { return particle::Status{bound_}; }
  int enable(const particle::Am18x5Configuration &c) {
    enables++;
    lastEnabled = c;
    if (enableResult) return enableResult;
    bound_ = true;
    stored = c;
    return 0;
  }
  particle::Am18x5Configuration getConfig() const { return stored; }
};

static particle::Am18x5Configuration fleet() {
  return particle::Am18x5Configuration().i2c(0).interruptPin(8).watchdogPin(5).capabilities(0x3);
}

int main() {
  FakeHal &h = fakeHal();

  time_t t = 0;
  h.chipTime = 1700000000;
  CHECK(ExternalRtc::get(t) && t == 1700000000 && h.lastSource == HAL_RTC_SOURCE_EXTERNAL);
  h.getResult = -1;
  t = 5;
  CHECK(!ExternalRtc::get(t) && t == 5);
  h.getResult = 0;

  CHECK(ExternalRtc::set(1750000000) && h.lastSource == HAL_RTC_SOURCE_EXTERNAL && h.lastUsec == 0);
  time_t back = 0;
  CHECK(ExternalRtc::get(back) && back == 1750000000);
  h.setResult = -3;
  CHECK(!ExternalRtc::set(1760000000) && h.chipTime == 1750000000);
  h.setResult = 0;

  // Read-back verification: the write only counts if the chip holds the time within the tolerance.
  CHECK(ExternalRtc::kWriteVerifyToleranceSec == 2);
  h.writeSkew = 2;
  CHECK(ExternalRtc::set(1750000000));
  h.writeSkew = -2;
  CHECK(ExternalRtc::set(1750000000));
  h.writeSkew = 3;
  CHECK(!ExternalRtc::set(1750000000) && h.sets > 0);
  h.writeSkew = -3;
  CHECK(!ExternalRtc::set(1750000000));
  h.writeSkew = 0;
  h.noStick = true;
  h.chipTime = 1700000000;
  CHECK(!ExternalRtc::set(1750000000));
  h.noStick = false;
  h.readbackResult = -5;
  CHECK(!ExternalRtc::set(1750000000));
  h.readbackResult = 0;
  h.setResult = -3;
  h.writeSkew = 0;
  CHECK(!ExternalRtc::set(1750000000));
  h.setResult = 0;
  CHECK(ExternalRtc::set(1750000000));

  h.sets = 0;
  fakeTime().valid = false;
  CHECK(!ExternalRtc::setFromSystem() && h.sets == 0);
  fakeTime().valid = true;
  CHECK(ExternalRtc::setFromSystem() && h.chipTime == fakeTime().now_);
  h.noStick = true;
  h.chipTime = 1;
  CHECK(!ExternalRtc::setFromSystem());
  h.noStick = false;

  h.valid = true;
  CHECK(ExternalRtc::isSet() && h.lastSource == HAL_RTC_SOURCE_EXTERNAL);
  h.valid = false;
  CHECK(!ExternalRtc::isSet());

  FakeRtc rtc;
  CHECK(ExternalRtc::ensureBound(rtc, fleet()) == 0 && rtc.enables == 1);
  CHECK(rtc.lastEnabled.wire == 0 && rtc.lastEnabled.intPin == 8 && rtc.lastEnabled.wdtPin == 5 && rtc.lastEnabled.caps == 0x3);
  CHECK(ExternalRtc::ensureBound(rtc, fleet()) == 0 && rtc.enables == 1);  // bound and matching: no re-bind

  FakeLog &log = fakeLog();
  int warns = log.warns;
  rtc.stored.intPin = 9;  // stored config differs
  CHECK(ExternalRtc::ensureBound(rtc, fleet()) == 0 && rtc.enables == 2 && log.warns == warns + 1);
  CHECK(rtc.stored.intPin == 8);

  rtc.stored.wdtPin = 4;  // watchdog pin differs
  CHECK(ExternalRtc::ensureBound(rtc, fleet()) == 0 && rtc.enables == 3);

  FakeRtc bad;
  bad.enableResult = -7;
  CHECK(ExternalRtc::ensureBound(bad, fleet()) == -7 && bad.enables == 1);  // failure is returned, never fatal

  FakeRtc missingCap;
  missingCap.bound_ = true;
  missingCap.stored = fleet().capabilities(0);
  CHECK(ExternalRtc::ensureBound(missingCap, fleet()) == 0 && missingCap.enables == 1);

  if (failures) return 1;
  std::printf("external_rtc_test: all checks passed\n");
  return 0;
}

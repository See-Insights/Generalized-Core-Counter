// WO-2026-10-09-002: the once-a-day config window (hold in
// Cloud::areLedgersSynced()). Part 1 of tests/config_window_hold_test.sh.
//
// The shell driver extracts the REAL Cloud::areLedgersSynced() body from
// src/cloud/LedgerClient.cpp, checks the copy byte-for-byte against the source
// (COPY_MISMATCH on any difference) and writes it to AREA_LEDGERS_SYNCED_H,
// which is #included below. Nothing here re-implements the gate: only the
// ledgers' lastSynced(), millis(), Particle.connected(),
// SystemConfig::get_lastConnection(), Clock::isTrusted(),
// DailyBoundary::todayAt() and hasPendingOutputLedgerSync() are stubbed.
//
// The function keeps its state in function-local statics, so every case runs
// in its own process (argv[1] = case name) and starts as a fresh boot.

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>

#ifndef AREA_LEDGERS_SYNCED_H
#error "AREA_LEDGERS_SYNCED_H must be injected"
#endif

#define ENABLE_LEDGER_TRACE 0

namespace ConnectivityPolicy {
constexpr unsigned long LEDGER_SYNC_TIMEOUT_MS = 10000UL;
}

namespace {
unsigned long fakeNowMs = 100000UL;
bool fakeConnected = false;
bool fakePending = false;
bool fakeTrusted = true;
time_t fakeLastConnection = 0;
time_t fakeTodayOpen = 0;
bool fakeHasConfigContent = false;
int holdEndLogs = 0;
std::string lastHoldEndReason;
} // namespace

unsigned long millis() { return fakeNowMs; }

struct LedgerData {
  bool has(const char *) const { return fakeHasConfigContent; }
};

struct Ledger {
  int64_t synced = 0;
  int64_t lastSynced() const { return synced; }
  LedgerData get() const { return LedgerData{}; }
};

namespace {
bool ledgerHasConfigContent(const LedgerData &ledger) { return ledger.has("sensor"); }

struct TestLog {
  void info(const char *fmt, const char *reason, unsigned long) {
    if (std::strstr(fmt, "ConfigHold") != nullptr) {
      holdEndLogs++;
      lastHoldEndReason = reason;
    }
  }
  template <typename... Args>
  void info(const char *, Args...) {}
  template <typename... Args>
  void warn(const char *, Args...) {}
};
TestLog Log;

struct ParticleStub {
  bool connected() const { return fakeConnected; }
};
ParticleStub Particle;
} // namespace

namespace SystemConfig {
bool get_verboseMode() { return false; }
time_t get_lastConnection() { return fakeLastConnection; }
uint8_t get_openTime() { return 6; }
} // namespace SystemConfig

namespace Clock {
bool isTrusted() { return fakeTrusted; }
} // namespace Clock

namespace DailyBoundary {
time_t todayAt(uint8_t) { return fakeTodayOpen; }
} // namespace DailyBoundary

class Cloud {
public:
  bool areLedgersSynced() const;
  bool hasPendingOutputLedgerSync() const { return fakePending; }
  Ledger defaultSettingsLedger;
  Ledger deviceSettingsLedger;
};

#include AREA_LEDGERS_SYNCED_H

namespace {

constexpr time_t DAY = 86400;
constexpr time_t E1 = 1800000000; // the first connection's epoch
constexpr unsigned long WINDOW = ConnectivityPolicy::LEDGER_SYNC_TIMEOUT_MS;

Cloud cloud;
int failures = 0;

void check(bool ok, const char *what) {
  if (!ok) {
    std::printf("FAIL: %s\n", what);
    failures++;
  }
}

bool call() { return cloud.areLedgersSynced(); }

// Advance the clock to `ms` after the case's t0 and call the gate.
unsigned long t0 = 0;
bool at(unsigned long ms) {
  fakeNowMs = t0 + ms;
  return call();
}

void warmLedgers() {
  cloud.defaultSettingsLedger.synced = 100;
  cloud.deviceSettingsLedger.synced = 100;
}

void connect(time_t epoch) {
  fakeConnected = true;
  fakeLastConnection = epoch;
  t0 = fakeNowMs;
}

void disconnect() {
  fakeConnected = false;
  call();
  fakeNowMs += 1000;
}

// A boot connection with no change pending that holds to the window and ends.
void completeHoldAt(time_t epoch) {
  connect(epoch);
  check(!at(0), "setup: the hold starts");
  check(at(WINDOW + 1), "setup: the hold ends at the window");
}

void bootTrusted() {
  fakeTrusted = true;
  fakeTodayOpen = E1 - 3600;
  warmLedgers();
  connect(E1);
  check(!at(0), "1: boot holds at gate entry although both ledgers are synced");
  check(!at(5000), "1: still holding with no input onSync");
  cloud.deviceSettingsLedger.synced = 200; // the input onSync
  check(at(6000), "1: returns true at once when an input lastSynced advances");
  check(holdEndLogs == 1 && lastHoldEndReason == "onSync", "1: one onSync log line");
  check(at(6001), "1: stays true after the hold");
}

void bootUntrusted() {
  fakeTrusted = false;
  fakeTodayOpen = 0;
  warmLedgers();
  connect(E1);
  check(!at(0), "2: an untrusted-clock boot still holds (ruling 4)");
  check(!at(9000), "2: still holding at 9 s");
  cloud.defaultSettingsLedger.synced = 150;
  check(at(9500), "2: ends on the input onSync");
}

void bootNoChange() {
  warmLedgers();
  fakeTodayOpen = E1 - 3600;
  connect(E1);
  check(!at(0), "3: holds");
  check(!at(WINDOW), "3: still holding at exactly the window");
  check(at(WINDOW + 1), "3: true once the window has passed");
  check(holdEndLogs == 1 && lastHoldEndReason == "timeout", "3: one timeout log line");
}

void anchor() {
  warmLedgers();
  fakeTodayOpen = E1 - 3600;
  connect(E1);
  fakePending = true;
  check(!at(0), "4: holds while the output is pending");
  check(!at(3000), "4: pending at 3 s");
  check(!at(7000), "4: pending at 7 s");
  fakePending = false; // the output clears at 7 s
  check(!at(12000), "4: 12 s from gate entry but only 5 s from the output clear: still holding");
  check(!at(WINDOW + 7000), "4: exactly the window after the clear: still holding");
  check(at(WINDOW + 7001), "4: true just after the window from the output clear");
}

void sameDayAfterCompleted() {
  warmLedgers();
  fakeTodayOpen = E1 - 3600;
  completeHoldAt(E1);
  disconnect();
  connect(E1 + 7200);
  check(at(0), "5: a second connection the same day returns true at once");
  check(holdEndLogs == 1, "5: no new hold log line");
}

void firstAfterOpen() {
  warmLedgers();
  fakeTodayOpen = E1 - 3600;
  completeHoldAt(E1);
  disconnect();
  fakeTodayOpen = E1 + DAY - 3600; // the next day's open
  connect(E1 + DAY);
  check(!at(0), "6: the first connection after today's open holds");
  check(at(WINDOW + 1), "6: and ends at the window");
  disconnect();
  connect(E1 + DAY + 7200);
  check(at(0), "6: the next connection that day does not hold");
}

void firstAfterOpenUntrusted() {
  warmLedgers();
  fakeTodayOpen = E1 - 3600;
  completeHoldAt(E1);
  disconnect();
  fakeTodayOpen = E1 + DAY - 3600;
  fakeTrusted = false;
  connect(E1 + DAY);
  check(at(0), "6b: with an untrusted clock there is no first-after-open hold");
}

void cutShort() {
  warmLedgers();
  fakeTodayOpen = E1 - 3600;
  connect(E1);
  check(!at(0), "7: boot holds");
  check(!at(3000), "7: cut short at 3 s");
  disconnect(); // teardown before the hold ends
  connect(E1 + 3600);
  check(!at(0), "7: the next connection holds again");
  check(holdEndLogs == 0, "7: no hold-end log for a cut-short hold");
  check(at(WINDOW + 1), "7: and it completes");
  disconnect();
  fakeTodayOpen = E1 + DAY - 3600;
  connect(E1 + DAY);
  check(!at(0), "7: next day's first connection holds");
  disconnect(); // cut short again
  connect(E1 + DAY + 3600);
  check(!at(0), "7: and holds again the same day after a cut-short daily hold");
}

void backwardClockUnequalBaselines() {
  cloud.defaultSettingsLedger.synced = 200;
  cloud.deviceSettingsLedger.synced = 100;
  fakeTodayOpen = E1 - 3600;
  connect(E1);
  check(!at(0), "10: holds with different baselines");
  check(!at(3000), "10: still holding with no change");
  cloud.deviceSettingsLedger.synced = 50; // a clock step: backwards, other ledger unchanged
  check(at(4000), "10: a backward move of one ledger ends the hold and returns true");
  check(holdEndLogs == 1 && lastHoldEndReason == "onSync", "10: one onSync log line");
}

void zeroConnectionEpoch() {
  fakeTrusted = false;
  warmLedgers();
  completeHoldAt(0);
  check(holdEndLogs == 1, "11: the first hold completed with lastConnection == 0");
  disconnect();
  connect(0);
  check(at(0), "11: the next connection does not pay the boot hold again");
  check(holdEndLogs == 1, "11: no new hold log line");
}

void warmOutsideHold() {
  warmLedgers();
  fakeTodayOpen = E1 - 3600;
  completeHoldAt(E1);
  disconnect();
  connect(E1 + 7200);
  check(at(0), "8a: both ledgers synced, outside a hold: true at once (4c2b734)");
  check(at(1), "8a: and again");
}

void partialOutsideHold() {
  warmLedgers();
  fakeTodayOpen = E1 - 3600;
  completeHoldAt(E1);
  disconnect();
  cloud.deviceSettingsLedger.synced = 0;
  fakeHasConfigContent = true;
  connect(E1 + 7200);
  check(!at(0), "8b: partial sync, inside the window: false");
  check(!at(WINDOW + 1), "8b: partial sync after the window with device content: false (alert 44 path)");
}

void emptyDeviceOutsideHold() {
  warmLedgers();
  fakeTodayOpen = E1 - 3600;
  completeHoldAt(E1);
  disconnect();
  cloud.deviceSettingsLedger.synced = 0;
  fakeHasConfigContent = false;
  connect(E1 + 7200);
  check(!at(0), "8c: default synced, device empty, inside the window: false");
  check(at(WINDOW + 1), "8c: after the window, device empty: true");
}

void coldBoot() {
  cloud.defaultSettingsLedger.synced = 0;
  cloud.deviceSettingsLedger.synced = 0;
  fakeTodayOpen = E1 - 3600;
  connect(E1);
  check(!at(0), "9: cold boot: false");
  check(at(WINDOW + 1), "9: cold boot: neither synced after the window is true (as before)");
}

} // namespace

int main(int argc, char **argv) {
  if (argc < 2) {
    std::printf("usage: %s <case>\n", argv[0]);
    return 2;
  }
  const std::string c = argv[1];
  if (c == "boot_trusted") bootTrusted();
  else if (c == "boot_untrusted") bootUntrusted();
  else if (c == "boot_no_change") bootNoChange();
  else if (c == "anchor") anchor();
  else if (c == "same_day") sameDayAfterCompleted();
  else if (c == "first_after_open") firstAfterOpen();
  else if (c == "first_after_open_untrusted") firstAfterOpenUntrusted();
  else if (c == "cut_short") cutShort();
  else if (c == "backward_clock_unequal") backwardClockUnequalBaselines();
  else if (c == "zero_connection_epoch") zeroConnectionEpoch();
  else if (c == "warm_outside_hold") warmOutsideHold();
  else if (c == "partial_outside_hold") partialOutsideHold();
  else if (c == "empty_device_outside_hold") emptyDeviceOutsideHold();
  else if (c == "cold_boot") coldBoot();
  else {
    std::printf("unknown case %s\n", c.c_str());
    return 2;
  }
  if (failures != 0) {
    std::printf("%s: %d check(s) failed\n", c.c_str(), failures);
    return 1;
  }
  std::printf("OK: %s\n", c.c_str());
  return 0;
}

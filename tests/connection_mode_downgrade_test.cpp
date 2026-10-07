// WO-2026-10-07-002 (Step 6 WO 1a) - the configured connection mode is never
// changed by a battery downgrade.
//
// Real code under test, linked or included verbatim:
//   - src/MyPersistentData.cpp          (the real sysStatus store and facades)
//   - src/power/BatteryAuthorityCommand.cpp (the real commit() decision)
//   - src/power/PowerManager.cpp        (the real effectiveConnectionMode())
//   - the connection-mode block of Cloud::applyModesConfig(), extracted
//     verbatim from src/cloud/ConfigApply.cpp by the .sh (that file builds on
//     Device OS LedgerData/Variant and cannot be compiled on the host).
//
// Host limit: StorageHelperRK::load() is a no-op, so "restart" is modelled by
// writing the persisted fields directly into the store, as load() would, and
// then reading with no help from any RAM state.

#include "Particle.h"
#include "StorageHelperRK.h"

#include "MyPersistentData.h"
#include "persist/PowerConfig.h"
#include "persist/SystemConfig.h"
#include "power/BatteryAuthority.h"
#include "power/PowerManager.h"

#include <iostream>
#include <vector>

namespace StorageHelperRK {
std::vector<FlushRecord> flushCalls;
std::vector<size_t> validateSizes;
int initializeCalls = 0;
}  // namespace StorageHelperRK

bool publishDiagnosticSafe(const char *, const char *, PublishFlags) { return true; }
bool isClockTrusted() { return true; }
namespace Clock {
bool isTimeValid() { return true; }
}

namespace {

struct Variant {};
int g_ledgerMode = 0;

bool getMergedIntValue(const Variant &, const Variant &, const char *, int &value) {
  value = g_ledgerMode;
  return true;
}

template <typename T>
bool validateRange(T value, T minimum, T maximum, const char *) {
  return value >= minimum && value <= maximum;
}

// Returns the `changed` flag the real block would report to applyModesConfig().
bool applyConnectionModeFromLedger(int ledgerMode) {
  g_ledgerMode = ledgerMode;
  Variant defaultModes;
  Variant deviceModes;
  int connectionMode = 0;
  bool changed = false;
  bool success = true;
#include "connection_mode_block.inc"
  (void)success;
  return changed;
}

int failures = 0;
#define CHECK(cond, msg)                                                       \
  do {                                                                         \
    if (!(cond)) {                                                             \
      std::cerr << "FAIL: " << (msg) << " (" #cond ")\n";                      \
      ++failures;                                                              \
    }                                                                          \
  } while (0)

constexpr uint8_t kConnected = SystemConfig::CONNECTED;
constexpr uint8_t kIntermittent = SystemConfig::INTERMITTENT;
constexpr uint8_t kKeepAlive = SystemConfig::INTERMITTENT_KEEP_ALIVE;

const char *kDisableLog = "Battery conservation: Disabling KEEP_ALIVE";
const char *kClearLog = "Battery recovery: clearing lowBatteryMode";
const char *kConfigLog = "Config: Connection mode ->";

void setStore(uint8_t sensorMode, uint8_t configuredMode, bool lowBatteryFlag, BatteryTier tier) {
  SystemConfig::set_sensorMode(sensorMode);
  SystemConfig::set_connectionMode(configuredMode);
  PowerConfig::set_lowBatteryMode(lowBatteryFlag);
  PowerConfig::set_currentBatteryTier(static_cast<uint8_t>(tier));
  Log.lines.clear();
}

void commitAt(BatteryTier tier) {
  const BatteryAuthority::Verdict verdict{tier, BatteryHealth::SocTrust::Trusted,
                                          SensorManager::VcellSampleState::Known};
  BatteryAuthority::commit(verdict, 50.0f);
}

uint8_t inUse() { return PowerManager::instance().effectiveConnectionMode(); }
uint8_t configured() { return SystemConfig::get_configuredConnectionMode(); }
bool flag() { return PowerConfig::get_lowBatteryMode(); }

void testDowngrade() {
  setStore(SystemConfig::OCCUPANCY, kKeepAlive, false, TIER_HEALTHY);
  CHECK(inUse() == kKeepAlive, "healthy: mode in use is the configured KEEP_ALIVE");

  commitAt(TIER_CONSERVING);
  CHECK(configured() == kKeepAlive, "downgrade leaves the configured mode at 3");
  CHECK(inUse() == kIntermittent, "downgrade: mode in use is INTERMITTENT");
  CHECK(flag(), "downgrade sets lowBatteryMode");
  CHECK(Log.count(kDisableLog) == 1, "downgrade logs once");

  commitAt(TIER_CONSERVING);
  commitAt(TIER_CRITICAL);
  CHECK(Log.count(kDisableLog) == 1, "later commits at CONSERVING or worse do not re-fire");
  CHECK(configured() == kKeepAlive && inUse() == kIntermittent && flag(),
        "state is unchanged by repeat commits");
}

void testRecovery() {
  setStore(SystemConfig::OCCUPANCY, kKeepAlive, false, TIER_HEALTHY);
  commitAt(TIER_CONSERVING);
  Log.lines.clear();

  commitAt(TIER_HEALTHY);
  CHECK(!flag(), "recovery clears lowBatteryMode");
  CHECK(inUse() == kKeepAlive, "recovery: mode in use is 3 again");
  CHECK(configured() == kKeepAlive, "recovery: the configured mode was never written");
  CHECK(Log.count("Battery recovery: restoring INTERMITTENT_KEEP_ALIVE") == 1, "recovery log string kept");
}

void testReapplyWhileDowngraded() {
  setStore(SystemConfig::OCCUPANCY, kKeepAlive, false, TIER_HEALTHY);
  commitAt(TIER_CONSERVING);
  Log.lines.clear();

  const bool changed = applyConnectionModeFromLedger(kKeepAlive);
  CHECK(!changed, "re-apply of an unchanged ledger value reports no change");
  CHECK(Log.count(kConfigLog) == 0, "re-apply writes nothing");
  CHECK(flag(), "re-apply leaves the downgrade flag set");
  CHECK(configured() == kKeepAlive && inUse() == kIntermittent, "re-apply: 3 configured, INTERMITTENT in use");

  commitAt(TIER_CONSERVING);
  CHECK(Log.count(kDisableLog) == 0, "the next commit does not flip-flop");
  CHECK(flag() && inUse() == kIntermittent, "still downgraded after the next commit");
}

void testLedgerChangeWhileDowngraded() {
  setStore(SystemConfig::OCCUPANCY, kKeepAlive, false, TIER_HEALTHY);
  commitAt(TIER_CONSERVING);
  Log.lines.clear();

  const bool changed = applyConnectionModeFromLedger(kIntermittent);
  CHECK(changed, "a changed ledger value is applied");
  CHECK(configured() == kIntermittent, "the configured mode becomes the ledger value (1)");
  CHECK(inUse() == kIntermittent, "mode in use is 1");

  commitAt(TIER_CONSERVING);
  CHECK(!flag(), "the flag clears at the next commit");
  CHECK(Log.count(kClearLog) == 1, "clear is logged once");
  CHECK(configured() == kIntermittent && inUse() == kIntermittent, "mode unchanged by the clear");

  setStore(SystemConfig::OCCUPANCY, kKeepAlive, false, TIER_HEALTHY);
  commitAt(TIER_CONSERVING);
  CHECK(applyConnectionModeFromLedger(kConnected), "ledger CONNECTED applied while downgraded");
  CHECK(configured() == kConnected && inUse() == kConnected, "CONNECTED is in use, not INTERMITTENT");
}

void testRestart() {
  setStore(SystemConfig::OCCUPANCY, kKeepAlive, true, TIER_CONSERVING);
  CHECK(inUse() == kIntermittent, "after reload, persisted 3 + flag gives INTERMITTENT in use");
  CHECK(configured() == kKeepAlive, "after reload, the configured mode is still 3");

  commitAt(TIER_CONSERVING);
  CHECK(Log.count(kDisableLog) == 0, "first commit after reload does not re-fire");
  commitAt(TIER_HEALTHY);
  CHECK(!flag() && inUse() == kKeepAlive, "recovery after reload restores 3");
}

void testCountingModeNeverDowngrades() {
  const BatteryTier tiers[] = {TIER_HEALTHY, TIER_CONSERVING, TIER_CRITICAL, TIER_SURVIVAL};
  for (BatteryTier tier : tiers) {
    setStore(SystemConfig::COUNTING, kKeepAlive, false, TIER_HEALTHY);
    commitAt(tier);
    CHECK(!flag(), "COUNTING: commit never sets the flag");
    CHECK(inUse() == kKeepAlive && configured() == kKeepAlive, "COUNTING: no downgrade at any tier");
  }

  setStore(SystemConfig::COUNTING, kKeepAlive, true, TIER_CONSERVING);
  CHECK(inUse() == kKeepAlive, "COUNTING: a stale flag does not change the mode in use");
  commitAt(TIER_CONSERVING);
  CHECK(!flag(), "COUNTING: a stale flag is cleared");
}

void testOtherModesAreNotDowngraded() {
  setStore(SystemConfig::OCCUPANCY, kConnected, true, TIER_CONSERVING);
  CHECK(inUse() == kConnected, "configured CONNECTED is never reported as INTERMITTENT");
  commitAt(TIER_CONSERVING);
  CHECK(!flag() && configured() == kConnected, "a flag with a non-KEEP_ALIVE configured mode is cleared");
}

void testMigrationFromOldOverwrite() {
  // Old firmware wrote INTERMITTENT over the configured value and set the flag.
  setStore(SystemConfig::OCCUPANCY, kIntermittent, true, TIER_CONSERVING);
  CHECK(inUse() == kIntermittent, "migration: before the first apply the stored 1 is in use");

  const bool changed = applyConnectionModeFromLedger(kKeepAlive);
  CHECK(changed, "migration: the first apply writes the ledger value");
  CHECK(configured() == kKeepAlive, "migration: configured is 3 after the first apply");
  CHECK(Log.count(kConfigLog) == 1, "migration: the write is logged once");

  setStore(SystemConfig::OCCUPANCY, kIntermittent, true, TIER_CONSERVING);
  applyConnectionModeFromLedger(kKeepAlive);
  commitAt(TIER_CONSERVING);
  CHECK(configured() == kKeepAlive, "migration: commit never rewrites the configured mode");
  CHECK(flag() && inUse() == kIntermittent, "migration: still downgraded while the tier is CONSERVING");
  commitAt(TIER_HEALTHY);
  CHECK(!flag() && inUse() == kKeepAlive, "migration: recovery lands on the ledger value 3");
}

}  // namespace

int main() {
  testDowngrade();
  testRecovery();
  testReapplyWhileDowngraded();
  testLedgerChangeWhileDowngraded();
  testRestart();
  testCountingModeNeverDowngrades();
  testOtherModesAreNotDowngraded();
  testMigrationFromOldOverwrite();

  if (failures != 0) {
    std::cerr << failures << " check(s) failed\n";
    return 1;
  }
  std::cout << "connection_mode_downgrade_test: all checks passed\n";
  return 0;
}

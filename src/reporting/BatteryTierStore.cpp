#include "reporting/BatteryTierStore.h"

#include "MyPersistentData.h"

namespace BatteryTierStore {

uint8_t currentBatteryTier() {
  return sysStatus.get_currentBatteryTier();
}

} // namespace BatteryTierStore

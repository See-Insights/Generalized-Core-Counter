#pragma once

#include <cstdint>

struct TestCurrentStatus {
  float socValue = 0.0f;
  uint8_t batteryStateValue = 0;
  mutable unsigned socReadCount = 0;
  mutable unsigned batteryStateReadCount = 0;

  float get_stateOfCharge() const {
    socReadCount++;
    return socValue;
  }

  uint8_t get_batteryState() const {
    batteryStateReadCount++;
    return batteryStateValue;
  }
};

struct TestSystemStatus {
  uint16_t connectAttemptBudgetSec = 0;
  uint8_t connectionAttemptCounter = 0;
  uint8_t currentBatteryTier = 0;
  uint8_t sensorMode = 0;
  uint8_t connectionMode = 0;
  bool lowBatteryMode = false;

  uint16_t get_connectAttemptBudgetSec() const { return connectAttemptBudgetSec; }
  uint8_t get_connectionAttemptCounter() const { return connectionAttemptCounter; }
  uint8_t get_currentBatteryTier() const { return currentBatteryTier; }
  uint8_t get_sensorMode() const { return sensorMode; }
  uint8_t get_configuredConnectionMode() const { return connectionMode; }
  bool get_lowBatteryMode() const { return lowBatteryMode; }
};

extern TestCurrentStatus testCurrent;
extern TestSystemStatus testSysStatus;


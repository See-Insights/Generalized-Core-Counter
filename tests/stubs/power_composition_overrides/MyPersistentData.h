#pragma once

#include <cstdint>

// Minimal host-side stand-in for MyPersistentData.h, scoped to what
// src/power/PowerPlatform.cpp needs to compile. Deliberately REPLICATES the
// real `current`/`sysStatus` macro definitions (WO-2026-08-25-001 Decision
// C5 host test) - PowerPlatform.cpp's applyDisableChargingBit() helper must
// not name any local `current`, because the real MyPersistentData.h
// #defines that identifier to `currentStatusData::instance()`. This stub
// keeps that hazard live on host so a regression would fail to compile here
// too, not only on-device.

struct TestCurrentStatus {
  float socValue = 0.0f;
};

struct TestSystemStatus {
  uint8_t currentBatteryTier = 0;
};

extern TestCurrentStatus testCurrent;
extern TestSystemStatus testSysStatus;

#define current testCurrent
#define sysStatus testSysStatus

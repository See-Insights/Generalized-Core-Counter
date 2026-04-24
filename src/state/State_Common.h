#pragma once

#include "Particle.h"
#include "MyPersistentData.h"
#include "state/StateHandlers.h"
#include "state/StateMachine.h"

// NOTE:
// This file was split from StateHandlers.cpp as a mechanical refactor.
// No behavioral changes were made.

// Forward declarations for helper functions shared across states
// These are implemented in logic-specific files but exposed here.

// Implemented in State_Idle.cpp
void ensureSensorEnabled(const char* context);

// Defined in Generalized-Core-Counter.cpp
extern bool publishDiagnosticSafe(const char* eventName, const char* data, PublishFlags flags);

// Defined in Generalized-Core-Counter.cpp
void dailyCleanup();

// Defined in Generalized-Core-Counter.cpp
void publishData();

// Defined in Generalized-Core-Counter.cpp
BatteryTier applyBatteryAwareConnectionModePolicy(float currentSoC);

// Defined in Generalized-Core-Counter.cpp
void setAppBreadcrumb(uint8_t code);

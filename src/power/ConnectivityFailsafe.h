#pragma once
#include "state/State_Common.h"
void connectivityFailsafeSupervisor();
constexpr uint32_t kPowerOffRecoverySec = 30; // the off time the vendored AB1805 call used (its default)
void powerOffForRecovery(); // timed POWER_OFF; returns only if Device OS did not cut power

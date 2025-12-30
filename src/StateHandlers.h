#pragma once

#include "StateMachine.h"

// Top-level state handlers used by the main loop switch
void handleIdleState();
void handleSleepingState();
void handleReportingState();
void handleConnectingState();
void handleFirmwareUpdateState();
void handleErrorState();

// Mode-specific handlers for COUNTING and OCCUPANCY
void handleCountingMode();
void handleOccupancyMode();
void updateOccupancyState();

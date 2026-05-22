#pragma once

#include "state/StateMachine.h"

/**
 * @file StateHandlers.h
 * @brief Public entry points for the main-loop state dispatch.
 */

/**
 * @brief Handles the IDLE_STATE wake-time decision and report scheduling logic.
 */
void handleIdleState();

/**
 * @brief Handles transition into low-power sleep and associated teardown work.
 */
void handleSleepingState();

/**
 * @brief Handles report preparation, queueing, and report-trigger decisions.
 */
void handleReportingState();

/**
 * @brief Handles network and cloud connection attempts plus the service window.
 */
void handleConnectingState();

/**
 * @brief Handles the bounded stay-awake window used for firmware updates.
 */
void handleFirmwareUpdateState();

/**
 * @brief Handles error-state containment and recovery backoff behavior.
 */
void handleErrorState();

/**
 * @brief Handles sensor processing when the device is configured for counting mode.
 */
void handleCountingMode();

/**
 * @brief Handles sensor processing when the device is configured for occupancy mode.
 */
void handleOccupancyMode();

/**
 * @brief Advances occupancy debounce and occupied or unoccupied transitions.
 */
void updateOccupancyState();

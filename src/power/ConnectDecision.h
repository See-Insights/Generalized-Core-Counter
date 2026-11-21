#pragma once
#include <stdint.h>

// WO-2026-11-19-001: the one place that decides whether a report connects. Pure: no Device OS calls, no globals, no logging.
namespace ConnectDecision {

enum class Action : uint8_t { CONNECT, WAIT };

struct Inputs {
  bool serviceRequest = false, occupancyChange = false, closeDue = false;  // map rows R2, R3, R4
  bool webhookHealthDue = false, cadenceDue = false;                       // R5, R9
  uint8_t effectiveMode = 0;        // SystemConfig::ConnectionMode
  uint8_t failureCount = 0;         // consecutive connect timeouts
  uint32_t msSinceLastFailure = 0;
};

struct Result {
  Action action;
  const char *reason;        // the StateReq reason
  uint32_t holdRemainingMs;  // non-zero only on "connect hold"
  const char *trigger;       // what asked for the connection ("none" when nothing did)
};

uint32_t holdMs(uint8_t failureCount);  // 1 h, 2 h, 4 h, then 6 h
Result decide(const Inputs &in);

} // namespace ConnectDecision

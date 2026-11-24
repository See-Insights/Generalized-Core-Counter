#include "power/ConnectDecision.h"
#include "persist/SystemConfig.h"
#include "power/ConnectivityPolicy.h"

namespace ConnectDecision {

uint32_t holdMs(uint8_t failureCount) {
  if (failureCount == 0) return 0;
  uint32_t ms = ConnectivityPolicy::HOLD_BASE_MS;
  for (uint8_t n = 1; n < failureCount && ms < ConnectivityPolicy::HOLD_MAX_MS; n++) ms <<= 1;
  return ms < ConnectivityPolicy::HOLD_MAX_MS ? ms : ConnectivityPolicy::HOLD_MAX_MS;
}

// Rules 2-7, the chain's order: first match wins. nullptr = nothing asks for a connection.
static const char *trigger(const Inputs &in) {
  if (in.serviceRequest) return "service request";
  if (in.occupancyChange) return "occupancy change";
  if (in.closeDue) return "daily close";
  if (in.webhookHealthDue) return "webhook health check";
  if (in.effectiveMode == SystemConfig::INTERMITTENT_KEEP_ALIVE) return "keep alive mode";
  if (in.cadenceDue) return "boundary aligned";
  return nullptr;
}

Result decide(const Inputs &in) {
  const char *why = trigger(in);
  // Rule 1, the hold after a failed connect: only a service request connects inside it.
  if (!in.serviceRequest && in.failureCount >= 1 && in.msSinceLastFailure < holdMs(in.failureCount)) {
    return {Action::WAIT, "connect hold", holdMs(in.failureCount) - in.msSinceLastFailure, why ? why : "none"};
  }
  if (why) return {Action::CONNECT, why, 0, why};
  return {Action::WAIT, "not aligned", 0, "none"};
}

} // namespace ConnectDecision

#include "power/ModemHealth.h"
#include "state/State_Common.h"
#include "observability/WakeCycleStats.h"

constexpr unsigned long MODEM_UNSTABLE_RECOVERY_TEARDOWN_MS = 5000UL;
constexpr uint8_t MODEM_UNSTABLE_CONNECT_TIMEOUT_THRESHOLD = 2;

void markModemUnstable(ModemUnstableReason reason, unsigned long elapsedMs) {
  const bool reasonChanged = !session.modemUnstable ||
                             session.modemUnstableReason != (uint8_t)reason;
  session.modemUnstable = true;
  session.modemUnstableReason = (uint8_t)reason;

  if (reasonChanged) {
    switch (reason) {
    case MODEM_UNSTABLE_REASON_SLOW_TEARDOWN:
      Log.warn("MODEM_HEALTH: unstable reason=slow_teardown elapsed=%lu", elapsedMs);
      break;
    case MODEM_UNSTABLE_REASON_CONNECT_TIMEOUT:
      Log.warn("MODEM_HEALTH: unstable reason=connect_timeout count=%u",
               (unsigned)session.modemConnectTimeoutCount);
      break;
    default:
      break;
    }
  }

  if (!session.modemStandbySuppressed) {
    Log.warn("MODEM_POLICY: standby temporarily disabled reason=unstable_modem");
    session.modemStandbySuppressed = true;
  }
}

void maybeClearModemUnstable(unsigned long teardownElapsedMs) {
  if (!session.modemUnstable) {
    return;
  }

  if (Observability::cycleStats().connect_result !=
          Observability::WakeCycleStats::ConnectResult::SUCCESS ||
      teardownElapsedMs > MODEM_UNSTABLE_RECOVERY_TEARDOWN_MS) {
    return;
  }

  session.modemUnstable = false;
  session.modemUnstableReason = MODEM_UNSTABLE_REASON_NONE;
  session.modemConnectTimeoutCount = 0;
  Log.info("MODEM_HEALTH: recovered");
  if (session.modemStandbySuppressed) {
    session.modemStandbySuppressed = false;
    Log.info("MODEM_POLICY: standby restored");
  }
}

void noteConnectTimeout() {
  if (session.modemConnectTimeoutCount < 0xFF) {
    session.modemConnectTimeoutCount++;
  }

  if (session.modemConnectTimeoutCount >= MODEM_UNSTABLE_CONNECT_TIMEOUT_THRESHOLD) {
    markModemUnstable(MODEM_UNSTABLE_REASON_CONNECT_TIMEOUT, 0);
  }
}

void noteConnectSuccess() {
  if (session.modemConnectTimeoutCount != 0) {
    session.modemConnectTimeoutCount = 0;
  }
}

void clearAtDailyClose() {
  if (session.modemUnstable) {
    session.modemUnstable = false;
    session.modemStandbySuppressed = false;
    session.modemUnstableReason = 0;
    session.modemConnectTimeoutCount = 0;
    Log.info("MODEM_POLICY: standby re-allowed at daily close; night modem-off decides");
  }
}

#include "state/SleepTeardown.h"
#include "state/State_Common.h"
#include "power/Connectivity.h"
#include "power/ModemHealth.h"
#include "persist/PowerConfig.h"
#include "cloud/BatteryBackoffPolicy.h"
#include "observability/WakeCycleStats.h"
#include "ThrashGuard.h"

namespace SleepTeardown {
namespace {
const char *connectionModeLabel(SystemConfig::ConnectionMode mode) {
  switch (mode) {
  case SystemConfig::CONNECTED:
    return "CONN";
  case SystemConfig::INTERMITTENT:
    return "INT";
  case SystemConfig::DISCONNECTED:
    return "DISC";
  case SystemConfig::INTERMITTENT_KEEP_ALIVE:
    return "IKA";
  default:
    return "?";
  }
}

const char *batteryTierLabel(uint8_t tier) {
  switch (tier) {
  case TIER_HEALTHY:
    return "H";
  case TIER_CONSERVING:
    return "C";
  case TIER_CRITICAL:
    return "CR";
  case TIER_SURVIVAL:
    return "S";
  default:
    return "?";
  }
}

void logTeardownContext(const char *prefix,
                       bool standbyRequested,
                       bool standbyEffective) {
  if (!SystemConfig::get_verboseMode()) {
    return;
  }

  Log.info("%s mode=%s standby=%d/%d tier=%s occ=%d",
           prefix,
           connectionModeLabel(static_cast<SystemConfig::ConnectionMode>(PowerManager::instance().effectiveConnectionMode())),
           standbyRequested ? 1 : 0,
           standbyEffective ? 1 : 0,
           batteryTierLabel(PowerConfig::get_currentBatteryTier()),
           CurrentReadings::get_occupied() ? 1 : 0);
}

// The nine teardown fields; the members below are the former inline code, so their bodies use the names unchanged.
struct Teardown {
  bool cloudDisconnectCompleteLogged = false, radioOffCompleteLogged = false, disconnectRequested = false;
  unsigned long disconnectRequestStartMs = 0, cloudDisconnectStartMs = 0, cloudDisconnectElapsedMs = 0,
                modemOffElapsedMs = 0, lastDisconnectProgressMs = 0, lastGateProgressMs = 0;

  void guardLostTimestamp() {
  // Safety: if disconnectRequested was latched but the timestamp got lost
  // (for example, due to an unexpected state-machine re-entry), reset so we
  // can actually issue the request instead of waiting forever.
  if (disconnectRequested && disconnectRequestStartMs == 0) {
    Log.warn("SLEEP: disconnectRequested latched without timestamp - resetting");
    disconnectRequested = false;
    cloudDisconnectStartMs = 0;
    cloudDisconnectElapsedMs = 0;
    modemOffElapsedMs = 0;
  }
  }
  Result run(bool useNetworkStandbyRequested, bool useNetworkStandbyEffective, bool &forceModemOffAfterStandbyFailure, int &standbySleepFailureError) {
  // If we're going to sleep with cellular network standby, do NOT wait for radio-off.
  // We only need to tear down the cloud session (quickly) before sleeping.
  bool stillOn;
#if Wiring_Cellular
  // WO-2026-10-04-001 item A2: Device OS has in-between modem states where
  // both isOn() and isOff() are false. System.sleep() then blocks up to 120 s
  // waiting for the modem, which is longer than the 60 s MCU watchdog, so the
  // non-standby gate tests !Cellular.isOff() instead of isOn().
  stillOn = useNetworkStandbyEffective ? Particle.connected() : (Particle.connected() || !Cellular.isOff());
#else
  // On WiFi platforms, waiting for WiFi.isOn() to flip false can take a long
  // time and is not required for safe low-power sleep. Only gate on cloud.
  stillOn = Particle.connected();
#endif

  // Compute disconnect budget once per loop to avoid duplicated logic.
  auto computeDisconnectBudgetMs = [&]() -> unsigned long {
    // Use ledger-configured budgets when available, with conservative defaults.
    uint16_t cloudBudgetSec = SystemConfig::get_cloudDisconnectBudgetSec();
    if (cloudBudgetSec < ConnectivityPolicy::DISCONNECT_BUDGET_MIN_SEC ||
        cloudBudgetSec > ConnectivityPolicy::DISCONNECT_BUDGET_MAX_SEC) {
      cloudBudgetSec = ConnectivityPolicy::DISCONNECT_CLOUD_DEFAULT_SEC;
    }

    uint16_t modemBudgetSec = SystemConfig::get_modemOffBudgetSec();
    if (modemBudgetSec < ConnectivityPolicy::DISCONNECT_BUDGET_MIN_SEC ||
        modemBudgetSec > ConnectivityPolicy::DISCONNECT_BUDGET_MAX_SEC) {
      modemBudgetSec = ConnectivityPolicy::DISCONNECT_MODEM_DEFAULT_SEC;
    }

    unsigned long budgetMs = (unsigned long)((modemBudgetSec > cloudBudgetSec) ? modemBudgetSec : cloudBudgetSec) * 1000UL;

    // When using network standby, shorten the disconnect grace period: we don't
    // want to burn connection budget waiting for a perfect teardown.
    if (useNetworkStandbyEffective) {
      budgetMs = (unsigned long)cloudBudgetSec * 1000UL;
      if (budgetMs > ConnectivityPolicy::DISCONNECT_STANDBY_MAX_MS) {
        budgetMs = ConnectivityPolicy::DISCONNECT_STANDBY_MAX_MS;
      }
    }
    return budgetMs;
  };

  unsigned long disconnectBudgetMs = computeDisconnectBudgetMs();

  // If cloud is already offline but the radio is still on (common after a transient
  // cloud drop), power down the radio before sleeping (unless using standby).
#if Wiring_Cellular
  if (!disconnectRequested && !useNetworkStandbyEffective && !Particle.connected() && Connectivity::isRadioPoweredOn()) {
    unsigned long requestMs = millis();
    cloudDisconnectCompleteLogged = true;
    cloudDisconnectElapsedMs = 0;
    if (SystemConfig::get_verboseMode()) {
      Log.info("SLEEP: cloud disconnect complete elapsed=0 ms");
    }
  #if HAL_PLATFORM_CELLULAR
    if (forceModemOffAfterStandbyFailure) {
      Log.warn("SLEEP: standby sleep failed; forcing modem-off reason=sleep_error err=%d",
               standbySleepFailureError);
      forceModemOffAfterStandbyFailure = false;
      standbySleepFailureError = SYSTEM_ERROR_NONE;
    }
  #endif
    logTeardownContext("SLEEP: modem-off start", useNetworkStandbyRequested, useNetworkStandbyEffective);
    Connectivity::requestRadioPowerOff();
    disconnectRequested = true;
    disconnectRequestStartMs = requestMs;
    Observability::cycleStats().markTeardownStart((uint32_t)disconnectRequestStartMs, useNetworkStandbyEffective);
    return Result::WAIT;
  }
#endif

  // Once cloud operations are satisfied (or timed out), initiate disconnect exactly once.
  if (Particle.connected() && !disconnectRequested) {
    unsigned long requestMs = millis();
    logTeardownContext("SLEEP: cloud disconnect start", useNetworkStandbyRequested, useNetworkStandbyEffective);
    cloudDisconnectStartMs = requestMs;
    cloudDisconnectElapsedMs = 0;
    if (useNetworkStandbyEffective) {
      Connectivity::requestCloudDisconnectOnly();
    } else {
      logTeardownContext("SLEEP: modem-off start", useNetworkStandbyRequested, useNetworkStandbyEffective);
      modemOffElapsedMs = 0;
      Connectivity::requestFullDisconnectAndRadioOff();
    }
    disconnectRequested = true;
    disconnectRequestStartMs = requestMs;

    // Observability: teardown window start.
    Observability::cycleStats().markTeardownStart((uint32_t)disconnectRequestStartMs, useNetworkStandbyEffective);

    return Result::WAIT;
  }

  // If disconnect was requested, wait (bounded) for it to take effect before sleeping.
  if (disconnectRequested && !cloudDisconnectCompleteLogged && !Particle.connected()) {
    cloudDisconnectElapsedMs = (cloudDisconnectStartMs == 0) ? 0UL : (millis() - cloudDisconnectStartMs);
    if (SystemConfig::get_verboseMode()) {
      Log.info("SLEEP: cloud disconnect complete elapsed=%lu ms standby=%d",
               cloudDisconnectElapsedMs,
               useNetworkStandbyEffective ? 1 : 0);
    }
    cloudDisconnectCompleteLogged = true;
  }
#if Wiring_Cellular
  if (disconnectRequested && !useNetworkStandbyEffective &&
      !radioOffCompleteLogged && Cellular.isOff()) {
    modemOffElapsedMs = (disconnectRequestStartMs == 0) ? 0UL : (millis() - disconnectRequestStartMs);
    if (SystemConfig::get_verboseMode()) {
      Log.info("SLEEP: modem-off complete elapsed=%lu ms", modemOffElapsedMs);
    }
    radioOffCompleteLogged = true;
  }
#endif

  if (disconnectRequested && stillOn) {
    if (disconnectRequestStartMs != 0 && (millis() - disconnectRequestStartMs) > disconnectBudgetMs) {
      Log.warn("SLEEP: disconnect/modem-off exceeded budget (%lu ms, cloud=%d radioOn=%d standby=%d) - raising alert 15",
               (unsigned long)(millis() - disconnectRequestStartMs),
               (int)Particle.connected(),
               (int)Connectivity::isRadioPoweredOn(),
               useNetworkStandbyEffective ? 1 : 0);
      markModemUnstable(MODEM_UNSTABLE_REASON_SLOW_TEARDOWN,
                        (unsigned long)(millis() - disconnectRequestStartMs));
      RecoveryState::raiseAlert(15);
      transitionTo(ERROR_STATE, "sleep-disconnect-timeout");
      disconnectRequested = false;
      disconnectRequestStartMs = 0;
      cloudDisconnectStartMs = 0;
      cloudDisconnectElapsedMs = 0;
      modemOffElapsedMs = 0;
      cloudDisconnectCompleteLogged = false;
    #if Wiring_Cellular
      radioOffCompleteLogged = false;
    #endif
      return Result::ERROR;
    }

    // Help DeviceOS make progress on the disconnect.
    // Throttle progress markers to avoid noisy per-loop updates.
    unsigned long nowMs = millis();
    if ((nowMs - lastDisconnectProgressMs) > 1000UL) {
      thrashGuard.markProgress("DISCONNECT_WAIT");
      lastDisconnectProgressMs = nowMs;
    }
    serviceAwakeWatchdog();
    Particle.process();
    return Result::WAIT;
  }

  // Observability: teardown completed (bounded wait finished and we are proceeding to sleep).
  if (disconnectRequested && !stillOn) {
    const unsigned long teardownElapsedMs =
        (disconnectRequestStartMs == 0) ? 0UL : (millis() - disconnectRequestStartMs);
    Log.info("Sleep: td=%lums cloud=%lums modem=%lums standby=%d/%d mode=%s tier=%s occ=%d",
             teardownElapsedMs,
             cloudDisconnectElapsedMs,
             modemOffElapsedMs,
             useNetworkStandbyRequested ? 1 : 0,
             useNetworkStandbyEffective ? 1 : 0,
         connectionModeLabel(static_cast<SystemConfig::ConnectionMode>(PowerManager::instance().effectiveConnectionMode())),
             batteryTierLabel(PowerConfig::get_currentBatteryTier()),
             CurrentReadings::get_occupied() ? 1 : 0);
    session.lastCloudDisconnectElapsedMs = cloudDisconnectElapsedMs;
    session.lastModemOffElapsedMs = modemOffElapsedMs;
    session.lastTotalTeardownElapsedMs = teardownElapsedMs;
    session.lastTeardownEndMs = millis();
    if (teardownElapsedMs > MODEM_UNSTABLE_SLOW_TEARDOWN_MS) {
      markModemUnstable(MODEM_UNSTABLE_REASON_SLOW_TEARDOWN, teardownElapsedMs);
    } else {
      maybeClearModemUnstable(teardownElapsedMs);
    }
    Observability::cycleStats().markTeardownEnd(millis());
    disconnectRequested = false;
    disconnectRequestStartMs = 0;
    cloudDisconnectStartMs = 0;
    cloudDisconnectElapsedMs = 0;
    modemOffElapsedMs = 0;
  }

  setAppBreadcrumb(22); // BREADCRUMB_SLEEP_GATE_START (was stale literal 18, colliding with BREADCRUMB_PUBLISH_QUEUE_EXIT)

  // Enforce sleep preconditions before any final sleep call in this state.
  // Non-standby cellular sleep requires both cloud disconnect and radio/modem off.
  // Standby sleep requires cloud disconnect only.
  auto sleepPreconditionsSatisfied = [&]() -> bool {
#if Wiring_Cellular
    if (!useNetworkStandbyEffective) {
      return !Particle.connected() && Cellular.isOff();
    }
#endif
    return !Particle.connected();
  };

  if (!sleepPreconditionsSatisfied()) {
    bool cloudConnected = Particle.connected();

    // Request teardown exactly once, then wait with existing budget handling.
    if (!disconnectRequested) {
      if (useNetworkStandbyEffective) {
        Log.warn("SLEEP: pre-sleep gate blocked (cloud=%d standby=1) - requesting disconnect",
                 (int)cloudConnected);
        logTeardownContext("SLEEP: cloud disconnect start", useNetworkStandbyRequested, useNetworkStandbyEffective);
        cloudDisconnectStartMs = millis();
        cloudDisconnectElapsedMs = 0;
        Connectivity::requestCloudDisconnectOnly();
      } else if (cloudConnected) {
        Log.warn("SLEEP: pre-sleep gate blocked (cloud=1 standby=0) - requesting disconnect + modem off");
        logTeardownContext("SLEEP: cloud disconnect start", useNetworkStandbyRequested, useNetworkStandbyEffective);
        logTeardownContext("SLEEP: modem-off start", useNetworkStandbyRequested, useNetworkStandbyEffective);
        cloudDisconnectStartMs = millis();
        cloudDisconnectElapsedMs = 0;
        modemOffElapsedMs = 0;
        Connectivity::requestFullDisconnectAndRadioOff();
      } else {
        Log.warn("SLEEP: pre-sleep gate blocked (cloud=0 radioOn=1 standby=0) - requesting modem off");
        cloudDisconnectCompleteLogged = true;
        cloudDisconnectElapsedMs = 0;
        if (SystemConfig::get_verboseMode()) {
          Log.info("SLEEP: cloud disconnect complete elapsed=0 ms");
        }
        logTeardownContext("SLEEP: modem-off start", useNetworkStandbyRequested, useNetworkStandbyEffective);
        modemOffElapsedMs = 0;
        Connectivity::requestRadioPowerOff();
      }

      disconnectRequested = true;
      disconnectRequestStartMs = millis();
      Observability::cycleStats().markTeardownStart((uint32_t)disconnectRequestStartMs, useNetworkStandbyEffective);
      return Result::WAIT;
    }

    if (disconnectRequestStartMs == 0) {
      disconnectRequestStartMs = millis();
    }

    unsigned long elapsedMs = millis() - disconnectRequestStartMs;
    if (elapsedMs > disconnectBudgetMs) {
      Log.warn("SLEEP: pre-sleep teardown exceeded budget (%lu ms, cloud=%d radioOn=%d standby=%d) - raising alert 15",
               elapsedMs,
               (int)Particle.connected(),
               (int)Connectivity::isRadioPoweredOn(),
               useNetworkStandbyEffective ? 1 : 0);
      markModemUnstable(MODEM_UNSTABLE_REASON_SLOW_TEARDOWN, elapsedMs);
      RecoveryState::raiseAlert(15);
      transitionTo(ERROR_STATE, "sleep-precondition-timeout");
      disconnectRequested = false;
      disconnectRequestStartMs = 0;
      cloudDisconnectStartMs = 0;
      cloudDisconnectElapsedMs = 0;
      modemOffElapsedMs = 0;
      cloudDisconnectCompleteLogged = false;
    #if Wiring_Cellular
      radioOffCompleteLogged = false;
    #endif
      return Result::ERROR;
    }

    unsigned long nowMs = millis();
    if ((nowMs - lastGateProgressMs) > 1000UL) {
      thrashGuard.markProgress("DISCONNECT_WAIT");
      lastGateProgressMs = nowMs;
    }
    serviceAwakeWatchdog();
    Particle.process();
    return Result::WAIT;
  }
  return Result::READY;
  }
} s;
} // namespace
void enter() { s = Teardown(); }
void guardLostTimestamp() { s.guardLostTimestamp(); }
bool disconnectRequested() { return s.disconnectRequested; }
Result run(bool a, bool b, bool &c, int &d) { return s.run(a, b, c, d); }
} // namespace SleepTeardown

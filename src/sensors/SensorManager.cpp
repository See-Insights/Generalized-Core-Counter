// Battery conect information -
// https://docs.particle.io/reference/device-os/firmware/boron/#batterystate-
const char *batteryContext[7] = {"Unknown",    "Not Charging", "Charging",
                                 "Charged",    "Discharging",  "Fault",
                                 "Disconnected"};

// Particle Functions
#include "sensors/SensorManager.h"
#include "MyPersistentData.h"  // Access sysStatus/sensorConfig
#include "observability/WakeCycleStats.h"
#include "power/Connectivity.h"
#include "power/PowerManager.h"
#include "power/PowerPlatform.h"
#include "power/PowerDiagnostics.h"
#include "power/ConnectivityPolicy.h"
#include "power/BatteryHealth.h"
#include "power/ChargeInhibit.h"
#include "power/ChargeInhibitPolicy.h"
#include "power/PmicFaultMonitor.h"
#include "state/StateMachine.h"
#include "sensors/BatteryAuthorityPolicy.h"
#include "sensors/SensorFactory.h"
#include "device_pinout.h"     // TMP36_SENSE_PIN for enclosure temperature
#include "PublishQueuePosixRK.h"

// Device-specific includes and definitions
// Use Particle feature detection for automatic platform identification

FuelGauge fuelGauge;

SensorManager *SensorManager::_instance;
retained uint16_t pmicAnomalyCount = 0;
retained uint32_t lastPmicAnomalyUptimeMs = 0;
retained time_t lastPmicAnomalyEpoch = 0;
retained float lastPmicAnomalySoc = 0.0f;
retained uint8_t lastPmicAnomalyChargeStatus = 0;
retained uint8_t lastPmicAnomalyPowerSource = 0;
retained uint8_t lastPmicAnomalyVbusStatus = 0;
bool pmicAnomalyActive = false;

uint32_t pmicAnomalyAgeSec() {
#if !defined(ENABLE_PMIC_FORENSICS) || !ENABLE_PMIC_FORENSICS
  return 0;
#else
  if (pmicAnomalyCount == 0) {
    return 0;
  }

  if (Time.isValid() && lastPmicAnomalyEpoch != 0 && Time.now() > lastPmicAnomalyEpoch) {
    return (uint32_t)(Time.now() - lastPmicAnomalyEpoch);
  }

  if (lastPmicAnomalyUptimeMs == 0) {
    return 0;
  }

  const uint32_t nowMs = millis();
  if (nowMs < lastPmicAnomalyUptimeMs) {
    return 0;
  }

  return (nowMs - lastPmicAnomalyUptimeMs) / 1000UL;
#endif
}

namespace {

#if defined(ENABLE_PMIC_FORENSICS) && ENABLE_PMIC_FORENSICS
constexpr uint8_t PMIC_ANOMALY_CONSECUTIVE_LIMIT = 3;
#endif

// BQ24195 PMIC REG09 fault register bit masks
// REG09[5:4] = CHRG_FAULT (charge fault status)
// REG09[3] = BAT_FAULT (battery fault status)
constexpr uint8_t PMIC_CHRG_FAULT_MASK = 0x30;  // Bits 5:4
constexpr uint8_t PMIC_BAT_FAULT_MASK = 0x08;   // Bit 3

#if HAL_PLATFORM_CELLULAR || PLATFORM_ID == PLATFORM_ARGON
bool batterySocIsValid(float soc) {
  return (soc == soc) && soc >= 0.0f && soc <= 100.0f;
}

bool batteryVoltageLooksUsable(float vcell) {
  return (vcell == vcell) && vcell > 2.5f && vcell < 5.0f;
}

bool batteryExtremeSocLooksSuspicious(float soc, float vcell) {
  if (!(vcell == vcell) || vcell <= 0.0f) {
    return false;
  }

  if (soc <= 0.0f && vcell > 3.60f) {
    return true;
  }

  if (soc >= 100.0f && vcell < 4.15f) {
    return true;
  }

  return false;
}

bool batterySampleLooksSuspicious(uint8_t battState, float soc, float vcell,
                                  bool ignoreUnknownBatteryState) {
  if (!batterySocIsValid(soc)) {
    return true;
  }

  if (batteryExtremeSocLooksSuspicious(soc, vcell)) {
    return true;
  }

  if (!ignoreUnknownBatteryState && battState == 0) {
    return true;
  }

  return false;
}

float estimateSocFromVoltage(float voltage) {
  float soc = (voltage - 3.0f) * (100.0f / (4.2f - 3.0f));
  if (soc < 0.0f) {
    soc = 0.0f;
  } else if (soc > 100.0f) {
    soc = 100.0f;
  }
  return soc;
}

#endif

const char *batterySampleContextPrefix(BatterySampleContext sampleContext) {
  switch (sampleContext) {
    case BatterySampleContext::Setup:
      return "Battery setup";
    case BatterySampleContext::PreSleep:
      return "Battery pre-sleep";
    case BatterySampleContext::PostWake:
      return "Battery post-wake";
    case BatterySampleContext::General:
    default:
      return "Battery sample";
  }
}

#if !(HAL_PLATFORM_CELLULAR && (PLATFORM_ID != PLATFORM_MSOM))
bool shouldLogBatterySample(BatterySampleContext sampleContext) {
  return sampleContext != BatterySampleContext::General;
}
#endif
#if HAL_PLATFORM_CELLULAR && (PLATFORM_ID != PLATFORM_MSOM)
uint8_t batteryStateFromPmicStatus(uint8_t chargeStatus, byte faultReg) {
  if (faultReg & (PMIC_CHRG_FAULT_MASK | PMIC_BAT_FAULT_MASK)) {
    return (uint8_t)PowerBatteryContext::Fault;
  }

  switch (chargeStatus) {
  case 1: // Pre-charge
  case 2: // Fast charging
    return (uint8_t)PowerBatteryContext::Charging;
  case 3: // Charge done
    return (uint8_t)PowerBatteryContext::Charged;
  case 0: // Not charging
  default:
    return (uint8_t)PowerBatteryContext::NotCharging;
  }
}

const char *compactPmicChargeLabel(uint8_t chargeStatus, byte faultReg) {
  if (faultReg & (PMIC_CHRG_FAULT_MASK | PMIC_BAT_FAULT_MASK)) {
    return "FAULT";
  }

  switch (chargeStatus) {
  case 1:
    return "PRE";
  case 2:
    return "FAST";
  case 3:
    return "DONE";
  case 0:
  default:
    return "NOT";
  }
}

#endif

} // namespace

#if HAL_PLATFORM_CELLULAR || PLATFORM_ID == PLATFORM_ARGON
// External linkage: also called from src/power/PmicFaultMonitor.cpp.
void boundedBatterySettleDelay(unsigned long delayMs) {
  const unsigned long startMs = millis();
  while ((millis() - startMs) < delayMs) {
    serviceAwakeWatchdog();
    Particle.process();
    const unsigned long elapsedMs = millis() - startMs;
    const unsigned long remainingMs = (elapsedMs < delayMs) ? (delayMs - elapsedMs) : 0UL;
    if (remainingMs == 0UL) {
      break;
    }
    delay((remainingMs > 20UL) ? 20UL : remainingMs);
  }
}
#endif

#if HAL_PLATFORM_CELLULAR && (PLATFORM_ID != PLATFORM_MSOM)
// External linkage: also called from src/power/PmicFaultMonitor.cpp.
unsigned long currentWakeAwakeMs() {
  const Observability::WakeCycleStats &stats = Observability::cycleStats();
  if (stats.total_awake_ms != 0) {
    return stats.total_awake_ms;
  }
  if (stats.wake_start_ms != 0) {
    const unsigned long nowMs = millis();
    return (nowMs >= stats.wake_start_ms) ? (nowMs - stats.wake_start_ms) : 0UL;
  }
  return 0UL;
}
#endif

// [static]
SensorManager &SensorManager::instance() {
  if (!_instance) {
    _instance = new SensorManager();
  }
  return *_instance;
}
SensorManager::SensorManager()
    : _sensor(nullptr),
      _lastPollTime(0),
      _batteryStabilizationPending(false),
  _firstBatterySampleTaken(false),
  _authoritativeBatterySampleActive(false),
  _authoritativeBatterySoc(0.0f),
  _authoritativeBatteryState(0),
  _authoritativeBatteryFallbackUsed(false),
  _cachedBatteryVcell(0.0f),
  _cachedChargeStateLabel("UNKNOWN"),
  _cachedBatteryVcellValid(false),
  _cachedBatteryVcellSampled(false) {}

SensorManager::~SensorManager() {}

bool SensorManager::cachedBatteryVoltage(float &vcell) const {
  if (!_cachedBatteryVcellValid) {
    return false;
  }

  vcell = _cachedBatteryVcell;
  return true;
}

SensorManager::VcellSampleState SensorManager::cachedBatteryVoltageState(float &vcell) const {
  // WO-2026-08-25-001 Amendment C, Decision C2 (AC-C6): distinguishes
  // "never sampled this boot" (Unavailable) from "sampled but implausible"
  // (Invalid) - both previously collapsed into the same `false` returned by
  // cachedBatteryVoltage(), which is why RuntimeReportingPolicy's guard
  // could not treat them differently.
  if (!_cachedBatteryVcellSampled) {
    vcell = 0.0f;
    return VcellSampleState::Unavailable;
  }
  if (!_cachedBatteryVcellValid) {
    vcell = _cachedBatteryVcell;
    return VcellSampleState::Invalid;
  }
  vcell = _cachedBatteryVcell;
  return VcellSampleState::Known;
}

const char *SensorManager::cachedChargeStateLabel() const {
  return _cachedChargeStateLabel;
}

BatteryHealth::SocTrust SensorManager::cachedSocTrust() const {
  return _cachedSocTrust;
}

bool SensorManager::chargeDisableConfigVerified() const {
  return _chargeDisableConfigVerified;
}

void SensorManager::setup() {
    if (!_sensor) {
        Log.error("No sensor assigned! Call setSensor() first.");
        return;
    }
    
    if (!_sensor->setup()) {
        Log.error("Sensor setup failed");
    }
}

void SensorManager::setSensor(ISensor* sensor) {
    if (sensor) {
        _sensor = sensor;
    } else {
        Log.error("Attempted to set null sensor");
    }
}

  void SensorManager::initializeFromConfig() {
    SensorType sensorType = static_cast<SensorType>(sysStatus.get_sensorType());
    ISensor* sensor = SensorFactory::createSensor(sensorType);

    if (!sensor) {
      Log.error("SensorFactory failed for type %d", (int)sensorType);
      _sensor = nullptr;
      return;
    }

    setSensor(sensor);

    if (!_sensor->initializeHardware()) {
      Log.error("Sensor hardware initialization failed for type %d", (int)sensorType);
    } else {
      Log.info("Sensor: %s ready=%d irq=%d",
               _sensor->getSensorType(),
               _sensor->isReady() ? 1 : 0,
               _sensor->usesInterrupt() ? 1 : 0);
    }
  }

bool SensorManager::loop() {
    if (!_sensor || !_sensor->isReady()) {
        return false;
    }
    
    // V3.23: Most sensors (like PIR) are interrupt-driven and should be 
    // serviced on every pass through the main loop.
    // Future polling sensors would use sensor.setting2 or setting3 for polling interval.
    if (_sensor->usesInterrupt()) {
        bool event = _sensor->loop();
        if (event && sysStatus.get_verboseMode()) {
            Log.info("SensorManager: event reported by interrupt-driven sensor");
        }
        return event;
    }
    
    // Polling mode - for future sensors that don't use interrupts
    // Polling interval would come from sensor.setting2 (in milliseconds)
    unsigned long currentTime = millis();
    uint32_t pollingInterval = sensorConfig.get_sensorSetting2(); // Polling interval in ms
    
    if (pollingInterval == 0) {
        // No polling interval set, check every loop
        return _sensor->loop();
    }
    
    if (currentTime - _lastPollTime >= pollingInterval) {
        _lastPollTime = currentTime;
        return _sensor->loop();
    }
    
    return false;
}

SensorData SensorManager::getSensorData() const {
    if (_sensor) {
        return _sensor->getData();
    }
    return SensorData();
}

bool SensorManager::isSensorReady() const {
    return _sensor && _sensor->isReady();
}

void SensorManager::onEnterSleep() {
  if (_sensor) {
    _sensor->onSleep();
    return;
  }

  // If a concrete sensor hasn't been initialized (common when booting while
  // outside open hours), still force the carrier sensor power rails off.
  pinMode(disableModule, OUTPUT);
  pinMode(ledPower, OUTPUT);
  digitalWrite(disableModule, HIGH); // active-low enable
  digitalWrite(ledPower, HIGH);      // active-low LED power
}

void SensorManager::onExitSleep() {
  if (_sensor) {
    if (!_sensor->onWake()) {
      Log.error("Sensor %s failed to wake correctly", _sensor->getSensorType());
    }
  }
}

void SensorManager::noteWakeFromLowPowerSleep() {
  _batteryStabilizationPending = true;
  _authoritativeBatterySampleActive = false;
  _authoritativeBatterySoc = 0.0f;
  _authoritativeBatteryState = 0;
  _authoritativeBatteryFallbackUsed = false;
}

float SensorManager::tmp36TemperatureC(int adcValue) {
  // Analog inputs have values from 0-4095, or
  // 12-bit precision. 0 = 0V, 4095 = 3.3V, 0.0008 volts (0.8 mV) per unit
  // The temperature sensor docs use millivolts (mV), so use 3300 as the factor
  // instead of 3.3.
  float mV = ((float)adcValue) * 3300 / 4095;

  // According to the TMP36 docs:
  // Offset voltage 500 mV, scaling 10 mV/deg C, output voltage at 25C = 750 mV
  // (77F) The offset voltage is subtracted from the actual voltage, allowing
  // negative temperatures with positive voltages.

  // Example value=969 mV=780.7 tempC=28.06884765625 tempF=82.52392578125

  // With the TMP36, with the flat side facing you, the pins are:
  // Vcc | Analog Out | Ground
  // You must put a 0.1 uF capacitor between the analog output and ground or
  // you'll get crazy inaccurate values!
  return (mV - 500) / 10;
}

bool SensorManager::readTmp112TemperatureC(float &tempC) {
  // TMP112A default 7-bit I2C address is 0x48.
  // Allow override at compile time for unusual board strapping.
#if defined(MUON_TMP112_I2C_ADDR)
  const uint8_t addr = (uint8_t)MUON_TMP112_I2C_ADDR;
#else
  const uint8_t addr = 0x48;
#endif

  // Temperature register pointer is 0x00.
  const uint8_t tempReg = 0x00;

  // Guard against interference with other I2C users (AB1805, FRAM, etc.).
  Wire.lock();

  Wire.beginTransmission(addr);
  Wire.write(tempReg);
  int status = Wire.endTransmission(false);
  if (status != 0) {
    Wire.unlock();
    return false;
  }

  const uint8_t toRead = 2;
  (void)Wire.requestFrom((int)addr, (int)toRead);
  if (Wire.available() < toRead) {
    Wire.unlock();
    return false;
  }

  uint8_t msb = (uint8_t)Wire.read();
  uint8_t lsb = (uint8_t)Wire.read();
  Wire.unlock();

  // TMP112A temperature is a signed 12-bit value left-justified in 16 bits.
  // Resolution is 0.0625 C per LSB.
  int16_t raw = (int16_t)((((uint16_t)msb) << 8) | (uint16_t)lsb);
  raw = (int16_t)(raw >> 4);
  // Sign-extend the 12-bit value (bit 11 is the sign after shifting).
  if (raw & 0x0800) {
    raw = (int16_t)(raw | 0xF000);
  }

  tempC = ((float)raw) * 0.0625f;
  return true;
}

namespace {

bool probeTmp112Present(uint8_t addr) {
  // Probe device presence without changing its configuration.
  Wire.lock();
  Wire.beginTransmission(addr);
  int status = Wire.endTransmission();
  Wire.unlock();
  return status == 0;
}

} // namespace

bool SensorManager::batteryState(BatterySampleContext sampleContext) {
#if HAL_PLATFORM_CELLULAR || PLATFORM_ID == PLATFORM_ARGON
  // Boron (cellular) and Argon (Wi-Fi) Gen 3 devices.
  bool fallbackUsed = false;
  bool previousKnownGoodUsed = false;
  uint8_t retryCount = 0;
  bool shouldConsiderStabilization =
      (sampleContext == BatterySampleContext::Setup) || _batteryStabilizationPending;
  if (!_firstBatterySampleTaken && System.resetReason() == RESET_REASON_POWER_MANAGEMENT) {
    shouldConsiderStabilization = true;
  }
  const float previousKnownGoodSoc = current.get_stateOfCharge();
  const bool previousKnownGoodSocValid = batterySocIsValid(previousKnownGoodSoc);

  auto readBatterySample = [&](uint8_t &battState,
                               float &rawFuelGaugeSoc,
                               float &normalizedSoc,
                               float &soc,
                               float &vcell,
                               int &powerSource,
                               const char *&socAuthorityTag,
                               bool &ignoreUnknownBatteryState) {
                #if HAL_PLATFORM_CELLULAR
    battState = System.batteryState();
                #else
                  battState = 0;
                #endif

  #if HAL_PLATFORM_CELLULAR
    powerSource = System.powerSource();
    PowerPlatform::noteObservedPowerSource(powerSource);
  #else
    powerSource = PowerPlatform::readPowerSource().source;
  #endif

#if HAL_PLATFORM_FUELGAUGE_MAX17043
    rawFuelGaugeSoc = fuelGauge.getSoC();
    normalizedSoc = fuelGauge.getNormalizedSoC();
    vcell = fuelGauge.getVCell();
    ignoreUnknownBatteryState = true;

    if (batterySocIsValid(rawFuelGaugeSoc)) {
      soc = rawFuelGaugeSoc;
      socAuthorityTag = "fuelgauge-raw";
    } else if (batterySocIsValid(normalizedSoc)) {
      soc = normalizedSoc;
      socAuthorityTag = "fuelgauge-normalized";
    } else {
      soc = rawFuelGaugeSoc;
      socAuthorityTag = "fuelgauge-raw";
    }
#else
    normalizedSoc = -1.0f;

  #if HAL_PLATFORM_CELLULAR
    rawFuelGaugeSoc = System.batteryCharge();
  #else
    rawFuelGaugeSoc = fuelGauge.getSoC();
  #endif

    soc = rawFuelGaugeSoc;
    vcell = fuelGauge.getVCell();
    socAuthorityTag = "system-batterycharge";
    ignoreUnknownBatteryState = false;
#endif
  };

  uint8_t battState = 0;
  float normalizedSoc = -1.0f;
  float rawFuelGaugeSoc = -1.0f;
  float soc = 0.0f;
  float vcell = 0.0f;
  int powerSource = 0;
  const char *socAuthorityTag = "system-batterycharge";
  bool ignoreUnknownBatteryState = false;

  readBatterySample(battState, rawFuelGaugeSoc, normalizedSoc, soc, vcell, powerSource,
                    socAuthorityTag, ignoreUnknownBatteryState);

  const float beforeStabilizationSoc = soc;
  bool shouldStabilize = false;
  const char *stabilizationReason = "none";

#if HAL_PLATFORM_FUELGAUGE_MAX17043
  if (shouldConsiderStabilization) {
    if (!batterySocIsValid(rawFuelGaugeSoc)) {
      shouldStabilize = true;
      stabilizationReason = "raw-invalid";
    } else if (batterySampleLooksSuspicious(battState, soc, vcell, ignoreUnknownBatteryState)) {
      shouldStabilize = true;
      stabilizationReason = "selected-suspicious";
    }
  }
#else
  if (shouldConsiderStabilization &&
      batterySampleLooksSuspicious(battState, soc, vcell, ignoreUnknownBatteryState)) {
    shouldStabilize = true;
    stabilizationReason = "selected-suspicious";
  }
#endif

  if (shouldStabilize) {
    fuelGauge.wakeup();
    boundedBatterySettleDelay(ConnectivityPolicy::BATTERY_WAKE_QUICKSTART_DELAY_MS);
    fuelGauge.quickStart();
    boundedBatterySettleDelay(ConnectivityPolicy::BATTERY_WAKE_QUICKSTART_DELAY_MS);

    readBatterySample(battState, rawFuelGaugeSoc, normalizedSoc, soc, vcell, powerSource,
                      socAuthorityTag, ignoreUnknownBatteryState);

    while (batterySampleLooksSuspicious(battState, soc, vcell, ignoreUnknownBatteryState) &&
           retryCount < ConnectivityPolicy::BATTERY_WAKE_MAX_RETRIES) {
      retryCount++;
      delay(ConnectivityPolicy::BATTERY_WAKE_RETRY_DELAY_MS);
      readBatterySample(battState, rawFuelGaugeSoc, normalizedSoc, soc, vcell, powerSource,
                        socAuthorityTag, ignoreUnknownBatteryState);
    }
  }

  if (batterySampleLooksSuspicious(battState, soc, vcell, ignoreUnknownBatteryState)) {
    if (batteryVoltageLooksUsable(vcell)) {
      socAuthorityTag = "voltage-fallback";
      soc = estimateSocFromVoltage(vcell);
      fallbackUsed = true;
    } else if (previousKnownGoodSocValid) {
      soc = previousKnownGoodSoc;
      previousKnownGoodUsed = true;
      socAuthorityTag = "previous-known-good";
    }
  }

  if (shouldStabilize) {
    Log.info("Battery stabilization: reason=%s before=%.2f after=%.2f raw=%.2f norm=%.2f vcell=%.3f",
             stabilizationReason,
             (double)beforeStabilizationSoc,
             (double)soc,
             (double)rawFuelGaugeSoc,
             (double)normalizedSoc,
             (double)vcell);
  }

  _batteryStabilizationPending = false;
  _firstBatterySampleTaken = true;

  const bool sampleIsPreRadio = !Particle.connected() && !Connectivity::isRadioPoweredOn();
  const bool sampleCanBeAuthoritative = sampleIsPreRadio &&
      (fallbackUsed || previousKnownGoodUsed ||
       !batterySampleLooksSuspicious(battState, soc, vcell, ignoreUnknownBatteryState));

  if (sampleCanBeAuthoritative) {
    _authoritativeBatterySampleActive = true;
    _authoritativeBatterySoc = soc;
    _authoritativeBatteryState = battState;
    _authoritativeBatteryFallbackUsed = fallbackUsed || previousKnownGoodUsed;
  }

  bool rejectAuthoritativeOverwrite = false;
  float authoritativeDelta = 0.0f;
    if (BatteryAuthorityPolicy::shouldEvaluatePostConnectDelta(
      shouldStabilize,
      _authoritativeBatterySampleActive,
      Particle.connected(),
      Connectivity::isRadioPoweredOn())) {
    const bool suspiciousCandidate = batterySampleLooksSuspicious(battState, soc, vcell, ignoreUnknownBatteryState);
    authoritativeDelta = BatteryAuthorityPolicy::batterySampleDelta(_authoritativeBatterySoc, soc);
    const bool unrealisticDelta =
      BatteryAuthorityPolicy::batterySampleHasUnrealisticDelta(_authoritativeBatterySoc, soc);
    const bool uncorroboratedIncrease =
      BatteryAuthorityPolicy::batterySampleHasUncorroboratedIncrease(
        _authoritativeBatterySoc, soc, vcell);
    const bool authoritativeFallbackLock = _authoritativeBatteryFallbackUsed;
    if (authoritativeFallbackLock || suspiciousCandidate || unrealisticDelta || uncorroboratedIncrease) {
      rejectAuthoritativeOverwrite = true;
      const char *rejectionReason = authoritativeFallbackLock ? "fallback-lock" :
          (suspiciousCandidate ? "suspicious" :
           (unrealisticDelta ? "sag-decrease" : "increase-not-vcell-corroborated"));
      Log.warn("Battery post-connect sample ignored: preRadio=%.1f postConnect=%.1f delta=%.1f",
               (double)_authoritativeBatterySoc,
               (double)soc,
               (double)authoritativeDelta);
      Log.warn("Battery authority: keeping pre-radio sample SoC=%.2f%% state=%s (%d)%s; rejecting later sample SoC=%.2f%% state=%s (%d) powerSource=%d suspicious=%s fallbackLock=%s delta=%.2f reason=%s",
               (double)_authoritativeBatterySoc,
               batteryContext[(_authoritativeBatteryState <= 6) ? _authoritativeBatteryState : 0],
               _authoritativeBatteryState,
               _authoritativeBatteryFallbackUsed ? " fallback=true" : "",
               (double)soc,
               batteryContext[(battState <= 6) ? battState : 0],
               battState,
               powerSource,
               suspiciousCandidate ? "true" : "false",
               authoritativeFallbackLock ? "true" : "false",
               (double)authoritativeDelta,
               rejectionReason);
    }
  }

  const char *authorityTag = socAuthorityTag;
  if (rejectAuthoritativeOverwrite) {
    authorityTag = "ignored-post-connect";
  }

  const bool logBatteryDetail = sysStatus.get_verboseMode() || shouldStabilize ||
      fallbackUsed || previousKnownGoodUsed || rejectAuthoritativeOverwrite;
  const float loggedSoc = rejectAuthoritativeOverwrite ? _authoritativeBatterySoc : soc;
  // Captured now (before vcell can be reassigned later) so the detail log
  // below reflects one consistent sample.
  const float loggedVcell = vcell;

#if !(HAL_PLATFORM_CELLULAR && (PLATFORM_ID != PLATFORM_MSOM))
  const uint8_t loggedBattState = rejectAuthoritativeOverwrite ? _authoritativeBatteryState : battState;
  const bool allowPreSleepBatteryLog =
      (sampleContext != BatterySampleContext::PreSleep) || (ENABLE_SLEEP_TRACE != 0);
  if (logBatteryDetail && allowPreSleepBatteryLog) {
    Log.info("%s: soc=%.2f raw=%.2f norm=%.2f vcell=%.3f authority=%s state=%s(%d) power=%d%s",
             batterySampleContextPrefix(sampleContext),
             (double)loggedSoc,
             (double)rawFuelGaugeSoc,
             (double)normalizedSoc,
             (double)vcell,
             authorityTag,
             batteryContext[(loggedBattState <= 6) ? loggedBattState : 0],
             loggedBattState,
             powerSource,
             rejectAuthoritativeOverwrite ? " candidateRejected=true" : "");
  }

  if (!rejectAuthoritativeOverwrite) {
    current.set_batteryState(battState);
    current.set_stateOfCharge(soc);
  }
#endif

#if HAL_PLATFORM_CELLULAR && (PLATFORM_ID != PLATFORM_MSOM)
  // refreshInputProfile() logs its own PowerDiag[...] line with this same
  // snapshot, so no separate log call is needed here.
  PowerManager::instance().refreshInputProfile();

  // WO-2026-08-25-001 Amendment C, Decision C1 (boot-ordering corollary):
  // this MUST run after refreshInputProfile() (so ChargeInhibit::apply()
  // sees this cycle's activeInputProfile) and BEFORE
  // PmicFaultMonitor::pollAndRemediate() (so `safeToCharge` reflects a
  // real measurement taken THIS call, not stale state left over from the
  // previous call or an unmeasured, persisted value). This is the single
  // production call site for this platform build - see
  // measureTemperatureAndApplyChargeDecision()'s header comment.
  bool safeToCharge = measureTemperatureAndApplyChargeDecision();

  // PMIC fault monitoring/remediation/telemetry (BQ24195, Boron only). See
  // PmicFaultMonitor.h for the contract (fault detection, escalating
  // remediation, alert codes 20/21/23, and the ChargeDiag forensic line).
  PMIC pmic(true); // true = lock I2C during operations
  PmicFaultMonitor::Registers pmicRegs =
      PmicFaultMonitor::pollAndRemediate(pmic, safeToCharge, battState, powerSource,
                                          chargeDisableConfigVerified());
  byte faultReg = pmicRegs.faultReg;
  uint8_t vbusStatus = pmicRegs.vbusStatus;
  uint8_t chargeStatus = pmicRegs.chargeStatus;
  bool powerGood = pmicRegs.powerGood;
  const PowerReport &powerReport = PowerManager::instance().latestReport();
  PmicFaultMonitor::logChargeDiag(pmicRegs, vcell, loggedSoc, powerSource);

#if defined(ENABLE_PMIC_FORENSICS) && ENABLE_PMIC_FORENSICS
  static uint8_t consecutivePmicContradictions = 0;
#endif
  const uint8_t systemBattState = battState;
  const uint8_t pmicBattState = batteryStateFromPmicStatus(chargeStatus, faultReg);
  _cachedBatteryVcell = vcell;
  _cachedBatteryVcellValid = batteryVoltageLooksUsable(vcell);
  _cachedBatteryVcellSampled = true; // a real vcell sample was taken this call, valid or not
  _cachedChargeStateLabel = compactPmicChargeLabel(chargeStatus, faultReg);
  if (sampleCanBeAuthoritative) {
    _authoritativeBatteryState = pmicBattState;
  }
  current.set_batteryState(pmicBattState);
  // Blocker 2 (WO-2026-08-25-001 round 3, AC-B4): the retired STALE_SOC
  // resync machinery's ResyncActions::commitSoc() was the only path that
  // committed an accepted Boron fuel-gauge sample to current.stateOfCharge.
  // Retiring the resync/latch/retry machinery (WO line 620) must not also
  // retire this ordinary commit - it is not stale-SOC correction, just the
  // ordinary "accepted sample updates persisted SOC" behavior. Gated on the
  // same authority fence as before (rejectAuthoritativeOverwrite): an
  // accepted sample commits; a rejected post-connect candidate does not
  // overwrite the pre-radio authoritative value.
  if (!rejectAuthoritativeOverwrite) {
    current.set_stateOfCharge(soc);
  }
  float finalAcceptedSoc = current.get_stateOfCharge();

#if defined(ENABLE_PMIC_FORENSICS) && ENABLE_PMIC_FORENSICS
  // Instrument contradictory PMIC state without changing charging behavior.
  const float effectiveSoc = rejectAuthoritativeOverwrite ? _authoritativeBatterySoc : soc;
  const bool pmicStateContradiction =
      safeToCharge &&
      batterySocIsValid(effectiveSoc) &&
      effectiveSoc < 20.0f &&
      (pmicBattState == (uint8_t)PowerBatteryContext::Charged ||
       pmicBattState == (uint8_t)PowerBatteryContext::NotCharging);

  if (pmicStateContradiction) {
    if (consecutivePmicContradictions < 0xFF) {
      consecutivePmicContradictions++;
    }
  } else {
    consecutivePmicContradictions = 0;
  }

  const bool pmicAnomalyNowActive =
      consecutivePmicContradictions >= PMIC_ANOMALY_CONSECUTIVE_LIMIT;
  const bool pmicAnomalyBecameActive = pmicAnomalyNowActive && !pmicAnomalyActive;
  pmicAnomalyActive = pmicAnomalyNowActive;

  if (pmicAnomalyBecameActive) {
    if (pmicAnomalyCount < 0xFFFF) {
      pmicAnomalyCount++;
    }
    lastPmicAnomalyUptimeMs = millis();
    lastPmicAnomalyEpoch = Time.isValid() ? Time.now() : 0;
    lastPmicAnomalySoc = effectiveSoc;
    lastPmicAnomalyChargeStatus = pmicBattState;
    lastPmicAnomalyPowerSource = (powerSource >= 0 && powerSource <= 0xFF) ? (uint8_t)powerSource : 0;
    lastPmicAnomalyVbusStatus = vbusStatus;

    Log.warn("PMIC_ANOM: soc=%.2f vcell=%.3f chargeStatus=%u chargeState=%s faultReg=0x%02X vbusStatus=%u powerGood=%d powerSource=%d temp=%.1f lowBattery=%d uptime=%lu lastResetReason=%d profile=%s",
             (double)effectiveSoc,
             (double)vcell,
             (unsigned)pmicBattState,
             batteryContext[(pmicBattState <= 6) ? pmicBattState : 0],
             faultReg,
             (unsigned)vbusStatus,
             powerGood ? 1 : 0,
             powerSource,
             (double)current.get_internalTempC(),
             sysStatus.get_lowBatteryMode() ? 1 : 0,
             (unsigned long)millis(),
             (int)System.resetReason(),
             PowerManager::compactProfileLabel(powerReport.activeInputProfile));
  }
#endif

  // F1 - BatteryHealth trust signal (WO-2026-08-25-001): replaces the
  // retired STALE_SOC latch/resync machinery. Pure trust signal only - does
  // not correct soc/vcell, latch, retry, or quickStart. See BatteryHealth.h.
  {
    const float effectiveSoc = rejectAuthoritativeOverwrite ? _authoritativeBatterySoc : soc;
    const bool chargingActive = (chargeStatus == 1 || chargeStatus == 2); // PRE or FAST
    const bool radioActive = Connectivity::isRadioPoweredOn();
    const BatteryHealth::Reading healthReading =
        BatteryHealth::evaluate(effectiveSoc, vcell, chargingActive, radioActive);
    // Cached for RuntimeReportingPolicy's trust-gated tier input (Amendment
    // B, Decision B1 / AC-B2) via SensorManager::cachedSocTrust().
    _cachedSocTrust = healthReading.trust;

    if (healthReading.trust != BatteryHealth::SocTrust::Trusted) {
      static BatteryHealth::SocTrust lastLoggedTrust = BatteryHealth::SocTrust::Trusted;
      if (healthReading.trust != lastLoggedTrust) {
        Log.warn("BatteryHealth: trust=%s soc=%.2f vcell=%.3fV restingEstimate=%.2f residual=%.2f",
                 healthReading.trust == BatteryHealth::SocTrust::Suspect ? "Suspect" : "Untrusted",
                 (double)healthReading.soc,
                 (double)healthReading.vcell,
                 (double)healthReading.restingSocEstimate,
                 (double)healthReading.residual);
        lastLoggedTrust = healthReading.trust;
      }
    }

    Observability::WakeCycleStats &stats = Observability::cycleStats();
    stats.stale_soc_suspected = (healthReading.trust != BatteryHealth::SocTrust::Trusted);
    stats.battery_vcell_mv = (uint16_t)(vcell * 1000.0f);
    stats.pmic_charge_status = chargeStatus;
    stats.pmic_vbus_status = vbusStatus;
    stats.pmic_power_source = (powerSource >= 0 && powerSource <= 0xFF) ? (uint8_t)powerSource : 0xFF;
    stats.pmic_power_good = powerGood ? 1 : 0;
    stats.pmic_fault_reg = faultReg;
  }

  const uint8_t finalLoggedBattState = pmicBattState;

  const bool allowPreSleepBatteryLog =
      (sampleContext != BatterySampleContext::PreSleep) || (ENABLE_SLEEP_TRACE != 0);
  if (logBatteryDetail && allowPreSleepBatteryLog) {
    Log.info("%s: soc=%.2f raw=%.2f norm=%.2f vcell=%.3f authority=%s state=%s(%d) systemState=%s(%d) power=%s(%d)%s",
             batterySampleContextPrefix(sampleContext),
             (double)loggedSoc,
             (double)rawFuelGaugeSoc,
             (double)normalizedSoc,
             (double)loggedVcell,
             authorityTag,
             batteryContext[(finalLoggedBattState <= 6) ? finalLoggedBattState : 0],
             finalLoggedBattState,
             batteryContext[(systemBattState <= 6) ? systemBattState : 0],
             systemBattState,
             PowerManager::powerSourceLabel(powerSource),
             powerSource,
             rejectAuthoritativeOverwrite ? " candidateRejected=true" : "");
  }

  PmicFaultMonitor::trackAndReport(pmicRegs, powerSource, soc, vcell, finalAcceptedSoc);
#endif // HAL_PLATFORM_CELLULAR && (PLATFORM_ID != PLATFORM_MSOM)

#elif PLATFORM_ID == 32 || PLATFORM_ID == 34
  // Photon 2 and P2:
  // Measure battery voltage (VBAT_MEAS on Photon 2, or same pin on P2
  // carrier) using A6 as described in the Photon 2 battery voltage docs.

  int raw = analogRead(A6);
  float voltage = raw / 819.2f; // Map ADC count (0-4095) to 0-5V

  // Approximate state-of-charge from voltage for a LiPo battery.
  // Treat 3.0V as 0% and 4.2V as 100%.
  float soc = (voltage - 3.0f) * (100.0f / (4.2f - 3.0f));
  if (soc < 0.0f) {
    soc = 0.0f;
  } else if (soc > 100.0f) {
    soc = 100.0f;
  }
  current.set_stateOfCharge(soc);

  // Photon 2/P2 cannot reliably determine charging state without a PMIC.
  // Always report "Unknown" since voltage alone can't distinguish between
  // charging and discharging at the same voltage level.
  uint8_t battState = 0; // Unknown

  const bool allowPreSleepBatteryLog =
      (sampleContext != BatterySampleContext::PreSleep) || (ENABLE_SLEEP_TRACE != 0);
  if ((shouldLogBatterySample(sampleContext) || sysStatus.get_verboseMode()) &&
      allowPreSleepBatteryLog) {
    Log.info("%s: soc=%.2f raw=-1.00 norm=-1.00 vcell=%.3f authority=voltage-estimated state=%s(%d) power=-1",
             batterySampleContextPrefix(sampleContext),
             (double)soc,
             (double)voltage,
             batteryContext[battState],
             battState);
  }
  
  current.set_batteryState(battState);

#else
  // Other Wi-Fi / SoM platforms: leave battery fields unchanged for now.
#endif

#if !(HAL_PLATFORM_CELLULAR && (PLATFORM_ID != PLATFORM_MSOM))
  // WO-2026-08-25-001 Amendment C, Decision C1: on this platform build,
  // nothing else needs `safeToCharge` earlier in this function (there is no
  // PmicFaultMonitor::pollAndRemediate() call to feed - see the other call
  // site above, mutually exclusive with this one by platform), so the
  // single coupled call happens here instead. Exactly one of the two call
  // sites in this file compiles for any given platform.
  measureTemperatureAndApplyChargeDecision();
#endif

  // Convenience: indicate whether battery is in a healthy range.
  return current.get_stateOfCharge() > 20.0f;
}

bool SensorManager::measureTemperatureAndApplyChargeDecision() {
  // =========================================================================
  // WO-2026-08-25-001 Amendment C, Decision C1: structural coupling.
  //
  // This function is the ONLY place enclosure temperature is read, and the
  // ONLY place the thermal charge-inhibit decision is evaluated/applied.
  // Both halves live in this one function body with NO return statement
  // between them (AC-C1, AC-C3): every path below - a genuine reading, a
  // failed/invalid reading, a still-accumulating TMP36 partial sample, or
  // the no-sensor platform stub - falls through unconditionally to the
  // decision at the bottom. `tempC`/`measuredThisCall` are plain locals;
  // the decision consumes THEM directly, never current.get_internalTempC()
  // (that field is written below purely for telemetry, strictly AFTER the
  // local values it is derived from are already fixed for this call).
  //
  // Because `measuredThisCall` is a fresh local every call rather than a
  // sticky boot-scoped flag, a genuine reading followed by later sensor
  // failures does not leave that earlier reading being treated as current
  // (Blocker D / AC-C4): each call's validity is judged solely on whether
  // THIS call produced a genuine reading.
  // =========================================================================

  // -------------------------------------------------------------------------
  // Half 1: measurement.
  // -------------------------------------------------------------------------
  // Default behavior:
  //  - If a TMP112A is present on the I2C bus (Muon), prefer it.
  //  - Otherwise, use TMP36 (if wired) or platform-specific stub.
  //
  // Compile-time controls:
  //  - Define MUON_HAS_TMP112 to force enable the TMP112A path.
  //  - Define DISABLE_TMP112_AUTODETECT to skip probing for TMP112A.

  float tempC = current.get_internalTempC(); // seed; every branch below overwrites or falls back explicitly
  bool measuredThisCall = false;             // set true ONLY by a genuine, in-range reading taken this call

#if defined(MUON_TMP112_I2C_ADDR)
  const uint8_t tmp112Addr = (uint8_t)MUON_TMP112_I2C_ADDR;
#else
  const uint8_t tmp112Addr = 0x48;
#endif

  static bool tmp112ProbeDone = false;
  static bool tmp112Present = false;

#if !defined(DISABLE_TMP112_AUTODETECT)
  if (!tmp112ProbeDone) {
    // Safe to call multiple times; ensures I2C is initialized even if nothing
    // else has yet started Wire.
    Wire.begin();
    tmp112Present = probeTmp112Present(tmp112Addr);
    tmp112ProbeDone = true;
    if (sysStatus.get_verboseMode()) {
      Log.info("TMP112A probe at 0x%02X: %s", tmp112Addr, tmp112Present ? "present" : "not found");
    }
  }
#endif

#if defined(MUON_HAS_TMP112)
  tmp112Present = true;
  tmp112ProbeDone = true;
#endif

  if (tmp112Present) {
    float reading;
    if (readTmp112TemperatureC(reading) && (reading > -50.0f && reading < 120.0f)) {
      tempC = reading;
      measuredThisCall = true;
    } else {
      float prev = current.get_internalTempC();
      tempC = (prev > -50.0f && prev < 120.0f) ? prev : 25.0f;
      Log.warn("TMP112A read failed/invalid - falling back to %4.2f C", (double)tempC);
    }
    current.set_internalTempC(tempC);
  }

#if (PLATFORM_ID == 32 || PLATFORM_ID == 34) && !defined(MUON_HAS_TMP36)
  // Photon 2 and P2 development platforms:
  // There is no TMP36 wired to an ADC-capable pin on the Photon 2 dev
  // carrier, so we cannot take a real analog temperature reading here.
  // Instead, use whatever value has been stored in internalTempC (for
  // example, set manually for testing), falling back to 25C if unset. This
  // is never a genuine measurement, so measuredThisCall is left false.
  {
    float stubTempC = current.get_internalTempC();
    if (!(stubTempC > -50.0f && stubTempC < 120.0f)) {
      stubTempC = 25.0f;
    }
    tempC = stubTempC;

    if (sysStatus.get_verboseMode()) {
      Log.info("P2/Photon2 stub: using internalTempC=%4.2f C (no TMP36 ADC)", (double)tempC);
    }

    current.set_internalTempC(tempC);
  }
#else
  if (!tmp112Present) {
    // Measure enclosure temperature using the TMP36 on the carrier board
    // (connected to TMP36_SENSE_PIN, typically A4).
    //
    // WO-2026-08-25-001 Decision C4: all 8 samples are taken INLINE, in this
    // one call - NOT spread across multiple batteryState()/coupled-call
    // invocations. The TMP36 read has no I2C, initialization, or settling
    // dependency, and each sample is a ~5us ADC read (measured), so all 8
    // cost ~40us total - negligible against a loop() cadence, and Chip
    // confirmed 2026-08-26 that cost is acceptable. Spreading sampling across
    // calls made the accumulator itself a `static` that a hibernate-induced
    // MCU reset would wipe mid-cycle, which was the root cause of both the
    // original round-3 blocker (inhibit could never release) and its mirror,
    // AC-C5 (a hot device could charge unboundedly after reboot while
    // samples re-accumulated). Taking all samples inline means a genuine
    // reading is available on THIS call, every call, with no accumulator
    // state to lose across a reset - do not re-introduce cross-call
    // accumulation to "optimize" this back to spread sampling (AC-C9).
    pinMode(TMP36_SENSE_PIN, INPUT);

    const int TMP36_SAMPLES = 8;
    int tmpRawSum = 0; // local to this call only - never static/retained

    for (int i = 0; i < TMP36_SAMPLES; i++) {
      tmpRawSum += analogRead(TMP36_SENSE_PIN);
    }

    int tmpRaw = tmpRawSum / TMP36_SAMPLES;

    // Consider extremely low readings as "sensor not present". With a
    // TMP36, even very cold temperatures should still be around 100mV
    // (roughly 120 ADC counts on a 3.3V/12-bit ADC), so an average below
    // ~50 counts is effectively 0V at the pin.
    bool sensorOk = (tmpRaw > 50 && tmpRaw < 4000);
    float sampledTempC = tmp36TemperatureC(tmpRaw);

    // If the TMP36 reading is clearly out of a plausible enclosure range
    // (for example, -50C from a raw 0 reading), or the sensor appears to
    // be disconnected, fall back to a prior stored value or a
    // conservative default so that charging guard rails and telemetry
    // still operate with a realistic value. This is the one remaining
    // AC-C3 fall-through path for TMP36: a bad/disconnected sensor still
    // leaves measuredThisCall=false and falls through to the decision below.
    if (!sensorOk || sampledTempC < -20.0f || sampledTempC > 80.0f) {
      float prev = current.get_internalTempC();
      float fallback = 25.0f; // conservative room-temperature default

      if (prev > -20.0f && prev < 80.0f) {
        fallback = prev;
      }

      Log.warn("TMP36 reading invalid or out of range (tmp36=%4.2f C, raw=%d, sensorOk=%s) - falling back to %4.2f C",
              (double)sampledTempC, tmpRaw, sensorOk ? "true" : "false", (double)fallback);
      tempC = fallback;
    } else {
      tempC = sampledTempC;
      measuredThisCall = true;
    }

    current.set_internalTempC(tempC);

    // Optional debug: log enclosure temperature when verbose logging is enabled.
    if (sysStatus.get_verboseMode()) {
      Log.info("Enclosure temperature (effective): %4.2f C (raw=%d)", (double)tempC, tmpRaw);
    }
  }
  // else: TMP112A already provided tempC/measuredThisCall above; TMP36
  // sampling is skipped entirely (previously an early return here - now it
  // simply falls through to the decision below using the TMP112 result, per
  // AC-C1/AC-C3).
#endif // PLATFORM_ID == 32 || PLATFORM_ID == 34

  // -------------------------------------------------------------------------
  // Half 2: evaluate and apply the thermal charge-inhibit decision.
  //
  // Consumes tempC / measuredThisCall LOCALS directly - never re-reads
  // current.get_internalTempC() (AC-C1 point 2). Reached unconditionally
  // from every measurement path above (AC-C1 point 3 / AC-C3).
  // -------------------------------------------------------------------------

  // F2a - thermal charge inhibit (WO-2026-08-25-001). Arm/release thresholds
  // are ledger-configurable and per-device overridable (see
  // MyPersistentData::get_thermalCharge*); compiled-in defaults are the
  // field-proven 37C/0C arm, 35C/3C release values, not the 45C LiPo-spec
  // value this function previously used.
  static bool inhibited = false;
  static bool inhibitedSyncedWithHardware = false;

  const ChargeInhibitPolicy::ThermalThresholds thresholds{
      sysStatus.get_thermalChargeArmHighC(),
      sysStatus.get_thermalChargeArmLowC(),
      sysStatus.get_thermalChargeReleaseHighC(),
      sysStatus.get_thermalChargeReleaseLowC(),
  };

#if HAL_PLATFORM_CELLULAR
  if (!inhibitedSyncedWithHardware) {
    // WO-2026-08-25-001 Amendment B, Decision B2: `inhibited` is a plain
    // (non-retained) static, so it always starts false on a fresh boot -
    // but the DCT-persisted DISABLE_CHARGING feature bit it controls
    // survives that same reset. Without this sync, a device that rebooted
    // with charging already disabled would look, to this function, exactly
    // like a device that has never been inhibited, and the code below would
    // go on to make an ARM/RELEASE decision as if starting clean instead of
    // recognizing it is already holding an armed inhibit. Read the real
    // hardware/DCT state once per boot instead of assuming "not inhibited".
    //
    // NOTE (Amendment C, AC-C5 bound - updated for Decision C5): PowerManager::
    // setup() -> refreshInputProfile() runs before this coupled call ever
    // executes (see Generalized-Core-Counter.cpp setup() ordering), but it
    // only calls applyInputProfile() conditionally - when `!report_.valid`
    // or the selected profile has changed (PowerManager.cpp) - not on every
    // call. Since Decision C5, applyInputProfile() also no longer replaces
    // the DCT power configuration wholesale: every profile config is passed
    // through applyDisableChargingBit() (PowerPlatform.cpp), which preserves
    // an existing DISABLE_CHARGING bit instead of clearing it. So on a
    // device that rebooted while hot, `inhibited` syncs to TRUE here,
    // matching the persisted DCT state, and charging stays disabled rather
    // than re-arming from a false negative. Decision C4 (TMP36 sampling
    // moved inline) remains relevant for the case where the bit was not
    // set: it bounds this function's own re-arm decision to a single
    // coupled call rather than leaving a gap across an unbounded number of
    // them. See IMPLEMENTATION_REPORT_R4C.md for the measured figure.
    const SystemPowerConfiguration currentConf = System.getPowerConfiguration();
    inhibited = currentConf.isFeatureSet(SystemPowerFeature::DISABLE_CHARGING);
    inhibitedSyncedWithHardware = true;
  }
#endif

  const bool previouslyInhibited = inhibited;

  // Temperature-validity gate (Amendment B, Decision B2 / AC-B5; Amendment C
  // AC-C4): delegated to the pure, host-tested
  // ChargeInhibitPolicy::evaluateThermalWithValidity() so the arm/hold/release
  // rules are exercised by the same function this production call site uses,
  // not a hand-written mirror of it. See that function's header comment for
  // the full rule set. `measuredThisCall` and `tempC` are this call's LOCALS
  // from Half 1 above, not any persisted/boot-scoped state.
  const ChargeInhibitPolicy::ThermalInhibitDecision decision =
      ChargeInhibitPolicy::evaluateThermalWithValidity(
          inhibited, measuredThisCall, tempC, thresholds);
  inhibited = decision.inhibited;
  const bool heldWithoutFreshTemp = decision.heldWithoutFreshTemp;

  const bool safe = !inhibited;

  Observability::WakeCycleStats &stats = Observability::cycleStats();
  stats.thermal_inhibit_held_without_fresh_temp = heldWithoutFreshTemp;
  if (heldWithoutFreshTemp) {
    static bool loggedHeldThisBoot = false;
    if (!loggedHeldThisBoot) {
      Log.warn("ChargeInhibit: holding inhibit with no fresh temperature "
               "measurement this call (lastLoggedInternalTempC=%.2f, "
               "possibly stale/persisted) - will not release until a real "
               "reading completes",
               (double)tempC);
      loggedHeldThisBoot = true;
    }
  }

#if HAL_PLATFORM_CELLULAR
  // Re-assert every call (not only on transitions): System.setPowerConfiguration()
  // replaces the entire DCT power config, so the only way this inhibit is
  // self-clearing without a latch is to rebuild and re-apply it every
  // battery measurement.
  const PowerReport &activeReport = PowerManager::instance().latestReport();
  const ChargeInhibit::ApplyResult applyResult =
      ChargeInhibit::apply(inhibited, activeReport.activeInputProfile);

  if (inhibited) {
    current.set_batteryState(1); // Reflect that we are "Not Charging"
  }

  if (inhibited && !previouslyInhibited) {
    Log.warn("Charging inhibited due to enclosure temperature: %4.2f C (armHigh=%.1f armLow=%.1f)",
             (double)tempC, (double)thresholds.armHighC, (double)thresholds.armLowC);
  } else if (!inhibited && previouslyInhibited) {
    Log.info("Charging inhibit cleared; enclosure temperature: %4.2f C", (double)tempC);
  }

  if (applyResult.supported && !applyResult.configReadbackVerified) {
    Log.warn("ChargeInhibit: setPowerConfiguration verify-applied failed (result=%d, inhibited=%d)",
             applyResult.systemResult, inhibited ? 1 : 0);
  }

  // DCT-config-readback-verified active disable = inhibited AND
  // System.getPowerConfiguration() confirmed the DISABLE_CHARGING bit is
  // set (not merely that some write happened to match some intent). This is
  // NOT a hardware guarantee - see chargeDisableConfigVerified()'s doc
  // comment.
  _chargeDisableConfigVerified = inhibited && applyResult.configReadbackVerified;
#else
  // On platforms without a PMIC API (such as Argon, Photon 2 / P2), we
  // do not control charging, but we still evaluate and log whether it
  // would be considered safe based on the same temperature thresholds.
  if (inhibited && !previouslyInhibited) {
    Log.warn("Charging would be inhibited due to enclosure temperature: %4.2f C (no PMIC on this platform)", (double)tempC);
  } else if (!inhibited && previouslyInhibited) {
    Log.info("Charging inhibit would clear; enclosure temperature: %4.2f C (no PMIC on this platform)", (double)tempC);
  }
  _chargeDisableConfigVerified = false; // No PMIC control on this platform.
#endif

  return safe;
}

void SensorManager::getSignalStrength() {
  char signalStr[64] = {0};  // Initialize buffer to zeros to prevent garbage output
  
#if HAL_PLATFORM_CELLULAR
  const char *radioTech[10] = {"Unknown",    "None",       "WiFi", "GSM",
                               "UMTS",       "CDMA",       "LTE",  "IEEE802154",
                               "LTE_CAT_M1", "LTE_CAT_NB1"};
  // New Signal Strength capability -
  // https://community.particle.io/t/boron-lte-and-cellular-rssi-funny-values/45299/8
  CellularSignal sig = Cellular.RSSI();

  auto rat = sig.getAccessTechnology();

  // float strengthVal = sig.getStrengthValue();
  float strengthPercentage = sig.getStrength();

  // float qualityVal = sig.getQualityValue();
  float qualityPercentage = sig.getQuality();

  snprintf(signalStr, sizeof(signalStr), "%s S:%2.0f%%, Q:%2.0f%%",
           radioTech[rat], strengthPercentage, qualityPercentage);
  Log.info(signalStr);
#elif HAL_PLATFORM_WIFI
  WiFiSignal sig = WiFi.RSSI();
  float strengthPercentage = sig.getStrength();
  float qualityPercentage = sig.getQuality();
  
  snprintf(signalStr, sizeof(signalStr), "WiFi S:%2.0f%%, Q:%2.0f%%",
           strengthPercentage, qualityPercentage);
  Log.info(signalStr);
#endif
}

// ============================================================================
// TEMPORARY DIAGNOSTIC: PMIC Charge Cycle Test
// ============================================================================
// Purpose: Determine if PMIC is stuck in FAST_CHARGE or will transition to DONE
// when charging is disabled and re-enabled. Captures comprehensive PMIC state
// before, during, and after the test cycle.
//
// Safety: Only runs once per boot, requires external power, SOC > 50%, safe temp.
// ============================================================================

int SensorManager::runPmicChargeCycleTest() {
#if HAL_PLATFORM_CELLULAR && (PLATFORM_ID != PLATFORM_MSOM)
  // One-shot guard: prevent accidental re-execution
  static bool testAlreadyRun = false;
  if (testAlreadyRun) {
    Log.warn("PMIC_TEST: Already executed this boot - skipping");
    return -1;  // Already run
  }

  Log.info("PMIC_TEST: ========== BEGIN CHARGE CYCLE DIAGNOSTIC ==========");

  // Safety checks before proceeding
  const float currentSoc = current.get_stateOfCharge();
  const float currentTemp = current.get_internalTempC();
  const int powerSource = System.powerSource();
  
  PMIC pmic(true);
  const byte systemStatus = pmic.getSystemStatus();
  const uint8_t vbusStatus = (systemStatus >> 6) & 0x03;
  const bool powerGood = (systemStatus & 0x04) != 0;

  // Safety gate: require external power, sufficient SOC, safe temperature
  if (vbusStatus == 0) {
    Log.error("PMIC_TEST: SAFETY ABORT - No external power detected (vbus=0)");
    return -2;
  }
  if (!powerGood) {
    Log.error("PMIC_TEST: SAFETY ABORT - Power not good (pg=0)");
    return -3;
  }
  if (currentSoc < 50.0f) {
    Log.error("PMIC_TEST: SAFETY ABORT - SOC too low (%.1f%% < 50%%)", (double)currentSoc);
    return -4;
  }
  if (currentTemp < 0.0f || currentTemp > 45.0f) {
    Log.error("PMIC_TEST: SAFETY ABORT - Temperature out of range (%.1f°C)", (double)currentTemp);
    return -5;
  }

  Log.info("PMIC_TEST: Safety checks passed - proceeding with test");
  Log.info("PMIC_TEST: soc=%.1f temp=%.1f src=%d vbus=%u pg=%u",
           (double)currentSoc, (double)currentTemp, powerSource, (unsigned)vbusStatus, powerGood ? 1u : 0u);

  // Helper lambda to capture comprehensive PMIC snapshot
  auto captureSnapshot = [&](const char* label) {
    const byte faultReg = pmic.getFault();
    const byte systemStatus = pmic.getSystemStatus();
    const uint8_t vbusStatus = (systemStatus >> 6) & 0x03;
    const uint8_t chargeStatus = (systemStatus >> 4) & 0x03;
    const bool powerGood = (systemStatus & 0x04) != 0;
    const bool thermalReg = (systemStatus & 0x02) != 0;
    const bool vsysMin = (systemStatus & 0x01) != 0;
    const bool chargingEnabled = pmic.isChargingEnabled();
    
    const float soc = fuelGauge.getSoC();
    const float vcell = fuelGauge.getVCell();
    const float temp = current.get_internalTempC();
    const int src = System.powerSource();
    
    // Power profile context
    const PowerReport &powerReport = PowerManager::instance().latestReport();
    const char* profLabel = (powerReport.activeInputProfile == PowerInputProfile::Solar35W) ? "SOLAR" : 
                           (powerReport.activeInputProfile == PowerInputProfile::UsbBench) ? "USB" : "UNKNOWN";
    const int expectedInputCurrentMa = 900;
    const int expectedChargeCurrentMa = (powerReport.activeInputProfile == PowerInputProfile::Solar35W) ? 900 : 896;
    const int expectedChargeVoltageMv = (powerReport.activeInputProfile == PowerInputProfile::Solar35W) ? 4208 : 4112;
    
    const char* chargeLabels[] = {"OFF", "PRE", "FAST", "DONE"};
    
    Log.info("PMIC_TEST: %s: soc=%.1f vcell=%.3f chg=%u(%s) vbus=%u pg=%u fault=0x%02X src=%d temp=%.1f",
             label,
             (double)soc,
             (double)vcell,
             (unsigned)chargeStatus,
             chargeLabels[chargeStatus],
             (unsigned)vbusStatus,
             powerGood ? 1u : 0u,
             faultReg,
             src,
             (double)temp);
    
    Log.info("PMIC_TEST: %s_CFG: prof=%s iin=%umA ichg=%umA vchg=%umV enabled=%d therm=%d vsysmin=%d",
             label,
             profLabel,
             (unsigned)expectedInputCurrentMa,
             (unsigned)expectedChargeCurrentMa,
             (unsigned)expectedChargeVoltageMv,
             chargingEnabled ? 1 : 0,
             thermalReg ? 1 : 0,
             vsysMin ? 1 : 0);
  };

  // PHASE 1: Baseline snapshot
  Log.info("PMIC_TEST: PHASE 1 - Capturing baseline state");
  captureSnapshot("BASELINE");

  // PHASE 2: Disable charging
  Log.info("PMIC_TEST: PHASE 2 - Disabling charging for 10 seconds");
  pmic.disableCharging();
  delay(500);  // Short settle delay
  captureSnapshot("DISABLED_T0");
  
  delay(5000);  // Wait 5 seconds
  captureSnapshot("DISABLED_T5");
  
  delay(4500);  // Wait another 4.5 seconds (total 10 seconds disabled)
  captureSnapshot("DISABLED_T10");

  // PHASE 3: Re-enable charging
  Log.info("PMIC_TEST: PHASE 3 - Re-enabling charging for 10 seconds");
  pmic.enableCharging();
  delay(500);  // Short settle delay
  captureSnapshot("ENABLED_T0");
  
  delay(5000);  // Wait 5 seconds
  captureSnapshot("ENABLED_T5");
  
  delay(4500);  // Wait another 4.5 seconds (total 10 seconds enabled)
  captureSnapshot("ENABLED_T10");

  // PHASE 4: Final state
  Log.info("PMIC_TEST: PHASE 4 - Final state after full cycle");
  captureSnapshot("FINAL");

  Log.info("PMIC_TEST: ========== END CHARGE CYCLE DIAGNOSTIC ==========");
  Log.info("PMIC_TEST: Total test duration: ~21 seconds");
  Log.info("PMIC_TEST: Charging has been RE-ENABLED and is now in normal operation");
  
  testAlreadyRun = true;
  return 0;  // Success
#else
  Log.warn("PMIC_TEST: Not supported on this platform (requires Boron with BQ24195 PMIC)");
  return -10;  // Platform not supported
#endif
}

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
#include "power/ConnectivityPolicy.h"
#include "state/StateMachine.h"
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

// Stale SOC detection (Phase 1: detection and instrumentation only)
retained uint8_t staleSocConsecutiveCount = 0;
retained uint16_t staleSocTotalCount = 0;

/**
 * @brief Publish stale SOC forensics event.
 * 
 * @details Sends a compact "stale_soc" event with diagnostic snapshot.
 *          Called once when stale SOC is first detected in a session.
 *          Payload includes SOC, voltage, PMIC state, and environmental context.
 */
static void publishStaleSocForensics(float soc, float vcell, bool highConfidence,
                                      uint8_t chargeStatus, uint8_t vbusStatus,
                                      bool powerGood, uint8_t faultReg,
                                      int powerSource, uint16_t totalCount,
                                      float internalTempC) {
  char payload[192];
  
  snprintf(payload,
           sizeof(payload),
           "{\"soc\":%.2f,\"vcell_mv\":%u,\"conf\":\"%s\",\"chg\":%u,\"vbus\":%u,\"pg\":%u,\"fault\":0x%02X,\"src\":%d,\"count\":%u,\"temp_c\":%.1f}",
           (double)soc,
           (unsigned)(vcell * 1000.0f),
           highConfidence ? "HIGH" : "LOW",
           (unsigned)chargeStatus,
           (unsigned)vbusStatus,
           powerGood ? 1u : 0u,
           faultReg,
           powerSource,
           (unsigned)totalCount,
           (double)internalTempC);
  
  PublishQueuePosix::instance().publish("stale_soc", payload, PRIVATE);
}

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

#if HAL_PLATFORM_CELLULAR || PLATFORM_ID == PLATFORM_ARGON
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
constexpr unsigned long CHARGE_SUMMARY_INTERVAL_MS = 15UL * 60UL * 1000UL;

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

uint8_t batteryStateFromPmicStatus(uint8_t chargeStatus, byte faultReg) {
  if (faultReg & 0x38) {
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

const char *compactPmicVbusLabel(uint8_t vbusStatus) {
  switch (vbusStatus) {
  case 1:
    return "USB";
  case 2:
    return "ADAPT";
  case 3:
    return "OTG";
  default:
    return "NONE";
  }
}

const char *compactPmicChargeLabel(uint8_t chargeStatus, byte faultReg) {
  if (faultReg & 0x38) {
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

const char *compactProfileLabel(PowerInputProfile profile) {
  switch (profile) {
  case PowerInputProfile::UsbBench:
    return "USB";
  case PowerInputProfile::Solar35W:
    return "SOLAR";
  case PowerInputProfile::NotApplicable:
    return "NA";
  default:
    return "?";
  }
}
#endif

} // namespace

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
  _authoritativeBatteryFallbackUsed(false) {}

SensorManager::~SensorManager() {}

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

namespace {

#if HAL_PLATFORM_CELLULAR || PLATFORM_ID == PLATFORM_ARGON
const float AUTHORITATIVE_POST_CONNECT_DELTA_THRESHOLD = 20.0f;

bool batterySampleHasUnrealisticDelta(float authoritativeSoc, float soc) {
  float delta = soc - authoritativeSoc;
  if (delta < 0.0f) {
    delta = -delta;
  }
  return delta >= AUTHORITATIVE_POST_CONNECT_DELTA_THRESHOLD;
}

float batterySampleDelta(float authoritativeSoc, float soc) {
  float delta = soc - authoritativeSoc;
  if (delta < 0.0f) {
    delta = -delta;
  }
  return delta;
}
#endif

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
  if (!shouldStabilize &&
      _authoritativeBatterySampleActive &&
      Particle.connected()) {
    const bool suspiciousCandidate = batterySampleLooksSuspicious(battState, soc, vcell, ignoreUnknownBatteryState);
    authoritativeDelta = batterySampleDelta(_authoritativeBatterySoc, soc);
    const bool unrealisticDelta = batterySampleHasUnrealisticDelta(_authoritativeBatterySoc, soc);
    const bool authoritativeFallbackLock = _authoritativeBatteryFallbackUsed;
    if (authoritativeFallbackLock || suspiciousCandidate || unrealisticDelta) {
      rejectAuthoritativeOverwrite = true;
      Log.warn("Battery post-connect sample ignored: preRadio=%.1f postConnect=%.1f delta=%.1f",
               (double)_authoritativeBatterySoc,
               (double)soc,
               (double)authoritativeDelta);
      Log.warn("Battery authority: keeping pre-radio sample SoC=%.2f%% state=%s (%d)%s; rejecting later sample SoC=%.2f%% state=%s (%d) powerSource=%d suspicious=%s fallbackLock=%s delta=%.2f",
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
               (double)authoritativeDelta);
    }
  }

  const char *authorityTag = socAuthorityTag;
  if (rejectAuthoritativeOverwrite) {
    authorityTag = "ignored-post-connect";
  }

  const bool logBatteryDetail = sysStatus.get_verboseMode() || shouldStabilize ||
      fallbackUsed || previousKnownGoodUsed || rejectAuthoritativeOverwrite;
  const float loggedSoc = rejectAuthoritativeOverwrite ? _authoritativeBatterySoc : soc;

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
#else
  if (!rejectAuthoritativeOverwrite) {
    current.set_stateOfCharge(soc);
  }
#endif

#if HAL_PLATFORM_CELLULAR && (PLATFORM_ID != PLATFORM_MSOM)
  // =========================================================================
  // PMIC Health Monitoring & Smart Remediation (BQ24195 PMIC)
  // =========================================================================
  // Supported platforms: Boron (Gen 3 cellular with BQ24195 PMIC)
  // Excluded platforms: M-SoM/Muon (uses Particle Power Module with MAX17043, not BQ24195)
  // 
  // Detects charging faults (1Hz amber LED = fault register set) and attempts
  // automatic recovery with escalating remediation levels to prevent thrashing.
  //
  // Alert Codes (auto-reported via webhook):
  //   20 = PMIC Thermal Shutdown (critical - charging stopped due to temp)
  //   21 = PMIC Charge Timeout (critical - safety timer expired, stuck charging)
  //   23 = PMIC Battery Fault (major - general charging issue)
  //
  // Log-Only Diagnostics (NOT alerted - transient/normal conditions):
  //   Input Fault: VBUS out of range (solar undervoltage common, backend detects sustained issues)
  //
  // Remediation Strategy:
  //   Level 0: Monitor only (log diagnostics, raise alert)
  //   Level 1: Soft reset (cycle charging off/on after 2+ consecutive faults)
  //   Level 2: Power cycle with watchdog (after 3+ consecutive faults)
  //   Cooldown: 1 hour minimum between remediation attempts
  //   Auto-Clear: Resets all counters when charging returns to healthy state
  //
  // This prevents the common "loss of charge until power cycle" issue by
  // detecting PMIC faults early and automatically attempting recovery before
  // requiring manual intervention.
  // =========================================================================
  
  // PMIC health monitoring (BQ24195 PMIC - Boron only)
  // Tracks charging faults and attempts smart remediation with escalation
  static unsigned long lastRemediationAttempt = 0;
  static uint8_t remediationLevel = 0; // 0=none, 1=soft reset, 2=power cycle
  static uint8_t consecutiveFaults = 0;
  const unsigned long REMEDIATION_COOLDOWN = 3600000; // 1 hour between attempts

  // Non-blocking remediation state machine (replaces delay-based sequencing)
  // Keeps loop() responsive while still performing the same charge-cycle actions.
  static bool remediationInProgress = false;
  static uint8_t remediationActiveLevel = 0; // 0=none, 1=soft, 2=aggressive
  static uint8_t remediationPhase = 0;       // 0=start, 1=wait
  static unsigned long remediationPhaseStartMs = 0;
  static bool immediateChargeFaultResetConsumed = false;
  static byte immediateChargeFaultResetFaultReg = 0;
  
  // Check if charging is intentionally disabled due to temperature BEFORE attempting remediation
  bool safeToCharge = isItSafeToCharge();
  
  PMIC pmic(true); // true = lock I2C during operations
  
  // Read REG09 (Fault Register)
  // NOTE: Use public API; PMIC::readRegister() is private in DeviceOS 6.3.4.
  byte faultReg = pmic.getFault();
  byte systemStatus = pmic.getSystemStatus();
  uint8_t vbusStatus = (systemStatus >> 6) & 0x03;
  uint8_t chargeStatus = (systemStatus >> 4) & 0x03;
  bool powerGood = (systemStatus & 0x04) != 0;
  bool thermalRegulation = (systemStatus & 0x02) != 0;
  bool inVsysMin = (systemStatus & 0x01) != 0;

  const char* chargeStatusStr[] = {"Off", "Pre", "Fast", "Done"};
  const PowerReport &powerReport = PowerManager::instance().latestReport();

  auto isUsbBackedSource = [](int source) {
    return source == 2 || source == 3 || source == 4;
  };

  bool persistentAfterImmediateChargeFaultReset = false;
  if (faultReg & 0x38) {
    const bool usbInputPresent = (vbusStatus == 1 || vbusStatus == 2 || vbusStatus == 3);
    const bool usbBackedFaultContext =
        powerReport.activeInputProfile == PowerInputProfile::UsbBench ||
        isUsbBackedSource(powerReport.reading.powerSource) ||
        isUsbBackedSource(powerSource) ||
        usbInputPresent;
    const bool severeBatteryState = (battState == 5 || battState == 6);
    const uint8_t initialChargeFault = (faultReg >> 3) & 0x07;
    const bool alreadyAttemptedForActiveFault =
        immediateChargeFaultResetConsumed &&
        immediateChargeFaultResetFaultReg == faultReg;
    const bool canAttemptImmediateChargeFaultReset =
        !alreadyAttemptedForActiveFault &&
        safeToCharge &&
        powerGood &&
        usbInputPresent &&
        usbBackedFaultContext &&
        !thermalRegulation &&
        !severeBatteryState &&
        initialChargeFault != 0x02;

    if (canAttemptImmediateChargeFaultReset) {
      Log.warn("PMIC: charge fault active; attempting charging reset");
      pmic.disableCharging();
      Log.warn("PMIC: charging disabled for fault reset");
      boundedBatterySettleDelay(500UL);
      pmic.enableCharging();
      Log.warn("PMIC: charging re-enabled after fault reset");
      boundedBatterySettleDelay(500UL);

      faultReg = pmic.getFault();
      systemStatus = pmic.getSystemStatus();
      vbusStatus = (systemStatus >> 6) & 0x03;
      chargeStatus = (systemStatus >> 4) & 0x03;
      powerGood = (systemStatus & 0x04) != 0;
      thermalRegulation = (systemStatus & 0x02) != 0;
      inVsysMin = (systemStatus & 0x01) != 0;
      immediateChargeFaultResetConsumed = true;
      immediateChargeFaultResetFaultReg = (byte)(faultReg & 0x38 ? faultReg : initialChargeFault << 3);
      lastRemediationAttempt = millis();

      Log.info("PMIC: fault reset result faultReg=0x%02x charge=%s",
               faultReg,
               chargeStatusStr[chargeStatus]);

      if (!(faultReg & 0x38)) {
        Log.info("PMIC: charge fault cleared");
        consecutiveFaults = 0;
        remediationLevel = 0;
        remediationInProgress = false;
        remediationActiveLevel = 0;
        remediationPhase = 0;
        immediateChargeFaultResetConsumed = false;
        immediateChargeFaultResetFaultReg = 0;

        int8_t currentAlert = current.get_alertCode();
        if (currentAlert == 21 || currentAlert == 23) {
          current.set_alertCode(0);
          current.set_lastAlertTime(0);
        }
      } else {
        persistentAfterImmediateChargeFaultReset = true;
      }
    }
  }
  
  // Check for charging faults (bits 3-5: CHRG_FAULT)
  if (faultReg & 0x38) {
    uint8_t chargeFault = (faultReg >> 3) & 0x07;
    consecutiveFaults++;
    
    switch(chargeFault) {
      case 0x01: // Input fault (VBUS overvoltage or undervoltage)
        // This triggers when VIN < powerSourceMinVoltage (5.08V) or > max
        // Most common cause: obscured/faulty solar panel insufficient voltage
        // LOG ONLY - transient voltage dips are normal (clouds, trees, dawn/dusk)
        // Backend detects sustained panel failures via multi-day SoC decline
        Log.info("PMIC: Input fault - VBUS out of range (likely solar variation)");
        if (persistentAfterImmediateChargeFaultReset) {
          Log.error("PMIC: charge fault persists after reset - alert 21");
          current.raiseAlert(21);
        }
        break;
      case 0x02: // Thermal shutdown
        Log.error("PMIC: Thermal shutdown - charging stopped due to temperature");
        current.raiseAlert(20); // Alert code 20: PMIC Thermal (critical)
        break;
      case 0x03: // Charge safety timer expired
        if (persistentAfterImmediateChargeFaultReset) {
          Log.error("PMIC: charge fault persists after reset - alert 21");
        } else {
          Log.error("PMIC: Charge safety timer expired - charging timeout (common stuck charging indicator)");
        }
        current.raiseAlert(21); // Alert code 21: PMIC Charge Timeout (critical)
        break;
      default:
        if (persistentAfterImmediateChargeFaultReset) {
          Log.error("PMIC: charge fault persists after reset - alert 21");
          current.raiseAlert(21);
        } else {
          Log.warn("PMIC: Charge fault detected (code=0x%02x)", chargeFault);
          current.raiseAlert(23); // Alert code 23: PMIC Battery Fault
        }
        break;
    }
    
    // Smart remediation with escalation and thrash prevention
    // CRITICAL SAFETY CHECK: Never attempt remediation if charging is disabled due to temperature
    if (!safeToCharge) {
      Log.info("PMIC: Fault detected but charging disabled due to temperature (%.1fC) - skipping remediation", 
               (double)current.get_internalTempC());
      // Don't escalate fault counters when temperature is the issue
      // Temperature will recover naturally without intervention
        remediationInProgress = false;
        remediationActiveLevel = 0;
        remediationPhase = 0;
    } else {
      unsigned long now = millis();
        // If we have an in-progress remediation, advance it without blocking.
        if (remediationInProgress) {
          switch (remediationActiveLevel) {
            case 1: {
              // Level 1: disableCharging -> wait 500ms -> enableCharging
              if (remediationPhase == 0) {
                pmic.disableCharging();
                remediationPhaseStartMs = now;
                remediationPhase = 1;
              } else if (now - remediationPhaseStartMs >= 500UL) {
                pmic.enableCharging();
                Log.info("PMIC: Charging re-enabled after soft reset");
                remediationInProgress = false;
                remediationActiveLevel = 0;
                remediationPhase = 0;
                lastRemediationAttempt = now;
              }
              break;
            }
            case 2: {
              // Level 2: disableCharging -> wait 1000ms -> set watchdog -> enableCharging
              if (remediationPhase == 0) {
                pmic.disableCharging();
                remediationPhaseStartMs = now;
                remediationPhase = 1;
              } else if (now - remediationPhaseStartMs >= 1000UL) {
                // Set watchdog to force reset if charging doesn't recover
                pmic.setWatchdog(0b01); // 40 seconds
                pmic.enableCharging();
                Log.info("PMIC: Charging re-enabled with watchdog supervision");
                remediationInProgress = false;
                remediationActiveLevel = 0;
                remediationPhase = 0;
                remediationLevel = 0; // Reset level after power cycle attempt
                lastRemediationAttempt = now;
              }
              break;
            }
            default:
              remediationInProgress = false;
              remediationActiveLevel = 0;
              remediationPhase = 0;
              break;
          }
        } else {
          // Not in progress; decide whether to start a remediation attempt.
          if (now - lastRemediationAttempt > REMEDIATION_COOLDOWN) {
            // Escalate remediation level based on consecutive faults
            if (consecutiveFaults >= 3 && remediationLevel < 2) {
              remediationLevel = 2; // Escalate to power cycle reset
            } else if (consecutiveFaults >= 2 && remediationLevel < 1) {
              remediationLevel = 1; // Escalate to disable/enable charging
            }

            switch (remediationLevel) {
              case 1:
                Log.warn("PMIC: Attempting soft remediation - cycle charging (level 1)");
                remediationInProgress = true;
                remediationActiveLevel = 1;
                remediationPhase = 0;
                break;

              case 2:
                Log.error("PMIC: Attempting aggressive remediation - power cycle reset (level 2)");
                remediationInProgress = true;
                remediationActiveLevel = 2;
                remediationPhase = 0;
                break;

              default:
                Log.info("PMIC: Fault detected but remediation level 0 - monitoring only");
                break;
            }
          } else {
            unsigned long remainingCooldown = (REMEDIATION_COOLDOWN - (now - lastRemediationAttempt)) / 60000;
            Log.info("PMIC: Fault detected but in cooldown period (%lu min remaining)", remainingCooldown);
          }
        }
    }
  } else {
    // No faults detected - clear counters if charging is healthy
    immediateChargeFaultResetConsumed = false;
    immediateChargeFaultResetFaultReg = 0;
    if (consecutiveFaults > 0) {
      Log.info("PMIC: Charging healthy - clearing fault counters");
      consecutiveFaults = 0;
      remediationLevel = 0;

        // Clear any pending remediation sequencing now that faults are gone.
        remediationInProgress = false;
        remediationActiveLevel = 0;
        remediationPhase = 0;
      
      // Clear PMIC-related alerts if they were active
      int8_t currentAlert = current.get_alertCode();
      if (currentAlert >= 20 && currentAlert <= 23) {
        Log.info("PMIC: Clearing battery/charging alert %d - charging resumed", currentAlert);
        current.set_alertCode(0);
        current.set_lastAlertTime(0);
      }
    }
    
    // Narrow recovery path for Alert 21 (charge timeout/stuck charging)
    // Clear when PMIC fault cleared and not in stuck fast-charging state
    int8_t currentAlert = current.get_alertCode();
    if (currentAlert == 21 && chargeStatus != 2) {
      // Alert 21 active, no PMIC fault, and not in fast charging - charge recovered
      Log.info("PMIC: clearing alert 21 after charge recovery");
      current.set_alertCode(0);
      current.set_lastAlertTime(0);
    }
  }

  static byte lastLoggedPmicSystemStatus = 0xFF;
  static byte lastLoggedPmicFaultReg = 0xFF;
  static int lastLoggedPmicPowerSource = -999;
  static PowerInputProfile lastLoggedPmicProfile = PowerInputProfile::NotApplicable;
#if defined(ENABLE_PMIC_FORENSICS) && ENABLE_PMIC_FORENSICS
  static uint8_t consecutivePmicContradictions = 0;
#endif
  const uint8_t systemBattState = battState;
  const uint8_t pmicBattState = batteryStateFromPmicStatus(chargeStatus, faultReg);
  if (sampleCanBeAuthoritative) {
    _authoritativeBatteryState = pmicBattState;
  }
  current.set_batteryState(pmicBattState);

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
             compactProfileLabel(powerReport.activeInputProfile));
  }
#endif

  // ========================================================================
  // Stale SOC Detection (Phase 1: detection and instrumentation only)
  // ========================================================================
  // Detects fuel gauge SOC that appears stale (not updated after charge).
  // Conservative detection criteria:
  // - SOC < 30%
  // - vCell >= 4.10V (preferred) or >= 4.05V (lower confidence)
  // - External power present (VBUS active)
  // - PMIC state: Charged/DONE or NotCharging with powerGood
  // - No PMIC fault
  // - Requires 2-3 consecutive samples before flagging
  //
  // Phase 1 scope: Detection and reporting only. No automatic remediation.
  {
    const float effectiveSoc = rejectAuthoritativeOverwrite ? _authoritativeBatterySoc : soc;
    const bool externalPowerPresent = (vbusStatus == 1 || vbusStatus == 2 || vbusStatus == 3);
    const bool pmicStateChargeDone = (chargeStatus == 3); // DONE
    const bool pmicStateNotChargingWithPower = (chargeStatus == 0 && powerGood);
    const bool pmicNoFault = !(faultReg & 0x38);
    const bool vcellHighConfidence = (vcell >= 4.10f);
    const bool vcellLowConfidence = (vcell >= 4.05f && vcell < 4.10f);
    
    const bool staleSocConditionsMet =
        batterySocIsValid(effectiveSoc) &&
        effectiveSoc < 30.0f &&
        (vcellHighConfidence || vcellLowConfidence) &&
        externalPowerPresent &&
        (pmicStateChargeDone || pmicStateNotChargingWithPower) &&
        pmicNoFault;
    
    if (staleSocConditionsMet) {
      if (staleSocConsecutiveCount < 0xFF) {
        staleSocConsecutiveCount++;
      }
      
      // Require 2 consecutive samples before flagging (conservative)
      if (staleSocConsecutiveCount >= 2) {
        if (staleSocTotalCount < 0xFFFF) {
          staleSocTotalCount++;
        }
        
        // Log and publish once when first detected (not every sample)
        static bool staleSocLoggedThisSession = false;
        static bool staleSocPublishedThisSession = false;
        if (!staleSocLoggedThisSession) {
          const char *confidenceLabel = vcellHighConfidence ? "HIGH" : "LOW";
          Log.warn("STALE_SOC: suspected stale fuel gauge reading - soc=%.2f vcell=%.3fV (conf=%s) vbus=%s chg=%s pg=%d fault=0x%02X src=%d consecutive=%u total=%u",
                   (double)effectiveSoc,
                   (double)vcell,
                   confidenceLabel,
                   compactPmicVbusLabel(vbusStatus),
                   compactPmicChargeLabel(chargeStatus, faultReg),
                   powerGood ? 1 : 0,
                   faultReg,
                   powerSource,
                   (unsigned)staleSocConsecutiveCount,
                   (unsigned)staleSocTotalCount);
          staleSocLoggedThisSession = true;
          
          // Publish forensics event for remote visibility
          if (!staleSocPublishedThisSession) {
            publishStaleSocForensics(effectiveSoc, vcell, vcellHighConfidence,
                                     chargeStatus, vbusStatus, powerGood, faultReg,
                                     powerSource, staleSocTotalCount,
                                     current.get_internalTempC());
            staleSocPublishedThisSession = true;
          }
        }
        
        // Capture diagnostic snapshot in WakeCycleStats
        Observability::WakeCycleStats &stats = Observability::cycleStats();
        stats.stale_soc_suspected = true;
        stats.battery_vcell_mv = (uint16_t)(vcell * 1000.0f);
        stats.pmic_charge_status = chargeStatus;
        stats.pmic_vbus_status = vbusStatus;
        stats.pmic_power_source = (powerSource >= 0 && powerSource <= 0xFF) ? (uint8_t)powerSource : 0xFF;
        stats.pmic_power_good = powerGood ? 1 : 0;
        stats.pmic_fault_reg = faultReg;
        stats.stale_soc_total_count = staleSocTotalCount;
      }
    } else {
      // Reset consecutive counter when conditions not met
      staleSocConsecutiveCount = 0;
    }
  }

  const uint8_t finalLoggedBattState = pmicBattState;
  const bool pmicStatusChanged =
      systemStatus != lastLoggedPmicSystemStatus || faultReg != lastLoggedPmicFaultReg;
  const bool pmicRouteChanged =
      powerSource != lastLoggedPmicPowerSource ||
      powerReport.activeInputProfile != lastLoggedPmicProfile;
#if defined(ENABLE_PMIC_TRACE) && ENABLE_PMIC_TRACE
  const bool pmicTraceEnabled = true;
#else
  const bool pmicTraceEnabled = false;
#endif
  const bool pmicFaultActive = (faultReg != 0);
  if (pmicTraceEnabled || pmicFaultActive) {
    Log.info("PMIC: vbus=%s chg=%s fault=%02x pg=%d th=%s vsys=%d prof=%s src=%s",
             compactPmicVbusLabel(vbusStatus),
             compactPmicChargeLabel(chargeStatus, faultReg),
             faultReg,
             powerGood ? 1 : 0,
             thermalRegulation ? "REG" : "OK",
             inVsysMin ? 1 : 0,
             compactProfileLabel(powerReport.activeInputProfile),
             PowerManager::powerSourceLabel(powerSource));
    lastLoggedPmicSystemStatus = systemStatus;
    lastLoggedPmicFaultReg = faultReg;
    lastLoggedPmicPowerSource = powerSource;
    lastLoggedPmicProfile = powerReport.activeInputProfile;
  } else {
    (void)pmicStatusChanged;
    (void)pmicRouteChanged;
  }
  if (powerReport.activeInputProfile == PowerInputProfile::Solar35W && vbusStatus == 1) {
    Log.warn("Power mismatch: prof=SOLAR vbus=USB");
  }

  const bool allowPreSleepBatteryLog =
      (sampleContext != BatterySampleContext::PreSleep) || (ENABLE_SLEEP_TRACE != 0);
  if (logBatteryDetail && allowPreSleepBatteryLog) {
    Log.info("%s: soc=%.2f raw=%.2f norm=%.2f vcell=%.3f authority=%s state=%s(%d) systemState=%s(%d) power=%s(%d)%s",
             batterySampleContextPrefix(sampleContext),
             (double)loggedSoc,
             (double)rawFuelGaugeSoc,
             (double)normalizedSoc,
             (double)vcell,
             authorityTag,
             batteryContext[(finalLoggedBattState <= 6) ? finalLoggedBattState : 0],
             finalLoggedBattState,
             batteryContext[(systemBattState <= 6) ? systemBattState : 0],
             systemBattState,
             PowerManager::powerSourceLabel(powerSource),
             powerSource,
             rejectAuthoritativeOverwrite ? " candidateRejected=true" : "");
  }

  {
    static bool chargeSummaryBaselineValid = false;
    static unsigned long chargeSummaryBaselineMs = 0;
    static float chargeSummaryBaselineSoc = 0.0f;
    static float chargeSummaryBaselineVcell = 0.0f;
    static int chargeSummaryBaselineSource = -1;
    static PowerInputProfile chargeSummaryBaselineProfile = PowerInputProfile::NotApplicable;

    const unsigned long nowMs = millis();
    const bool sourceOrProfileChanged =
        chargeSummaryBaselineValid &&
        (chargeSummaryBaselineSource != powerSource ||
         chargeSummaryBaselineProfile != powerReport.activeInputProfile);

    if (!chargeSummaryBaselineValid || sourceOrProfileChanged) {
      chargeSummaryBaselineValid = true;
      chargeSummaryBaselineMs = nowMs;
      chargeSummaryBaselineSoc = soc;
      chargeSummaryBaselineVcell = vcell;
      chargeSummaryBaselineSource = powerSource;
      chargeSummaryBaselineProfile = powerReport.activeInputProfile;
      if (sysStatus.get_verboseMode()) {
        Log.info("Charge: base soc=%.2f v=%.3f",
                 (double)soc,
                 (double)vcell);
      }
    } else if ((nowMs - chargeSummaryBaselineMs) >= CHARGE_SUMMARY_INTERVAL_MS) {
      const Observability::WakeCycleStats &stats = Observability::cycleStats();
      Log.info("Charge: soc=%.1f d15=%+.2f v=%.3f dv=%+.3f chg=%s src=%s prof=%s a=%lus c=%lus t=%lus",
               (double)soc,
               (double)(soc - chargeSummaryBaselineSoc),
               (double)vcell,
               (double)(vcell - chargeSummaryBaselineVcell),
               compactPmicChargeLabel(chargeStatus, faultReg),
               PowerManager::powerSourceLabel(powerSource),
               compactProfileLabel(powerReport.activeInputProfile),
               (unsigned long)(currentWakeAwakeMs() / 1000UL),
               (unsigned long)(stats.connect_duration_ms / 1000UL),
               (unsigned long)(stats.teardown_duration_ms / 1000UL));
      chargeSummaryBaselineMs = nowMs;
      chargeSummaryBaselineSoc = soc;
      chargeSummaryBaselineVcell = vcell;
      chargeSummaryBaselineSource = powerSource;
      chargeSummaryBaselineProfile = powerReport.activeInputProfile;
    }
  }

  // Detect stuck charging state (charging for >6 hours at same SoC)
  static uint8_t lastChargeStatus = 0xFF;
  static unsigned long chargeStateStartTime = 0;
  static float fastChargeStartSoc = -1.0f;
  static float fastChargeStartVcell = -1.0f;

  if (chargeStatus == 2) { // Fast Charging
    if (lastChargeStatus != 2) {
      chargeStateStartTime = millis(); // Just entered fast charging
      fastChargeStartSoc = soc;
      fastChargeStartVcell = vcell;
    } else if (chargeStateStartTime != 0) {
      const float socGain = batterySocIsValid(fastChargeStartSoc) ? (soc - fastChargeStartSoc) : 0.0f;
      const float vcellGain = batteryVoltageLooksUsable(fastChargeStartVcell) ? (vcell - fastChargeStartVcell) : 0.0f;
      const bool meaningfulChargeProgress = (socGain >= 0.5f) || (vcellGain >= 0.015f);
      if (meaningfulChargeProgress) {
        chargeStateStartTime = millis();
        fastChargeStartSoc = soc;
        fastChargeStartVcell = vcell;
      } else if (millis() - chargeStateStartTime > 6UL * 3600000UL) { // 6 hours
        const unsigned long awakeMs = currentWakeAwakeMs();
        const unsigned long connectMs = Observability::cycleStats().connect_duration_ms;
        const unsigned long teardownMs = Observability::cycleStats().teardown_duration_ms;
        const bool ambiguousHighLoad = inVsysMin || thermalRegulation || awakeMs > 300000UL ||
                                       connectMs > 120000UL || teardownMs > 10000UL;
        if (ambiguousHighLoad) {
          Log.warn("PMIC: Fast Charging 6+ hours with limited gain under load socGain=%.2f vcellGain=%.3f awake=%lu conn=%lu td=%lu vsysMin=%d thermal=%d",
                   (double)socGain,
                   (double)vcellGain,
                   awakeMs,
                   connectMs,
                   teardownMs,
                   inVsysMin ? 1 : 0,
                   thermalRegulation ? 1 : 0);
          chargeStateStartTime = millis();
          fastChargeStartSoc = soc;
          fastChargeStartVcell = vcell;
        } else {
          Log.error("PMIC: Stuck in Fast Charging for 6+ hours with no material gain soc=%.1f socGain=%.2f vcell=%.3f vcellGain=%.3f",
                    (double)soc,
                    (double)socGain,
                    (double)vcell,
                    (double)vcellGain);
          current.raiseAlert(21); // Charge timeout alert
        }
      }
    }
  } else {
    chargeStateStartTime = 0; // Not charging or charge done
    fastChargeStartSoc = -1.0f;
    fastChargeStartVcell = -1.0f;
  }

  lastChargeStatus = chargeStatus;
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

  // -------------------------------------------------------------------------
  // Temperature source selection
  // -------------------------------------------------------------------------
  // Default behavior:
  //  - If a TMP112A is present on the I2C bus (Muon), prefer it.
  //  - Otherwise, use TMP36 (if wired) or platform-specific stub.
  //
  // Compile-time controls:
  //  - Define MUON_HAS_TMP112 to force enable the TMP112A path.
  //  - Define DISABLE_TMP112_AUTODETECT to skip probing for TMP112A.

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
    float tempC;
    if (!readTmp112TemperatureC(tempC) || !(tempC > -50.0f && tempC < 120.0f)) {
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
  // example, set manually for testing), falling back to 25C if unset.

  float tempC = current.get_internalTempC();
  if (!(tempC > -50.0f && tempC < 120.0f)) {
    tempC = 25.0f;
  }

  if (sysStatus.get_verboseMode()) {
    Log.info("P2/Photon2 stub: using internalTempC=%4.2f C (no TMP36 ADC)", (double)tempC);
  }

  current.set_internalTempC(tempC);

#else
  // Measure enclosure temperature using the TMP36 on the carrier board
  // (connected to TMP36_SENSE_PIN, typically A4).
  // Non-blocking sampling: spread 8 samples across multiple batteryState()
  // calls to avoid blocking the main loop. Each call takes one sample (~5µs ADC
  // read) until all samples are collected, then computes the average.
  if (tmp112Present) {
    // TMP112A already provided a temperature this cycle; skip TMP36 sampling.
    // This avoids unnecessary ADC activity on boards where both might exist.
    isItSafeToCharge();
    return current.get_stateOfCharge() > 20.0f;
  }
  pinMode(TMP36_SENSE_PIN, INPUT);

  const int TMP36_SAMPLES = 8;
  static int sampleIndex = 0;     // Tracks how many samples taken this cycle
  static int tmpRawSum = 0;       // Running sum of ADC readings

  if (sampleIndex < TMP36_SAMPLES) {
    // Take one ADC sample per call, accumulate into sum
    int v = analogRead(TMP36_SENSE_PIN);
    tmpRawSum += v;
    sampleIndex++;
    
    // Not done yet; use previous temperature value and return early.
    // Caller can call batteryState() again on next loop to continue sampling.
    return current.get_stateOfCharge() > 20.0f;
  }

  // All samples collected; compute average and reset for next cycle
  int tmpRaw = tmpRawSum / TMP36_SAMPLES;
  sampleIndex = 0;
  tmpRawSum = 0;

  // Consider extremely low readings as "sensor not present". With a TMP36,
  // even very cold temperatures should still be around 100mV (roughly 120
  // ADC counts on a 3.3V/12-bit ADC), so an average below ~50 counts is
  // effectively 0V at the pin.
  bool sensorOk = (tmpRaw > 50 && tmpRaw < 4000);
  float tempC = tmp36TemperatureC(tmpRaw);

  // If the TMP36 reading is clearly out of a plausible enclosure range
  // (for example, -50C from a raw 0 reading), or the sensor appears to be
  // disconnected, fall back to a prior stored value or a conservative
  // default so that charging guard rails and telemetry still operate with
  // a realistic value.
  if (!sensorOk || tempC < -20.0f || tempC > 80.0f) {
    float prev = current.get_internalTempC();
    float fallback = 25.0f; // conservative room-temperature default

    if (prev > -20.0f && prev < 80.0f) {
      fallback = prev;
    }

    Log.warn("TMP36 reading invalid or out of range (tmp36=%4.2f C, raw=%d, sensorOk=%s) - falling back to %4.2f C",
             (double)tempC, tmpRaw, sensorOk ? "true" : "false", (double)fallback);
    tempC = fallback;
  }

  current.set_internalTempC(tempC);

  // Optional debug: log enclosure temperature when verbose logging is enabled
  if (sysStatus.get_verboseMode()) {
    Log.info("Enclosure temperature (effective): %4.2f C (raw=%d)", (double)tempC, tmpRaw);
  }

#endif // PLATFORM_ID == 32 || PLATFORM_ID == 34

  // Apply temperature-based charging guard rails (see reference implementation).
  // On cellular platforms this will enable/disable PMIC charging based on
  // current.get_internalTempC(); on others it is a no-op.
  isItSafeToCharge();

  // Convenience: indicate whether battery is in a healthy range.
  return current.get_stateOfCharge() > 20.0f;
}

bool SensorManager::isItSafeToCharge() // Returns a true or false if the battery
                                       // is in a safe charging range based on
                                       // enclosure temperature
{
  float temp = current.get_internalTempC();
  // Apply simple hysteresis around the recommended LiPo charge range
  // to avoid rapid toggling near the temperature boundaries. When
  // charging is currently allowed, we disable if temp < 0C or > 45C.
  // When charging is currently disallowed, we only re-enable once
  // temp has returned to a tighter 2C-43C window.
  static bool lastSafe = true;
#if HAL_PLATFORM_CELLULAR
  const bool previousSafe = lastSafe;
#endif

  bool safe;
  if (lastSafe) {
    safe = !(temp < 0.0f || temp > 45.0f);
  } else {
    safe = !(temp < 2.0f || temp > 43.0f);
  }
  lastSafe = safe;

#if HAL_PLATFORM_CELLULAR
  // On Boron (cellular Gen 3), a BQ24195 PMIC is available so we
  // actually enable/disable charging based on the enclosure
  // temperature.
  const bool safeStateChanged = (safe != previousSafe);
  PMIC pmic(true);

  if (!safe) {
    pmic.disableCharging();
    current.set_batteryState(1); // Reflect that we are "Not Charging"

    Log.warn("Charging disabled due to enclosure temperature: %4.2f C", (double)temp);
  } else {
    pmic.enableCharging();

    if (safeStateChanged || sysStatus.get_verboseMode()) {
      Log.info("Charging enabled; enclosure temperature: %4.2f C", (double)temp);
    }
  }
#else
  // On platforms without a PMIC API (such as Argon, Photon 2 / P2), we
  // do not control charging, but we still evaluate and log whether it
  // would be considered safe based on the same temperature range.
  if (!safe) {
    Log.warn("Charging would be disabled due to enclosure temperature: %4.2f C (no PMIC on this platform)", (double)temp);
  } else if (sysStatus.get_verboseMode()) {
    Log.info("Charging would be enabled; enclosure temperature: %4.2f C (no PMIC on this platform)", (double)temp);
  }
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

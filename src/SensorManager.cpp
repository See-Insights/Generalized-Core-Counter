// Battery conect information -
// https://docs.particle.io/reference/device-os/firmware/boron/#batterystate-
const char *batteryContext[7] = {"Unknown",    "Not Charging", "Charging",
                                 "Charged",    "Discharging",  "Fault",
                                 "Diconnected"};

// Particle Functions
#include "SensorManager.h"
#include "MyPersistentData.h"  // Access sysStatus/sensorConfig
#include "SensorFactory.h"
#include "device_pinout.h"     // TMP36_SENSE_PIN for enclosure temperature

// Device-specific includes and definitions
// Use Particle feature detection for automatic platform identification

// FuelGauge fuelGauge;                                // Needed to address
// issue with updates in low battery state

SensorManager *SensorManager::_instance;

// [static]
SensorManager &SensorManager::instance() {
  if (!_instance) {
    _instance = new SensorManager();
  }
  return *_instance;
}
SensorManager::SensorManager() : _sensor(nullptr), _lastPollTime(0) {}

SensorManager::~SensorManager() {}

void SensorManager::setup() {
    Log.info("Initializing SensorManager");
    
    if (!_sensor) {
        Log.error("No sensor assigned! Call setSensor() first.");
        return;
    }
    
    if (!_sensor->setup()) {
        Log.error("Sensor setup failed");
    } else {
        Log.info("Sensor setup completed: %s", _sensor->getSensorType());
    }
}

void SensorManager::setSensor(ISensor* sensor) {
    if (sensor) {
        _sensor = sensor;
        Log.info("Sensor set: %s", sensor->getSensorType());
    } else {
        Log.error("Attempted to set null sensor");
    }
}

  void SensorManager::initializeFromConfig() {
    Log.info("Initializing sensor from configuration");

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
      Log.info("Sensor hardware initialized; type=%d, usesInterrupt=%s", (int)sensorType,
               _sensor->usesInterrupt() ? "true" : "false");
    }
  }

bool SensorManager::loop() {
    if (!_sensor || !_sensor->isReady()) {
        return false;
    }
    
    unsigned long currentTime = millis();
    uint16_t pollingRate = sensorConfig.get_pollingRate() * 1000; // Convert to ms
    
  // Interrupt-driven sensors should be serviced on every pass through
  // the main loop regardless of pollingRate.
  if (_sensor->usesInterrupt() || pollingRate == 0) {
    bool event = _sensor->loop();
    if (event && sysStatus.get_verboseMode()) {
      Log.info("SensorManager: event reported by interrupt-driven sensor");
    }
    return event;
  }
    
    // Polling mode - check sensor at specified intervals
    if (currentTime - _lastPollTime >= pollingRate) {
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
    Log.info("SensorManager entering sleep; notifying sensor %s", _sensor->getSensorType());
    _sensor->onSleep();
  }
}

void SensorManager::onExitSleep() {
  if (_sensor) {
    Log.info("SensorManager exiting sleep; waking sensor %s", _sensor->getSensorType());
    if (!_sensor->onWake()) {
      Log.error("Sensor %s failed to wake correctly", _sensor->getSensorType());
    }
  }
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

bool SensorManager::batteryState() {

#if HAL_PLATFORM_CELLULAR || PLATFORM_ID == PLATFORM_ARGON
  // Boron (cellular) and Argon (Wi-Fi) Gen 3 devices:
  // Use built-in System battery APIs backed by the fuel gauge
  // (and a BQ24195 PMIC on Boron only).
  current.set_batteryState(System.batteryState());
  current.set_stateOfCharge(System.batteryCharge());

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

  // Coarse battery state: we don't have a reliable CHG pin on Photon 2/P2
  // (per docs), so infer from voltage only.
  uint8_t battState = 0; // Unknown
  if (voltage >= 4.1f) {
    battState = 3; // "Charged"
  } else if (voltage >= 3.6f) {
    battState = 2; // "Charging/Normal"
  } else if (voltage > 0.1f) {
    battState = 4; // "Discharging / Low"
  }
  current.set_batteryState(battState);

#else
  // Other Wi-Fi / SoM platforms: leave battery fields unchanged for now.
#endif

#if PLATFORM_ID == 32 || PLATFORM_ID == 34
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
  // Take multiple samples and average to reduce noise, and explicitly
  // detect the "sensor missing / grounded" case (very low ADC reading).
  pinMode(TMP36_SENSE_PIN, INPUT);

  const int TMP36_SAMPLES = 8;
  int tmpRawSum = 0;
  int tmpRaw = 0;
  for (int i = 0; i < TMP36_SAMPLES; i++) {
    int v = analogRead(TMP36_SENSE_PIN);
    tmpRawSum += v;
    // Short settle delay between samples; keeps cloud serviced via
    // background System threads so we can use delay() safely here.
    delay(5);
  }
  tmpRaw = tmpRawSum / TMP36_SAMPLES;

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

  // Reference range based on typical LiPo specs (0C to 45C). Below 0C,
  // most manufacturers recommend no charging at all.
  bool safe = !(temp < 0.0f || temp > 45.0f);

#if HAL_PLATFORM_CELLULAR
  // On Boron (cellular Gen 3), a BQ24195 PMIC is available so we
  // actually enable/disable charging based on the enclosure
  // temperature.
  PMIC pmic(true);

  if (!safe) {
    pmic.disableCharging();
    current.set_batteryState(1); // Reflect that we are "Not Charging"

    Log.warn("Charging disabled due to enclosure temperature: %4.2f C", (double)temp);
  } else {
    pmic.enableCharging();

    if (sysStatus.get_verboseMode()) {
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
  char signalStr[64];  // Declare outside platform-specific blocks
  
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

  snprintf(signalStr, sizeof(signalStr), "%s S:%2.0f%%, Q:%2.0f%% ",
           radioTech[rat], strengthPercentage, qualityPercentage);
  Log.info(signalStr);
#elif HAL_PLATFORM_WIFI
  WiFiSignal sig = WiFi.RSSI();
  float strengthPercentage = sig.getStrength();
  float qualityPercentage = sig.getQuality();
  
  snprintf(signalStr, sizeof(signalStr), "WiFi S:%2.0f%%, Q:%2.0f%% ",
           strengthPercentage, qualityPercentage);
  Log.info(signalStr);
#endif
}

/**
 * @brief soft delay let's us process Particle functions and service the sensor
 * interrupts while pausing
 *
 * @details takes a single unsigned long input in millis
 *
 */
inline void SensorManager::softDelay(uint32_t t) {
  for (uint32_t ms = millis(); millis() - ms < t; Particle.process())
    ; //  safer than a delay()
}

// Battery conect information -
// https://docs.particle.io/reference/device-os/firmware/boron/#batterystate-
const char *batteryContext[7] = {"Unknown",    "Not Charging", "Charging",
                                 "Charged",    "Discharging",  "Fault",
                                 "Diconnected"};

// Particle Functions
#include "SensorManager.h"
#include "MyPersistentData.h"  // Add this line to access sensorConfig and current

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

bool SensorManager::loop() {
    if (!_sensor || !_sensor->isReady()) {
        return false;
    }
    
    unsigned long currentTime = millis();
    uint16_t pollingRate = sensorConfig.get_pollingRate() * 1000; // Convert to ms
    
    // If pollingRate is 0, sensor operates in interrupt mode
    if (pollingRate == 0) {
        return _sensor->loop();
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

  // fuelGauge.quickStart();                                               //
  // May help us re-establish a baseline for SoC softDelay(1000);

  /*
  current.set_batteryState(System.batteryState());                      // Call
  before isItSafeToCharge() as it may overwrite the context
  current.set_stateOfCharge(System.batteryCharge());                   // Assign
  to system value
  */

  // if (current.get_stateOfCharge() > 60) return true;
  // else return false;

  return true;
}

bool SensorManager::isItSafeToCharge() // Returns a true or false if the battery
                                       // is in a safe charging range - only
                                       // works for Boron
{
#if HAL_PLATFORM_CELLULAR
  PMIC pmic(true);
  if (current.get_internalTempC() < 0 ||
      current.get_internalTempC() >
          37) {             // Reference: (32 to 113 but with safety)
    pmic.disableCharging(); // It is too cold or too hot to safely charge the
                            // battery
    current.set_batteryState(1); // Overwrites the values from the batteryState
                                 // API to reflect that we are "Not Charging"
    return false;
  } else {
    pmic.enableCharging(); // It is safe to charge the battery
    return true;
  }
#else
  return true; // If we are not using the cellular radio, then we charge
               // regardless of temperature
#endif
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

/**
 * @file SensorManager.h
 * @author Chip McClelland (chip@seeinisghts.com)
 * @brief Singleton wrapper around ISensor implementations.
 *
 * @details SensorManager owns a single ISensor instance and handles
 *          initialization, polling, and utility helpers like battery
 *          status, temperature conversion, and signal strength reporting.
 *          It provides a uniform interface to the rest of the firmware,
 *          regardless of which concrete sensor is attached.
 */

#ifndef SENSORMANAGER_H
#define SENSORMANAGER_H

#include "Particle.h"
#include "sensors/ISensor.h"

extern char internalTempStr[16];
extern char signalStr[64];

// Retained PMIC contradiction forensics.
extern retained uint16_t pmicAnomalyCount;
extern retained uint32_t lastPmicAnomalyUptimeMs;
extern retained float lastPmicAnomalySoc;
extern retained uint8_t lastPmicAnomalyChargeStatus;
extern retained uint8_t lastPmicAnomalyPowerSource;
extern retained uint8_t lastPmicAnomalyVbusStatus;
extern bool pmicAnomalyActive;
uint32_t pmicAnomalyAgeSec();

enum class BatterySampleContext : uint8_t {
    General = 0,
    Setup,
    PreSleep,
    PostWake,
};

/**
 * @brief Convenience macro for accessing the SensorManager singleton.
 *
 * Usage:
 * @code
 *   measure.setup();
 *   if (measure.loop()) {
 *       auto data = measure.getSensorData();
 *   }
 * @endcode
 */
#define measure SensorManager::instance()

class SensorManager {
public:
    /**
     * @brief Get the SensorManager singleton instance.
     */
    static SensorManager &instance();

    /**
     * @brief Initialize the active sensor and any manager state.
     */
    void setup();

    /**
     * @brief Poll the active sensor; call from the main loop.
     *
     * @return true if new sensor data is available.
     */
    bool loop();

    /**
     * @brief Set the concrete ISensor implementation to use.
     *
     * @param sensor Pointer to a concrete ISensor (not owned).
     */
    void setSensor(ISensor* sensor);

    /**
     * @brief Get the latest sensor data from the active sensor.
     */
    SensorData getSensorData() const;

    /**
     * @brief Check whether the active sensor is initialized and ready.
     */
    bool isSensorReady() const;

    /**
     * @brief Create and initialize the active sensor based on configuration.
     *
     * Uses sysStatus.get_sensorType() and SensorFactory to select the
     * concrete implementation, then calls the sensor's initializeHardware().
     */
    void initializeFromConfig();

    /**
     * @brief Notify the sensor that the device is entering deep sleep.
     */
    void onEnterSleep();

    /**
     * @brief Notify the sensor that the device has woken from deep sleep.
     */
    void onExitSleep();

    /**
     * @brief Mark that the next battery sample occurs immediately after low-power wake.
     */
    void noteWakeFromLowPowerSleep();

    /** @name Utility functions
     *  Helpers for temperature, battery, radio signal, and delays.
     */
    ///@{
    /**
     * @brief Convert TMP36 ADC reading to degrees Celsius.
     *
     * @param adcValue Raw ADC value from the TMP36 input.
     * @return Temperature in degrees Celsius.
     */
    float tmp36TemperatureC(int adcValue);

    /**
     * @brief Read TMP112A temperature (I2C) in degrees Celsius.
     *
     * Intended for platforms/carriers (like Muon) that include an onboard
     * TMP112A temperature sensor.
     *
     * @param[out] tempC Filled with temperature in degrees Celsius on success.
     * @return true on success, false on I2C error or missing device.
     */
    bool readTmp112TemperatureC(float &tempC);

    /**
     * @brief Determine whether the battery is present and not critically low.
     */
    bool batteryState(BatterySampleContext sampleContext = BatterySampleContext::General);

    bool cachedBatteryVoltage(float &vcell) const;
    const char *cachedChargeStateLabel() const;

    /**
     * @brief Determine whether it is safe to charge the battery.
     */
    bool isItSafeToCharge();

    /**
     * @brief Update global signal strength strings for logging/telemetry.
     */
    void getSignalStrength();

    /**
     * @brief TEMPORARY DIAGNOSTIC: Test PMIC charge state machine behavior.
     * 
     * @details Controlled experiment to determine if PMIC is stuck in FAST_CHARGE
     *          or will transition to DONE when charging is cycled. Only executes
     *          once per boot with safety checks. Logs comprehensive PMIC state
     *          before/during/after disabling and re-enabling charging.
     * 
     * @return 0 on success, negative error code on failure
     */
    int runPmicChargeCycleTest();

    ///@}
    
protected:
    SensorManager();
    virtual ~SensorManager();
    SensorManager(const SensorManager &) = delete;
    SensorManager &operator=(const SensorManager &) = delete;
    
    /** @brief Pointer to the singleton instance. */
    static SensorManager *_instance;

    /** @brief Currently active sensor implementation (not owned). */
    ISensor* _sensor;

    /** @brief Timestamp of the last sensor poll (millis). */
    unsigned long _lastPollTime;

    /** @brief One-shot flag to stabilize battery measurement after low-power wake. */
    bool _batteryStabilizationPending;

    /** @brief Track whether the first post-boot battery sample has already run. */
    bool _firstBatterySampleTaken;

    /** @brief Whether this wake cycle has a trusted pre-radio battery sample. */
    bool _authoritativeBatterySampleActive;

    /** @brief Cached SoC from the trusted pre-radio battery sample. */
    float _authoritativeBatterySoc;

    /** @brief Cached battery state from the trusted pre-radio battery sample. */
    uint8_t _authoritativeBatteryState;

    /** @brief Whether the trusted pre-radio sample used voltage fallback. */
    bool _authoritativeBatteryFallbackUsed;

    /** @brief Cached battery voltage from the latest batteryState() sample. */
    float _cachedBatteryVcell;

    /** @brief Cached compact PMIC charge label from the latest batteryState() sample. */
    const char *_cachedChargeStateLabel;

    /** @brief Whether the cached battery voltage is valid. */
    bool _cachedBatteryVcellValid;
};

#endif /* SENSORMANAGER_H */
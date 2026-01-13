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
#include "ISensor.h"

extern char internalTempStr[16];
extern char signalStr[64];

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
     * @brief Determine whether the battery is present and not critically low.
     */
    bool batteryState();

    /**
     * @brief Determine whether it is safe to charge the battery.
     */
    bool isItSafeToCharge();

    /**
     * @brief Update global signal strength strings for logging/telemetry.
     */
    void getSignalStrength();

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
};

#endif /* SENSORMANAGER_H */
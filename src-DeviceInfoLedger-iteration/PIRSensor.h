// src/PIRSensor.h
#ifndef PIRSENSOR_H
#define PIRSENSOR_H

#include "ISensor.h"
#include "Particle.h"

/**
 * @brief PIR (Passive Infrared) Motion Sensor Implementation
 * 
 * This is a placeholder/template for future PIR sensor implementation.
 * PIR sensors detect motion by measuring infrared light radiating from objects.
 * 
 * Typical usage:
 * - Motion detection for occupancy tracking
 * - Presence detection for people counting
 * - Security/intrusion detection
 * 
 * Hardware Requirements:
 * - PIR sensor module (e.g., HC-SR501, AM312)
 * - Digital input pin for sensor signal
 * - Power supply (typically 3.3V or 5V)
 */
class PIRSensor : public ISensor {
public:
    /**
     * @brief Get singleton instance
     */
    static PIRSensor& instance() {
        static PIRSensor _instance;
        return _instance;
    }

    /**
     * @brief Initialize the PIR sensor
     * @return true if initialization successful
     */
    bool setup() override {
        Log.info("PIR Sensor setup - basic interrupt-driven implementation");
        _isReady = true;  // Mark as ready - this is a simple sensor
        return true;      // Return true so device doesn't reset
    }

    /**
     * @brief Poll the PIR sensor for motion detection
     * @return true if new motion detected
     * 
     * @note Currently interrupt-driven only. Returns false in polling mode.
     *       Motion will be detected via interrupt in the main code.
     */
    bool loop() override {
        // PIR sensor is interrupt-driven, not polled
        // Motion detection happens in sensorISR() in main code
        return false;
    }

    /**
     * @brief Get latest sensor reading
     * @return SensorData with motion detection info
     */
    SensorData getData() const override {
        return _data;
    }

    /**
     * @brief Get sensor type identifier
     */
    const char* getSensorType() const override {
        return "PIR";  // String literal, no allocation
    }

    /**
     * @brief Check if sensor is ready
     */
    bool isReady() const override {
        return _isReady;
    }

    /**
     * @brief Reset sensor state
     */
    void reset() override {
        _data = SensorData();
        strncpy(_data.sensorType, "PIR", sizeof(_data.sensorType) - 1);
        _data.sensorType[sizeof(_data.sensorType) - 1] = '\0';
        // TODO: Reset any sensor-specific state
    }

private:
    PIRSensor() : _isReady(false) {
        strncpy(_data.sensorType, "PIR", sizeof(_data.sensorType) - 1);
        _data.sensorType[sizeof(_data.sensorType) - 1] = '\0';  // Ensure null termination
    }
    ~PIRSensor() {}
    
    // Prevent copying
    PIRSensor(const PIRSensor&) = delete;
    PIRSensor& operator=(const PIRSensor&) = delete;

    bool _isReady;
    SensorData _data;
    
    // TODO: Add PIR-specific private members:
    // pin_t _motionPin;
    // bool _motionDetected;
    // unsigned long _lastMotionTime;
};

#endif /* PIRSENSOR_H */

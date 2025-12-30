// src/PIRSensor.h
#ifndef PIRSENSOR_H
#define PIRSENSOR_H

#include "ISensor.h"
#include "Particle.h"
#include "device_pinout.h"
#include "MyPersistentData.h"  // for sysStatus (verboseMode)

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
        // Configure PIR-related pins from device_pinout
        pinMode(intPin, INPUT_PULLDOWN);   // PIR interrupt output with pull-down
        pinMode(disableModule, OUTPUT);    // Sensor enable line (active LOW)
        pinMode(ledPower, OUTPUT);         // Sensor indicator LED power

        // Enable sensor and LED by default
        digitalWrite(disableModule, LOW);  // Bring low to turn sensor ON
        digitalWrite(ledPower, LOW);       // Active-LOW: bring low to turn LED ON

        // Attach interrupt on RISING edge (PIR output is active-high).
        attachInterrupt(intPin, pirISR, RISING);

        reset();           // Ensure SensorData is initialized
        _isReady = true;   // Mark as ready
        return true;       // Return true so device doesn't reset
    }

    /**
     * @brief Poll the PIR sensor for motion detection
     * @return true if new motion detected
     * 
     * @note Currently interrupt-driven only. Returns false in polling mode.
     *       Motion is detected via a hardware interrupt (pirISR) which sets
     *       an internal flag. This method returns true once per interrupt
     *       and clears the flag.
     */
    bool loop() override {
        if (!_isReady) {
            return false;
        }

        // If no motion flag, nothing to report
        if (!_motionDetected) {
            return false;
        }

        // Clear flag and apply debounce: ignore events within 500 ms
        _motionDetected = false;
        unsigned long nowMs = millis();
        if (nowMs - _lastEventMs < 500) {
            return false;
        }
        _lastEventMs = nowMs;

        _data.timestamp = Time.now();
        _data.hasNewData = true;
        return true;
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
        // Clear any pending motion
        _motionDetected = false;
    }

    /**
     * @brief This sensor uses a hardware interrupt for motion events.
     */
    bool usesInterrupt() const override { return true; }

    /**
     * @brief Prepare sensor for deep sleep: detach ISR and power down.
     */
    void onSleep() override {
        if (!_isReady) {
            return;
        }

        detachInterrupt(intPin);
        digitalWrite(disableModule, HIGH); // Disable sensor module
        digitalWrite(ledPower, HIGH);      // Active-LOW: bring high to turn LED OFF
        _isReady = false;
        Log.info("PIR sensor powered down for sleep");
    }

    /**
     * @brief Wake sensor from deep sleep: power up and re-attach ISR.
     */
    bool onWake() override {
        // For ULTRA_LOW_POWER naps we normally keep the PIR powered
        // and its interrupt attached across sleep so it can wake the
        // MCU. In that case we should NOT clear the motion flag here,
        // otherwise the wake-causing event is lost before the main
        // loop can count it.

        pinMode(disableModule, OUTPUT);
        pinMode(ledPower, OUTPUT);
        pinMode(intPin, INPUT_PULLDOWN);

        digitalWrite(disableModule, LOW);
        digitalWrite(ledPower, LOW);       // Active-LOW: LED ON after wake

        attachInterrupt(intPin, pirISR, RISING);

        _isReady = true;
        return true;
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

    // Debounce state: last accepted motion event time (ms)
    unsigned long _lastEventMs = 0;

    // PIR-specific state
    static volatile bool _motionDetected;  // Set by ISR, read/cleared in loop()
    static volatile uint32_t _isrCount;    // Counts how many times ISR fired

    // ISR handler
    static void pirISR();
};

#endif /* PIRSENSOR_H */

// src/ISensor.h
#ifndef ISENSOR_H
#define ISENSOR_H

#include "Particle.h"

/**
 * @brief Generic sensor data structure.
 *
 * This structure is intentionally generic so different sensor types
 * (PIR, ultrasonic, gesture, etc.) can share the same layout. The
 * meaning of each numeric/boolean field is defined by the @ref sensorType
 * string.
 *
 * Examples:
 *   - PIR: hasNewData=true when motion edge detected; primary/secondary unused.
 *   - GestureFace: primary=faceNumber, secondary=faceScore,
 *                  aux1=gestureType, aux2=gestureScore.
 *   - Ultrasonic: primary=distance (cm), secondary=signal quality, etc.
 *
 * Uses only POD types and fixed-size char[] to avoid heap use.
 */
struct SensorData {
    /** When the data was captured (Unix time). */
    time_t timestamp;

    /** Type of sensor ("PIR", "Ultrasonic", etc.). */
    char sensorType[16];

    /** Flag indicating if this record contains new data. */
    bool hasNewData;

    /** Main numeric value (count, distance, faceNumber, ...). */
    uint16_t primary;

    /** Secondary numeric value (score, quality, ...). */
    uint16_t secondary;

    /** Auxiliary numeric field 1. */
    uint16_t aux1;

    /** Auxiliary numeric field 2. */
    uint16_t aux2;

    /** Generic boolean flag (e.g., motionDetected, occupied, inRange). */
    bool flag1;

    /** Spare boolean flag for future use. */
    bool flag2;

    /**
     * @brief Construct a new SensorData with default values.
     */
    SensorData() : timestamp(0), hasNewData(false),
                   primary(0), secondary(0), aux1(0), aux2(0),
                   flag1(false), flag2(false) {
        sensorType[0] = '\0';
    }
    
    /**
     * @brief Convert sensor data to JSON string for publishing
     * @param buffer Character buffer to write JSON into
     * @param bufferSize Size of the buffer
     * @return true if JSON was created successfully
     */
    bool toJSON(char* buffer, size_t bufferSize) const;
};

/**
 * @brief Abstract interface for all sensors
 * 
 * This allows the main code to work with any sensor type without
 * knowing the implementation details. All sensors must implement this interface.
 */
class ISensor {
public:
    virtual ~ISensor() {}
    
    /**
     * @brief Initialize the sensor hardware
     * @return true if successful, false otherwise
     */
    virtual bool setup() = 0;
    
    /**
     * @brief Poll the sensor for new data
     * @return true if new data is available and stored, false otherwise
     */
    virtual bool loop() = 0;
    
    /**
     * @brief Get the latest sensor data
     * @return SensorData structure with current readings
     */
    virtual SensorData getData() const = 0;
    
    /**
     * @brief Get sensor type identifier
     * @return Pointer to const char identifying the sensor type (must remain valid)
     */
    virtual const char* getSensorType() const = 0;
    
    /**
     * @brief Check if sensor is initialized and ready
     * @return true if ready, false otherwise
     */
    virtual bool isReady() const = 0;
    
    /**
     * @brief Reset sensor state and clear any cached data
     */
    virtual void reset() = 0;

    /**
     * @brief Initialize underlying hardware after power-on.
     *
     * Default implementation just calls setup(); sensors with more
     * complex power sequencing can override this.
     */
    virtual bool initializeHardware() { return setup(); }

    /**
     * @brief Notification that the device is entering deep sleep.
     *
     * Sensors can override to detach interrupts and power down.
     */
    virtual void onSleep() {}

    /**
     * @brief Notification that the device is waking from deep sleep.
     *
     * Return false if the sensor failed to reinitialize correctly.
     */
    virtual bool onWake() { return true; }

    /**
     * @brief Whether this sensor uses a hardware interrupt for events.
     */
    virtual bool usesInterrupt() const { return false; }

    /**
     * @brief Health check for the sensor.
     *
     * Return false if a fault has been detected (e.g. stuck interrupt
     * line, repeated I2C errors, etc.).
     */
    virtual bool isHealthy() const { return true; }

    /**
     * @brief Last sensor-specific error code (if any).
     */
    virtual int lastErrorCode() const { return 0; }
};

// Implementation of SensorData::toJSON
inline bool SensorData::toJSON(char* buffer, size_t bufferSize) const {
    if (!buffer || bufferSize < 100) return false;
    
    JSONBufferWriter writer(buffer, bufferSize);
    writer.beginObject();
    
    writer.name("sensorType").value(sensorType);  // Already char*, no .c_str() needed
    writer.name("timestamp").value((int)timestamp);
    
    // Only include non-default values to save bandwidth
    if (primary > 0) {
        writer.name("primary").value(primary);
    }
    if (secondary > 0) {
        writer.name("secondary").value(secondary);
    }
    if (aux1 > 0) {
        writer.name("aux1").value(aux1);
    }
    if (aux2 > 0) {
        writer.name("aux2").value(aux2);
    }
    if (flag1) {
        writer.name("flag1").value(flag1);
    }
    if (flag2) {
        writer.name("flag2").value(flag2);
    }

    writer.endObject();
    
    return writer.dataSize() > 0;
}

#endif /* ISENSOR_H */
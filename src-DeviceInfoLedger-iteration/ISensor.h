// src/ISensor.h
#ifndef ISENSOR_H
#define ISENSOR_H

#include "Particle.h"

/**
 * @brief Generic sensor data structure
 * 
 * This structure holds data that can be populated by any sensor type.
 * Unused fields can be left at default values (0).
 * 
 * NOTE: Uses char array instead of String to avoid dynamic allocation
 */
struct SensorData {
    time_t timestamp;           // When the data was captured
    char sensorType[16];        // Type of sensor (e.g., "GestureFace", "PIR", "Ultrasonic") - Fixed size, no dynamic allocation
    bool hasNewData;            // Flag indicating if this is new data
    
    // Gesture/Face specific fields
    uint16_t faceNumber;        // Number of faces detected (0 if not applicable)
    uint16_t faceScore;         // Confidence score for face detection (0-100)
    uint16_t gestureType;       // Type of gesture detected (0 if none)
    uint16_t gestureScore;      // Confidence score for gesture (0-100)
    
    // Future sensor types can add fields here:
    // uint16_t peopleCount;    // For people counters
    // float distance;          // For ultrasonic sensors
    // bool motionDetected;     // For PIR sensors
    // uint16_t objectCount;    // For vehicle counters
    
    SensorData() : timestamp(0), hasNewData(false), faceNumber(0), 
                   faceScore(0), gestureType(0), gestureScore(0) {
        sensorType[0] = '\0';  // Initialize to empty string
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
};

// Implementation of SensorData::toJSON
inline bool SensorData::toJSON(char* buffer, size_t bufferSize) const {
    if (!buffer || bufferSize < 100) return false;
    
    JSONBufferWriter writer(buffer, bufferSize);
    writer.beginObject();
    
    writer.name("sensorType").value(sensorType);  // Already char*, no .c_str() needed
    writer.name("timestamp").value((int)timestamp);
    
    // Only include non-zero values to save bandwidth
    if (gestureType > 0) {
        writer.name("gesturetype").value(gestureType);
        writer.name("gesturescore").value(gestureScore);
    }
    if (faceNumber > 0) {
        writer.name("facenumber").value(faceNumber);
        writer.name("facescore").value(faceScore);
    }
    
    writer.endObject();
    
    return writer.dataSize() > 0;
}

#endif /* ISENSOR_H */
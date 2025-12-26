// src/SensorFactory.h
#ifndef SENSORFACTORY_H
#define SENSORFACTORY_H

#include "ISensor.h"
#include "PIRSensor.h"  // PIR is the default sensor

// Future sensors can be added here:
// #include "GestureFaceSensor.h"
// #include "UltrasonicSensor.h"

/**
 * @brief Enumeration of available sensor types
 * 
 * Add new sensor types here as they are implemented
 */
enum class SensorType : uint8_t {
    GESTURE_FACE = 0,
    PIR = 1,
    ULTRASONIC = 2,
    // Add more sensor types as needed
};

/**
 * @brief Factory for creating sensor instances
 * 
 * This centralizes sensor creation and makes it easy to switch sensors
 * without modifying the main application code.
 */
class SensorFactory {
public:
    /**
     * @brief Create a sensor instance based on the specified type
     * 
     * @param type The sensor type to instantiate
     * @return Pointer to ISensor implementation, or nullptr if type not implemented
     * 
     * @note To add a new sensor:
     *       1. Create the sensor class implementing ISensor
     *       2. Add the include at the top of this file
     *       3. Add a case statement here
     */
    static ISensor* createSensor(SensorType type) {
        switch(type) {
            case SensorType::PIR:
                Log.info("Creating PIR sensor");
                return &PIRSensor::instance();
            
            // Add more sensors as they are implemented
            // case SensorType::GESTURE_FACE:
            //     Log.info("Creating GestureFace sensor");
            //     return &GestureFaceSensor::instance();
            // 
            // case SensorType::ULTRASONIC:
            //     Log.info("Creating Ultrasonic sensor");
            //     return &UltrasonicSensor::instance();
            
            default:
                Log.error("Sensor type %d not yet implemented", (int)type);
                return nullptr;
        }
    }
    
    /**
     * @brief Get sensor type name as string
     * @param type The sensor type
     * @return const char* name of the sensor type (string literal, no allocation)
     */
    static const char* getSensorTypeName(SensorType type) {
        switch(type) {
            case SensorType::GESTURE_FACE: return "GestureFace";
            case SensorType::PIR: return "PIR";
            case SensorType::ULTRASONIC: return "Ultrasonic";
            default: return "Unknown";
        }
    }
};

#endif /* SENSORFACTORY_H */
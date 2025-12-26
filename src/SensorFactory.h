// src/SensorFactory.h
#ifndef SENSORFACTORY_H
#define SENSORFACTORY_H

#include "ISensor.h"
#include "PIRSensor.h"  // PIR is the default sensor

// Future sensors can be added here:
// #include "GestureFaceSensor.h"
// #include "UltrasonicSensor.h"

/**
 * @brief Enumeration of available sensor types (backward-compatible IDs).
 *
 * These numeric values are part of the external contract and must
 * remain stable across firmware versions so that previously deployed
 * devices and cloud tools interpret sensorType consistently.
 *
 *  -  0: VEHICLE_PRESSURE       (Vehicle Pressure Sensor)
 *  -  1: PIR                    (Pedestrian Infrared Sensor)
 *  -  2: VEHICLE_MAGNETOMETER   (Vehicle Magnetometer Sensor)
 *  -  3: RAIN_BUCKET            (Rain bucket / tipping bucket sensor)
 *  -  4: VIBRATION_BASIC        (Basic vibration / motion sensor)
 *  -  5: VIBRATION_ADVANCED     (Advanced vibration + magnetometer)
 *  - 10: INDOOR_OCCUPANCY       (Indoor room occupancy sensor)
 *  - 11: OUTDOOR_OCCUPANCY      (Outdoor occupancy sensor)
 *  - 12: OPENMV_OCCUPANCY       (OpenMV machine vision occupancy)
 *  - 13: ACCEL_PRESENCE         (Accelerometer-based presence sensor)
 *  - 20: SOIL_MOISTURE          (Soil moisture data sensor)
 *  - 21: DISTANCE               (Ultrasonic/TOF distance sensor)
 *  - 90: LORA_GATEWAY           (LoRA gateway device acting as sensor hub)
 */
enum class SensorType : uint8_t {
    VEHICLE_PRESSURE     = 0,
    PIR                  = 1,   ///< Pedestrian Infrared Sensor
    VEHICLE_MAGNETOMETER = 2,
    RAIN_BUCKET          = 3,
    VIBRATION_BASIC      = 4,
    VIBRATION_ADVANCED   = 5,

    INDOOR_OCCUPANCY     = 10,
    OUTDOOR_OCCUPANCY    = 11,
    OPENMV_OCCUPANCY     = 12,
    ACCEL_PRESENCE       = 13,

    SOIL_MOISTURE        = 20,
    DISTANCE             = 21,

    LORA_GATEWAY         = 90,
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
            case SensorType::VEHICLE_PRESSURE:     return "VehiclePressure";
            case SensorType::PIR:                  return "PIR";
            case SensorType::VEHICLE_MAGNETOMETER: return "VehicleMagnetometer";
            case SensorType::RAIN_BUCKET:          return "RainBucket";
            case SensorType::VIBRATION_BASIC:      return "VibrationBasic";
            case SensorType::VIBRATION_ADVANCED:   return "VibrationAdvanced";

            case SensorType::INDOOR_OCCUPANCY:     return "IndoorOccupancy";
            case SensorType::OUTDOOR_OCCUPANCY:    return "OutdoorOccupancy";
            case SensorType::OPENMV_OCCUPANCY:     return "OpenMVOccupancy";
            case SensorType::ACCEL_PRESENCE:       return "AccelPresence";

            case SensorType::SOIL_MOISTURE:        return "SoilMoisture";
            case SensorType::DISTANCE:             return "Distance";

            case SensorType::LORA_GATEWAY:         return "LoRaGateway";
            default: return "Unknown";
        }
    }
};

#endif /* SENSORFACTORY_H */
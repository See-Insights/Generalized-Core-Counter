/**
 * @file Config.h
 * @brief Global compile-time configuration options and enums.
 *
 * This header is reserved for project-wide configuration values and
 * enumerations (such as sensor type selection) that are shared across
 * modules. Some configuration is currently defined in other headers and
 * implementation files; this file provides a single place for Doxygen
 * to anchor high-level configuration documentation.
 */

#ifndef CONFIG_H
#define CONFIG_H

/**
 * @brief Sensor type ID mapping (for sysStatus.sensorType).
 *
 * These IDs are used across the fleet and must remain stable
 * for backward compatibility.
 *
 *  -  0: Vehicle Pressure Sensor
 *  -  1: Pedestrian Infrared Sensor (PIR)
 *  -  2: Vehicle Magnetometer Sensor
 *  -  3: Rain Bucket Sensor
 *  -  4: Vibration / Motion Sensor - Basic
 *  -  5: Vibration Sensor - Advanced (Accel + Magnetometer)
 *  - 10: Indoor Room Occupancy Sensor
 *  - 11: Outdoor Occupancy Sensor
 *  - 12: OpenMV Machine Vision Occupancy Sensor
 *  - 13: Accelerometer Presence Sensor
 *  - 20: Soil Moisture Sensor
 *  - 21: Distance Sensor (Ultrasonic / TOF)
 *  - 90: LoRA Gateway (gateway device)
 */
// See SensorFactory.h::SensorType for the corresponding enum.

#endif /* CONFIG_H */

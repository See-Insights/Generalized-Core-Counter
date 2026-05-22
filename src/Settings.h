/**
 * @file Settings.h
 * @author Chip McClelland
 * @email chip@seeinsights.com
 * @brief Central catalog of shared build, hardware-override, and project settings.
 *
 * @details
 * This header is the single documentation entry point for settings that affect
 * how the firmware is built or adapted to specific carrier hardware.
 *
 * Build profile settings:
 * - `DEV_BUILD`: Set to `1` for a developer build and `0` for a field build.
 * - `FIELD_BUILD`: Derived convenience flag that becomes true when `DEV_BUILD=0`.
 * - `ALLOW_BLOCKING_SERIAL_WAITS`: Allows explicit blocking waits for USB serial.
 *   This should remain `0` for field deployments.
 * Optional hardware override settings:
 * - `MUON_TMP36_SENSE_PIN`: Overrides which analog-capable pin is used for the
 *   TMP36 carrier temperature sensor.
 * - `MUON_HAS_TMP36`: Force-enables the TMP36 sampling path for carriers that
 *   provide the sensor even when the platform default does not.
 * - `MUON_HAS_TMP112`: Force-enables the TMP112A I2C temperature path.
 * - `DISABLE_TMP112_AUTODETECT`: Skips TMP112A probing at boot.
 * - `MUON_TMP112_I2C_ADDR`: Overrides the TMP112A 7-bit I2C address.
 *
 * Project-level settings:
 * - `ProjectConfig::webhookEventName()`: Legacy webhook fallback retained for
 *   compatibility with older products and test fixtures.
 *
 * Sensor type IDs used across the fleet:
 * - `0`: Vehicle Pressure Sensor
 * - `1`: Pedestrian Infrared Sensor (PIR)
 * - `2`: Vehicle Magnetometer Sensor
 * - `3`: Rain Bucket Sensor
 * - `4`: Vibration / Motion Sensor - Basic
 * - `5`: Vibration Sensor - Advanced (Accel + Magnetometer)
 * - `10`: Indoor Room Occupancy Sensor
 * - `11`: Outdoor Occupancy Sensor
 * - `12`: OpenMV Machine Vision Occupancy Sensor
 * - `13`: Accelerometer Presence Sensor
 * - `20`: Soil Moisture Sensor
 * - `21`: Distance Sensor (Ultrasonic / TOF)
 * - `90`: LoRA Gateway
 *
 * @copyright Copyright (c) 2026 Chip McClelland
 * @license MIT License
 */

#pragma once

#include "BuildProfile.h"
#include "ProjectConfig.h"
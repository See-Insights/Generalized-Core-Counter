/**
 * @file FirmwareVersion.h
 * @brief Centralized firmware version metadata.
 *
 * This header defines the integer product version used by Particle's firmware
 * management system, as well as the human-readable version string and release notes.
 *
 * FIRMWARE_PRODUCT_VERSION:
 *   - Integer version recognized by PRODUCT_VERSION() macro.
 *   - Should be incremented when starting a new firmware release line.
 *   - Required by Particle Product firmware management.
 *
 * FIRMWARE_VERSION and FIRMWARE_RELEASE_NOTES:
 *   - Human-readable version string (e.g., "19", "19.1", "20.0").
 *   - Used for logging, status reporting, and user documentation.
 *   - Point releases (e.g., 19.1, 19.2) share the same PRODUCT_VERSION.
 */

#ifndef FIRMWARE_VERSION_H
#define FIRMWARE_VERSION_H

/**
 * @brief Particle Product integer version (must be an integer).
 *
 * This value is used with PRODUCT_VERSION() to identify the firmware
 * in Particle's cloud console and OTA update system.
 */
#define FIRMWARE_PRODUCT_VERSION 24

/**
 * @brief Current firmware release string (e.g., "19", "19.1", "20.0").
 */
extern const char* FIRMWARE_VERSION;

/**
 * @brief One-line summary of this firmware release.
 */
extern const char* FIRMWARE_RELEASE_NOTES;

#endif /* FIRMWARE_VERSION_H */

/**
 * @file Version.h
 * @brief Firmware release metadata (version and notes).
 *
 * This header centralizes the firmware version string and a
 * single-line release note so that logging, cloud status, and
 * documentation all stay in sync.
 */

#ifndef VERSION_H
#define VERSION_H

/**
 * @brief Current firmware release string (e.g. "3.01").
 */
extern const char* FIRMWARE_VERSION;

/**
 * @brief One-line summary of this firmware release.
 */
extern const char* FIRMWARE_RELEASE_NOTES;

#endif /* VERSION_H */

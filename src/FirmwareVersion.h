/**
 * @file FirmwareVersion.h
 * @brief The one place the firmware version lives.
 *
 * Both values are edited together by bump_version.sh and staged by release.sh:
 * - FIRMWARE_PRODUCT_VERSION: the integer Particle's firmware management uses
 *   with PRODUCT_VERSION(): major * 100 + iteration (v49.1 -> 4901). Every
 *   iteration is its own product version; the iteration that passes its soak
 *   is the release, with no rebuild.
 * - FIRMWARE_VERSION: the human-readable string used for logging, status
 *   reporting and documentation: v<major>.<iteration>[-name].
 *
 * Release notes are NOT here: they live in CHANGELOG.md, out of the binary.
 */

#ifndef FIRMWARE_VERSION_H
#define FIRMWARE_VERSION_H

/** @brief Particle Product integer version (must be an integer). */
#define FIRMWARE_PRODUCT_VERSION 5001

/** @brief Current firmware release string. */
inline const char* FIRMWARE_VERSION = "v50.1-NativeRTC";

#endif /* FIRMWARE_VERSION_H */

// src/SensorDefinitions.h
#ifndef SENSORDEFINITIONS_H
#define SENSORDEFINITIONS_H

#include "SensorFactory.h"

/**
 * @brief Static metadata for each supported sensor type.
 *
 * This provides a single place to review which sensor types are
 * defined in the firmware and to adjust device-specific defaults
 * (such as LED behavior, interrupt usage, etc.).
 */
struct SensorDefinition {
    SensorType   type;              ///< SensorType enum value
    const char*  name;              ///< Short name for logging / display
    bool         ledDefaultOn;      ///< true if LED should be ON at boot (polarity-specific)
    bool         usesInterrupt;     ///< true if sensor uses a hardware interrupt line
};

namespace SensorDefinitions {

// NOTE: Keep this table in sync with SensorType in SensorFactory.h.
// Only a subset of sensor types are currently implemented.
static constexpr SensorDefinition DEFINITIONS[] = {
    // Vehicle pressure sensor (legacy tire sensor) - LED enable is ACTIVE-HIGH
    { SensorType::VEHICLE_PRESSURE, "VehiclePressure", true,  true },

    // PIR pedestrian sensor (current default) - LED enable is ACTIVE-LOW
    { SensorType::PIR,              "PIR",             false, true },

    // Additional sensor types can be added here as they are implemented.
};

/**
 * @brief Lookup helper to get the SensorDefinition for a given SensorType.
 * @return Pointer to definition, or nullptr if not found.
 */
inline const SensorDefinition* getDefinition(SensorType type) {
    for (const auto& def : DEFINITIONS) {
        if (def.type == type) {
            return &def;
        }
    }
    return nullptr;
}

} // namespace SensorDefinitions

#endif /* SENSORDEFINITIONS_H */

#pragma once

#include "Particle.h"

namespace Connectivity {

/**
 * @brief Returns whether the active platform radio is currently powered.
 *
 * @return true when the Wi-Fi or cellular radio is on
 */
inline bool isRadioPoweredOn() {
#if Wiring_WiFi
  return WiFi.isOn();
#elif Wiring_Cellular
  return Cellular.isOn();
#else
  return false;
#endif
}

/**
 * @brief Requests a radio-only disconnect and power-off on the active platform.
 */
inline void requestRadioPowerOff() {
#if Wiring_Cellular
  Cellular.disconnect();
  Cellular.off();
#elif Wiring_WiFi
  WiFi.disconnect();
  WiFi.off();
#endif
}

/**
 * @brief Requests both a Particle cloud disconnect and radio power-down.
 */
inline void requestFullDisconnectAndRadioOff() {
  Particle.disconnect();
  requestRadioPowerOff();
}

/**
 * @brief Requests a Particle cloud disconnect while leaving radio teardown to the caller.
 */
inline void requestCloudDisconnectOnly() {
  Particle.disconnect();
}

} // namespace Connectivity

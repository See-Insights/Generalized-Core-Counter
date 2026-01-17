#pragma once

#include "Particle.h"

namespace Connectivity {

inline bool isRadioPoweredOn() {
#if Wiring_WiFi
  return WiFi.isOn();
#elif Wiring_Cellular
  return Cellular.isOn();
#else
  return false;
#endif
}

inline void requestRadioPowerOff() {
#if Wiring_Cellular
  Cellular.disconnect();
  Cellular.off();
#elif Wiring_WiFi
  WiFi.disconnect();
  WiFi.off();
#endif
}

inline void requestFullDisconnectAndRadioOff() {
  Particle.disconnect();
  requestRadioPowerOff();
}

} // namespace Connectivity

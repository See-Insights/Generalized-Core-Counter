#pragma once
#include "Particle.h"
#if PLATFORM_ID == PLATFORM_BORON
bool isDeviceOsAtLeast6400();
bool clearAb1805StaleAlarmInterrupts();
bool shouldUseBoronRtcAlarmHibernate(uint32_t requestedSleepSec, time_t &rtcNow);
#endif

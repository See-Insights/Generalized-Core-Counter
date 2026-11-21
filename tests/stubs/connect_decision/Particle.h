#pragma once
// Empty on purpose (WO-2026-11-19-001). power/ConnectDecision.cpp reaches Particle.h only
// through power/ConnectivityPolicy.h, for HOLD_BASE_MS and HOLD_MAX_MS, and must not use
// anything from Device OS: with this stub, any such use fails to compile on the host.
#include <stdint.h>
#include <time.h>

#pragma once

// src/cloud/DeviceStatusPublisher.cpp includes "state/StateMachine.h" but
// does not reference any symbol from it directly on the paths this harness
// exercises. Provided as an empty stub so the real (Particle-state-machine-
// heavy) header does not need to be pulled onto the host build.

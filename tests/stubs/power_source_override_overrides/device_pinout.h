#pragma once

// Cloud.h pulls in "device_pinout.h" purely for extern pin declarations that
// nothing exercised by this harness ever references (no pin I/O happens on
// the publisher's telemetry-writing path). Deliberately empty so the real
// src/device_pinout.h (which needs the on-device pin_t/Particle pin API) is
// not required on the host.

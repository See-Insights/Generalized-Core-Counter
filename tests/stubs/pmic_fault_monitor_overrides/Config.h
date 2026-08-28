#pragma once

// WO-2026-08-24-001: the real DeviceStatusPublisher.cpp firmware object now
// reads the ENABLE_*/DEV_BUILD/etc. build-flag macros directly (to compute
// its "flags" witness field), so this host stub pulls in the real,
// unmodified src/BuildProfile.h (found via the -I"$repo_root/src" test
// compiler flag) rather than redefining those flags here -- keeping this
// test bound to whatever BuildProfile.h actually says, not a copy of it.
#include "BuildProfile.h"
// Thin wrapper around the existing power-source-override stub, adding only
// what the REAL src/cloud/Cloud.cpp needs that the shared stub doesn't yet
// provide: the (unscoped, global) SensorMode enum used by
// Cloud::getWebhookName()'s switch statement. getWebhookName() is never
// invoked at runtime by this test, but Cloud.cpp is compiled as a whole
// translation unit, so its enum constants (COUNTING/OCCUPANCY/MEASUREMENT)
// must resolve at compile time regardless of runtime reachability.
//
// Reusing the shared stub via a relative include (rather than duplicating
// its ~200 lines) keeps this override in sync with
// tests/stubs/power_source_override_overrides/MyPersistentData.h and avoids
// any risk of the two silently diverging.
#pragma once

#include "../power_source_override_overrides/MyPersistentData.h"

enum SensorMode {
	COUNTING     = 0,
	OCCUPANCY    = 1,
	MEASUREMENT  = 2
};

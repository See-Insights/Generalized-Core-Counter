// Thin wrapper around the existing power-source-override stub. Since
// WO-2026-09-23-001 (Step 5) the operating-mode enum Cloud.cpp's
// getWebhookName() switch needs is SystemConfig::SensorMode, supplied by that
// stub's persist/SystemConfig.h, so this file no longer has to add a global
// enum of its own - it exists only to keep the two override directories
// sharing one definition instead of silently diverging.
#pragma once

#include "../power_source_override_overrides/MyPersistentData.h"

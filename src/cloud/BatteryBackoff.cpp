#include "cloud/Cloud.h"
#include "cloud/BatteryBackoffPolicy.h"
#include "reporting/ReportingPolicy.h"

// *************** Battery-Aware Connection Management ***************

// WO-2026-09-21 Step 4: Cloud::calculateBatteryTier() deleted - it was an
// unguarded path (no vcell floor, no trust check) reachable from the
// connectivity failsafe, diverging from the properly guarded
// power/BatteryAuthority.h::evaluate() on exactly the cases that matter (a
// gauge spike/drop shape, or a live gauge fault). Every former caller now
// consumes BatteryAuthority::evaluate() directly.

// [static]
uint16_t Cloud::getIntervalMultiplier(BatteryTier tier) {
    return BatteryBackoff::intervalMultiplier(tier);
}

// [static]
float Cloud::getConnectionBackoffMultiplier(uint16_t lastDurationSec) {
    // Evaluate connection history to apply additional backoff
    // in problematic locations (poor signal, tower congestion, etc.)

    if (lastDurationSec == 0) {
        // Failed connection or first attempt - apply strong backoff
        Log.info("Connection history: failed/first attempt - applying 2.0x backoff");
        return 2.0f;
    } else if (lastDurationSec < 60) {
        // Fast connection - excellent location
        Log.trace("Connection history: fast (%ds) - no additional backoff", lastDurationSec);
        return 1.0f;
    } else if (lastDurationSec < 180) {
        // Normal connection time - acceptable
        Log.trace("Connection history: normal (%ds) - no additional backoff", lastDurationSec);
        return 1.0f;
    } else if (lastDurationSec < 300) {
        // Slow connection - marginal location
        Log.info("Connection history: slow (%ds) - applying 1.5x backoff", lastDurationSec);
        return 1.5f;
    } else {
        // Very slow connection - problematic location
        Log.info("Connection history: problem (%ds) - applying 2.0x backoff", lastDurationSec);
        return 2.0f;
    }
}

// WO-2026-09-21 Step 4: Cloud::testBatteryBackoffLogic() deleted - dead code
// (zero callers) that used the persisted currentBatteryTier field as scratch
// space across ~19 temporary sets/restores per invocation, worse than merely
// dead. tests/battery_tier_guard_test.cpp/power_tier_test.cpp already cover
// the tier-calculation logic as real host tests.

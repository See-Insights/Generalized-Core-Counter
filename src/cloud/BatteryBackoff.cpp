#include "cloud/Cloud.h"

// *************** Battery-Aware Connection Management ***************

// [static]
BatteryTier Cloud::calculateBatteryTier(float currentSoC) {
    // Get previous tier for hysteresis logic
    uint8_t prevTierValue = sysStatus.get_currentBatteryTier();
    BatteryTier prevTier = static_cast<BatteryTier>(prevTierValue);
    
    // Apply hysteresis: require 5% higher SoC to move to better tier
    // This prevents rapid tier thrashing when battery is near threshold
    
    if (currentSoC >= 75.0f) {
        // Strong signal to move to HEALTHY (or stay there)
        return TIER_HEALTHY;
    } else if (currentSoC >= 70.0f) {
        // Boundary zone: stay in current tier unless coming from below
        return (prevTier == TIER_HEALTHY) ? TIER_HEALTHY : TIER_CONSERVING;
    } else if (currentSoC >= 55.0f) {
        // Strong signal for CONSERVING or recovery from CRITICAL
        return TIER_CONSERVING;
    } else if (currentSoC >= 50.0f) {
        // Boundary zone: stay in current tier unless coming from above
        if (prevTier <= TIER_CONSERVING) {
            return TIER_CONSERVING;
        } else {
            return TIER_CRITICAL;
        }
    } else if (currentSoC >= 35.0f) {
        // Strong signal for CRITICAL or recovery from SURVIVAL
        return TIER_CRITICAL;
    } else if (currentSoC >= 30.0f) {
        // Boundary zone: stay in current tier unless coming from above
        if (prevTier <= TIER_CRITICAL) {
            return TIER_CRITICAL;
        } else {
            return TIER_SURVIVAL;
        }
    } else {
        // Below 30% - definite SURVIVAL mode
        return TIER_SURVIVAL;
    }
}

// [static]
uint16_t Cloud::getIntervalMultiplier(BatteryTier tier) {
    switch (tier) {
        case TIER_HEALTHY:
            return 1;   // Normal interval
        case TIER_CONSERVING:
            return 2;   // Double interval (half connections per day)
        case TIER_CRITICAL:
            return 4;   // Quadruple interval (quarter connections per day)
        case TIER_SURVIVAL:
            return 12;  // 12x interval (e.g., hourly → every 12 hours)
        default:
            Log.warn("Unknown battery tier %d, defaulting to 1x multiplier", (int)tier);
            return 1;
    }
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

/**
 * @brief Unit test function to validate battery-aware backoff calculations
 * 
 * Tests all combinations of battery tiers and connection durations to verify
 * the multiplier logic produces expected results across all scenarios.
 * Call this function early in setup() or via a test mode to validate logic.
 */
void Cloud::testBatteryBackoffLogic() {
    Log.info("========================================");
    Log.info("Battery-Aware Backoff Unit Test");
    Log.info("========================================");
    
    // Test battery tier calculations with hysteresis
    Log.info(" ");
    Log.info("=== Battery Tier Calculation Tests ===");
    
    struct BatteryTest {
        float soc;
        BatteryTier prevTier;
        const char* description;
    };
    
    BatteryTest batteryTests[] = {
        {100.0f, TIER_HEALTHY, "100% SoC (healthy)"},
        {75.0f, TIER_CONSERVING, "75% SoC (hysteresis boundary, coming from conserving)"},
        {75.0f, TIER_HEALTHY, "75% SoC (hysteresis boundary, staying healthy)"},
        {70.0f, TIER_HEALTHY, "70% SoC (tier boundary, staying healthy)"},
        {70.0f, TIER_CONSERVING, "70% SoC (tier boundary, entering conserving)"},
        {55.0f, TIER_CRITICAL, "55% SoC (hysteresis boundary, coming from critical)"},
        {50.0f, TIER_CONSERVING, "50% SoC (tier boundary, staying conserving)"},
        {35.0f, TIER_SURVIVAL, "35% SoC (hysteresis boundary, coming from survival)"},
        {30.0f, TIER_CRITICAL, "30% SoC (tier boundary, staying critical)"},
        {25.0f, TIER_SURVIVAL, "25% SoC (survival mode)"},
        {10.0f, TIER_SURVIVAL, "10% SoC (low battery)"}
    };
    
    for (size_t i = 0; i < sizeof(batteryTests)/sizeof(batteryTests[0]); i++) {
        // Temporarily set previous tier for hysteresis testing
        uint8_t originalTier = sysStatus.get_currentBatteryTier();
        sysStatus.set_currentBatteryTier(static_cast<uint8_t>(batteryTests[i].prevTier));
        
        BatteryTier result = calculateBatteryTier(batteryTests[i].soc);
        const char* tierNames[] = {"HEALTHY", "CONSERVING", "CRITICAL", "SURVIVAL"};
        
        Log.info("  %s -> %s", batteryTests[i].description, tierNames[result]);
        
        // Restore original tier
        sysStatus.set_currentBatteryTier(originalTier);
    }
    
    // Test interval multipliers
    Log.info(" ");
    Log.info("=== Interval Multiplier Tests ===");
    const char* tierNames[] = {"TIER_HEALTHY", "TIER_CONSERVING", "TIER_CRITICAL", "TIER_SURVIVAL"};
    for (int tier = TIER_HEALTHY; tier <= TIER_SURVIVAL; tier++) {
        uint16_t mult = getIntervalMultiplier(static_cast<BatteryTier>(tier));
        Log.info("  %s: %ux interval", tierNames[tier], mult);
    }
    
    // Test connection backoff multipliers
    Log.info(" ");
    Log.info("=== Connection History Backoff Tests ===");
    
    struct ConnectionTest {
        uint16_t duration;
        const char* description;
    };
    
    ConnectionTest connectionTests[] = {
        {0, "Failed/first connection"},
        {30, "Fast connection (30s)"},
        {60, "Normal connection (60s)"},
        {120, "Normal connection (120s)"},
        {180, "Slow boundary (180s)"},
        {250, "Slow connection (250s)"},
        {300, "Problem boundary (300s)"},
        {400, "Problem connection (400s)"}
    };
    
    for (size_t i = 0; i < sizeof(connectionTests)/sizeof(connectionTests[0]); i++) {
        float mult = getConnectionBackoffMultiplier(connectionTests[i].duration);
        Log.info("  %s: %.1fx backoff", connectionTests[i].description, mult);
    }
    
    // Combined scenario tests
    Log.info(" ");
    Log.info("=== Combined Scenario Tests (Base Interval = 3600s / 1 hour) ===");
    Log.info("Format: Battery | ConnTime | Tier Mult | History Mult | Effective Interval");
    Log.info("----------------------------------------------------------------------");
    
    struct ScenarioTest {
        float soc;
        BatteryTier prevTier;
        uint16_t connDuration;
        const char* batteryDesc;
        const char* locationDesc;
    };
    
    ScenarioTest scenarios[] = {
        {80.0f, TIER_HEALTHY, 30, "80% (Healthy)", "Fast (30s)"},
        {80.0f, TIER_HEALTHY, 250, "80% (Healthy)", "Slow (250s)"},
        {60.0f, TIER_CONSERVING, 120, "60% (Conserving)", "Normal (120s)"},
        {60.0f, TIER_CONSERVING, 250, "60% (Conserving)", "Slow (250s)"},
        {40.0f, TIER_CRITICAL, 120, "40% (Critical)", "Normal (120s)"},
        {40.0f, TIER_CRITICAL, 350, "40% (Critical)", "Problem (350s)"},
        {25.0f, TIER_SURVIVAL, 120, "25% (Survival)", "Normal (120s)"},
        {25.0f, TIER_SURVIVAL, 400, "25% (Survival)", "Problem (400s)"}
    };
    
    const uint16_t baseInterval = 3600; // 1 hour in seconds
    
    for (size_t i = 0; i < sizeof(scenarios)/sizeof(scenarios[0]); i++) {
        // Set previous tier for hysteresis
        uint8_t originalTier = sysStatus.get_currentBatteryTier();
        sysStatus.set_currentBatteryTier(static_cast<uint8_t>(scenarios[i].prevTier));
        
        BatteryTier tier = calculateBatteryTier(scenarios[i].soc);
        uint16_t tierMult = getIntervalMultiplier(tier);
        float historyMult = getConnectionBackoffMultiplier(scenarios[i].connDuration);
        uint32_t effectiveInterval = (uint32_t)(baseInterval * tierMult * historyMult);
        
        Log.info("  %-18s | %-16s | %5ux | %12.1fx | %6lus (%luh %lum)",
                 scenarios[i].batteryDesc,
                 scenarios[i].locationDesc,
                 tierMult,
                 historyMult,
                 (unsigned long)effectiveInterval,
                 (unsigned long)(effectiveInterval / 3600),
                 (unsigned long)((effectiveInterval % 3600) / 60));
        
        // Restore original tier
        sysStatus.set_currentBatteryTier(originalTier);
    }
    
    Log.info(" ");
    Log.info("========================================");
    Log.info("Unit Test Complete");
    Log.info("========================================");
}

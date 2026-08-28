/**
 * @file SensorManager.h
 * @author Chip McClelland (chip@seeinisghts.com)
 * @brief Singleton wrapper around ISensor implementations.
 *
 * @details SensorManager owns a single ISensor instance and handles
 *          initialization, polling, and utility helpers like battery
 *          status, temperature conversion, and signal strength reporting.
 *          It provides a uniform interface to the rest of the firmware,
 *          regardless of which concrete sensor is attached.
 */

#ifndef SENSORMANAGER_H
#define SENSORMANAGER_H

#include "Particle.h"
#include "sensors/ISensor.h"
#include "power/BatteryHealth.h"

extern char internalTempStr[16];
extern char signalStr[64];

// Retained PMIC contradiction forensics.
extern retained uint16_t pmicAnomalyCount;
extern retained uint32_t lastPmicAnomalyUptimeMs;
extern retained float lastPmicAnomalySoc;
extern retained uint8_t lastPmicAnomalyChargeStatus;
extern retained uint8_t lastPmicAnomalyPowerSource;
extern retained uint8_t lastPmicAnomalyVbusStatus;
extern bool pmicAnomalyActive;
uint32_t pmicAnomalyAgeSec();

enum class BatterySampleContext : uint8_t {
    General = 0,
    Setup,
    PreSleep,
    PostWake,
};

/**
 * @brief Convenience macro for accessing the SensorManager singleton.
 *
 * Usage:
 * @code
 *   measure.setup();
 *   if (measure.loop()) {
 *       auto data = measure.getSensorData();
 *   }
 * @endcode
 */
#define measure SensorManager::instance()

class SensorManager {
public:
    /**
     * @brief Get the SensorManager singleton instance.
     */
    static SensorManager &instance();

    /**
     * @brief Initialize the active sensor and any manager state.
     */
    void setup();

    /**
     * @brief Poll the active sensor; call from the main loop.
     *
     * @return true if new sensor data is available.
     */
    bool loop();

    /**
     * @brief Set the concrete ISensor implementation to use.
     *
     * @param sensor Pointer to a concrete ISensor (not owned).
     */
    void setSensor(ISensor* sensor);

    /**
     * @brief Get the latest sensor data from the active sensor.
     */
    SensorData getSensorData() const;

    /**
     * @brief Check whether the active sensor is initialized and ready.
     */
    bool isSensorReady() const;

    /**
     * @brief Create and initialize the active sensor based on configuration.
     *
     * Uses sysStatus.get_sensorType() and SensorFactory to select the
     * concrete implementation, then calls the sensor's initializeHardware().
     */
    void initializeFromConfig();

    /**
     * @brief Notify the sensor that the device is entering deep sleep.
     */
    void onEnterSleep();

    /**
     * @brief Notify the sensor that the device has woken from deep sleep.
     */
    void onExitSleep();

    /**
     * @brief Mark that the next battery sample occurs immediately after low-power wake.
     */
    void noteWakeFromLowPowerSleep();

    /** @name Utility functions
     *  Helpers for temperature, battery, radio signal, and delays.
     */
    ///@{
    /**
     * @brief Convert TMP36 ADC reading to degrees Celsius.
     *
     * @param adcValue Raw ADC value from the TMP36 input.
     * @return Temperature in degrees Celsius.
     */
    float tmp36TemperatureC(int adcValue);

    /**
     * @brief Determine whether the battery is present and not critically low.
     */
    bool batteryState(BatterySampleContext sampleContext = BatterySampleContext::General);

    /**
     * @brief WO-2026-08-25-001 Amendment C, Decision C2 (AC-C6): total,
     *        three-state classification of the cached battery voltage.
     *
     * `cachedBatteryVoltage()` below collapses "never sampled this boot"
     * and "sampled but implausible" into the same `false` - the exact gap
     * that let RuntimeReportingPolicy's guard skip both the trust
     * substitution AND the 3.5 V floor for an INVALID vcell (Blocker B).
     * Callers that must not treat those two cases the same way (i.e. the
     * tier guard) should use this instead.
     */
    enum class VcellSampleState : uint8_t { Known, Invalid, Unavailable };

    /**
     * @brief Classifies the cached vcell as Known / Invalid / Unavailable.
     *
     * @param[out] vcell Filled with the cached value for Known and Invalid
     *        (so a caller that wants the raw reading for logging can still
     *        get it); left at 0.0f for Unavailable.
     * @return Known if a sample was taken this boot and looks physically
     *         plausible (batteryVoltageLooksUsable()); Invalid if a sample
     *         was taken but is NOT plausible (<=2.5V, >=5V, or NaN); or
     *         Unavailable if no sample has been taken this boot yet (e.g.
     *         before the first Boron sample, or a non-Boron platform that
     *         never populates vcell at all).
     */
    VcellSampleState cachedBatteryVoltageState(float &vcell) const;

    bool cachedBatteryVoltage(float &vcell) const;
    const char *cachedChargeStateLabel() const;

    /**
     * @brief F1's trust signal from the most recent batteryState() sample
     *        (WO-2026-08-25-001, Amendment B / AC-B2).
     *
     * Defaults to Trusted on platforms/boots where batteryState() has not
     * yet run the F1 evaluation (e.g. before the first Boron sample, or on
     * non-Boron platforms which do not run BatteryHealth::evaluate() at
     * all) - matching legacy behavior (SOC-driven tiering) until a real
     * trust signal is available.
     */
    BatteryHealth::SocTrust cachedSocTrust() const;

    /**
     * @brief Whether the thermal ChargeInhibit disable's DCT config was
     *        READ BACK as matching the intended state, as of the most
     *        recent measureTemperatureAndApplyChargeDecision() call
     *        (WO-2026-08-25-001 Amendment B Blocker 4, renamed under
     *        Amendment C Decision C3 point 3).
     *
     * This is ChargeInhibit::ApplyResult::configReadbackVerified - it
     * confirms System.getPowerConfiguration() read back DISABLE_CHARGING as
     * set, NOT that the PMIC hardware has actually stopped charging. Device
     * OS applies power configuration to the PMIC ASYNCHRONOUSLY
     * (system_power_manager's handleCharging() thread reconciles the PMIC
     * against the config on its own schedule); nothing here checks
     * PMIC::isChargingEnabled(). PmicFaultMonitor::pollAndRemediate() uses
     * this (together with the fault class) as a same-cycle proxy for "a
     * disable is very likely already in effect or about to be" - not as
     * hardware proof - when deciding whether a raw PMIC toggle would fight
     * it.
     */
    bool chargeDisableConfigVerified() const;

    /**
     * @brief Update global signal strength strings for logging/telemetry.
     */
    void getSignalStrength();

    /**
     * @brief TEMPORARY DIAGNOSTIC: Test PMIC charge state machine behavior.
     * 
     * @details Controlled experiment to determine if PMIC is stuck in FAST_CHARGE
     *          or will transition to DONE when charging is cycled. Only executes
     *          once per boot with safety checks. Logs comprehensive PMIC state
     *          before/during/after disabling and re-enabling charging.
     * 
     * @return 0 on success, negative error code on failure
     */
    int runPmicChargeCycleTest();

    ///@}
    
protected:
    SensorManager();
    virtual ~SensorManager();
    SensorManager(const SensorManager &) = delete;
    SensorManager &operator=(const SensorManager &) = delete;
    
    /** @brief Pointer to the singleton instance. */
    static SensorManager *_instance;

    /** @brief Currently active sensor implementation (not owned). */
    ISensor* _sensor;

    /** @brief Timestamp of the last sensor poll (millis). */
    unsigned long _lastPollTime;

    /** @brief One-shot flag to stabilize battery measurement after low-power wake. */
    bool _batteryStabilizationPending;

    /** @brief Track whether the first post-boot battery sample has already run. */
    bool _firstBatterySampleTaken;

    /** @brief Whether this wake cycle has a trusted pre-radio battery sample. */
    bool _authoritativeBatterySampleActive;

    /** @brief Cached SoC from the trusted pre-radio battery sample. */
    float _authoritativeBatterySoc;

    /** @brief Cached battery state from the trusted pre-radio battery sample. */
    uint8_t _authoritativeBatteryState;

    /** @brief Whether the trusted pre-radio sample used voltage fallback. */
    bool _authoritativeBatteryFallbackUsed;

    /** @brief Cached battery voltage from the latest batteryState() sample. */
    float _cachedBatteryVcell;

    /** @brief Cached compact PMIC charge label from the latest batteryState() sample. */
    const char *_cachedChargeStateLabel;

    /** @brief Whether the cached battery voltage is valid. */
    bool _cachedBatteryVcellValid;

    /** @brief Whether a vcell sample has been taken at all this boot yet
     *  (WO-2026-08-25-001 Amendment C, Decision C2 / AC-C6) - distinct from
     *  _cachedBatteryVcellValid, which is only meaningful once a sample has
     *  actually been taken. See cachedBatteryVoltageState(). */
    bool _cachedBatteryVcellSampled;

    /** @brief Cached F1 trust signal from the latest batteryState() sample. */
    BatteryHealth::SocTrust _cachedSocTrust = BatteryHealth::SocTrust::Trusted;

    /** @brief Cached ChargeInhibit::ApplyResult::configReadbackVerified from
     *  the latest measureTemperatureAndApplyChargeDecision() call (see
     *  chargeDisableConfigVerified() accessor - DCT readback only, not
     *  hardware-verified PMIC state; see that accessor's doc comment). */
    bool _chargeDisableConfigVerified = false;

private:
    /**
     * @brief Read TMP112A temperature (I2C) in degrees Celsius.
     *
     * WO-2026-08-25-001 Amendment C, Decision C1 (AC-C1 point 1, closing the
     * Codex Stage 7 Final finding): this is the genuine enclosure-temperature
     * ACQUISITION half. It must not be reachable except through
     * measureTemperatureAndApplyChargeDecision() below - it was previously
     * `public`, which meant any caller could obtain an enclosure-temperature
     * reading WITHOUT the charge decision being evaluated, defeating the
     * structural coupling AC-C1 requires. It is now `private` (not merely
     * `protected`, which a derived class could still call) with exactly one
     * call site, inside measureTemperatureAndApplyChargeDecision(). If a
     * caller needs a temperature value, it must come from that coupled
     * call's result or from the persisted telemetry field
     * (current.get_internalTempC()) - never from an independent acquisition.
     *
     * @param[out] tempC Filled with temperature in degrees Celsius on success.
     * @return true on success, false on I2C error or missing device.
     */
    bool readTmp112TemperatureC(float &tempC);

    /**
     * @brief F2a/C1 - coupled enclosure-temperature measurement AND
     *        thermal charge-inhibit evaluation/application
     *        (WO-2026-08-25-001 Amendment C, Decision C1).
     *
     * This is the ONLY function that reads the enclosure temperature
     * sensor (TMP112A or TMP36, whichever this platform/build uses), and
     * the ONLY function that evaluates and applies the thermal
     * charge-inhibit decision - there is no separate, independently
     * callable "read temperature" entry point and no separate,
     * independently callable "evaluate/apply decision" entry point. Both
     * halves live in one function body with no return between them (see
     * the .cpp definition for the full rationale and AC-C1..AC-C4).
     *
     * Genuinely `private` (not `protected`, which a derived class could
     * still call): batteryState() is the only caller, from exactly one call
     * site per platform build (see the .cpp), so this cannot be invoked -
     * and therefore the measurement half cannot run without the decision
     * half - from anywhere else in the codebase, including a hypothetical
     * future subclass.
     *
     * @return true if it is currently safe to charge (i.e. NOT thermally
     *         inhibited); on platforms without PMIC charge control this is
     *         still computed and logged, but not enforced.
     */
    bool measureTemperatureAndApplyChargeDecision();
};

#endif /* SENSORMANAGER_H */
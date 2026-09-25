#pragma once

/**
 * @file SystemConfig.h
 * @brief Facade: system configuration and operational bookkeeping.
 *
 * WO-2026-09-23-001 (Step 5), decision B: this facade's remit is "system
 * configuration and operational bookkeeping" - modes, schedule, webhook
 * config, connection budgets, the attempt counter, connection history,
 * serial configuration, and the separate /usr/sensor.dat sensor-settings
 * store (nested namespace SensorSettings).
 *
 * INCLUDES ARE THE POINT OF THIS HEADER. It must never include
 * StorageHelperRK.h, MyPersistentData.h, Particle.h, or a sibling facade -
 * doing so reintroduces exactly the coupling Step 5 exists to remove. The
 * storage-backed sysStatusData/sensorConfigData class definitions are
 * private to MyPersistentData.cpp; everything here is a free function that
 * forwards to them.
 *
 * Accessor names deliberately match the pre-split sysStatus./sensorConfig.
 * member names so consumer conversion is a mechanical receiver change
 * (sysStatus.get_openTime() -> SystemConfig::get_openTime()) rather than a
 * rename with review burden.
 */

#include <stddef.h>
#include <stdint.h>
#include <time.h>

namespace SystemConfig {

// *************** Operating Mode Enumerations ***************
// Relocated from MyPersistentData.h unchanged in value. Now namespace-scoped,
// so call sites read SystemConfig::COUNTING rather than bare COUNTING.

/**
 * @brief Sensor mode defines how the device processes sensor data.
 */
enum SensorMode {
    COUNTING    = 0,  // Count each detection event
    OCCUPANCY   = 1,  // Track occupied/unoccupied state
    MEASUREMENT = 2   // Analog sensor readings
};

/**
 * @brief Connection mode defines power and connectivity behavior.
 */
enum ConnectionMode {
    CONNECTED               = 0,  // Always connected, frequent reporting
    INTERMITTENT            = 1,  // Disconnect and sleep between reports
    DISCONNECTED            = 2,  // Stay offline unless manually overridden
    INTERMITTENT_KEEP_ALIVE = 3   // Network standby during open hours
};

/**
 * @brief Reporting mode defines what triggers a report.
 */
enum ReportingMode {
    SCHEDULED              = 0,  // Time-based reporting
    ON_CHANGE              = 1,  // State change triggers
    THRESHOLD              = 2,  // Threshold crossing
    SCHEDULED_OR_THRESHOLD = 3   // Either condition triggers
};

/**
 * @brief Sampling mode defines how the sensor is read.
 */
enum SamplingMode {
    INTERRUPT = 0,  // Hardware interrupt-driven
    POLLING   = 1   // Periodic timer-based
};

// *************** Persisted string capacities ***************
// Finding 2: ConfigApply.cpp sizes its staging buffers from
// sizeof(sysStatusData::SysData::timeZoneStr) / ::webhookName. Those are
// private to the .cpp after the split, so the capacities are published here
// instead. MyPersistentData.cpp carries a static_assert binding each
// constant to the real field size, so they cannot drift apart silently.

constexpr size_t kTimeZoneCapacity    = 39;
constexpr size_t kWebhookNameCapacity = 64;

// *************** Store lifecycle (/usr/sysStatus.dat) ***************

/// Forwards to sysStatusData::setup() - withSaveDelayMs(100).load().
void setup();

/// Forwards to sysStatusData::loop() - deferred flush servicing.
void loop();

/// Forwards to sysStatus.flush(true) - synchronous forced write.
void flushNow();

/**
 * @brief Forwards to sysStatusData::validate() with the same size argument
 *        the pre-split call site passed.
 *
 * Finding 2: ConfigApply.cpp currently calls sysStatus.validate(sizeof(sysStatus)),
 * i.e. sizeof(sysStatusData) - the class object, not SysData. That exact
 * value is preserved inside the implementation. Callers must not compute a
 * sizeof() of their own; doing so against any facade type would silently
 * change validation behavior.
 */
bool validateStoredData();

// *************** Verbose / serial / diagnostics configuration ***************

bool get_verboseMode();
void set_verboseMode(bool value);

uint16_t get_verboseTimeoutMin();
void set_verboseTimeoutMin(uint16_t value);

time_t get_verboseModeStartTime();
void set_verboseModeStartTime(time_t value);

bool get_serialConnected();
void set_serialConnected(bool value);

// *************** Schedule and timing configuration ***************

const char *get_timeZoneStrCStr();
bool set_timeZoneStr(const char *str);

uint8_t get_openTime();
void set_openTime(uint8_t value);

uint8_t get_closeTime();
void set_closeTime(uint8_t value);

uint16_t get_reportingInterval();
void set_reportingInterval(uint16_t value);

time_t get_lastReport();
void set_lastReport(time_t value);

time_t get_lastHookResponse();
void set_lastHookResponse(time_t value);

time_t get_lastDailyCleanup();
void set_lastDailyCleanup(time_t value);

time_t get_lastTimeSync();
void set_lastTimeSync(time_t value);

// *************** Operating modes ***************

/// sysStatus-level sensor type (0 = pressure, 1 = PIR). Distinct from
/// SensorSettings::get_sensorType(), which is the /usr/sensor.dat field -
/// Finding 3 requires the two stay distinct.
uint8_t get_sensorType();
void set_sensorType(uint8_t value);

uint8_t get_sensorMode();
void set_sensorMode(uint8_t value);

uint8_t get_connectionMode();
void set_connectionMode(uint8_t value);

uint8_t get_reportingMode();
void set_reportingMode(uint8_t value);

uint8_t get_samplingMode();
void set_samplingMode(uint8_t value);

bool get_disconnectedMode();
void set_disconnectedMode(bool value);

bool get_enableHibernateSleep();
void set_enableHibernateSleep(bool value);

// *************** Connection budgets and connection history ***************
// Decision B: these were the fields with no home in the original four-way
// split; they live here rather than in a fifth facade.

uint16_t get_connectAttemptBudgetSec();
void set_connectAttemptBudgetSec(uint16_t value);

uint16_t get_cloudDisconnectBudgetSec();
void set_cloudDisconnectBudgetSec(uint16_t value);

uint16_t get_modemOffBudgetSec();
void set_modemOffBudgetSec(uint16_t value);

uint8_t get_connectionAttemptCounter();
void set_connectionAttemptCounter(uint8_t value);

time_t get_lastConnection();
void set_lastConnection(time_t value);

uint16_t get_lastConnectionDuration();
void set_lastConnectionDuration(uint16_t value);

/// Test-mode connection duration override (0xFFFF = disabled).
uint16_t get_testConnectionDurationOverride();
void set_testConnectionDurationOverride(uint16_t value);

// *************** Webhook configuration ***************

const char *get_webhookNameCStr();
bool set_webhookName(const char *str);

bool get_webhookEnabled();
void set_webhookEnabled(bool value);

uint32_t get_webhookTimeoutMs();
void set_webhookTimeoutMs(uint32_t value);

// *************** Ledger configuration provenance ***************

bool get_hasValidLedgerConfig();
void set_hasValidLedgerConfig(bool value);

/// Config::Source enum persisted for diagnostics.
uint8_t get_configSource();
void set_configSource(uint8_t value);

/**
 * @brief The separate /usr/sensor.dat store (sensorConfigData / SensorData).
 *
 * Finding 3: this is a third PersistentDataFile-backed record, not a view of
 * sysStatus. It is nested here - rather than given a fifth facade - because
 * decision B folds sensor settings into SystemConfig's remit, and nesting
 * keeps its own get_sensorType() unambiguously distinct from the
 * sysStatus-level one above.
 */
namespace SensorSettings {

/// Forwards to sensorConfigData::setup().
void setup();

/// Forwards to sensorConfigData::loop().
void loop();

/**
 * @brief Forwards to sensorConfigData::validate() with the same size
 *        argument the pre-split call site passed (sizeof(sensorConfigData)).
 */
bool validateStoredData();

/// /usr/sensor.dat sensor type (1 = PIR, 2 = Analog, 3 = Ultrasonic, ...).
uint8_t get_sensorType();
void set_sensorType(uint8_t value);

uint32_t get_sensorSetting1();
void set_sensorSetting1(uint32_t value);

uint32_t get_sensorSetting2();
void set_sensorSetting2(uint32_t value);

uint32_t get_sensorSetting3();
void set_sensorSetting3(uint32_t value);

uint32_t get_sensorSetting4();
void set_sensorSetting4(uint32_t value);

}  // namespace SensorSettings

}  // namespace SystemConfig

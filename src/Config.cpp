#include "Config.h"

#include "MyPersistentData.h"

namespace Config {

namespace {

void setFailureReason(const char **failureReason, const char *reason) {
    if (failureReason) {
        *failureReason = reason;
    }
}

} // namespace

const char *sourceToString(Source source) {
    switch (source) {
    case CONFIG_SOURCE_DEFAULT:
        return "DEFAULT";
    case CONFIG_SOURCE_STORAGE:
        return "STORAGE";
    case CONFIG_SOURCE_LEDGER:
        return "LEDGER";
    default:
        return "UNKNOWN";
    }
}

Source getSource() {
    uint8_t raw = sysStatus.get_configSource();
    if (raw > (uint8_t)CONFIG_SOURCE_LEDGER) {
        return CONFIG_SOURCE_DEFAULT;
    }
    return (Source)raw;
}

void setSource(Source source, const char *reason, bool persist) {
    Source prior = getSource();
    if (persist) {
        sysStatus.set_configSource((uint8_t)source);
    }

    if (prior != source || reason != nullptr) {
#if ENABLE_CONFIG_TRACE
        if (reason && reason[0] != '\0') {
            Log.info("ConfigSource: %s reason=%s", sourceToString(source), reason);
        } else {
            Log.info("ConfigSource: %s", sourceToString(source));
        }
#endif
    }
}

bool validateConfigFields(bool logFailures, const char **failureReason) {
    bool valid = true;
    setFailureReason(failureReason, "none");

    const char *tz = sysStatus.get_timeZoneStrCStr();
    if (!tz || tz[0] == '\0') {
        if (logFailures) {
            Log.warn("ConfigInvalid: timing.timezone missing");
        }
        setFailureReason(failureReason, "timing.timezone missing");
        valid = false;
    }

    uint8_t openHour = sysStatus.get_openTime();
    if (openHour > 23) {
        if (logFailures) {
            Log.warn("ConfigInvalid: timing.openHour=%u out of range", (unsigned)openHour);
        }
        if (valid) {
            setFailureReason(failureReason, "timing.openHour out of range");
        }
        valid = false;
    }

    uint8_t closeHour = sysStatus.get_closeTime();
    if (closeHour > 23) {
        if (logFailures) {
            Log.warn("ConfigInvalid: timing.closeHour=%u out of range", (unsigned)closeHour);
        }
        if (valid) {
            setFailureReason(failureReason, "timing.closeHour out of range");
        }
        valid = false;
    }

    uint16_t reportingIntervalSec = sysStatus.get_reportingInterval();
    if (reportingIntervalSec == 0) {
        if (logFailures) {
            Log.warn("ConfigInvalid: timing.reportingIntervalSec missing/zero");
        }
        if (valid) {
            setFailureReason(failureReason, "timing.reportingIntervalSec missing/zero");
        }
        valid = false;
    }

    uint32_t occupancyDebounceMs = sensorConfig.get_sensorSetting1();
    if (occupancyDebounceMs == 0) {
        if (logFailures) {
            Log.warn("ConfigInvalid: sensor.setting1 (occupancy debounce) missing/zero");
        }
        if (valid) {
            setFailureReason(failureReason, "sensor.setting1 missing/zero");
        }
        valid = false;
    }

    return valid;
}

bool isValid(bool logFailures, const char **failureReason) {
    bool valid = validateConfigFields(logFailures, failureReason);

    const Source source = getSource();
    const bool ledgerValid = sysStatus.get_hasValidLedgerConfig();
    const bool sourceValid =
        (source == CONFIG_SOURCE_LEDGER) ||
        ((source == CONFIG_SOURCE_STORAGE) && ledgerValid);

    if (!sourceValid) {
        if (logFailures) {
            Log.warn("ConfigInvalid: source=%s ledgerValid=%d is not a trusted cache state",
                     sourceToString(source),
                     ledgerValid ? 1 : 0);
        }
        if (valid) {
            setFailureReason(failureReason, "source/ledger cache not trusted");
        }
        valid = false;
    }

    return valid;
}

bool isValid(bool logFailures) {
    return isValid(logFailures, nullptr);
}

uint16_t reportingIntervalSecForRuntime() {
    if (!isValid(false)) {
        return DEFAULT_REPORT_INTERVAL_SEC;
    }

    uint16_t interval = sysStatus.get_reportingInterval();
    return interval == 0 ? DEFAULT_REPORT_INTERVAL_SEC : interval;
}

uint32_t occupancyDebounceMsForRuntime() {
    if (!isValid(false)) {
        return DEFAULT_OCCUPANCY_DEBOUNCE_MS;
    }

    uint32_t debounceMs = sensorConfig.get_sensorSetting1();
    return debounceMs == 0 ? DEFAULT_OCCUPANCY_DEBOUNCE_MS : debounceMs;
}

void markLedgerConfigurationValid() {
    if (!sysStatus.get_hasValidLedgerConfig()) {
        sysStatus.set_hasValidLedgerConfig(true);
    }
    setSource(CONFIG_SOURCE_LEDGER, "ledger-apply");
    sysStatus.flush(true);
}

void markStorageConfigurationLoaded() {
    setSource(CONFIG_SOURCE_STORAGE, "boot-cache");
}

void markFactoryDefaultsActive() {
    sysStatus.set_hasValidLedgerConfig(false);
    setSource(CONFIG_SOURCE_DEFAULT, "factory-defaults");
}

void logDiagnostics(const char *tag) {
#if ENABLE_CONFIG_TRACE
    const bool valid = isValid(false);
    const char *tz = sysStatus.get_timeZoneStrCStr();
    if (!tz) {
        tz = "";
    }

    Log.info("%s: source=%s valid=%d timezone=%s open=%u close=%u report=%u",
             tag ? tag : "ConfigDiag",
             sourceToString(getSource()),
             valid ? 1 : 0,
             tz,
             (unsigned)sysStatus.get_openTime(),
             (unsigned)sysStatus.get_closeTime(),
             (unsigned)sysStatus.get_reportingInterval());
#else
    (void)tag;
#endif
}

} // namespace Config

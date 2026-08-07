#include "power/PowerDiagnostics.h"

#include "BuildProfile.h"
#include "MyPersistentData.h"
#include "power/PowerManager.h"
#include "power/PowerPlatform.h"

#if defined(ENABLE_DIAGNOSTICS_PUBLISH_MODE) && ENABLE_DIAGNOSTICS_PUBLISH_MODE
#include <cstdarg>
#include "PublishQueuePosixRK.h"
#endif

namespace PowerDiagnostics {

namespace {

constexpr int kPowerSourceUnknown = 0;
constexpr int kPowerSourceVin = 1;
constexpr int kPowerSourceUsbHost = 2;
constexpr int kPowerSourceUsbAdapter = 3;
constexpr int kPowerSourceUsbOtg = 4;
constexpr int kPowerSourceBattery = 5;

#if defined(ENABLE_DIAGNOSTICS_PUBLISH_MODE) && ENABLE_DIAGNOSTICS_PUBLISH_MODE

// Numeric reason codes for the batched "pdiag" payload - kept short to
// leave byte headroom under PublishQueuePosixRK's ~622-byte event ceiling.
// Codes 0-9 come from logPowerState()'s `reason` string; 10 and 11 are
// synthetic codes for the ChargeDiag and stale-SOC-resync captures, which
// aren't routed through logPowerState().
constexpr uint8_t kReasonSetup = 0;
constexpr uint8_t kReasonPostWakeSetup = 1;
constexpr uint8_t kReasonPostRefreshInputProfile = 2;
constexpr uint8_t kReasonProfileChange = 3;
constexpr uint8_t kReasonConnectSuccess = 4;
constexpr uint8_t kReasonPreHibernate = 5;
constexpr uint8_t kReasonPreSleepUlp = 6;
constexpr uint8_t kReasonPreSleepStopFallback = 7;
constexpr uint8_t kReasonPreSleepStopTimerOnly = 8;
constexpr uint8_t kReasonPostWake = 9;
constexpr uint8_t kReasonChargeDiag = 10;
constexpr uint8_t kReasonResync = 11;
constexpr uint8_t kReasonUnknown = 255;

uint8_t reasonCodeFor(const char *reason) {
  if (!reason) return kReasonUnknown;
  if (strcmp(reason, "setup") == 0) return kReasonSetup;
  if (strcmp(reason, "post-wake-setup") == 0) return kReasonPostWakeSetup;
  if (strcmp(reason, "post-refreshInputProfile") == 0) return kReasonPostRefreshInputProfile;
  if (strcmp(reason, "profile-change") == 0) return kReasonProfileChange;
  if (strcmp(reason, "connect-success") == 0) return kReasonConnectSuccess;
  if (strcmp(reason, "pre-hibernate") == 0) return kReasonPreHibernate;
  if (strcmp(reason, "pre-sleep-ulp") == 0) return kReasonPreSleepUlp;
  if (strcmp(reason, "pre-sleep-stop-fallback") == 0) return kReasonPreSleepStopFallback;
  if (strcmp(reason, "pre-sleep-stop-timer-only") == 0) return kReasonPreSleepStopTimerOnly;
  if (strcmp(reason, "post-wake") == 0) return kReasonPostWake;
  return kReasonUnknown;
}

constexpr uint8_t kDiagBatchCapacity = 12;

struct DiagBatchEntry {
  uint8_t reasonCode;
  int8_t powerSource;
  uint8_t profile;
  uint8_t vbusStatus;
  uint8_t powerGood;
  uint8_t chargeStatus;
  uint8_t faultReg;
  uint8_t charging;
  float soc;
  float vcell;
};

DiagBatchEntry diagBatch[kDiagBatchCapacity];
uint8_t diagBatchCount = 0;
uint16_t diagBatchDroppedCount = 0;

void appendDiagBatchEntry(const DiagBatchEntry &entry) {
  if (diagBatchCount >= kDiagBatchCapacity) {
    if (diagBatchDroppedCount < 0xFFFF) {
      diagBatchDroppedCount++;
    }
    return;
  }
  diagBatch[diagBatchCount++] = entry;
}

// Appends a formatted string to `buf` (capacity bufSize) at *offset, advancing
// *offset only when the formatted text fits *in full*. vsnprintf() returns the
// length it *would* have written (not the truncated length actually written),
// so that return value can never be trusted to advance *offset - doing so
// would let *offset run past bufSize and silently drop everything written
// after (including a closing "]}"), producing invalid JSON. Instead, if the
// text wouldn't fully fit, this leaves *offset untouched (any partial bytes
// vsnprintf wrote get overwritten by the next successful append at the same
// offset) and returns false so the caller can bail out of adding that entry
// cleanly rather than emit a partial one.
bool appendFormatted(char *buf, size_t bufSize, size_t *offset, const char *fmt, ...) {
  if (*offset >= bufSize) return false;
  va_list args;
  va_start(args, fmt);
  const int written = vsnprintf(buf + *offset, bufSize - *offset, fmt, args);
  va_end(args);
  if (written < 0) {
    return false;
  }
  if ((size_t)written >= bufSize - *offset) {
    // vsnprintf truncated the output - it did not fully fit.
    return false;
  }
  *offset += (size_t)written;
  return true;
}

#endif // ENABLE_DIAGNOSTICS_PUBLISH_MODE

} // namespace

const char *availabilityLabel(PowerAvailability availability) {
  switch (availability) {
  case PowerAvailability::Valid:
    return "valid";
  case PowerAvailability::Unknown:
    return "unknown";
  case PowerAvailability::NotAvailable:
    return "not-available";
  case PowerAvailability::Fallback:
    return "fallback";
  }

  return "unknown";
}

const char *batteryContextLabel(PowerBatteryContext context) {
  switch (context) {
  case PowerBatteryContext::Unknown:
    return "Unknown";
  case PowerBatteryContext::NotCharging:
    return "Not Charging";
  case PowerBatteryContext::Charging:
    return "Charging";
  case PowerBatteryContext::Charged:
    return "Charged";
  case PowerBatteryContext::Discharging:
    return "Discharging";
  case PowerBatteryContext::Fault:
    return "Fault";
  case PowerBatteryContext::Disconnected:
    return "Disconnected";
  case PowerBatteryContext::NotApplicable:
    return "Not Applicable";
  }

  return "Unknown";
}

const char *tierLabel(PowerTier tier) {
  switch (tier) {
  case PowerTier::Healthy:
    return "Healthy";
  case PowerTier::Conserving:
    return "Conserving";
  case PowerTier::Critical:
    return "Critical";
  case PowerTier::Survival:
    return "Survival";
  case PowerTier::Unknown:
    return "Unknown";
  }

  return "Unknown";
}

const char *inputProfileLabel(PowerInputProfile profile) {
  switch (profile) {
  case PowerInputProfile::UsbBench:
    return "UsbBench";
  case PowerInputProfile::Solar35W:
    return "Solar";
  case PowerInputProfile::Auto:
    return "Auto";
  case PowerInputProfile::NotApplicable:
    return "NotApplicable";
  }

  return "NotApplicable";
}

const char *powerSourceLabel(int powerSource) {
  switch (powerSource) {
  case kPowerSourceUnknown:
    return "UNKNOWN";
  case kPowerSourceVin:
    return "VIN";
  case kPowerSourceUsbHost:
    return "USB_HOST";
  case kPowerSourceUsbAdapter:
    return "USB_ADAPTER";
  case kPowerSourceUsbOtg:
    return "USB_OTG";
  case kPowerSourceBattery:
    return "BATTERY";
  default:
    return "UNAVAILABLE";
  }
}

const char *profileSelectionReasonLabel(PowerProfileSelectionReason reason) {
  switch (reason) {
  case PowerProfileSelectionReason::Unknown:
    return "unknown";
  case PowerProfileSelectionReason::UsbPowerSource:
    return "usb_power_source";
  case PowerProfileSelectionReason::VinPowerSource:
    return "vin_power_source";
  case PowerProfileSelectionReason::BatteryKeepLast:
    return "battery_keep_last";
  case PowerProfileSelectionReason::BatteryFallback:
    return "battery_fallback";
  case PowerProfileSelectionReason::UnknownSourceKeepLast:
    return "unknown_source_keep_last";
  case PowerProfileSelectionReason::UnknownSourceFallback:
    return "unknown_source_fallback";
  case PowerProfileSelectionReason::UnsupportedPlatform:
    return "unsupported_platform";
  }

  return "unknown";
}

void logPowerState(const char *reason, bool forceLog) {
  // Monotonic counter for diagnostic tracking
  static uint32_t logCounter = 0;
  (void)forceLog; // Suppression disabled for diagnostic purposes

  // Read current power state
  const PowerReport &report = PowerManager::instance().latestReport();
  const int powerSource = report.reading.powerSource;
  const PowerInputProfile profile = report.activeInputProfile;
  const float soc = PowerManager::instance().soc();

  // Read PMIC state if available
  uint8_t vbusStatus = 0;
  bool powerGood = false;
  bool pmicAvailable = false;

#if HAL_PLATFORM_CELLULAR && (PLATFORM_ID != PLATFORM_MSOM)
  if (PowerPlatform::hasPmic()) {
    PMIC pmic(true);
    const byte systemStatus = pmic.getSystemStatus();
    vbusStatus = (systemStatus >> 6) & 0x03;
    powerGood = (systemStatus & 0x04) != 0;
    pmicAvailable = true;
  }
#endif

  // Read Nordic USB registers if available
  uint32_t usbAddr = 0;
  uint32_t usbRegStatus = 0;
  bool nordicUsbAvailable = false;

#if HAL_PLATFORM_NRF52840
  // NRF_USBD and NRF_POWER registers are only available on nRF52840
  usbAddr = NRF_USBD->USBADDR;
  usbRegStatus = NRF_POWER->USBREGSTATUS;
  nordicUsbAvailable = true;
#endif

  // Build compact log message with counter
  logCounter++;
  if (pmicAvailable && nordicUsbAvailable) {
    Log.warn("PowerDiag[%lu]: %s source=%s profile=%s vbus=%u pg=%d soc=%.1f%% usbAddr=0x%lx usbReg=0x%lx",
             (unsigned long)logCounter,
             reason,
             powerSourceLabel(powerSource),
             inputProfileLabel(profile),
             (unsigned)vbusStatus,
             powerGood ? 1 : 0,
             soc,
             (unsigned long)usbAddr,
             (unsigned long)usbRegStatus);
  } else if (pmicAvailable) {
    Log.warn("PowerDiag[%lu]: %s source=%s profile=%s vbus=%u pg=%d soc=%.1f%%",
             (unsigned long)logCounter,
             reason,
             powerSourceLabel(powerSource),
             inputProfileLabel(profile),
             (unsigned)vbusStatus,
             powerGood ? 1 : 0,
             soc);
  } else if (nordicUsbAvailable) {
    Log.warn("PowerDiag[%lu]: %s source=%s profile=%s soc=%.1f%% usbAddr=0x%lx usbReg=0x%lx",
             (unsigned long)logCounter,
             reason,
             powerSourceLabel(powerSource),
             inputProfileLabel(profile),
             soc,
             (unsigned long)usbAddr,
             (unsigned long)usbRegStatus);
  } else {
    Log.warn("PowerDiag[%lu]: %s source=%s profile=%s soc=%.1f%%",
             (unsigned long)logCounter,
             reason,
             powerSourceLabel(powerSource),
             inputProfileLabel(profile),
             soc);
  }

#if defined(ENABLE_DIAGNOSTICS_PUBLISH_MODE) && ENABLE_DIAGNOSTICS_PUBLISH_MODE
  {
    DiagBatchEntry entry{};
    entry.reasonCode = reasonCodeFor(reason);
    entry.powerSource = (int8_t)powerSource;
    entry.profile = (uint8_t)profile;
    entry.vbusStatus = vbusStatus;
    entry.powerGood = powerGood ? 1 : 0;
    entry.soc = soc;
    appendDiagBatchEntry(entry);
  }
#endif
}

#if defined(ENABLE_DIAGNOSTICS_PUBLISH_MODE) && ENABLE_DIAGNOSTICS_PUBLISH_MODE

void recordChargeDiagEvent(uint8_t chargeStatus, uint8_t faultReg, bool charging,
                            float vcell, float soc, int powerSource,
                            PowerInputProfile profile) {
  DiagBatchEntry entry{};
  entry.reasonCode = kReasonChargeDiag;
  entry.powerSource = (int8_t)powerSource;
  entry.profile = (uint8_t)profile;
  entry.chargeStatus = chargeStatus;
  entry.faultReg = faultReg;
  entry.charging = charging ? 1 : 0;
  entry.soc = soc;
  entry.vcell = vcell;
  appendDiagBatchEntry(entry);
}

void recordResyncEvent(float soc, float vcell) {
  DiagBatchEntry entry{};
  entry.reasonCode = kReasonResync;
  entry.soc = soc;
  entry.vcell = vcell;
  appendDiagBatchEntry(entry);
}

void flushDiagBatch() {
  if (diagBatchCount == 0) {
    diagBatchDroppedCount = 0;
    return;
  }

  char payload[700];
  size_t offset = 0;
  bool truncated = (diagBatchDroppedCount > 0);

  // Reserve room at the tail of the buffer for the closing "]}" (or
  // "],\"trunc\":1}" if serialization has to bail out early) so the header
  // and per-entry writes below can never consume the bytes needed to
  // terminate the JSON. This guarantees flushDiagBatch() always emits valid,
  // parseable JSON, whether every entry fits or the buffer fills partway
  // through the batch.
  constexpr size_t kClosingReserve = 16; // >= strlen("],\"trunc\":1}") + NUL
  const size_t entryBufSize = sizeof(payload) - kClosingReserve;

  appendFormatted(payload, entryBufSize, &offset, "{\"v\":1,\"n\":%u,\"ev\":[", (unsigned)diagBatchCount);

  for (uint8_t i = 0; i < diagBatchCount; i++) {
    const DiagBatchEntry &e = diagBatch[i];
    const char *sep = (i > 0) ? "," : "";
    bool wrote;
    if (e.reasonCode == kReasonChargeDiag) {
      wrote = appendFormatted(payload, entryBufSize, &offset,
              "%s{\"r\":%u,\"c\":%u,\"i\":%u,\"f\":%u,\"vc\":%.3f,\"soc\":%.1f,\"s\":%d,\"p\":%u}",
              sep, (unsigned)e.reasonCode, (unsigned)e.chargeStatus, (unsigned)e.charging,
              (unsigned)e.faultReg, (double)e.vcell, (double)e.soc,
              (int)e.powerSource, (unsigned)e.profile);
    } else if (e.reasonCode == kReasonResync) {
      wrote = appendFormatted(payload, entryBufSize, &offset,
              "%s{\"r\":%u,\"soc\":%.1f,\"vc\":%.3f}",
              sep, (unsigned)e.reasonCode, (double)e.soc, (double)e.vcell);
    } else {
      wrote = appendFormatted(payload, entryBufSize, &offset,
              "%s{\"r\":%u,\"s\":%d,\"p\":%u,\"vb\":%u,\"pg\":%u,\"soc\":%.1f}",
              sep, (unsigned)e.reasonCode, (int)e.powerSource, (unsigned)e.profile,
              (unsigned)e.vbusStatus, (unsigned)e.powerGood, (double)e.soc);
    }
    if (!wrote) {
      // This entry (comma separator included) didn't fully fit - stop here
      // rather than emit a partial/corrupt entry. Nothing past the last
      // successfully-written entry was touched, so the array is still valid
      // once closed below.
      truncated = true;
      break;
    }
  }

  appendFormatted(payload, sizeof(payload), &offset, truncated ? "],\"trunc\":1}" : "]}");

  PublishQueuePosix::instance().publish("pdiag", payload, PRIVATE);

  diagBatchCount = 0;
  diagBatchDroppedCount = 0;
}

#endif // ENABLE_DIAGNOSTICS_PUBLISH_MODE

} // namespace PowerDiagnostics
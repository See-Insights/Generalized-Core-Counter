#include "power/PowerDiagnostics.h"

#include "MyPersistentData.h"
#include "power/PowerManager.h"
#include "power/PowerPlatform.h"

namespace PowerDiagnostics {

namespace {

constexpr int kPowerSourceUnknown = 0;
constexpr int kPowerSourceVin = 1;
constexpr int kPowerSourceUsbHost = 2;
constexpr int kPowerSourceUsbAdapter = 3;
constexpr int kPowerSourceUsbOtg = 4;
constexpr int kPowerSourceBattery = 5;

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
  const float soc = current.get_stateOfCharge();

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
}

} // namespace PowerDiagnostics
#include "power/PowerDiagnostics.h"

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

} // namespace PowerDiagnostics
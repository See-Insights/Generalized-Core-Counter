#include "power/PowerManager.h"

#include "../Config.h"
#include "MyPersistentData.h"
#include "power/PowerDiagnostics.h"
#include "power/PowerPlatform.h"

namespace {

constexpr int kPowerSourceVin = 1;
constexpr int kPowerSourceUsbHost = 2;
constexpr int kPowerSourceUsbAdapter = 3;
constexpr int kPowerSourceUsbOtg = 4;
constexpr int kPowerSourceBattery = 5;

PowerInputProfile configuredFallbackProfile() {
#if defined(FIELD_BUILD) && FIELD_BUILD
  return sysStatus.get_solarPowerMode() ? PowerInputProfile::Solar35W
                                        : PowerInputProfile::UsbBench;
#else
  return PowerInputProfile::UsbBench;
#endif
}

PowerInputProfile selectInputProfile(
    const PowerPlatform::PowerSourceSnapshot &snapshot,
    PowerInputProfile fallbackProfile,
    PowerInputProfile lastAppliedProfile,
    PowerProfileSelectionReason &reason) {
  switch (snapshot.source) {
  case kPowerSourceUsbHost:
    reason = PowerProfileSelectionReason::UsbPowerSource;
    return PowerInputProfile::UsbBench;
  case kPowerSourceUsbAdapter:
    reason = PowerProfileSelectionReason::UsbPowerSource;
    return PowerInputProfile::UsbBench;
  case kPowerSourceUsbOtg:
    reason = PowerProfileSelectionReason::UsbPowerSource;
    return PowerInputProfile::UsbBench;
  case kPowerSourceVin:
    reason = PowerProfileSelectionReason::VinPowerSource;
    return PowerInputProfile::Solar35W;
  case kPowerSourceBattery:
    if (lastAppliedProfile != PowerInputProfile::NotApplicable) {
      reason = PowerProfileSelectionReason::BatteryKeepLast;
      return lastAppliedProfile;
    }
    reason = PowerProfileSelectionReason::BatteryFallback;
    return fallbackProfile;
  default:
    reason = PowerProfileSelectionReason::UnknownSourceFallback;
    return fallbackProfile;
  }
}

bool shouldLogProfileDecision(const PowerReport &previousReport,
                             const PowerReport &nextReport) {
  return !previousReport.valid ||
         previousReport.activeInputProfile != nextReport.activeInputProfile ||
         previousReport.reading.powerSource != nextReport.reading.powerSource ||
         previousReport.inputProfileReason != nextReport.inputProfileReason ||
         previousReport.inputProfileStatus != nextReport.inputProfileStatus ||
         previousReport.powerConfigurationResult !=
             nextReport.powerConfigurationResult;
}

const char *compactProfileLabel(PowerInputProfile profile) {
  switch (profile) {
  case PowerInputProfile::UsbBench:
    return "USB";
  case PowerInputProfile::Solar35W:
    return "SOLAR";
  case PowerInputProfile::NotApplicable:
    return "NA";
  default:
    return "?";
  }
}

} // namespace

PowerManager &PowerManager::instance() {
  static PowerManager instance;
  return instance;
}

PowerManager::PowerManager() : report_(), setupComplete_(false) {
  report_.capabilities = PowerPlatform::detectCapabilities();
  report_.action.chargingControlAvailable = report_.capabilities.hasChargingControl;
}

bool PowerManager::setup() {
  report_.capabilities = PowerPlatform::detectCapabilities();
  report_.action.chargingControlAvailable = report_.capabilities.hasChargingControl;
  refreshInputProfile();
  setupComplete_ = true;
  return true;
}

bool PowerManager::refreshInputProfile() {
  report_.capabilities = PowerPlatform::detectCapabilities();
  report_.action.chargingControlAvailable = report_.capabilities.hasChargingControl;

  PowerReport nextReport = report_;
  nextReport.valid = true;
  nextReport.powerConfigurationResult = SYSTEM_ERROR_NONE;

  if (!nextReport.capabilities.hasPmicPowerConfiguration) {
    nextReport.activeInputProfile = PowerInputProfile::NotApplicable;
    nextReport.inputProfileReason =
        PowerProfileSelectionReason::UnsupportedPlatform;
    nextReport.inputProfileStatus = PowerAvailability::NotAvailable;
    nextReport.reading.powerSource = -1;
    nextReport.reading.powerSourceStatus = PowerAvailability::NotAvailable;

    if (shouldLogProfileDecision(report_, nextReport)) {
      Log.info("Power: prof=%s src=%s cfg=%d",
               compactProfileLabel(nextReport.activeInputProfile),
               powerSourceLabel(nextReport.reading.powerSource),
               nextReport.powerConfigurationResult);
    }

    report_ = nextReport;
    return true;
  }

  const PowerPlatform::PowerSourceSnapshot sourceSnapshot =
      PowerPlatform::readPowerSource();
  nextReport.reading.powerSource = sourceSnapshot.source;
  nextReport.reading.powerSourceStatus = sourceSnapshot.status;

  const PowerInputProfile fallbackProfile = configuredFallbackProfile();
  PowerProfileSelectionReason selectionReason =
      PowerProfileSelectionReason::Unknown;
  const PowerInputProfile selectedProfile =
      selectInputProfile(sourceSnapshot,
                         fallbackProfile,
                         report_.activeInputProfile,
                         selectionReason);
  nextReport.activeInputProfile = selectedProfile;
  nextReport.inputProfileReason = selectionReason;
  nextReport.inputProfileStatus =
      (selectedProfile == PowerInputProfile::NotApplicable)
          ? PowerAvailability::NotAvailable
        : ((sourceSnapshot.status == PowerAvailability::Unknown ||
          sourceSnapshot.status == PowerAvailability::NotAvailable ||
          sourceSnapshot.status == PowerAvailability::Fallback)
                 ? PowerAvailability::Fallback
                 : PowerAvailability::Valid);

  const bool shouldApplyProfile =
      !report_.valid || report_.activeInputProfile != selectedProfile;
  if (shouldApplyProfile) {
    const PowerPlatform::PowerConfigurationApplyResult applyResult =
        PowerPlatform::applyInputProfile(selectedProfile);
    nextReport.powerConfigurationResult = applyResult.systemResult;
    if (!applyResult.applied) {
      nextReport.inputProfileStatus = PowerAvailability::Fallback;
    }
  }

  if (shouldLogProfileDecision(report_, nextReport)) {
    Log.info("Power: prof=%s src=%s cfg=%d",
             compactProfileLabel(nextReport.activeInputProfile),
             powerSourceLabel(nextReport.reading.powerSource),
             nextReport.powerConfigurationResult);
  }

  report_ = nextReport;
  return report_.powerConfigurationResult == SYSTEM_ERROR_NONE;
}

const PowerReport &PowerManager::latestReport() const {
  return report_;
}

const PowerCapabilities &PowerManager::capabilities() const {
  return report_.capabilities;
}

bool PowerManager::hasPmic() const {
  return report_.capabilities.hasPmic;
}

bool PowerManager::hasFuelGauge() const {
  return report_.capabilities.hasFuelGauge;
}

const char *PowerManager::availabilityLabel(PowerAvailability availability) {
  return PowerDiagnostics::availabilityLabel(availability);
}

const char *PowerManager::batteryContextLabel(PowerBatteryContext context) {
  return PowerDiagnostics::batteryContextLabel(context);
}

const char *PowerManager::tierLabel(PowerTier tier) {
  return PowerDiagnostics::tierLabel(tier);
}

const char *PowerManager::inputProfileLabel(PowerInputProfile profile) {
  return PowerDiagnostics::inputProfileLabel(profile);
}

const char *PowerManager::powerSourceLabel(int powerSource) {
  return PowerDiagnostics::powerSourceLabel(powerSource);
}

const char *PowerManager::profileSelectionReasonLabel(
    PowerProfileSelectionReason reason) {
  return PowerDiagnostics::profileSelectionReasonLabel(reason);
}
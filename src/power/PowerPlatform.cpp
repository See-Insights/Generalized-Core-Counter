#include "power/PowerPlatform.h"

#include "MyPersistentData.h"
#include "power/PowerDiagnostics.h"

namespace PowerPlatform {

namespace {

constexpr int kPowerSourceUnknown = 0;
constexpr int kPowerSourceVin = 1;
constexpr int kPowerSourceUsbHost = 2;
constexpr int kPowerSourceUsbAdapter = 3;
constexpr int kPowerSourceUsbOtg = 4;
constexpr int kPowerSourceBattery = 5;

bool isRecognizedPowerSource(int source) {
  switch (source) {
  case kPowerSourceUnknown:
  case kPowerSourceVin:
  case kPowerSourceUsbHost:
  case kPowerSourceUsbAdapter:
  case kPowerSourceUsbOtg:
  case kPowerSourceBattery:
    return true;
  default:
    return false;
  }
}

bool isUnavailablePowerSource(int source) {
  return source < 0 || !isRecognizedPowerSource(source);
}

int lastObservedPowerSource = -1;

#if HAL_PLATFORM_CELLULAR && (PLATFORM_ID != PLATFORM_MSOM)
constexpr int USB_BENCH_MAX_CURRENT_MA = 900;
constexpr int USB_BENCH_MIN_VOLTAGE_MV = 3880;
constexpr int USB_BENCH_CHARGE_CURRENT_MA = 896;
constexpr int USB_BENCH_CHARGE_VOLTAGE_MV = 4112;

constexpr int SOLAR_MAX_CURRENT_MA = 900;
constexpr int SOLAR_MIN_VOLTAGE_MV = 5080;
constexpr int SOLAR_CHARGE_CURRENT_MA = 900;
constexpr int SOLAR_CHARGE_VOLTAGE_MV = 4208;

void logAppliedProfileConfig(PowerInputProfile profile,
                             int sourceMaxCurrentMa,
                             int sourceMinVoltageMv,
                             int chargeCurrentMa,
                             int chargeVoltageMv,
                             int systemResult) {
  const PowerSourceSnapshot snapshot = readPowerSource();
  PMIC pmic(true);
  const bool chargingEnabled = pmic.isChargingEnabled();

  if (systemResult != SYSTEM_ERROR_NONE) {
    Log.warn("PowerCfg: source=%d inputCurrent=%u minSystemVoltage=%u chargeVoltage=%u chargingEnabled=%d profile=%s result=%d",
             snapshot.source,
             (unsigned)sourceMaxCurrentMa,
             (unsigned)sourceMinVoltageMv,
             (unsigned)chargeVoltageMv,
             chargingEnabled ? 1 : 0,
             PowerDiagnostics::inputProfileLabel(profile),
             systemResult);
    return;
  }

  Log.info("PowerCfg: source=%d inputCurrent=%u minSystemVoltage=%u chargeVoltage=%u chargingEnabled=%d profile=%s",
           snapshot.source,
           (unsigned)sourceMaxCurrentMa,
           (unsigned)sourceMinVoltageMv,
           (unsigned)chargeVoltageMv,
           chargingEnabled ? 1 : 0,
           PowerDiagnostics::inputProfileLabel(profile));
}

SystemPowerConfiguration makeUsbBenchConfiguration() {
  SystemPowerConfiguration conf;
  conf.powerSourceMaxCurrent(USB_BENCH_MAX_CURRENT_MA)
      .powerSourceMinVoltage(USB_BENCH_MIN_VOLTAGE_MV)
      .batteryChargeCurrent(USB_BENCH_CHARGE_CURRENT_MA)
      .batteryChargeVoltage(USB_BENCH_CHARGE_VOLTAGE_MV);
  return conf;
}

SystemPowerConfiguration makeSolarConfiguration() {
  SystemPowerConfiguration conf;
  // Do not enable USE_VIN_SETTINGS_WITH_USB_HOST here. On Boron / Device OS
  // 6.4.1 this can remap a real USB_HOST source to VIN, causing mixed
  // USB/Solar deployments to apply the 5080mV SOLAR VINDPM threshold and
  // throttle USB charging.
  conf.powerSourceMaxCurrent(SOLAR_MAX_CURRENT_MA)
      .powerSourceMinVoltage(SOLAR_MIN_VOLTAGE_MV)
      .batteryChargeCurrent(SOLAR_CHARGE_CURRENT_MA)
      .batteryChargeVoltage(SOLAR_CHARGE_VOLTAGE_MV);
  return conf;
}
#endif

} // namespace

PowerCapabilities detectCapabilities() {
  PowerCapabilities capabilities;

#if HAL_PLATFORM_CELLULAR
  capabilities.hasFuelGauge = true;
  capabilities.hasSoc = true;
  capabilities.hasBatteryVoltage = true;
  capabilities.hasBatteryContext = true;
  capabilities.hasPowerSource = true;
#endif

#if PLATFORM_ID == PLATFORM_ARGON
  capabilities.hasFuelGauge = true;
  capabilities.hasSoc = true;
  capabilities.hasBatteryVoltage = true;
  capabilities.hasBatteryContext = true;
  capabilities.hasPowerSource = true;
#endif

#if PLATFORM_ID == 32 || PLATFORM_ID == 34
  capabilities.hasSoc = true;
  capabilities.hasBatteryVoltage = true;
#endif

#if HAL_PLATFORM_CELLULAR && (PLATFORM_ID != PLATFORM_MSOM)
  capabilities.hasPmic = true;
  capabilities.hasChargingControl = true;
  capabilities.hasPmicPowerConfiguration = true;
  capabilities.hasPmicRemediation = true;
#endif

  return capabilities;
}

bool hasPmic() {
  return detectCapabilities().hasPmic;
}

bool hasFuelGauge() {
  return detectCapabilities().hasFuelGauge;
}

void noteObservedPowerSource(int source) {
  if (!isUnavailablePowerSource(source)) {
    lastObservedPowerSource = source;
  }
}

PowerSourceSnapshot readPowerSource() {
  PowerSourceSnapshot snapshot;

#if HAL_PLATFORM_CELLULAR
  if (!detectCapabilities().hasPowerSource) {
    snapshot.status = PowerAvailability::NotAvailable;
    return snapshot;
  }

  const int rawSource = System.powerSource();
  if (isUnavailablePowerSource(rawSource)) {
    if (!isUnavailablePowerSource(lastObservedPowerSource)) {
      snapshot.source = lastObservedPowerSource;
      snapshot.status = PowerAvailability::Fallback;
      return snapshot;
    }
    snapshot.status = PowerAvailability::NotAvailable;
    return snapshot;
  }

  snapshot.source = rawSource;
  noteObservedPowerSource(rawSource);
  snapshot.status =
      (snapshot.source == kPowerSourceUnknown) ? PowerAvailability::Unknown
                                               : PowerAvailability::Valid;
#else
  snapshot.status = PowerAvailability::NotAvailable;
#endif
  return snapshot;
}

PowerConfigurationApplyResult applyInputProfile(PowerInputProfile profile) {
  PowerConfigurationApplyResult result;

#if HAL_PLATFORM_CELLULAR && (PLATFORM_ID != PLATFORM_MSOM)
  result.supported = true;

  switch (profile) {
  case PowerInputProfile::UsbBench:
    result.systemResult = System.setPowerConfiguration(makeUsbBenchConfiguration());
    result.applied = (result.systemResult == SYSTEM_ERROR_NONE);
    logAppliedProfileConfig(profile,
                           USB_BENCH_MAX_CURRENT_MA,
                           USB_BENCH_MIN_VOLTAGE_MV,
                           USB_BENCH_CHARGE_CURRENT_MA,
                           USB_BENCH_CHARGE_VOLTAGE_MV,
                           result.systemResult);
    return result;

  case PowerInputProfile::Solar35W:
    result.systemResult = System.setPowerConfiguration(makeSolarConfiguration());
    result.applied = (result.systemResult == SYSTEM_ERROR_NONE);
    logAppliedProfileConfig(profile,
                           SOLAR_MAX_CURRENT_MA,
                           SOLAR_MIN_VOLTAGE_MV,
                           SOLAR_CHARGE_CURRENT_MA,
                           SOLAR_CHARGE_VOLTAGE_MV,
                           result.systemResult);
    return result;

  case PowerInputProfile::Auto:
  case PowerInputProfile::NotApplicable:
    result.systemResult = SYSTEM_ERROR_NONE;
    return result;
  }
#else
  (void)profile;
#endif

  result.systemResult = SYSTEM_ERROR_NONE;
  return result;
}

} // namespace PowerPlatform

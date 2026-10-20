// WO-2026-10-20-001: the shared power-source codes and PMIC REG09 masks keep
// their values. Compiled against the real headers (src/ only, no stubs).
#include "power/PmicFaultMonitor.h"
#include "power/PowerPlatform.h"

#include <cstdio>

// The stubs' mirror of the codes, included in a namespace so it cannot clash
// with the real header's definitions.
namespace mirror {
#include "stubs/power/PowerSourceCodes.h"
}

static_assert(kPowerSourceUnknown == 0, "kPowerSourceUnknown");
static_assert(kPowerSourceVin == 1, "kPowerSourceVin");
static_assert(kPowerSourceUsbHost == 2, "kPowerSourceUsbHost");
static_assert(kPowerSourceUsbAdapter == 3, "kPowerSourceUsbAdapter");
static_assert(kPowerSourceUsbOtg == 4, "kPowerSourceUsbOtg");
static_assert(kPowerSourceBattery == 5, "kPowerSourceBattery");

static_assert(mirror::kPowerSourceUnknown == kPowerSourceUnknown, "mirror kPowerSourceUnknown");
static_assert(mirror::kPowerSourceVin == kPowerSourceVin, "mirror kPowerSourceVin");
static_assert(mirror::kPowerSourceUsbHost == kPowerSourceUsbHost, "mirror kPowerSourceUsbHost");
static_assert(mirror::kPowerSourceUsbAdapter == kPowerSourceUsbAdapter, "mirror kPowerSourceUsbAdapter");
static_assert(mirror::kPowerSourceUsbOtg == kPowerSourceUsbOtg, "mirror kPowerSourceUsbOtg");
static_assert(mirror::kPowerSourceBattery == kPowerSourceBattery, "mirror kPowerSourceBattery");

static_assert(kPmicChargeFaultMask == 0x30, "kPmicChargeFaultMask");
static_assert(kPmicBatFaultMask == 0x08, "kPmicBatFaultMask");
static_assert(kPmicNtcFaultMask == 0x07, "kPmicNtcFaultMask");

int main() {
  std::puts("PASS: power-source codes 0-5 and PMIC masks 0x30/0x08/0x07");
  return 0;
}

#pragma once

// Test-side mirror of the power-source codes in src/power/PowerPlatform.h,
// shared by every PowerPlatform.h stub. The stubs cannot include the real
// header (it redefines PowerSourceSnapshot), so power_constants_values_test.cpp
// static_asserts that every value here equals the real header's.
constexpr int kPowerSourceUnknown = 0;
constexpr int kPowerSourceVin = 1;
constexpr int kPowerSourceUsbHost = 2;
constexpr int kPowerSourceUsbAdapter = 3;
constexpr int kPowerSourceUsbOtg = 4;
constexpr int kPowerSourceBattery = 5;

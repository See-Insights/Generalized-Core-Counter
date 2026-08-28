#pragma once

#include <cmath>
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cstdio>
#include <string>

// Minimal host-side stand-in for Particle.h, scoped to what
// src/power/PmicFaultMonitor.cpp (and the real src/power/PowerManager.cpp it
// links against for PowerManager::instance()/latestReport()) actually need
// (WO-2026-08-25-001 Amendment C, Decision C3 host test).

constexpr int SYSTEM_ERROR_NONE = 0;

struct TestLog {
  template <typename... Args>
  void info(const char *, Args...) {}

  template <typename... Args>
  void warn(const char *, Args...) {}

  template <typename... Args>
  void error(const char *, Args...) {}
};

inline TestLog Log;

#define PLATFORM_BORON 88
#define PLATFORM_ID PLATFORM_BORON

struct TestTimeClass {
  long now() const { return 1700000000L; }
  bool isValid() const { return true; }
};
inline TestTimeClass Time;

struct TestSystemClass {
  unsigned long freeMemory() const { return 65536UL; }
  int resetReason() const { return 0; }
  unsigned long resetReasonData() const { return 0UL; }
};
inline TestSystemClass System;

struct TestParticleClass {
  bool connected() const { return false; }
};
inline TestParticleClass Particle;

struct TestCellularClass {
  bool ready() const { return false; }
};
inline TestCellularClass Cellular;

inline unsigned long g_testMillis = 1000UL;
inline unsigned long millis() { return g_testMillis; }

struct TestNrfUsbdRegs {
  uint32_t USBADDR = 0;
};

struct TestNrfPowerRegs {
  uint32_t USBREGSTATUS = 0;
};

inline TestNrfUsbdRegs testNrfUsbdRegs;
inline TestNrfPowerRegs testNrfPowerRegs;

#define NRF_USBD (&testNrfUsbdRegs)
#define NRF_POWER (&testNrfPowerRegs)

// retained storage qualifier - no-op on host.
#define retained

// Test-controllable stand-in for Device OS's BQ24195 driver surface. Every
// method PmicFaultMonitor.cpp calls on `PMIC &pmic` is reproduced; nothing
// else. getFault() intentionally supports returning two DIFFERENT values on
// successive calls (via faultRegQueue), so a test can assert the production
// double-read-and-OR fix (WO-2026-08-25-001 Amendment C, Decision C3 point 4)
// actually consumes both reads rather than only the first or second.
class PMIC {
 public:
  uint8_t faultRegQueue[2] = {0, 0};
  int faultCallCount = 0;
  uint8_t systemStatusValue = 0;
  int disableChargingCallCount = 0;
  int enableChargingCallCount = 0;
  int setWatchdogCallCount = 0;

  uint8_t getFault() {
    const uint8_t v = faultRegQueue[faultCallCount < 2 ? faultCallCount : 1];
    faultCallCount++;
    return v;
  }

  uint8_t getSystemStatus() { return systemStatusValue; }
  void disableCharging() { disableChargingCallCount++; }
  void enableCharging() { enableChargingCallCount++; }
  void setWatchdog(int) { setWatchdogCallCount++; }
};

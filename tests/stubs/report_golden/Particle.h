#pragma once

// Minimal Particle.h fake for report_payload_golden_test: just enough for the real
// ISensor.h / PIRSensor.h / SensorFactory.h / device_pinout.h to compile. Time and
// millis() are deliberately absent: the test's own counted fakes are used instead.
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <ctime>

typedef uint16_t pin_t;
enum PinMode { INPUT, OUTPUT, INPUT_PULLDOWN };
enum PinState { LOW = 0, HIGH = 1 };
enum InterruptMode { CHANGE, RISING, FALLING };
#define PIN_INVALID (0xff)  // Device OS 6.4.1 hal/inc/pinmap_hal.h:75

inline void pinMode(pin_t, PinMode) {}
inline void digitalWrite(pin_t, int) {}
inline void attachInterrupt(pin_t, void (*)(), InterruptMode) {}
inline void detachInterrupt(pin_t) {}

struct JSONBufferWriter {
  JSONBufferWriter(char *, size_t) {}
  void beginObject() {}
  void endObject() {}
  size_t dataSize() { return 0; }
  JSONBufferWriter &name(const char *) { return *this; }
  template <typename T> JSONBufferWriter &value(T) { return *this; }
};

// Counts SensorFactory's "not yet implemented" errors.
static int g_logErrors = 0;
static struct { void error(const char *, ...) { ++g_logErrors; } } Log;

#pragma once

// Minimal Particle.h fake for pir_sensor_lifecycle_test: records GPIO/interrupt calls.
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

typedef uint16_t pin_t;
enum PinMode { INPUT, OUTPUT, INPUT_PULLDOWN };
enum PinState { LOW = 0, HIGH = 1 };
enum InterruptMode { RISING = 1 };

extern std::vector<std::string> g_calls;
extern void (*g_isr)();
extern unsigned long g_millis;

inline const char *pinName(pin_t p) {
  return p == 13 ? "intPin" : p == 12 ? "disableModule" : p == 11 ? "ledPower" : "?";
}
inline void pinMode(pin_t p, PinMode m) {
  g_calls.push_back(std::string("pinMode(") + pinName(p) + "," +
                    (m == OUTPUT ? "OUTPUT" : m == INPUT_PULLDOWN ? "INPUT_PULLDOWN" : "INPUT") + ")");
}
inline void digitalWrite(pin_t p, int v) {
  g_calls.push_back(std::string("digitalWrite(") + pinName(p) + "," + (v ? "HIGH" : "LOW") + ")");
}
inline void attachInterrupt(pin_t p, void (*isr)(), InterruptMode) {
  g_calls.push_back(std::string("attachInterrupt(") + pinName(p) + ",RISING)");
  g_isr = isr;
}
inline void detachInterrupt(pin_t p) {
  g_calls.push_back(std::string("detachInterrupt(") + pinName(p) + ")");
  g_isr = nullptr;
}
inline unsigned long millis() { return g_millis; }

struct TimeFake { long now() { return 1000; } };
inline TimeFake Time;

struct JSONBufferWriter {
  JSONBufferWriter(char *, size_t) {}
  void beginObject() {}
  void endObject() {}
  size_t dataSize() { return 0; }
  JSONBufferWriter &name(const char *) { return *this; }
  template <typename T> JSONBufferWriter &value(T) { return *this; }
};

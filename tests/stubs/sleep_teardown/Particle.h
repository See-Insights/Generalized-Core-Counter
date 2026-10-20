#pragma once
// Host fakes for the sleep-teardown equivalence harness: a time-scripted world
// (cloud, radio, modem) behind Particle / Cellular / WiFi, the clock, and a Log
// that records level and text. Every request and log line goes to sim::events.
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

constexpr int SYSTEM_ERROR_NONE = 0;
#ifndef Wiring_Cellular
#define Wiring_Cellular 0
#endif
#ifndef Wiring_WiFi
#define Wiring_WiFi 0
#endif

namespace sim {
inline std::vector<std::string> events;
inline unsigned long now = 0;
struct Reads { int connected = 0, isOff = 0, isOn = 0, millis = 0, process = 0, watchdog = 0; };
inline Reads reads;

inline void emit(const char *fmt, ...) {
  char buf[512];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  events.push_back(buf);
}

// Absolute times (ms) at which a scheduled change lands; -1 = never.
struct World {
  bool cloud = true, radioOn = true, modemOff = false;
  long cloudDropAt = -1, radioOnFalseAt = -1, modemOffAt = -1;
  long cloudDelay = -1, modemDelay = -1;
  int flickerAfter = 0;  // >0: the cloud reads as connected from this pass's (n+1)th read on
  void settle() {
    if (cloudDropAt >= 0 && (long)now >= cloudDropAt) { cloud = false; cloudDropAt = -1; }
    if (radioOnFalseAt >= 0 && (long)now >= radioOnFalseAt) { radioOn = false; radioOnFalseAt = -1; }
    if (modemOffAt >= 0 && (long)now >= modemOffAt) { modemOff = true; modemOffAt = -1; }
  }
  void requestCloudDrop() {
    if (cloud && cloudDropAt < 0 && cloudDelay >= 0) cloudDropAt = (long)now + cloudDelay;
  }
  // The radio drops out of "on" halfway, then reports off at modemDelay.
  void requestRadioOff() {
    if (modemDelay < 0 || modemOff) return;
    if (radioOnFalseAt < 0 && radioOn) radioOnFalseAt = (long)now + modemDelay / 2;
    if (modemOffAt < 0) modemOffAt = (long)now + modemDelay;
  }
};
inline World world;
}  // namespace sim

inline uint32_t millis() {
  sim::reads.millis++;
  return (uint32_t)sim::now;
}

struct LogStub {
  void add(const char *level, const char *fmt, va_list ap) {
    char buf[512];
    vsnprintf(buf, sizeof(buf), fmt, ap);
    sim::events.push_back(std::string("LOG ") + level + " " + buf);
  }
  void info(const char *fmt, ...) { va_list ap; va_start(ap, fmt); add("INFO", fmt, ap); va_end(ap); }
  void warn(const char *fmt, ...) { va_list ap; va_start(ap, fmt); add("WARN", fmt, ap); va_end(ap); }
};
inline LogStub Log;

struct ParticleStub {
  bool connected() {
    sim::world.settle();
    sim::reads.connected++;
    if (sim::world.flickerAfter > 0 && sim::reads.connected > sim::world.flickerAfter) return true;
    return sim::world.cloud;
  }
  void disconnect() {
    sim::emit("CALL Particle.disconnect");
    sim::world.requestCloudDrop();
  }
  void process() { sim::reads.process++; }
};
inline ParticleStub Particle;

struct CellularStub {
  bool isOn() {
    sim::world.settle();
    sim::reads.isOn++;
    return sim::world.radioOn;
  }
  bool isOff() {
    sim::world.settle();
    sim::reads.isOff++;
    return sim::world.modemOff;
  }
  void disconnect() { sim::emit("CALL Cellular.disconnect"); }
  void off() {
    sim::emit("CALL Cellular.off");
    sim::world.requestRadioOff();
  }
};
inline CellularStub Cellular;

struct WiFiStub {
  bool isOn() {
    sim::world.settle();
    sim::reads.isOn++;
    return sim::world.radioOn;
  }
  void disconnect() { sim::emit("CALL WiFi.disconnect"); }
  void off() {
    sim::emit("CALL WiFi.off");
    sim::world.requestRadioOff();
  }
};
inline WiFiStub WiFi;

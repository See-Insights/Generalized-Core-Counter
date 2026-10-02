# DRAFT (not filed): `SystemSleepConfiguration` move assignment leaks the existing wake-source list

*Draft prepared 2026-10-02 for WO-2026-10-02-003. Chip decides whether and where to file it.*

---

**Title:** `SystemSleepConfiguration::operator=(SystemSleepConfiguration&&)` leaks the destination's `wakeup_sources` list

**Device OS:** 6.4.1. The same code is in 6.1.1, 6.3.3, 6.3.5, and the current `develop` branch (`system/inc/system_sleep_configuration.h`).
**Platform observed:** Boron (nRF52840). The header is platform-independent, so other platforms should be affected the same way.

## Summary

The move assignment operator copies the source's `hal_sleep_config_t` over the destination with `memcpy` and then nulls the source's pointer. It **never frees the destination's existing `wakeup_sources` linked list**. Each node in that list was allocated by a builder call (`.gpio()`, `.duration()`, `.network()`, …). After the assignment, nothing points to the old list, and the destructor can no longer free it.

```cpp
// system/inc/system_sleep_configuration.h (6.4.1, lines 205–210)
SystemSleepConfiguration& operator=(SystemSleepConfiguration&& config) {
    valid_ = config.valid_;
    memcpy(&config_, &config.config_, sizeof(hal_sleep_config_t));   // overwrites this->config_.wakeup_sources
    config.config_.wakeup_sources = nullptr;
    return *this;
}

// destructor (lines 213–224): the only place the list is freed
~SystemSleepConfiguration() {
    auto wakeupSource = config_.wakeup_sources;
    while (wakeupSource) { auto next = wakeupSource->next; delete wakeupSource; wakeupSource = next; }
}
```

## Impact

A common pattern is to keep one long-lived configuration and reset it before each sleep:

```cpp
SystemSleepConfiguration config;          // global or static
...
config = SystemSleepConfiguration();      // "start fresh"
config.mode(SystemSleepMode::ULTRA_LOW_POWER).gpio(D2, RISING).duration(60s);
System.sleep(config);
```

That pattern leaks one complete wake-source list on every sleep. In our field firmware (Boron, Device OS 6.4.1), a list of 2 GPIO + 1 RTC + 1 network standby source leaked **about 117–141 bytes per sleep cycle**, which matches the node sizes plus heap_4 block overhead. Free memory fell **about 22 KB in 3 hours** on a device that sleeps between frequent PIR wakes. Devices that run for days without a reset eventually run low on heap.

## Minimal reproduction

```cpp
#include "Particle.h"
SYSTEM_MODE(SEMI_AUTOMATIC);
SerialLogHandler logHandler;

SystemSleepConfiguration config;   // long-lived

void setup() { waitFor(Serial.isConnected, 10000); }

void loop() {
    config = SystemSleepConfiguration();                       // move assignment: old list is leaked
    config.mode(SystemSleepMode::ULTRA_LOW_POWER)
          .gpio(D2, RISING)
          .duration(2s);
    System.sleep(config);
    Log.info("freeMemory=%lu", (unsigned long)System.freeMemory());   // falls steadily, cycle after cycle
}
```

**Expected:** `freeMemory` stays level from cycle to cycle.
**Actual:** it falls by the size of one wake-source list (plus allocator overhead) on every cycle.

Replacing the global with a local `SystemSleepConfiguration` inside `loop()` keeps free memory level, because the destructor then frees the list.

## Suggested fix

Free (or swap) the destination's existing list before taking ownership of the source's, for example:

```cpp
SystemSleepConfiguration& operator=(SystemSleepConfiguration&& config) {
    if (this != &config) {
        freeWakeupSources();               // same loop as the destructor
        valid_ = config.valid_;
        memcpy(&config_, &config.config_, sizeof(hal_sleep_config_t));
        config.config_.wakeup_sources = nullptr;
    }
    return *this;
}
```

`std::swap`-style move semantics would also work: the moved-from temporary's destructor would then free the old list.

## Our workaround

Use a fresh local `SystemSleepConfiguration` for every `System.sleep()` call, and never assign to an existing one (Particle's own examples already do this).

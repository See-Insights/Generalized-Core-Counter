#pragma once

// Lightweight host-test stub for PublishQueuePosixRK. Captures the last
// publish() call so tests can inspect the exact payload PowerDiagnostics
// would have queued on-device.

#include <cstring>
#include <string>

constexpr int PRIVATE = 0;
constexpr int PUBLIC = 1;

class PublishQueuePosix {
public:
  static PublishQueuePosix &instance() {
    static PublishQueuePosix inst;
    return inst;
  }

  bool publish(const char *eventName, const char *data, int /*flags1*/, int /*flags2*/ = 0) {
    lastEventName = eventName ? eventName : "";
    lastData = data ? data : "";
    publishCount++;
    return true;
  }

  std::string lastEventName;
  std::string lastData;
  unsigned publishCount = 0;
};

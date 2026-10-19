#pragma once
#include <cstdint>

namespace Observability {
struct WakeCycleStats {
  enum class ConnectResult : int8_t { NOT_ATTEMPTED = -1, SUCCESS = 0, TIMEOUT = 1, ABORTED = 2 };
  ConnectResult connect_result = ConnectResult::NOT_ATTEMPTED;
};
static WakeCycleStats &cycleStats() {
  static WakeCycleStats s;
  return s;
}
}  // namespace Observability

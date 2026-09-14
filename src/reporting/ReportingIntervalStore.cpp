#include "reporting/ReportingIntervalStore.h"

// Relative include, matching every other src/reporting/, src/state/, and
// src/cloud/ file's convention: Device OS ships its own services/inc header
// of the same name, and a bare non-relative include can resolve to that one
// under the local toolchain's include order.
#include "../Config.h"

namespace ReportingIntervalStore {

uint16_t reportingIntervalSec() {
  return Config::reportingIntervalSecForRuntime();
}

} // namespace ReportingIntervalStore

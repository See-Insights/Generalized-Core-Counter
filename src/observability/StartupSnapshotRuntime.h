#pragma once

#include "observability/StartupSnapshot.h"

namespace Observability {

/** Capture the current execution's snapshot after setup completes successfully. */
void captureSuccessfulInitialization();

/** Return the current execution's snapshot, finalizing its epoch once if needed. */
const StartupSnapshot &currentStartupSnapshot();

} // namespace Observability

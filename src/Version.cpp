#include "Version.h"

// Bump this string and the release notes on each new release.
// Keep the value in sync with the Doxyfile PROJECT_NUMBER.

const char* FIRMWARE_VERSION = "v26-NoPdiag";
const char* FIRMWARE_RELEASE_NOTES = "Diagnostics publish off by default: release builds emit no diagnostics events, so diagnostics cannot fill the publish queue and cost reports";

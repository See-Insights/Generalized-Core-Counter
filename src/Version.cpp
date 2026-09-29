#include "Version.h"

// Bump this string and the release notes on each new release.
// Keep the value in sync with the Doxyfile PROJECT_NUMBER.

const char* FIRMWARE_VERSION = "v27-SmallFixes";
const char* FIRMWARE_RELEASE_NOTES = "Small known fixes: closing report connects at close, only our webhook reply clears the wait, status payload overflow guard, live local time in TimeDiag, cloud builds use the vendored libraries";

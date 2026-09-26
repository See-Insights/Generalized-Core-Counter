#include "Version.h"

// Bump this string and the release notes on each new release.
// Keep the value in sync with the Doxyfile PROJECT_NUMBER.

const char* FIRMWARE_VERSION = "v24-Pubq-Ack-B";
const char* FIRMWARE_RELEASE_NOTES = "Bench B: require explicit WITH_ACK for every queued send, including persisted events; retain bench diagnostics";

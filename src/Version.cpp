#include "Version.h"

// Bump this string and the release notes on each new release.
// Keep the value in sync with the Doxyfile PROJECT_NUMBER.

const char* FIRMWARE_VERSION = "v25-WithAck";
const char* FIRMWARE_RELEASE_NOTES = "Publish-with-ack report delivery: queue retained until cloud ack, sleep gate waits on queue depth";

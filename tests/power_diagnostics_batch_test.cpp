// Host-side regression test for PowerDiagnostics's bench-only diagnostics
// batch/flush path (ENABLE_DIAGNOSTICS_PUBLISH_MODE=1 only).
//
// Compiles the real src/power/PowerDiagnostics.cpp (and PowerManager.cpp, to
// satisfy the PowerManager::instance() calls inside logPowerState()) against
// lightweight host stubs under tests/stubs/diag_overrides/ plus a fake
// PublishQueuePosix that captures the exact "pdiag" payload PowerDiagnostics
// would have queued on-device.
//
// This test exercises fix #2 (buffer-write truncation must never produce
// invalid JSON): it fills the diagnostics accumulator with
// ChargeDiag-shaped entries up to and beyond the point where the local
// 700-byte serialization buffer can hold them, and confirms the resulting
// "pdiag" payload is valid, parseable JSON at both the last-entry-that-fits
// boundary and the first-entry-that-overflows case, with correctly closed
// "]}" in every case.
//
// JSON validity is confirmed via a real JSON parser: this file writes each
// captured payload to stdout, and the accompanying
// power_diagnostics_batch_test.sh driver pipes each line through Python's
// json.loads() to verify it parses without throwing.

#include "power/PowerDiagnostics.h"

#include <cstdio>
#include <string>

#include "MyPersistentData.h"
#include "PublishQueuePosixRK.h"

// Required by src/power/PowerManager.cpp's soc()/batteryState() pass-through
// accessors (linked in so PowerDiagnostics.cpp's logPowerState() -> 
// PowerManager::instance() calls resolve, even though this test never calls
// logPowerState() itself).
TestCurrentStatus testCurrent;
TestSystemStatus testSysStatus;

namespace {

// Adds one maximum-width ChargeDiag-shaped entry to the accumulator, i.e. the
// widest realistic byte footprint recordChargeDiagEvent() can produce, so a
// handful of entries reliably exercises the local serialization buffer's
// capacity limits.
void addMaxWidthChargeDiagEntry() {
  PowerDiagnostics::recordChargeDiagEvent(
      /*chargeStatus=*/250, /*faultReg=*/250, /*charging=*/true,
      /*vcell=*/4.199f, /*soc=*/100.0f, /*powerSource=*/-100,
      /*profile=*/PowerInputProfile::Solar35W);
}

// Fills the accumulator with `n` max-width ChargeDiag entries, flushes, and
// returns the exact payload PublishQueuePosix::publish() would have been
// given. Each call starts from an empty accumulator (flushDiagBatch() always
// resets it), so this can be called repeatedly to sweep increasing entry
// counts without any special reset hook.
std::string flushWithEntryCount(uint8_t n) {
  for (uint8_t i = 0; i < n; i++) {
    addMaxWidthChargeDiagEntry();
  }
  PublishQueuePosix::instance().lastData.clear();
  PublishQueuePosix::instance().publishCount = 0;
  PowerDiagnostics::flushDiagBatch();
  return PublishQueuePosix::instance().lastData;
}

bool endsWith(const std::string &s, const std::string &suffix) {
  if (s.size() < suffix.size()) return false;
  return s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

} // namespace

int main() {
  // kDiagBatchCapacity is 12 (accumulator slots) but the local serialization
  // buffer is only 700 bytes; a dozen max-width ChargeDiag entries (~63
  // bytes each) total well over 700 bytes, so sweeping 1..12 entries crosses
  // the serialization-buffer boundary while staying within the 12-slot
  // accumulator cap (recordChargeDiagEvent never triggers the "n > 12"
  // accumulator-drop path in this test).
  std::string lastFitting;
  std::string firstOverflowing;
  int lastFittingCount = -1;
  int firstOverflowingCount = -1;

  for (uint8_t n = 1; n <= 12; n++) {
    const std::string payload = flushWithEntryCount(n);

    if (payload.empty()) {
      fprintf(stderr, "FAIL: flushDiagBatch() produced no payload for n=%u\n", (unsigned)n);
      return 1;
    }

    // Every payload emitted - truncated or not - must be validly terminated.
    if (payload.front() != '{' || payload.back() != '}') {
      fprintf(stderr, "FAIL: payload for n=%u is not brace-terminated: %s\n", (unsigned)n, payload.c_str());
      return 1;
    }

    const bool hasTrunc = payload.find("\"trunc\":1") != std::string::npos;
    if (!hasTrunc) {
      lastFitting = payload;
      lastFittingCount = n;
    } else if (firstOverflowingCount < 0) {
      firstOverflowing = payload;
      firstOverflowingCount = n;
    }

    // Echo every payload (JSON-validated by the shell driver) so both the
    // "everything fit" and "it started truncating" regions of the sweep are
    // covered, not just the two boundary payloads picked out below.
    printf("%s\n", payload.c_str());
  }

  if (lastFittingCount < 0 || firstOverflowingCount < 0) {
    fprintf(stderr,
            "FAIL: sweep of 1..12 max-width entries never crossed the "
            "serialization-buffer boundary (lastFitting=%d firstOverflowing=%d) - "
            "adjust the test's entry width or count so it still exercises the bug\n",
            lastFittingCount, firstOverflowingCount);
    return 1;
  }

  // Boundary case: the largest entry count that still fit entirely within
  // the 700-byte buffer ("buffer effectively full", no truncation needed).
  if (!endsWith(lastFitting, "]}")) {
    fprintf(stderr, "FAIL: boundary-full payload (n=%d) did not end with ]}: %s\n",
            lastFittingCount, lastFitting.c_str());
    return 1;
  }
  if (lastFitting.find("\"trunc\":1") != std::string::npos) {
    fprintf(stderr, "FAIL: boundary-full payload (n=%d) unexpectedly marked truncated: %s\n",
            lastFittingCount, lastFitting.c_str());
    return 1;
  }

  // Overflow case: one more entry than the buffer can hold - forces
  // mid-batch truncation. Must still close cleanly and flag "trunc":1.
  if (!endsWith(firstOverflowing, "],\"trunc\":1}")) {
    fprintf(stderr, "FAIL: overflow payload (n=%d) did not end with ],\"trunc\":1}: %s\n",
            firstOverflowingCount, firstOverflowing.c_str());
    return 1;
  }

  // Also sweep to the full 12-slot accumulator capacity explicitly, since
  // that is the realistic worst case called out in the bug report (a long
  // connected/idle stretch filling every accumulator slot before any flush
  // point is reached).
  const std::string fullCapacityPayload = flushWithEntryCount(12);
  if (!endsWith(fullCapacityPayload, "],\"trunc\":1}") &&
      !endsWith(fullCapacityPayload, "]}")) {
    fprintf(stderr, "FAIL: full 12-entry payload has invalid closing: %s\n",
            fullCapacityPayload.c_str());
    return 1;
  }
  printf("%s\n", fullCapacityPayload.c_str());

  fprintf(stderr,
          "OK: boundary-full at n=%d (%zu bytes, no trunc), "
          "first-overflow at n=%d (%zu bytes, trunc:1)\n",
          lastFittingCount, lastFitting.size(),
          firstOverflowingCount, firstOverflowing.size());

  return 0;
}

// WO-2026-09-22-001: relocated occupancyStartTime future-date corruption
// clamp. Host-side logic mirror of
// currentStatusData::revalidateOccupancyStartTimeIfTimeAvailable()
// (src/MyPersistentData.cpp) - real hardware types (Clock, StorageHelperRK)
// make compiling that function directly impractical on the host, so this
// mirrors its decision logic exactly and the paired fidelity checks below
// (occupancy_start_time_clamp_test.sh) confirm the real function matches.
//
// The clamp itself is unchanged from the one removed by the
// RuntimeReportingPolicy Clock-routing dispatch (see git history for
// src/MyPersistentData.cpp) - only its call site moved, from a point in
// boot where a time source can never exist yet, to one where it can.
#include <cassert>
#include <cstdio>
#include <ctime>

namespace {

// Mirrors revalidateOccupancyStartTimeIfTimeAvailable()'s body exactly:
// no-op unless a time source exists and the record claims to be occupied;
// otherwise clamps a future-dated start to now. Returns the (possibly
// clamped) occupancyStartTime.
time_t mirrorRevalidate(bool timeValid, bool occupied, time_t start, time_t now) {
  if (!timeValid || !occupied) {
    return start;
  }
  if (start > now + 5) {
    return now;
  }
  return start;
}

void testNoTimeSourceIsNoOp() {
  // The exact condition this relocation exists to fix: at the OLD call
  // site, timeValid was always false, so this branch must leave a
  // corrupted future-dated start completely untouched (matching the OLD
  // behavior's inability to ever clamp there) - the fix is about WHERE this
  // runs, not changing what happens when it can't run yet.
  const time_t now = 1700000000;
  const time_t farFuture = now + 1000000;
  assert(mirrorRevalidate(/*timeValid=*/false, /*occupied=*/true, farFuture, now) == farFuture);
}

void testNotOccupiedIsNoOp() {
  const time_t now = 1700000000;
  const time_t farFuture = now + 1000000;
  assert(mirrorRevalidate(/*timeValid=*/true, /*occupied=*/false, farFuture, now) == farFuture);
}

void testPlausibleStartIsUnchanged() {
  const time_t now = 1700000000;
  assert(mirrorRevalidate(true, true, now - 3600, now) == now - 3600);
  // Exactly at the +5s tolerance boundary: not clamped.
  assert(mirrorRevalidate(true, true, now + 5, now) == now + 5);
}

void testFutureDatedStartIsClampedToNow() {
  const time_t now = 1700000000;
  // One second past the +5s tolerance: clamped.
  assert(mirrorRevalidate(true, true, now + 6, now) == now);
  const time_t farFuture = now + 1000000;
  assert(mirrorRevalidate(true, true, farFuture, now) == now);
}

} // namespace

int main() {
  testNoTimeSourceIsNoOp();
  testNotOccupiedIsNoOp();
  testPlausibleStartIsUnchanged();
  testFutureDatedStartIsClampedToNow();
  std::printf("occupancy_start_time_clamp_test: all assertions passed\n");
  return 0;
}

// Host test for Clock::openness()'s full truth table (WO-2026-09-19 Step 3b).
//
// Clock.cpp cannot be compiled standalone on the host (heavy Particle/
// MyPersistentData/LocalTimeCache/Cloud dependencies - see tests/README.md's
// established precedent). This test instead runs a hand-written MIRROR of
// openness()'s branching structure - not a reimplementation of what "open"
// means (that arithmetic is isWithinOpenHoursForHour(), already covered by
// this repo's other clock tests), but of the three-input decision itself:
// (trusted, configValid, openNow) -> Openness. tests/clock_openness_test.sh
// separately traces the REAL src/time/Clock.cpp to confirm the real
// function checks trust first, then config validity, then falls through to
// the same open/closed computation - so this mirror cannot silently drift
// from production without that fidelity check failing.
//
// The property under test, stated in the dispatch: {trusted, open} -> Open,
// {trusted, closed} -> Closed, {untrusted, *} -> Unknown - openness() must
// never fail open. "Untrusted" covers BOTH ways this function's inputs can
// be missing: the clock itself (Clock::isTrusted() false) and the open/
// close-hour configuration (Config::isValid(false) false) - both collapse
// to the same Unknown, deliberately, per Clock::openness()'s own doc
// comment ("must never fail open silently").

#include <cassert>
#include <cstdio>

namespace {

enum class Openness { Open, Closed, Unknown };

// Mirror of Clock::openness()'s branching structure (src/time/Clock.cpp).
Openness mirrorOpenness(bool trusted, bool configValid, bool openNow) {
  if (!trusted) {
    return Openness::Unknown;
  }
  if (!configValid) {
    return Openness::Unknown;
  }
  return openNow ? Openness::Open : Openness::Closed;
}

void testTrustedConfigValidOpenIsOpen() {
  assert(mirrorOpenness(true, true, true) == Openness::Open);
}

void testTrustedConfigValidClosedIsClosed() {
  assert(mirrorOpenness(true, true, false) == Openness::Closed);
}

void testUntrustedIsUnknownRegardlessOfConfigOrOpenNow() {
  assert(mirrorOpenness(false, true, true) == Openness::Unknown);
  assert(mirrorOpenness(false, true, false) == Openness::Unknown);
  assert(mirrorOpenness(false, false, true) == Openness::Unknown);
  assert(mirrorOpenness(false, false, false) == Openness::Unknown);
}

void testTrustedButConfigInvalidIsUnknownRegardlessOfOpenNow() {
  assert(mirrorOpenness(true, false, true) == Openness::Unknown);
  assert(mirrorOpenness(true, false, false) == Openness::Unknown);
}

// Sensitivity check: the mirror must not be vacuously true - Open and
// Closed must both be reachable, and distinctly, when trusted+configValid.
void testOpenAndClosedAreDistinctReachableOutcomes() {
  assert(mirrorOpenness(true, true, true) != mirrorOpenness(true, true, false));
}

} // namespace

int main() {
  testTrustedConfigValidOpenIsOpen();
  testTrustedConfigValidClosedIsClosed();
  testUntrustedIsUnknownRegardlessOfConfigOrOpenNow();
  testTrustedButConfigInvalidIsUnknownRegardlessOfOpenNow();
  testOpenAndClosedAreDistinctReachableOutcomes();

  printf("clock_openness_test: all assertions passed\n");
  return 0;
}

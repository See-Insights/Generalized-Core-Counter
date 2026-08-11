#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "state/SleepPrepSpanTiming.h"

namespace {

void testNormalSpan() {
	uint32_t spanStart = 0;
	maybeStartSleepPrepSpan(spanStart, 1000);
	assert(spanStart == 1000);
	const uint32_t elapsed = closeSleepPrepSpan(spanStart, 16000);
	assert(elapsed == 15000);
	assert(spanStart == 0);
}

void testSleepingStateSelfTransition() {
	// Mirrors SLEEPING_STATE -> SLEEPING_STATE self-transitions
	// (sleep-timer-occupied-suppress-report, sleep-pir-return-to-sleep):
	// two real spans back to back through the same zero-gate mechanism,
	// with no FSM state value involved at all in these pure functions.
	uint32_t spanStart = 0;

	// Cycle 1 starts at t=1 (nonzero, to avoid the zero-start/unset
	// ambiguity that maybeStartSleepPrepSpan()'s zero-gate relies on).
	maybeStartSleepPrepSpan(spanStart, 1);
	assert(spanStart == 1);
	const uint32_t cycle1Elapsed = closeSleepPrepSpan(spanStart, 15001); // 15s dwell
	assert(cycle1Elapsed == 15000);
	assert(spanStart == 0);

	// `state` never changed (SLEEPING_STATE -> SLEEPING_STATE), but a brand
	// new real span begins immediately - the zero-gate must start it fresh.
	maybeStartSleepPrepSpan(spanStart, 15001); // cycle 2 starts where cycle 1 ended
	assert(spanStart == 15001);
	const uint32_t cycle2Elapsed = closeSleepPrepSpan(spanStart, 15001 + 8000); // 8s dwell
	assert(cycle2Elapsed == 8000);
	assert(spanStart == 0);

	assert(cycle1Elapsed != cycle2Elapsed);
	assert(cycle2Elapsed != 0);
}

void testHibernatePoisoningRegression() {
	// Before the fix: a span starts (nonzero timestamp), then an abandoned
	// call stack (HIBERNATE success / System.reset()) never calls
	// closeSleepPrepSpan(). Without resetSleepPrepSpanOnBoot() at boot, the
	// stale nonzero value blocks maybeStartSleepPrepSpan() from
	// re-initializing on the next real SLEEPING_STATE dwell.
	uint32_t spanStart = 0;
	maybeStartSleepPrepSpan(spanStart, 5000); // span starts, then "reset" happens - never closed
	assert(spanStart == 5000);

	const uint32_t staleValue = spanStart;
	// Simulate the next dwell's attempt to start a fresh span WITHOUT first
	// calling resetSleepPrepSpanOnBoot() - this proves the bug would occur.
	maybeStartSleepPrepSpan(spanStart, 90000);
	assert(spanStart == staleValue); // unchanged: the zero-gate blocked re-init, exactly the bug

	// Now apply the fix.
	resetSleepPrepSpanOnBoot(spanStart);
	assert(spanStart == 0);

	// A subsequent maybeStartSleepPrepSpan() call correctly starts a fresh
	// span from the new nowMs.
	maybeStartSleepPrepSpan(spanStart, 90000);
	assert(spanStart == 90000);
	const uint32_t freshElapsed = closeSleepPrepSpan(spanStart, 90000 + 42);
	assert(freshElapsed == 42);
	assert(spanStart == 0);
}

} // namespace

int main() {
	testNormalSpan();
	testSleepingStateSelfTransition();
	testHibernatePoisoningRegression();
	printf("All SleepPrepSpanTiming tests passed\n");
	return 0;
}

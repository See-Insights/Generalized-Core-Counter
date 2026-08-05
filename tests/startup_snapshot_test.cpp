#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "observability/StartupSnapshot.h"

using Observability::StartupSnapshot;
using Observability::StartupSnapshotInputs;
using Observability::StartupSnapshotStore;

namespace {

StartupSnapshotInputs inputsFor(int resetReason,
		time_t epoch,
		uint32_t uptimeSec,
		uint32_t resetCount) {
	StartupSnapshotInputs inputs;
	inputs.resetReason = resetReason;
	inputs.firmwareVersion = "20.0-dev";
	inputs.deviceOsVersion = "6.4.1";
	inputs.resetCount = resetCount;
	inputs.completionEpoch = epoch;
	inputs.completionUptimeSec = uptimeSec;
	return inputs;
}

void testColdBootSnapshot() {
	StartupSnapshotStore store;
	store.capture(inputsFor(40, 1784000000, 4, 1));
	const StartupSnapshot &snapshot = store.forStatus(1784003600, 3604, true);
	assert(snapshot.captured);
	assert(snapshot.epoch == 1784000000);
	assert(strcmp(snapshot.reason, "power-down") == 0);
	assert(strcmp(snapshot.firmware, "20.0-dev") == 0);
	assert(strcmp(snapshot.deviceOS, "6.4.1") == 0);
	assert(snapshot.resetCount == 1);
}

void testWatchdogAndOtaReasons() {
	StartupSnapshotStore watchdogStore;
	watchdogStore.capture(inputsFor(60, 1784000000, 5, 7));
	assert(strcmp(watchdogStore.snapshot().reason, "watchdog") == 0);

	StartupSnapshotStore otaStore;
	otaStore.capture(inputsFor(70, 1784000100, 6, 7));
	assert(strcmp(otaStore.snapshot().reason, "update") == 0);
}

void testNormalStatusUpdatesPreserveStartup() {
	StartupSnapshotStore store;
	store.capture(inputsFor(20, 1784000000, 3, 2));
	const StartupSnapshot first = store.forStatus(1784003600, 3603, true);
	int changingRuntimeValue = 10;
	changingRuntimeValue = 99;
	const StartupSnapshot second = store.forStatus(1784007200, 7203, true);
	assert(changingRuntimeValue == 99);
	assert(first.epoch == second.epoch);
	assert(strcmp(first.reason, second.reason) == 0);
	assert(strcmp(first.firmware, second.firmware) == 0);
	assert(strcmp(first.deviceOS, second.deviceOS) == 0);
	assert(first.resetCount == second.resetCount);
}

void testRebootReplacesSnapshot() {
	StartupSnapshotStore store;
	store.capture(inputsFor(20, 1784000000, 3, 2));
	const time_t firstEpoch = store.snapshot().epoch;
	store.capture(inputsFor(60, 1785000000, 5, 3));
	assert(store.snapshot().epoch != firstEpoch);
	assert(store.snapshot().epoch == 1785000000);
	assert(strcmp(store.snapshot().reason, "watchdog") == 0);
	assert(store.snapshot().resetCount == 3);
}

void testDeferredEpochFinalizesOnce() {
	StartupSnapshotStore store;
	store.capture(inputsFor(20, 0, 5, 2));
	assert(store.forStatus(0, 8, false).epoch == 0);
	const StartupSnapshot firstValid = store.forStatus(1784000100, 105, true);
	assert(firstValid.epoch == 1784000000);
	assert(store.forStatus(1784007200, 7205, true).epoch == firstValid.epoch);
}

} // namespace

int main() {
	testColdBootSnapshot();
	testWatchdogAndOtaReasons();
	testNormalStatusUpdatesPreserveStartup();
	testRebootReplacesSnapshot();
	testDeferredEpochFinalizesOnce();
	puts("startup_snapshot_test: PASS");
	return 0;
}

#include "observability/StartupSnapshot.h"

#include <stdio.h>

namespace {

void copyText(char *destination, size_t destinationSize, const char *source) {
	if (!source) {
		source = "";
	}
	snprintf(destination, destinationSize, "%s", source);
}

} // namespace

namespace Observability {

const char *resetReasonName(int resetReason) {
	switch (resetReason) {
		case 0:
			return "none";
		case 20:
			return "pin-reset";
		case 30:
			return "power-management";
		case 40:
			return "power-down";
		case 50:
			return "brownout";
		case 60:
			return "watchdog";
		case 70:
			return "update";
		case 80:
			return "update-error";
		case 90:
			return "update-timeout";
		case 100:
			return "factory-reset";
		case 110:
			return "safe-mode";
		case 120:
			return "dfu-mode";
		case 130:
			return "panic";
		case 140:
			return "user";
		case 150:
			return "config-update";
		case 10:
		default:
			return "unknown";
	}
}

void StartupSnapshotStore::capture(const StartupSnapshotInputs &inputs) {
	snapshot_ = StartupSnapshot();
	snapshot_.epoch = inputs.completionEpoch;
	copyText(snapshot_.reason, sizeof(snapshot_.reason), resetReasonName(inputs.resetReason));
	copyText(snapshot_.firmware, sizeof(snapshot_.firmware), inputs.firmwareVersion);
	copyText(snapshot_.deviceOS, sizeof(snapshot_.deviceOS), inputs.deviceOsVersion);
	snapshot_.resetCount = inputs.resetCount;
	snapshot_.captured = true;
	completionUptimeSec_ = inputs.completionUptimeSec;
}

const StartupSnapshot &StartupSnapshotStore::forStatus(time_t nowEpoch,
		uint32_t currentUptimeSec,
		bool timeValid) {
	if (snapshot_.captured && snapshot_.epoch <= 0 && timeValid && nowEpoch > 0) {
		const uint32_t elapsedSec = currentUptimeSec >= completionUptimeSec_
			? currentUptimeSec - completionUptimeSec_
			: 0;
		snapshot_.epoch = nowEpoch > (time_t)elapsedSec
			? nowEpoch - (time_t)elapsedSec
			: nowEpoch;
	}
	return snapshot_;
}

const StartupSnapshot &StartupSnapshotStore::snapshot() const {
	return snapshot_;
}

StartupSnapshotStore &startupSnapshotStore() {
	static StartupSnapshotStore store;
	return store;
}

} // namespace Observability

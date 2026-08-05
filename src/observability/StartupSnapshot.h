#pragma once

#include <stdint.h>
#include <time.h>

namespace Observability {

struct StartupSnapshotInputs {
	int resetReason = 0;
	const char *firmwareVersion = "";
	const char *deviceOsVersion = "";
	uint32_t resetCount = 0;
	time_t completionEpoch = 0;
	uint32_t completionUptimeSec = 0;
};

struct StartupSnapshot {
	time_t epoch = 0;
	char reason[24] = "unknown";
	char firmware[32] = "";
	char deviceOS[24] = "";
	uint32_t resetCount = 0;
	bool captured = false;
};

/**
 * @brief Boot-scoped store for the immutable initialization snapshot.
 */
class StartupSnapshotStore {
public:
	void capture(const StartupSnapshotInputs &inputs);
	const StartupSnapshot &forStatus(time_t nowEpoch,
		uint32_t currentUptimeSec,
		bool timeValid);
	const StartupSnapshot &snapshot() const;

private:
	StartupSnapshot snapshot_;
	uint32_t completionUptimeSec_ = 0;
};

const char *resetReasonName(int resetReason);
StartupSnapshotStore &startupSnapshotStore();

} // namespace Observability

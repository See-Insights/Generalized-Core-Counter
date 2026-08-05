#include "observability/StartupSnapshotRuntime.h"

#include "Particle.h"
#include "../MyPersistentData.h"
#include "../FirmwareVersion.h"

namespace Observability {

void captureSuccessfulInitialization() {
	const String deviceOsVersion = System.version();
	StartupSnapshotInputs inputs;
	inputs.resetReason = System.resetReason();
	inputs.firmwareVersion = FIRMWARE_VERSION;
	inputs.deviceOsVersion = deviceOsVersion.c_str();
	inputs.resetCount = sysStatus.get_resetCount();
	inputs.completionEpoch = Time.isValid() ? Time.now() : 0;
	inputs.completionUptimeSec = millis() / 1000UL;
	startupSnapshotStore().capture(inputs);
}

const StartupSnapshot &currentStartupSnapshot() {
	return startupSnapshotStore().forStatus(
		Time.now(), millis() / 1000UL, Time.isValid());
}

} // namespace Observability

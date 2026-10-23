#pragma once
namespace SleepTeardown {
enum class Result { WAIT, READY, ERROR };
void enter();  void guardLostTimestamp();
bool disconnectRequested();
Result run(bool useNetworkStandbyRequested, bool useNetworkStandbyEffective, bool &forceModemOffAfterStandbyFailure, int &standbySleepFailureError);
}

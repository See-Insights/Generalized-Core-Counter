#pragma once

#include <stdint.h>

namespace BatteryAuthorityPolicy {

constexpr float POST_CONNECT_DELTA_THRESHOLD = 20.0f;
constexpr uint8_t STALE_SOC_TRIGGER_CONSECUTIVE_COUNT = 2;
constexpr uint8_t STALE_SOC_RESYNC_COOLDOWN_WAKE_CYCLES = 3;

enum class ChargeStatus : uint8_t {
  NotCharging = 0,
  Pre = 1,
  Fast = 2,
  Done = 3,
};

struct StaleSocSample {
  float soc;
  float vcell;
  uint8_t chargeStatus;
  uint8_t vbusStatus;
  bool powerGood;
  uint8_t faultReg;
};

class ResyncActions {
public:
  virtual ~ResyncActions() = default;
  virtual void quickStart() = 0;
  virtual void settle() = 0;
  virtual void readSample(float &soc, float &vcell) = 0;
  virtual void commitSoc(float soc) = 0;
};

struct SocCommitResolution {
  bool shouldCommit;
  bool initialSampleStale;
  bool resyncAttempted;
  bool settledSampleStale;
  float soc;
  float vcell;
};

float batterySampleDelta(float authoritativeSoc, float soc);
bool batterySampleHasUnrealisticDelta(float authoritativeSoc, float soc);
bool batterySampleHasUncorroboratedIncrease(float authoritativeSoc, float soc, float vcell);
bool shouldEvaluatePostConnectDelta(bool shouldStabilize,
                                    bool authoritativeSampleActive,
                                    bool cloudConnected,
                                    bool radioPoweredOn);
bool staleSocConditionsMet(const StaleSocSample &sample);
bool quietForFuelGaugeResync(bool radioPoweredOn, uint8_t chargeStatus);
bool shouldResyncFuelGauge(bool staleSocConditionsMet,
                           uint8_t consecutiveCount,
                           uint8_t wakeCyclesSinceResync,
                           bool radioPoweredOn,
                           uint8_t chargeStatus);
SocCommitResolution resolveSocCommit(const StaleSocSample &sample,
                                     bool rejectAuthoritativeOverwrite,
                                     uint8_t consecutiveCount,
                                     uint8_t wakeCyclesSinceResync,
                                     bool radioPoweredOn,
                                     ResyncActions &actions);
bool stuckChargingHasMeaningfulProgress(float baselineSoc,
                                        float acceptedSoc,
                                        float baselineVcell,
                                        float vcell);
uint8_t noteWakeCycle(uint8_t wakeCyclesSinceResync);

} // namespace BatteryAuthorityPolicy
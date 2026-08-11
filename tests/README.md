# Host regression tests

The reporting-policy calculation is isolated from Particle Device OS so its
battery tiers, effective intervals, boundary alignment, and open-window filter
can be checked with the host compiler:

```sh
c++ -std=c++11 -Wall -Wextra -Werror -Isrc \
  tests/reporting_policy_test.cpp src/reporting/ReportingPolicy.cpp \
  -o /tmp/reporting_policy_test
/tmp/reporting_policy_test
```

The runtime adapter is validated by the normal Particle firmware compile.

The boot-scoped startup snapshot has a separate host test:

```sh
c++ -std=c++11 -Wall -Wextra -Werror -Isrc \
  tests/startup_snapshot_test.cpp src/observability/StartupSnapshot.cpp \
  -o /tmp/startup_snapshot_test
/tmp/startup_snapshot_test
```

## WO-2026-08-11-001 (SLEEP_PREP threshold exclusion) host test

`noteLoopStageDuration()` (`src/Generalized-Core-Counter.cpp`) pulls in heavy
Particle/PublishQueuePosix dependencies and can't be compiled standalone on
the host. This test parses the actual shipped source directly to confirm
`LOOP_STAGE_SLEEP_PREP` is excluded from the
`kLoopStageWarnThresholdMs`/`kLoopStageErrorThresholdMs`
WARN/ERROR escalation while still logging at `INFO`, that the thresholds and
other stages' escalation behavior are unchanged, and runs simulated scenarios
matching the work order's required tests:

```sh
python3 tests/loop_stage_sleep_prep_exclusion_test.py
```

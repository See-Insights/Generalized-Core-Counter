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

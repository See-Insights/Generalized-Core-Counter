The main clutter is **five unused named entries, two switches that only change telemetry, split version metadata, and logging configuration outside the build profile**. Most remaining constants have real consumers and should stay with their owners.

Read-only inspection completed at `a6a283cdf3ae5894bb7d2d3084cf133361110481`, branch `wo/2026-09-29-001-v27-small-fixes`. No edits, commits, builds, or tests. Working tree remains unchanged apart from the supplied dispatch file. `lib/` and `tests/` were excluded.

Model: **gpt-6-astra**. Reasoning: **high**.

The workflow is at [AI_DEVELOPMENT_WORKFLOW.md](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/AI_DEVELOPMENT_WORKFLOW.md), not `docs/AI_DEVELOPMENT_WORKFLOW.md`.

**Scope and notation**

The tables cover the requested named-setting searches, including all `constexpr`/`static const` data declarations found, plus related hardware overrides, plain `const` settings, and runtime configuration paths. Function-local constants are included too. Include guards, ordinary runtime state, and function declarations matching `static const` were excluded.

All source paths below are relative to `src/`. `Main` means `Generalized-Core-Counter.cpp`; other filenames are unambiguous within the inventory.

Status labels overlap:

- **L — live:** a source consumer exists and changing the value affects behavior or reported data.
- **D — dead:** no meaningful consumer in the inspected source.
- **N — never changed across the inspected build definitions:** the declaration has a fixed initializer and no supported build override. This does **not** mean it never changed historically, or should be deleted.
- **B — bench-only:** used by an explicitly enabled bench path; included in the live count.

“Read” gives representative consumers, not every occurrence. Declarations, comments, and a switch’s own validation checks do not establish liveness. No binary-presence claims are made.

The named-entry tally is **219**; arrays count once, conditional alternative definitions count once, and independently defined duplicates count separately.

| Kind | Count |
|---|---:|
| Build switches and log filters | 31 |
| Tuning constants/defaults | 111 |
| Firmware version identity | 3 |
| Other: schemas, codes, masks, pins, paths, metadata | 74 |
| **Total** | **219** |

| Status | Count |
|---|---:|
| Live, including conditional/bench consumers | 214 |
| Dead | 5 |
| Never changed across inspected build definitions | 187 |
| Bench-only subset | 22 |

The runtime Ledger fields are mapped separately below; they are not added to the compile-time tally.

The requested starting files are:

| File | What it actually holds |
|---|---|
| `Version.h` | Duplicate declarations of the firmware string and release notes. |
| `Version.cpp` | Definitions of the firmware string and release notes. |
| `FirmwareVersion.h` | Product version integer plus duplicate string/notes declarations. |
| `Settings.h` | Documentation catalog and includes of `BuildProfile.h`/`ProjectConfig.h`; no setting definitions itself. The filename is capitalized. |
| `BuildProfile.h` | Sixteen overrideable build macros, validation checks, and conditional `DEBUG_SERIAL` definition. |
| `cloud/Particle_Functions.h` | Singleton interface; no configuration values. |
| `cloud/Particle_Functions.cpp` | System mode, reset-info feature, hard-coded serial log level, eight category filters, and diagnostic variable registration. |
| `Config.h` | Eight live factory defaults plus runtime configuration validation/provenance API; **not merely a wrapper**. |
| `Config.cpp` | Runtime validation, fallback selection, provenance, and configuration diagnostics. |
| `ProjectConfig.h` | Unused legacy webhook-name helper. |

Other configuration-bearing files and owners found in `src/`:

| File | Configuration held |
|---|---|
| `Generalized-Core-Counter.cpp` | Watchdog/forensic timing, reset delay, startup/queue configuration, serial behavior, and a build-flags witness. |
| `power/ConnectivityPolicy.h` | Connection, teardown, Ledger, failsafe, webhook, heap, battery-wake, and serial timing constants. |
| `state/State_Connect.cpp` | Modem instability threshold, connection diagnostics/recovery timing, OTA timeout alias. |
| `state/State_Sleep.cpp` | Sleep/teardown thresholds, hibernate limits, Wi-Fi shutdown timing, diagnostic masks/register identifiers. |
| `state/State_Report.cpp` | Reconnect deferral after modem instability. |
| `ThrashGuard.h`, `ThrashGuard.cpp` | Log cooldown, no-progress timeouts, backoff, escalation windows/count. |
| `time/ClockTrust.h` | Clock freshness/retry thresholds and unavailable-age sentinel. |
| `time/Clock.cpp` | Accepted epoch range. |
| `time/RtcSkewTest.h` | Bench RTC skew and fallback-anchor constants. |
| `power/PowerPlatform.cpp` | USB/solar power profiles and power-source codes. |
| `power/PowerManager.cpp` | FIELD/DEV fallback-profile selection and duplicated power-source codes. |
| `power/PowerDiagnostics.cpp` | Duplicated power-source codes, diagnostic reason codes, batch capacity/reserve. |
| `power/PmicFaultMonitor.cpp` | Fault masks, summary interval, remediation cooldown. |
| `power/PowerTier.h` | Voltage and SOC thresholds. |
| `power/BatteryHealth.cpp` | OCV curve and residual/bias thresholds. |
| `power/BatteryAuthorityPolicy.h`, `.cpp` | SOC-change threshold and inline battery-validation/progress thresholds. |
| `power/ChargeInhibitPolicy.h` | Four thermal defaults, safety ceiling, validation/hysteresis. |
| `power/BatteryAuthorityCommand.cpp` | Battery-tier display labels. |
| `cloud/BatteryBackoffPolicy.h` | Literal SOC hysteresis boundaries and reporting multipliers. |
| `cloud/BatteryBackoff.cpp` | Legacy connection-duration backoff rules; helper currently has no source caller. |
| `reporting/ReportingPolicy.cpp` | Future-boundary search limit. |
| `cloud/Cloud.h`, `.cpp` | Status payload/tracker capacities and runtime webhook-name fallback. |
| `cloud/DeviceStatusPublisher.cpp` | Ledger schema/data capacity, duplicated build-flags witness, configuration fingerprint. |
| `cloud/ConfigApply.cpp` | Ledger merge precedence, accepted fields, validation limits, application to storage. |
| `cloud/LedgerClient.cpp` | Four Ledger names and input synchronization behavior. |
| `MyPersistentData.h`, `.cpp` | Storage schemas, paths, save delays, factory initialization, thermal fallback. |
| `persist/SystemConfig.h` | Runtime configuration facade, mode identifiers, persisted-string capacities. |
| `persist/PowerConfig.h` | Runtime power/thermal configuration facade. |
| `device_pinout.h`, `.cpp` | Pin declarations, platform mappings, TMP36 override. |
| `sensors/SensorManager.cpp` | PMIC anomaly constants, sensor hardware overrides, temperature defaults, battery labels. |
| `sensors/SensorDefinitions.h` | Sensor metadata/default LED table. |

`cloud/ConfigMerge.cpp` contains only `#include "cloud/Cloud.h"`: it holds no merge implementation or settings. `diagnostics/ConnectivityFailsafeTest.*` consumes bench settings rather than defining another profile.

The build-switch inventory follows. Every value in `BuildProfile.h` except `DEBUG_SERIAL` is protected by `#ifndef`; command-line `-D` can override it. An override being possible is distinguished from an actual configured build using it.

| Name | Definition/current default | Read | Alternate build evidence | Kind; status |
|---|---|---|---|---|---|
| `DEV_BUILD` | `BuildProfile.h:62` = `0` | `BuildProfile.h:69,254`; Main:1478 | Documented `-DDEV_BUILD=1`; no dedicated task | Switch; L |
| `FIELD_BUILD` | `BuildProfile.h:69` = `(!DEV_BUILD)` | `MyPersistentData.cpp:137`; `power/PowerManager.cpp:19` | Derived or independently overrideable | Switch; L |
| `ALLOW_BLOCKING_SERIAL_WAITS` | `BuildProfile.h:79` = `0` | Main:887,1030; `State_Sleep.cpp:1492` | Documented `-D…=1` | Switch; L,B |
| `CONNECTIVITY_FAILSAFE_TEST_MODE` | `BuildProfile.h:93` = `0` | `ConnectivityPolicy.h:105`; Main:385,2553; `State_Sleep.cpp:993` | `.vscode/tasks.json:69,81,90,102` sets `1` | Switch; L,B |
| `ENABLE_PMIC_FORENSICS` | `BuildProfile.h:109` = `1` | `SensorManager.cpp:44,70,745,772`; Main:2176,2223 | Documented comparison override `0` | Switch; L |
| `ENABLE_LEDGER_TRACE` | `BuildProfile.h:117` = `0` | `Cloud.cpp:98`; `LedgerClient.cpp:128`; `ConfigApply.cpp:158`; `DeviceStatusPublisher.cpp:159` | Documented `1`; no dedicated task | Switch; L |
| `ENABLE_CONNECT_TRACE` | `BuildProfile.h:125` = `0` | `State_Connect.cpp:297,357,436` | Documented `1`; no dedicated task | Switch; L |
| `ENABLE_CONNECT_DECISION_TRACE` | `BuildProfile.h:133` = `0` | `State_Connect.cpp:277`; `State_Report.cpp:213,229` | Documented `1`; no dedicated task | Switch; L |
| `ENABLE_PERF_TRACE` | `BuildProfile.h:141` = `0` | Main:2065,2116 | Documented `1`; no dedicated task | Switch; L |
| `ENABLE_GATE_TRACE` | `BuildProfile.h:149` = `0` | `State_Sleep.cpp:485,540,573` | Documented `1`; no dedicated task | Switch; L |
| `ENABLE_SLEEP_TRACE` | `BuildProfile.h:157` = `0` | Main:923,1769; `State_Sleep.cpp:1341`; `SensorManager.cpp:695` | Documented `1`; no dedicated task | Switch; L |
| `ENABLE_PMIC_TRACE` | `BuildProfile.h:165` = `0` | Main:1488; `DeviceStatusPublisher.cpp:230` **only** | Documented `1`, but only changes witness bit | Switch; L, metadata only |
| `ENABLE_CONFIG_TRACE` | `BuildProfile.h:173` = `0` | `Config.cpp:46,180`; `Cloud.cpp:564,608` | Documented `1`; no dedicated task | Switch; L |
| `ENABLE_PMIC_CHARGE_CYCLE_TEST` | `BuildProfile.h:195` = `0` | Main:1490; `DeviceStatusPublisher.cpp:232` **only** | Documented `1`; advertised test body is absent | Switch; L, metadata only |
| `ENABLE_RTC_SKEW_TEST` | `BuildProfile.h:225` = `0` | Main:51,505,1166 | Documented bench override; Boron additionally required | Switch; L,B |
| `ENABLE_DIAGNOSTICS_PUBLISH_MODE` | `BuildProfile.h:246` = `0` | `PowerDiagnostics.cpp:25,358,372`; `PmicFaultMonitor.cpp:416`; `State_Sleep.cpp:1120,1488` | Recorded bench `EXTRA_CFLAGS="-DENABLE_DIAGNOSTICS_PUBLISH_MODE=1"` | Switch; L,B |
| `DEBUG_SERIAL` | `BuildProfile.h:255`, defined empty only when DEV | **Never**, except its own definition guard | DEV condition defines it; no functional consumer | Switch; D |
| `SERIAL_LOG_LEVEL` | `Particle_Functions.cpp:12` = `3` | Same file:15–33 selects handler | **Unconditional definition**; command-line override is not honored cleanly | Switch; L,N |

The PMIC trace and charge-cycle flags are deliberately **not counted as dead**: changing them changes reported metadata. They are nevertheless misleading controls because their advertised feature consumers are gone.

Log filters are eight additional settings:

| Category | Definition/current value | Read | Build variation | Kind; status |
|---|---|---|---|---|
| `mux` | `Particle_Functions.cpp:24` = WARN | Handler construction:22 | Only included in level-3 branch | Switch/filter; L,N |
| `system.nm` | Same:25 = WARN | Same:22 | Same | Switch/filter; L,N |
| `system` | Same:26 = WARN | Same:22 | Same | Switch/filter; L,N |
| `comm.dtls` | Same:27 = WARN | Same:22 | Same | Switch/filter; L,N |
| `comm.protocol` | Same:28 = WARN | Same:22 | Same | Switch/filter; L,N |
| `comm.protocol.handshake` | Same:29 = WARN | Same:22 | Same | Switch/filter; L,N |
| `net.pppncp` | Same:30 = WARN | Same:22 | Same | Switch/filter; L,N |
| `app.ab1805` | Same:31 = WARN | Same:22 | Same | Switch/filter; L,N |

The handler mapping is `0→NONE`, `1→ERROR`, `2→WARN`, `3→INFO with filters`, `4→ALL`. There is no final `#else`/validation for an unsupported level. Only one handler declaration is selected; these are not five concurrent handlers.

Hardware overrides are documented in `Settings.h` but consumed elsewhere:

| Name | Current default/location | Read | Build variation | Kind; status |
|---|---|---|---|---|
| `MUON_TMP36_SENSE_PIN` | Undefined; fallback at `device_pinout.cpp:34–39` | Same:35 | External `-D`; otherwise S4 on P2, A4 elsewhere | Switch; L |
| `MUON_HAS_TMP36` | Undefined; `SensorManager.cpp:1014` | Same condition | Defining it enables the analog path on P2/Photon2 | Switch; L |
| `MUON_HAS_TMP112` | Undefined; `SensorManager.cpp:996` | Same:996–998 | Defining it forces TMP112 presence | Switch; L |
| `DISABLE_TMP112_AUTODETECT` | Undefined; `SensorManager.cpp:983` | Same condition | Defining it disables probing | Switch; L |
| `MUON_TMP112_I2C_ADDR` | Undefined; fallback `0x48` at `SensorManager.cpp:430,977` | Same:427–430,974–977 | External address override | Switch; L |

No inspected build task sets these hardware overrides. Presence-tested flags treat `-DMUON_HAS_TMP112=0`, for example, as **defined**, so that spelling still enables the path.

Two fixed system settings also live in `Particle_Functions.cpp`:

| Setting | Definition/value | Read | Build variation | Kind; status |
|---|---|---|---|---|
| System mode | `Particle_Functions.cpp:8` = `SEMI_AUTOMATIC` | Device OS registration macro | None in source | Other; L,N |
| Reset information | Same:10 = enable `FEATURE_RESET_INFO` | Device OS startup registration | None in source | Other; L,N |

Two names are documentation remnants rather than current settings: `ENABLE_PMIC_REGISTER_DUMP` appears only in the usage example at `BuildProfile.h:29`; `ENABLE_BORON_USB_SOURCE_OVERRIDE` appears in historical explanations. Neither has a current definition or feature guard. They are excluded from the tally.

The version and legacy project settings are:

| Name | Definition/current value | Read | Build variation | Kind; status |
|---|---|---|---|---|
| `FIRMWARE_PRODUCT_VERSION` | `FirmwareVersion.h:28` = `27` | Main:39, `PRODUCT_VERSION(...)` | No profile override; release script edits source | Version; L,N |
| `FIRMWARE_VERSION` | `Version.cpp:6` = `"v27-SmallFixes"` | Main:1507,2226,2255; `DeviceStatusPublisher.cpp:212`; `StartupSnapshotRuntime.cpp:13` | No profile override; release script edits source | Version; L,N |
| `FIRMWARE_RELEASE_NOTES` | `Version.cpp:7`; exact text below | **Never**; remaining occurrences are declarations/comments | Release script edits source | Version; D,N |
| `ProjectConfig::webhookEventName()` | `ProjectConfig.h:31–32` = `"Ubidots-Counter-Hook-v1"` | **Never** | No override | Other; D,N |

Current release-notes value:

> Small known fixes: closing report connects at close, only our webhook reply clears the wait, status payload overflow guard, live local time in TimeDiag, cloud builds use the vendored libraries

For the remaining tables, **“fixed” means no build override found**. Conditional availability is called out separately. Fixed defaults may still feed runtime values that Ledger later replaces.

The eight defaults in `Config.h` are all live:

| Name | Definition/value | Read | Build variation | Kind; status |
|---|---|---|---|---|
| `DEFAULT_TIMEZONE` | `Config.h:30` = `"UTC0"` | `MyPersistentData.cpp:139`; Main:1401; `DeviceStatusPublisher.cpp:661` | Fixed | Tuning; L,N |
| `DEFAULT_OPEN_HOUR` | Same:31 = `6` | `MyPersistentData.cpp:141`; `DeviceStatusPublisher.cpp:662` | Fixed | Tuning; L,N |
| `DEFAULT_CLOSE_HOUR` | Same:32 = `22` | `MyPersistentData.cpp:142`; `DeviceStatusPublisher.cpp:663` | Fixed | Tuning; L,N |
| `DEFAULT_REPORT_INTERVAL_SEC` | Same:33 = `3600` | `Config.cpp:146,150`; `MyPersistentData.cpp:143`; `Clock.cpp:554`; `State_Sleep.cpp:981` | Fixed | Tuning; L,N |
| `DEFAULT_OCCUPANCY_DEBOUNCE_MS` | Same:34 = `60000` | `Config.cpp:155,159`; `MyPersistentData.cpp:606`; `DeviceStatusPublisher.cpp:660` | Fixed | Tuning; L,N |
| `DEFAULT_CONNECT_ATTEMPT_BUDGET_SEC` | Same:35 = `300` | `MyPersistentData.cpp:154` | Fixed | Tuning; L,N |
| `DEFAULT_CLOUD_DISCONNECT_BUDGET_SEC` | Same:36 = `15` | `MyPersistentData.cpp:155` | Fixed | Tuning; L,N |
| `DEFAULT_MODEM_OFF_BUDGET_SEC` | Same:37 = `30` | `MyPersistentData.cpp:156` | Fixed | Tuning; L,N |

`power/ConnectivityPolicy.h` contains the largest collection:

| Name | Definition/value | Read | Build variation | Kind; status |
|---|---|---|---|---|
| `CONNECT_BUDGET_DEFAULT_MS` | :62 = `300000` | `State_Connect.cpp:97,114,219` | Fixed | Tuning; L,N |
| `CONNECT_BUDGET_DEEP_MS` | :63 = `660000` | `State_Connect.cpp:123`; `State_Sleep.cpp:1212` | Fixed | Tuning; L,N |
| `CONNECT_BUDGET_DEFAULT_SEC` | :64 = `300` | **Never** | Fixed | Tuning; D,N |
| `CONNECT_BUDGET_CONFIG_MIN_SEC` | :65 = `120` | `State_Connect.cpp:108,112` | Fixed | Tuning; L,N |
| `CONNECT_BUDGET_CONFIG_MAX_SEC` | :66 = `900` | `State_Connect.cpp:109` | Fixed | Tuning; L,N |
| `DEEP_ATTEMPT_COUNTER_THRESHOLD` | :81 = `3` | `State_Connect.cpp:120,276,555` | Fixed | Tuning; L,N |
| `DEEP_ATTEMPT_SOC_THRESHOLD` | :82 = `50%` | `State_Connect.cpp:121` | Fixed | Tuning; L,N |
| `FIRMWARE_UPDATE_MAX_MS` | :95 = `300000` | `State_Connect.cpp:188`; `State_Sleep.cpp:1213` | Fixed | Tuning; L,N |
| `CONNECTIVITY_FAILSAFE_STALE_SEC` | :111 = `43200` | Main:2602; `ConnectivityFailsafeTest.cpp:162` | Test mode: :106 = `300` | Tuning; L |
| `CONNECTIVITY_FAILSAFE_COOLDOWN_SEC` | :112 = `21600` | Main:2620; `ConnectivityFailsafeTest.cpp:169` | Test mode: :107 = `900` | Tuning; L |
| `CONNECTIVITY_FAILSAFE_JITTER_MAX_SEC` | :113 = `1800` | Main:388,398; `ConnectivityFailsafeTest.cpp:171` | Test mode: :108 = `0` | Tuning; L |
| `CONNECTIVITY_FAILSAFE_TEST_MAX_CLOSED_SLEEP_SEC` | :114 = `0` | `State_Sleep.cpp:1000`; `ConnectivityFailsafeTest.cpp:197` | Test mode: :109 = `60`; consumers bench-gated | Tuning; L,B |
| `CONNECTIVITY_FAILSAFE_ALERT` | :116 = `45` | Main:2519,2653 | Fixed | Other; L,N |
| `CLOUD_OPS_GATE_TIMEOUT_MS` | :133 = `30000` | `State_Sleep.cpp:148` | Fixed | Tuning; L,N |
| `CLOUD_OPS_STATUS_LOG_INTERVAL_MS` | :134 = `5000` | `State_Sleep.cpp:572` | Fixed; trace path | Tuning; L,N |
| `OUTPUT_LEDGER_SYNC_TIMEOUT_MS` | :149 = `70000` cellular | `State_Sleep.cpp:500,501,1214` | :151 = `30000` non-cellular | Tuning; L |
| `LEDGER_SYNC_TIMEOUT_MS` | :167 = `10000` cellular | `LedgerClient.cpp:126,132,156` | :169 = `5000` non-cellular | Tuning; L |
| `DISCONNECT_BUDGET_MIN_SEC` | :192 = `5` | `State_Sleep.cpp:707,713` | Fixed | Tuning; L,N |
| `DISCONNECT_BUDGET_MAX_SEC` | :193 = `120` | `State_Sleep.cpp:708,714` | Fixed | Tuning; L,N |
| `DISCONNECT_CLOUD_DEFAULT_SEC` | :194 = `15` | `State_Sleep.cpp:709` | Fixed | Tuning; L,N |
| `DISCONNECT_MODEM_DEFAULT_SEC` | :195 = `30` | `State_Sleep.cpp:715,1215` | Fixed | Tuning; L,N |
| `DISCONNECT_STANDBY_MAX_MS` | :196 = `5000` | `State_Sleep.cpp:724,725` | Fixed | Tuning; L,N |
| `BATTERY_WAKE_QUICKSTART_DELAY_MS` | :204 = `500` | `SensorManager.cpp:587,589` | Fixed | Tuning; L,N |
| `BATTERY_WAKE_RETRY_DELAY_MS` | :205 = `200` | `SensorManager.cpp:597` | Fixed | Tuning; L,N |
| `BATTERY_WAKE_MAX_RETRIES` | :206 = `2` | `SensorManager.cpp:595` | Fixed | Tuning; L,N |
| `WEBHOOK_LONGTERM_ALERT40_SEC` | :220 = `10800` | `State_Report.cpp:138` | Fixed | Tuning; L,N |
| `WEBHOOK_LONGTERM_FORCE_CONNECT_MIN_INTERVAL_SEC` | :221 = `1800` | `State_Report.cpp:149` | Fixed | Tuning; L,N |
| `WEBHOOK_LONGTERM_ESCALATE_TO_ERROR_SEC` | :222 = `21600` | `State_Report.cpp:158` | Fixed | Tuning; L,N |
| `WEBHOOK_LONGTERM_CONNECTED_RECENTLY_SEC` | :223 = `7200` | `State_Report.cpp:160` | Fixed | Tuning; L,N |
| `WEBHOOK_LONGTERM_ESCALATION_COOLDOWN_SEC` | :224 = `10800` | `State_Report.cpp:162` | Fixed | Tuning; L,N |
| `CONNECT_ALIGNMENT_TOLERANCE_SEC` | :233 = `30` | `RuntimeReportingPolicy.cpp:52` | Fixed | Tuning; L,N |
| `NIGHTLY_HEAP_WARN_THRESHOLD_BYTES` | :249 = `51200` | `State_Sleep.cpp:1167,1170` | Fixed | Tuning; L,N |
| `NIGHTLY_HEAP_RESET_THRESHOLD_BYTES` | :250 = `46080` | `State_Sleep.cpp:1153,1157` | Fixed | Tuning; L,N |
| `DEBUG_SERIAL_REENUM_DELAY_MS` | :265 = `500` | `State_Sleep.cpp:1496` | Fixed; runtime serial setting can activate it | Tuning; L,N |
| `DEBUG_SERIAL_WAIT_TIMEOUT_MS` | :266 = `30000` | Main:1038; `State_Sleep.cpp:1499` | Same | Tuning; L,N |
| `DEBUG_SERIAL_WAIT_POLL_DELAY_MS` | :267 = `100` | Main:1041; `State_Sleep.cpp:1503` | Same | Tuning; L,N |
| `DEBUG_SERIAL_POST_CONNECT_DELAY_MS` | :268 = `500` | Main:769,1045; `State_Sleep.cpp:1507` | Same; boot settle also uses it | Tuning; L,N |

Despite their names and comments, the four `DEBUG_SERIAL_*` constants are **not dead and not exclusively bench-only**. Ledger-controlled serial configuration reaches them in field builds.

Lifecycle, state-machine, time, and reporting constants are:

| Name | Definition/value | Read | Build variation | Kind; status |
|---|---|---|---|---|
| `AWAKE_WATCHDOG_TIMEOUT_MS` | Main:163 = `60000` | Main:910,926 | Fixed; watchdog-capable path | Tuning; L,N |
| `REPORT_FORENSICS_SLOW_LOG_THRESHOLD_MS` | Main:164 = `250` | Main:2065,2116 | Fixed; `ENABLE_PERF_TRACE` consumer | Tuning; L,N |
| `REPORT_FORENSICS_ABNORMAL_WARN_THRESHOLD_MS` | Main:165 = `1000` | Main:2061,2112 | Fixed | Tuning; L,N |
| `kLoopStageWarnThresholdMs` | Main:480 = `2000` | Main:730 | Fixed | Tuning; L,N |
| `kLoopStageErrorThresholdMs` | Main:481 = `10000` | Main:728 | Fixed | Tuning; L,N |
| `kLoopForensicsSnapshotIntervalMs` | Main:482 = `1000` | Main:632 | Fixed | Tuning; L,N |
| `resetWait` | Main:757 = `30000` | `State_Error.cpp:158,169` | Fixed | Tuning; L,N |
| `MODEM_UNSTABLE_CONNECT_TIMEOUT_THRESHOLD` | `State_Connect.cpp:24` = `2` | Same:144 | Fixed | Tuning; L,N |
| `firmwareUpdateMaxMs` | Same:188 = `ConnectivityPolicy::FIRMWARE_UPDATE_MAX_MS` | Same:771 | Alias, fixed `300000` | Tuning; L,N |
| `CONNECT_HEARTBEAT_MS` | Same:233 = `30000` | Same:427 | Fixed | Tuning; L,N |
| `CONNECT_DIAG_LOG_MS` | Same:234 = `30000` | Same:431 | Fixed | Tuning; L,N |
| `CLOUD_RECOVER_STAGE1_MS` | Same:235 = `60000` | Same:299,396 | Fixed | Tuning; L,N |
| `CLOUD_RECOVER_STAGE2_MS` | Same:236 = `120000` | Same:300,409 | Fixed | Tuning; L,N |
| `MODEM_UNSTABLE_SLOW_TEARDOWN_MS` | `State_Sleep.cpp:28` = `10000` | Same:855 | Fixed | Tuning; L,N |
| `MODEM_UNSTABLE_RECOVERY_TEARDOWN_MS` | Same:29 = `5000` | Same:133 | Fixed | Tuning; L,N |
| `kGateBlockLogThresholdMs` | Same:169 = `10000` | Same:537,632 | Fixed | Tuning; L,N |
| `kMinHibernateSleepSec` | Same:280 = `900` | Same:282 | Fixed; Boron hibernate path | Tuning; L,N |
| `kMaxHibernateSleepSec` | Same:281 = `36000` | Same:282 | Same | Tuning; L,N |
| `WIFI_OFF_GUARD_MAX_MS` | Same:1272 = `2000` | Same:1293 | Fixed; Wi-Fi path | Tuning; L,N |
| `WIFI_OFF_GUARD_RETRY_MS` | Same:1273 = `400` | Same:1284 | Same | Tuning; L,N |
| `MODEM_UNSTABLE_RECONNECT_DEFER_MS` | `State_Report.cpp:25` = `30000` | Same:220,222 | Fixed | Tuning; L,N |
| `BACKOFF_MS` | `ThrashGuard.cpp:32` = `10000` | Same:129 | Fixed | Tuning; L,N |
| `TIER2_WINDOW_MS` | Same:33 = `600000` | Same:121 | Fixed | Tuning; L,N |
| `TIER3_WINDOW_MS` | Same:34 = `3600000` | Same:114,124 | Fixed | Tuning; L,N |
| `TIER3_TRIP_COUNT` | Same:35 = `3` | Same:124 | Fixed | Tuning; L,N |
| `logCooldownMs_` | `ThrashGuard.h:65` = `300000` | `ThrashGuard.cpp:78` | Setter at :40 exists, but no source caller | Tuning; L,N |
| `kMaxSyncAgeMs` | `ClockTrust.h:42` = `86400000` | Same:94,112,286,298 | Fixed | Tuning; L,N |
| `kMinResyncRetryIntervalMs` | Same:54 = `60000` | Same:133 | Fixed | Tuning; L,N |
| `kMinRtcWriteRetryIntervalMs` | Same:151 = `60000` | Same:179 | Fixed | Tuning; L,N |
| `kEpochMin` | `Clock.cpp:576` = `1704067200` | Same:578 | Fixed: 2024-01-01 inclusive | Tuning; L,N |
| `kEpochMax` | Same:577 = `2051222400` | Same:578 | Fixed: 2035-01-01 exclusive | Tuning; L,N |
| `kSkewSeconds` | `RtcSkewTest.h:353` = `-28800` | Same:371; Main:1228 | Fixed; RTC bench hook only | Tuning; L,N,B |
| `kFallbackAnchor` | Same:360 = `1767225600` | Same:381 | Same | Tuning; L,N,B |
| `kSaneReadingFloor` | Same:365 = `1577836800` | Same:378 | Same | Tuning; L,N,B |
| `kMaxFutureBoundaryChecks` | `ReportingPolicy.cpp:7` = `64` | Same:39 | Fixed | Tuning; L,N |

Power, battery, and sensor tuning is:

| Name | Definition/value | Read | Build variation | Kind; status |
|---|---|---|---|---|
| `USB_BENCH_MAX_CURRENT_MA` | `PowerPlatform.cpp:38` = `900` | Same:81,244 | Fixed; PMIC-capable cellular path | Tuning; L,N |
| `USB_BENCH_MIN_VOLTAGE_MV` | Same:39 = `3880` | Same:82,245 | Same | Tuning; L,N |
| `USB_BENCH_CHARGE_CURRENT_MA` | Same:40 = `896` | Same:83,246 | Same | Tuning; L,N |
| `USB_BENCH_CHARGE_VOLTAGE_MV` | Same:41 = `4112` | Same:84,247 | Same | Tuning; L,N |
| `SOLAR_MAX_CURRENT_MA` | Same:43 = `900` | Same:94,259 | Same | Tuning; L,N |
| `SOLAR_MIN_VOLTAGE_MV` | Same:44 = `5080` | Same:95,260 | Same | Tuning; L,N |
| `SOLAR_CHARGE_CURRENT_MA` | Same:45 = `900` | Same:96,261 | Same | Tuning; L,N |
| `SOLAR_CHARGE_VOLTAGE_MV` | Same:46 = `4208` | Same:97,262 | Same | Tuning; L,N |
| `kCriticalVcell` | `PowerTier.h:37` = `3.5 V` | `PowerTier.cpp:37`; `BatteryTierGuard.cpp:14` | Fixed | Tuning; L,N |
| `kFullSocThreshold` | Same:41 = `75%` | `PowerTier.cpp:8` | Fixed | Tuning; L,N |
| `kReducedSocThreshold` | Same:42 = `55%` | `PowerTier.cpp:11` | Fixed | Tuning; L,N |
| `kLowSocThreshold` | Same:43 = `35%` | `PowerTier.cpp:14` | Fixed | Tuning; L,N |
| `POST_CONNECT_DELTA_THRESHOLD` | `BatteryAuthorityPolicy.h:7` = `20 percentage points` | `.cpp:22,27` | Fixed | Tuning; L,N |
| `kOcvKnots` | `BatteryHealth.cpp:19` = curve below | Same:57–66 | Fixed | Tuning; L,N |
| `kUntrustedResidual` | Same:38 = `20` | Same:94,95 | Fixed | Tuning; L,N |
| `kSuspectResidual` | Same:41 = `12` | Same:96,97 | Fixed | Tuning; L,N |
| `kBiasAllowance` | Same:44 = `8` | Same:94–97 | Fixed | Tuning; L,N |
| `ThermalThresholds::armHighC` | `ChargeInhibitPolicy.h:24` = `37°C` | Same:57,63,116; `MyPersistentData.cpp:521,526` | Compiled default fixed; Ledger can replace runtime value | Tuning; L,N |
| `ThermalThresholds::armLowC` | Same:25 = `0°C` | Same:60,116; `MyPersistentData.cpp:521,526` | Same | Tuning; L,N |
| `ThermalThresholds::releaseHighC` | Same:26 = `35°C` | Same:57,111 | Same | Tuning; L,N |
| `ThermalThresholds::releaseLowC` | Same:27 = `3°C` | Same:60,112 | Same | Tuning; L,N |
| `kCellChargeMaxC` | Same:35 = `45°C` | Same:63 | Fixed validation ceiling | Tuning; L,N |
| `kChargeSummaryIntervalMs` | `PmicFaultMonitor.cpp:22` = `900000` | Same:470 | Fixed | Tuning; L,N |
| `kRemediationCooldownMs` | Same:118 = `3600000` | Same:293,315 | Fixed | Tuning; L,N |
| `PMIC_ANOMALY_CONSECUTIVE_LIMIT` | `SensorManager.cpp:71` = `3` | Same:791 | Fixed; `ENABLE_PMIC_FORENSICS` gated | Tuning; L,N |

`kOcvKnots` contains `(volts, SOC%)`:
`(3.00,0), (3.30,5), (3.50,10), (3.60,20), (3.70,30), (3.80,40), (3.90,50), (3.95,60), (4.00,70), (4.05,80), (4.10,90), (4.20,100)`.

The `USB_BENCH_*` names are misleading as a status classification: these values are used in ordinary firmware when the USB profile is selected. They are **not bench-build-only**.

Capacity settings are:

| Name | Definition/value | Read | Build variation | Kind; status |
|---|---|---|---|---|
| `LEDGER_REQUEST_TRACKER_MAX` | `Cloud.cpp:26` = `16` | Same:28,330 | Fixed | Tuning; L,N |
| `DEVICE_STATUS_PAYLOAD_CAPACITY` | `Cloud.h:397` = `896` | Same:402; `DeviceStatusPublisher.cpp:190,358,369,398` | Fixed | Tuning; L,N |
| `kDeviceDataPayloadCapacity` | `DeviceStatusPublisher.cpp:54` = `512` | Same:530,563 | Fixed | Tuning; L,N |
| `kTimeZoneCapacity` | `SystemConfig.h:80` = `39` | `ConfigApply.cpp:247`; `MyPersistentData.cpp:998,999` | Fixed; storage-size assertions | Tuning; L,N |
| `kWebhookNameCapacity` | Same:81 = `64` | `ConfigApply.cpp:550`; `MyPersistentData.cpp:1000,1001` | Fixed; storage-size assertions | Tuning; L,N |
| `kDiagBatchCapacity` | `PowerDiagnostics.cpp:64` = `12` | Same:86,91 | Fixed; diagnostics-publish flag required | Tuning; L,N,B |
| `kClosingReserve` | Same:405 = `16` | Same:406 | Same | Tuning; L,N,B |

The broad constant search also finds identifiers and metadata that should not be mistaken for adjustable settings:

| Name | Definition/value | Read | Build variation | Kind; status |
|---|---|---|---|---|
| `SYS_DATA_MAGIC` | `MyPersistentData.h:447` = `0x20a15e75` | `.cpp:69` | Fixed | Other; L,N |
| `SYS_DATA_VERSION` | Same:448 = `4` | `.cpp:69` | Fixed storage schema | Other; L,N |
| `SENSOR_DATA_MAGIC` | Same:592 = `0x20a47e74` | `.cpp:575` | Fixed | Other; L,N |
| `SENSOR_DATA_VERSION` | Same:593 = `1` | `.cpp:575` | Fixed storage schema | Other; L,N |
| `CURRENT_DATA_MAGIC` | Same:823 = `0x20a99e74` | `.cpp:676` | Fixed | Other; L,N |
| `CURRENT_DATA_VERSION` | Same:824 = `1` | `.cpp:676` | Fixed storage schema | Other; L,N |
| `kLoopForensicsMagic` | Main:474 = `0x57444631` | Main:614,621 | Fixed | Other; L,N |
| `kLoopForensicsVersion` | Main:479 = `2` | Main:615,622 | Fixed retained-state schema | Other; L,N |
| `kLedgerSchemaVersion` | `DeviceStatusPublisher.cpp:53` = `2` | Same:210,359,399,535,564 | Fixed output schema | Other; L,N |
| `REASON_NOPROGRESS` | `ThrashGuard.cpp:36` = `1` | Same:112 | Fixed | Other; L,N |
| `kReportedSyncAgeUnavailableMs` | `ClockTrust.h:329` = `0xffffffff` | Same:356,359; `DeviceStatusPublisher.cpp:313` | Fixed sentinel | Other; L,N |
| `kOcvKnotCount` | `BatteryHealth.cpp:33` = array-derived `12` | Same:60,61,64 | Derived | Other; L,N |
| `PMIC_CHRG_FAULT_MASK` | `SensorManager.cpp:77` = `0x30` | Same:155,172 | Fixed; forensics gated | Other; L,N |
| `PMIC_BAT_FAULT_MASK` | Same:78 = `0x08` | Same:155,172 | Same | Other; L,N |
| `kPmicChargeFaultMask` | `PmicFaultMonitor.cpp:19` = `0x30` | Same:24,129,153,159,197,397 | Fixed | Other; L,N |
| `kPmicBatFaultMask` | Same:20 = `0x08` | Same:388,397 | Fixed | Other; L,N |
| `kPmicNtcFaultMask` | Same:21 = `0x07` | Same:189 | Fixed | Other; L,N |
| `kGateBlockerQueue` | `State_Sleep.cpp:171` = `0x01` | Same:181,521 | Fixed | Other; L,N |
| `kGateBlockerLedger` | Same:172 = `0x02` | Same:183,524 | Fixed | Other; L,N |
| `kGateBlockerWebhook` | Same:173 = `0x04` | Same:187,527 | Fixed | Other; L,N |
| `kGateBlockerUpdate` | Same:174 = `0x08` | Same:195,530 | Fixed | Other; L,N |
| `kGateBlockerUninitialized` | Same:175 = `0xff` | Same:332,351,604,661,687 | Fixed | Other; L,N |
| `kRegStatus` | Same:237 = `0x0f` | Same:247 | Fixed; Boron path | Other; L,N |
| `kRegIntMask` | Same:238 = `0x12` | Same:242,252 | Same | Other; L,N |
| `kMaskAieTie` | Same:239 = `0x0c` | Same:251 | Same | Other; L,N |

Six power-source identifiers are independently repeated in three files—**18 definitions**, all fixed, live, and kind “other”:

| Name/value | `PowerManager.cpp` definition → read | `PowerPlatform.cpp` definition → read | `PowerDiagnostics.cpp` definition → read |
|---|---|---|---|
| `kPowerSourceUnknown = 0` | :11 → :154 | :10 → :19,190 | :18 → :188 |
| `kPowerSourceVin = 1` | :12 → :38,153 | :11 → :20 | :19 → :190 |
| `kPowerSourceUsbHost = 2` | :13 → :33,165 | :12 → :21 | :20 → :192 |
| `kPowerSourceUsbAdapter = 3` | :14 → :34 | :13 → :22 | :21 → :194 |
| `kPowerSourceUsbOtg = 4` | :15 → :35 | :14 → :23 | :22 → :196 |
| `kPowerSourceBattery = 5` | :16 → :41 | :15 → :24 | :23 → :198 |

Diagnostic reason identifiers all live in `PowerDiagnostics.cpp`, are fixed, and only exist in the diagnostics-publish path:

| Name | Definition/value | Read | Kind; status |
|---|---|---|---|
| `kReasonSetup` | :36 = `0` | :51 | Other; L,N,B |
| `kReasonPostWakeSetup` | :37 = `1` | :52 | Other; L,N,B |
| `kReasonPostRefreshInputProfile` | :38 = `2` | :53 | Other; L,N,B |
| `kReasonProfileChange` | :39 = `3` | :54 | Other; L,N,B |
| `kReasonConnectSuccess` | :40 = `4` | :55 | Other; L,N,B |
| `kReasonPreHibernate` | :41 = `5` | :56 | Other; L,N,B |
| `kReasonPreSleepUlp` | :42 = `6` | :57 | Other; L,N,B |
| `kReasonPreSleepStopFallback` | :43 = `7` | :58 | Other; L,N,B |
| `kReasonPreSleepStopTimerOnly` | :44 = `8` | :59 | Other; L,N,B |
| `kReasonPostWake` | :45 = `9` | :60 | Other; L,N,B |
| `kReasonChargeDiag` | :46 = `10` | :378,414 | Other; L,N,B |
| `kReasonUnknown` | :47 = `255` | :50,61 | Other; L,N,B |

Metadata arrays are counted once each:

| Name | Definition/value | Read | Build variation | Kind; status |
|---|---|---|---|---|
| `batteryContext` | Main:1981 = Unknown, Not Charging, Charging, Charged, Discharging, Fault, Disconnected | Main:2025,2041 | Fixed | Other; L,N |
| `batteryContext` | `SensorManager.cpp:3`, same seven labels | Same:666,670,704,810,871,873,914 | Fixed duplicate | Other; L,N |
| `kTierNames` | `BatteryAuthorityCommand.cpp:25` = HEALTHY, CONSERVING, CRITICAL, SURVIVAL | Same:26 | Fixed | Other; L,N |
| `kLabels` | `PmicFaultMonitor.cpp:31` = OFF, PRE, FAST, DONE | Same:32 | Fixed | Other; L,N |
| `DEFINITIONS` | `SensorDefinitions.h:25`: pressure→`VehiclePressure,true,true`; PIR→`PIR,false,true` | Same:40; Main:1114–1118 | Fixed | Other; L,N |

`DEFINITIONS` is live as a table, but not every field is independently consumed by production behavior; the startup consumer uses `ledDefaultOn`.

Hardware pins and persistence paths complete the named inventory:

| Name | Definition/value | Read | Build variation | Kind; status |
|---|---|---|---|---|
| `TMP36_SENSE_PIN` | `device_pinout.cpp:35,37,39` = override / S4 / A4 | `SensorManager.cpp:1053,1059` | Override or P2/other selection | Other; L |
| `BUTTON_PIN` | Same:42 = `D4` | Main:972,1603; `State_Sleep.cpp:1095,1320` | Fixed symbolic pin | Other; L,N |
| `BLUE_LED` | Same:43 = `D7` | **Never** | Fixed | Other; D,N |
| `WAKEUP_PIN` | Same:44 = `WKP` | `State_Sleep.cpp:1086,1087,1094` | Fixed symbolic pin; platform supplies mapping | Other; L,N |
| `intPin` | Same:63,68,76 = P2 `S2`, otherwise `SCK` | `PIRSensor.h:41,50`; `State_Sleep.cpp:1321` | Platform selection | Other; L |
| `disableModule` | Same:64,69,77 = P2 `S0`, otherwise `MOSI` | `PIRSensor.h:42,46`; `SensorManager.cpp:381,383` | Platform selection | Other; L |
| `ledPower` | Same:65,70,78 = P2 `S1`, otherwise `MISO` | Main:1112–1118; `PIRSensor.h:43,47` | Platform selection | Other; L |
| `persistentDataPathSystem` | `MyPersistentData.cpp:57` = `/usr/sysStatus.dat` | Same:69 | Fixed | Other; L,N |
| `persistentDataPathSensor` | Same:563 = `/usr/sensor.dat` | Same:575 | Fixed | Other; L,N |
| `persistentDataPathCurrent` | Same:664 = `/usr/current.dat` | Same:676 | Fixed | Other; L,N |

The build environment also supplies `PLATFORM_ID`, platform identifiers, `Wiring_Cellular`, `Wiring_WiFi`, `Wiring_Watchdog`, and HAL capability macros. These are external platform selectors rather than locally defined settings. Representative consumers are `ConnectivityPolicy.h:148,166`, `PowerPlatform.cpp:37`, `SensorManager.cpp:519`, and Main:113,174.

Repository build selection already differs: `project.properties:3–4` says Device OS `6.4.1`, platform `p2`; `.vscode/settings.json:2–3` selects `6.4.1`, `boron`; `.vscode/tasks.json:9–13` offers Boron, P2, and Argon. There is no single release/bench profile selector today.

Runtime configuration needs to remain a separate concern from build switches. `ConfigApply.cpp:29–90` gives **device Ledger > default Ledger > existing stored/default value** precedence. Device values cannot be established from this repository-only inventory.

The 27 Ledger fields accepted by `ConfigApply.cpp` are below. All are kind **other: runtime configuration**, all have source reads, and all can vary without rebuilding. “Metadata only” means the field is read for configuration comparison/fingerprinting but no operational consumer was found.

| Ledger field | Apply location | Compiled fallback/initialization visible in `src/` | Operational read/status |
|---|---|---|---|
| `messaging.serial` | `ConfigApply.cpp:197` | No explicit field initializer in `src/` | Main:1030; `State_Sleep.cpp:1492`; L |
| `messaging.verboseMode` | :205 | `false`, `MyPersistentData.cpp:135` | `SensorManager.cpp:685`; `State_Sleep.cpp:79`; L |
| `messaging.verboseTimeoutMin` | :213 | `60`, `MyPersistentData.cpp:152` | `DeviceStatusPublisher.cpp:101`; L, metadata only; no auto-expiry consumer |
| `timing.timezone` | :249 | `UTC0`, `Config.h:30` | Main:1401–1404; L |
| `timing.reportingIntervalSec` | :264 | `3600`, `Config.h:33` | `Config.cpp:149`; reporting policy; L |
| `timing.openHour` | :279 | `6`, `Config.h:31` | `Config.cpp:69`; clock scheduling; L |
| `timing.closeHour` | :291 | `22`, `Config.h:32` | `Config.cpp:80`; clock scheduling; L |
| `timing.connectAttemptBudgetSec` | :304 | `300`, `Config.h:35` | `State_Connect.cpp:108–114`; L |
| `sensor.type` | :350 | `1`, `MyPersistentData.cpp:605` | `DeviceStatusPublisher.cpp:659`; L, config metadata; see separate sensor-type stores below |
| `sensor.setting1` | :363 | `60000`, `MyPersistentData.cpp:606` | `Config.cpp:158`; occupancy debounce; L |
| `sensor.setting2` | :379 | `0`, `MyPersistentData.cpp:607` | `SensorManager.cpp:347`; polling path; L |
| `sensor.setting3` | :387 | `0`, `MyPersistentData.cpp:608` | `DeviceStatusPublisher.cpp:105`; L, metadata only |
| `sensor.setting4` | :395 | `0`, `MyPersistentData.cpp:609` | `DeviceStatusPublisher.cpp:106`; L, metadata only |
| `modes.sensorMode` | :430 | COUNTING=`0`, `MyPersistentData.cpp:148` | `State_Idle.cpp:45`; Main:2003; L |
| `modes.connectionMode` | :445 | CONNECTED=`0`, `MyPersistentData.cpp:149` | `State_Idle.cpp:115`; `State_Sleep.cpp:367`; L |
| `modes.reportingMode` | :467 | SCHEDULED=`0`, `MyPersistentData.cpp:150` | `DeviceStatusPublisher.cpp:114`; L, metadata only |
| `modes.samplingMode` | :483 | INTERRUPT=`0`, `MyPersistentData.cpp:151` | `DeviceStatusPublisher.cpp:115`; L, metadata only; sensor object chooses interrupt behavior |
| `modes.cloudDisconnectBudgetSec` | :497 | `15`, `Config.h:36` | `State_Sleep.cpp:707–709`; L |
| `modes.modemOffBudgetSec` | :510 | `30`, `Config.h:37` | `State_Sleep.cpp:713–715`; L |
| `modes.enableHibernateSleep` | :522 | `false`, `MyPersistentData.cpp:157` | Hibernate selection in `State_Sleep.cpp`; L |
| `reporting.webhook.name` | :552 | No explicit initializer; convention fallback in `Cloud.cpp:655–665` | `Cloud.cpp:648`; L |
| `reporting.webhook.enabled` | :566 | No explicit field initializer in `src/` | `DeviceStatusPublisher.cpp:120`; L, metadata only; no publish-enable gate found |
| `reporting.webhook.timeoutMs` | :574 | Runtime fallback `20000`, Main:1695–1696 | Main:1694–1700; L |
| `power.thermalChargeInhibit.armHighC` | :630 | `37°C`, `ChargeInhibitPolicy.h:24` | Thermal policy via `PowerConfig` getters; L |
| `power.thermalChargeInhibit.armLowC` | :631 | `0°C`, same:25 | Same; L |
| `power.thermalChargeInhibit.releaseHighC` | :632 | `35°C`, same:26 | Same; L |
| `power.thermalChargeInhibit.releaseLowC` | :633 | `3°C`, same:27 | Same; L |

Other runtime/storage settings worth separating during cleanup:

| Setting | Definition/default | Read | Variation; kind/status |
|---|---|---|---|
| System-store `sensorType` | `MyPersistentData.cpp:140` = `1` | `SensorManager.cpp:307`; Main:1113,2683 | Persisted; other/L |
| `solarPowerMode` | `MyPersistentData.cpp:137` = `FIELD_BUILD ? true : false` | `PowerManager.cpp:20` | Build-dependent initial value, then persisted; other/L |
| `lowPowerMode` | `MyPersistentData.cpp:138` = `false` | Storage/facade accessors only | Legacy compatibility field; no operational consumer found |
| `testConnectionDurationOverride` | Main:1049–1050 changes zero to `0xffff` | Same check; no connection-duration application found | Stored remnant; only initialization behavior remains |
| `structuresVersion` | `MyPersistentData.cpp:134` = `2` | Getter exists; no consumer found | Storage metadata remnant, distinct from `SYS_DATA_VERSION` |

Literal configuration also exists outside the requested named-constant patterns. These should be listed with their owners during consolidation, rather than silently lost:

| Rule/settings | Location/current values | Consumer/variation | Kind/status |
|---|---|---|---|
| Thrash no-progress timeouts | `ThrashGuard.cpp:58–73`: sleep/init/error `60s`; connect/report `120s`; OTA `180s`; idle `0` | Same:92; fixed | Tuning; L,N |
| SOC hysteresis boundaries | `BatteryBackoffPolicy.h:21–36`: `75/70`, `55/50`, `35/30%` | `BatteryAuthority.cpp:36,56`; fixed | Tuning; L,N |
| Tier reporting multipliers | Same:45–55: healthy `1`, conserving `2`, critical `4`, survival `12` | `RuntimeReportingPolicy.cpp:48`; fixed | Tuning; L,N |
| Connection-duration backoff | `BatteryBackoff.cpp:24–43`: `0→2×`, `<60→1×`, `<180→1×`, `<300→1.5×`, otherwise `2×` | `getConnectionBackoffMultiplier()` has no source caller | Tuning; behavior dead |
| Battery validity/progress | `BatteryAuthorityPolicy.cpp:8,12,28,47`: SOC `0–100`; voltage `2.5–5.0`; corroboration `4.00V`; progress `0.5%` or `0.015V` | Enclosing policy helpers; fixed | Tuning; L,N |
| Persistence save delays | `MyPersistentData.cpp:79,584,685`: `100/250/250ms` | Store setup chains | Tuning; L,N |
| Publish queue capacity | Main:1126 = `800` | Queue setup at same call | Tuning; L,N |
| Forced startup serial wait | Main:888–889 = `10000ms` wait + `1000ms` delay | `ALLOW_BLOCKING_SERIAL_WAITS` gated | Tuning; L,N,B |
| Serial baud argument | Main:1031; `State_Sleep.cpp:1495` = `9600` | Serial initialization | Other; L,N |
| Boot-storm response | Main:873,880 = `6` early resets, `600000ms` sleep | Same branch | Tuning; L,N |
| Webhook timeout fallback/range | Main:1695–1696 = accept `5000–120000ms`, fallback `20000ms` | Main:1700 | Tuning; L,N |
| Webhook name conventions | `Cloud.cpp:655,659,662,665` = unknown/counting/occupancy/measurement `-webhook-v1` | `Cloud::getWebhookName()` | Other; L,N |
| Ledger names | `LedgerClient.cpp:29,33,36,38` = default-settings/device-settings/device-status/device-data | `Cloud::setup()` | Other; L,N |
| Charge-config write attempts | `ChargeInhibit.cpp:28` = `2` | Same retry loop | Tuning; L,N |
| TMP112 address fallback | `SensorManager.cpp:430,977` = `0x48` | Read/probe paths | Overrideable by `MUON_TMP112_I2C_ADDR`; tuning/L |
| Temperature validity/fallback | `SensorManager.cpp:1003,1008,1023–1024`: `−50 < T < 120°C`, fallback `25°C` | Temperature sampling paths | Tuning; L,N |

These literal rules are supplemental to the 219 named-entry tally. This is a source configuration inventory, not an assertion that every numeric literal in the firmware is a configurable parameter.

The overlaps and drift are:

1. **Version identity is split and documentation has already drifted.** Product `27` lives in `FirmwareVersion.h`; string/notes live in `Version.cpp`; both headers declare the same pointers. Main adds redundant `extern` declarations at :82–83, and `DeviceStatusPublisher.cpp:38` adds another. README:11 and Doxyfile:5 still say `20.1-PowerMgt`.

2. **The release helpers preserve that split incorrectly.** `bump_version.sh:70,73,76,79,82` edits five identity/description locations. `release.sh:167` stages `Version.cpp` but omits the `FirmwareVersion.h` that the bump helper edits. Also, `bump_version.sh:26` extracts the product number by removing a decimal suffix; that does not parse the current `v27-SmallFixes` naming format into integer `27`.

3. **Release notes have no source consumer.** Move their authoritative content to `CHANGELOG.md` and remove the firmware definition/declarations. This inspection does not establish whether linker garbage collection already removes the unused text from a particular binary.

4. **Logging has three independent control layers:** compile-time trace flags, the hard-coded handler level/category filters, and runtime `verboseMode`. Enabling a trace macro does not override the handler’s severity filtering. Conversely, several trace flags guard `Log.info()` calls, so they are not simply aliases for TRACE severity.

5. **Two switches advertise missing behavior.** `ENABLE_PMIC_TRACE` and `ENABLE_PMIC_CHARGE_CYCLE_TEST` survive in definitions, validation, comments, and telemetry bits. Neither controls the advertised feature. `ENABLE_PMIC_REGISTER_DUMP` survives only as documentation.

6. **The build-flags witness is duplicated.** Main:1477–1503 and `DeviceStatusPublisher.cpp:219–246` independently construct the same bitmask. It omits RTC skew, independently overridden `FIELD_BUILD`, serial log level, and hardware overrides. It still reports the two obsolete feature switches.

7. **Connection defaults are duplicated.** The `300s` factory default in `Config.h:35` overlaps `ConnectivityPolicy.h:62,64`; the seconds constant at :64 is unused. Disconnect defaults `15/30s` appear in both `Config.h:36–37` and `ConnectivityPolicy.h:194–195`.

8. **Validation boundaries are duplicated—and one differs.** Disconnect limits `5–120s` occur in `ConfigApply.cpp:498,511` and `ConnectivityPolicy.h:192–193`. Webhook Ledger validation accepts `1000–60000ms` at `ConfigApply.cpp:575`, but runtime accepts `5000–120000ms` at Main:1695. A configured value of `1000ms` is accepted then replaced by the runtime `20000ms` fallback.

9. **Battery thresholds repeat across distinct policies.** `75/55/35%` appears in `PowerTier.h` and literal form in `BatteryBackoffPolicy.h`. They serve different policy roles; consolidate shared values only while retaining the backoff hysteresis semantics.

10. **Hardware identifiers repeat.** Six power-source codes are copied three times; PMIC `0x30/0x08` masks twice; TMP112 default address twice; battery labels twice. `WAKEUP_PIN=WKP` is named in pinout, while Main:1132 directly uses `WKP` for RTC setup.

11. **Runtime sensor type has two stores.** Ledger application writes `SystemConfig::SensorSettings::sensorType`, while factory creation reads `SystemConfig::sensorType`. Both initialize to `1`, but they are distinct fields. Do not merge them mechanically as duplicate declarations.

12. **Some runtime controls are metadata-only too.** Seven accepted Ledger fields lack an operational consumer beyond comparisons/fingerprinting: verbose timeout, sensor-store type, sensor settings 3/4, reporting mode, sampling mode, and webhook enabled. These merit a separate interface-retirement decision rather than being folded into the build profile.

The one-page consolidation recommendation is:

- **Keep one build profile:** retain `BuildProfile.h`, with explicit release and bench selections, every active diagnostic/trace switch, serial severity and category policy, and a single shared build-witness definition. Keep hardware overrides visibly grouped there. Remove `DEBUG_SERIAL`; retire the two metadata-only switches and their obsolete documentation. Bench selection should not automatically enable destructive RTC skew.
- **Keep one firmware identity source:** put product integer and display string together in `FirmwareVersion.h`. Move release prose to `CHANGELOG.md`. Retire `Version.h` and the existing `Version.cpp` after moving the live identity. Update the bump/release scripts and documentation generation to consume that source.
- **Keep runtime defaults and owner tuning distinct:** retain `Config.h`/`Config.cpp` for runtime defaults, validation, and provenance; correct their misleading wrapper documentation. Leave tuning with its owners, using this inventory as the index. A universal tuning header would recreate the clutter.

This yields roughly three central entry headers—`BuildProfile.h`, `FirmwareVersion.h`, and `Config.h`—while retaining implementations and domain policy files where behavior belongs.

| Current file/group | Proposed disposition |
|---|---|
| `BuildProfile.h` | Keep; absorb active logging and hardware build controls. |
| `FirmwareVersion.h` | Keep; become sole integer/string identity source. |
| `Version.h`, `Version.cpp` | Merge live identity into `FirmwareVersion.h`; delete after consumers migrate. |
| `Settings.h` | Delete after direct includes replace its two-header indirection; move useful catalog text to documentation. |
| `ProjectConfig.h` | Delete; its webhook helper is unused and is not the runtime fallback. |
| `Config.h`, `Config.cpp` | Keep; these contain live runtime configuration behavior. Deduplicate owner defaults deliberately. |
| `cloud/Particle_Functions.h/.cpp` | Keep functional registration/system setup; move logging policy out. |
| `cloud/ConfigMerge.cpp` | Delete as an empty translation unit, subject to build-file reference cleanup. |
| `cloud/ConfigApply.cpp`, `LedgerClient.cpp`, persistence facades/storage | Keep as runtime configuration owners. |
| Power, clock, connectivity, reporting, sensor and state policy files | Keep tuning locally; remove the unused seconds alias and `BLUE_LED`; consolidate repeated identifiers with their owners. |
| `bump_version.sh`, `release.sh` | Update for the single identity source, current version format, and complete staging list. |

The highest-value cleanup is small: remove the five dead named entries, retire the two ineffective feature switches, collapse the version/catalog wrappers, centralize logging policy and the build witness, and correct the release scripts. The **187 fixed declarations are not 187 deletion candidates**; most are useful thresholds, capacities, schema identifiers, or hardware definitions with real consumers.
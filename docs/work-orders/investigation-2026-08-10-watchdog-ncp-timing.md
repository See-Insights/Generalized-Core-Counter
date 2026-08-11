# Investigation: bench-soak watchdog faults (not yet a Work Order)

**Status: investigation only, per explicit instruction — no code changes, no
fix scoped yet. Findings ready to inform a future Work Order.**

## Summary

The working hypothesis going in was that slow Boron NCP/cellular cold-start
after a reset was tripping some watchdog. That hypothesis does not survive
the evidence. The actual mechanism is a pre-existing stall in the
sleep-entry path (breadcrumb 21, loop stage `"sleep"`) that occurs on
multiple devices running completely unrelated firmware, including one on
`main` with none of this branch's diagnostics-publish-mode or Phase 1
PowerManager code. There is a second, less certain fault signature
(breadcrumb 18, loop stage `"connectivity"`) that turned out to be a stale
breadcrumb artifact, not reliable evidence of where the stall actually
occurred. Neither pattern is explained by NCP timing, and one specific new
code path (a diagnostics-publish-mode flush call sitting between a
breadcrumb write and `System.sleep()` in the HIBERNATE branch) was
investigated directly and ruled out for the specific fault examined, based
on this device's own timezone/open-hours configuration.

## Evidence gathered

### Device telemetry (pulled directly from AWS via `./tools/telemetry` +
### raw S3 forensics payloads, not inferred)

Bench device: **Boron-Dev-09** (`e00fce68399ee6244a963935`), running
`20.1-PowerMgt` (this branch, `refactor/powermanager-phase1`).

Three watchdog faults on 2026-08-07, dedicated forensics payloads (not the
general `status` event, which can carry a stale watchdog snapshot from an
earlier fault — confirmed by decoding `System_Reset_Reason` values
directly from Device OS source, see below):

| Time (UTC) | breadcrumb | loop stage | elapsed | queue | state |
|---|---:|---|---:|---:|---:|
| 03:01:03 | 18 | `connectivity` | 0 | 1 | 4 (CONNECTING_STATE) |
| 07:40:49 | 18 | `connectivity` | 0 | 1 | 4 |
| 10:02:52 | 21 | `sleep` | 18 | 2 | 3 |

Cross-checked against `RESET_REASON_*` values from Device OS
(`services/inc/system_defs.h`, fetched directly from
`particle-iot/device-os`): `RESET_REASON_PIN_RESET=20`,
`RESET_REASON_WATCHDOG=60`. The `status` event immediately after the
07:40:49 fault reports `resetReason:20` (a pin reset on a *later* boot,
not the watchdog boot itself), so its `lastWatchdogBreadcrumb` field is
stale — this is why the dedicated `watchdog` event payload, not the
adjacent `status` event, is the trustworthy source here.

### Fleet-wide comparison — the decisive evidence

Pulled watchdog fault history for two other devices via the same tooling:

- **Boron-Dev-11**, running `21.0_Test` (`main` branch — zero Phase 1
  PowerManager migration, zero diagnostics-publish-mode code): 3 watchdog
  faults on 2026-08-10, **all three** `{"bc":21,"stage":"sleep","queue":0,...}`
  — same breadcrumb/stage signature as Boron-Dev-09's 10:02:52 fault,
  with an *empty* publish queue each time.
- **Boron-Dev-14**, also `21.0_Test`: 3 watchdog faults in a rapid
  boot-loop (within 6 seconds of each other) on 2026-08-09, all three
  `{"bc":21,"stage":"sleep",...}` again.

The `bc:21/stage:"sleep"` fault pattern is present on devices running
firmware with none of this branch's changes. It is a pre-existing
characteristic of the sleep-entry path itself, not something introduced
by diagnostics-publish-mode or the Phase 1 PowerManager migration.

### Breadcrumb map and watchdog timeout analysis (Codex, verified)

Full breadcrumb → code-location table built and spot-checked. Headline
findings:

- The shortest real guard is the **generic 60-second awake hardware
  watchdog** (`Generalized-Core-Counter.cpp:146`, started early in
  `setup()`), refreshed only at the **end of a complete `loop()`
  iteration** (`Generalized-Core-Counter.cpp:1217`). There is no explicit
  refresh inside `handleConnectingState()`. The 660s/300s connect budgets
  and the 124s AB1805 watchdog are not the operative guard — an isolated
  ~88s NCP delay would not trip either of those, but *any* single
  synchronous operation blocking a full loop traversal for 60s would trip
  the awake watchdog regardless of total connect time. This directly
  explains why incident 2 (successful 89s hibernate-wake connect,
  continuing to feed the watchdog each loop) didn't fault while a
  similar-duration NCP delay elsewhere could.
- Breadcrumb values 18–21 carry a real, pre-existing collision: pre-v15
  sleep-path code still writes literal numeric breadcrumbs (18, 19, 20,
  21) that the current enum reassigned to unrelated cloud/report-phase
  names. The `stage` field in the retained loop-forensics struct is what
  actually disambiguates — not the breadcrumb number alone.
- **Breadcrumb 18 with `stage:"connectivity"` is a stale-breadcrumb
  artifact, not a reliable connectivity-path indicator.** Loop stage is
  set independently of the breadcrumb: every new loop iteration marks
  `stage=connectivity` before the state dispatch, without writing a fresh
  breadcrumb. The only two writers of literal 18 are end-of-loop
  `PUBLISH_QUEUE_EXIT` and the sleep-precondition gate — neither is a
  connectivity-path breadcrumb. The next *real* connect-path breadcrumb
  (6, immediately before `Particle.connect()`) was never reached in these
  faults. This means the earlier working hypothesis's implicit read of
  "breadcrumb 18/connectivity = stuck talking to the NCP" is not
  supported — the stall (if it's a stall, rather than the watchdog simply
  not yet having reached the connect call) could be anywhere between loop
  entry and breadcrumb 6, which is mostly bookkeeping, not NCP I/O.

### New code (diagnostics-publish-mode, Phase 1 PowerManager): investigated directly, not inferred

- `PowerDiagnostics::flushDiagBatch()` (gated behind
  `ENABLE_DIAGNOSTICS_PUBLISH_MODE`, default 0) sits **directly between**
  `setAppBreadcrumb(21)` and `System.sleep()` in the HIBERNATE branch only
  (`State_Sleep.cpp:1079-1086`). It does **not** sit before `System.sleep()`
  in the regular ULP branch (`State_Sleep.cpp:1333`) — there, the flush
  only runs after wake.
- **This device's actual ledger settings** (`enableHibernateSleep: true`,
  `timezone: SGT-8`, open hours 06:00–22:00) were checked directly, not
  assumed from repository defaults. 10:02 UTC = 18:02 local — inside open
  hours. HIBERNATE requires closed hours to fire at all, regardless of the
  enable flag. **The 10:02:52 fault could not have gone through the
  HIBERNATE branch** under this device's actual configuration, which rules
  out the one code location where the new flush call sits in a
  theoretically risky position, for this specific incident.
- Independently, the enqueue path itself (`flushDiagBatch()` →
  `PublishQueuePosix::publish()` → `publishCommon()`) was traced in full:
  network transmission and retry/backoff happen entirely in a separate
  background thread under a separate mutex and do not block the caller. A
  60-second stall from normal queue/network behavior is ruled out. The
  only architecturally-possible (not evidenced) stall would require an
  abnormal filesystem I/O stall while holding the PublishQueue's
  untimed recursive mutex — which would be a pre-existing
  `PublishQueuePosixRK`/filesystem characteristic, not something
  diagnostics-publish-mode introduces, and there is no evidence this
  occurred (Boron-Dev-11/-14's identical fault signature with zero
  diagnostics-publish-mode code present rules this specific code path out
  as the common-cause explanation).
- Every other Phase 1 PowerManager migration call (`PowerManager::instance().soc()`
  substitutions, `refreshInputProfile()` timing) was traced and confirmed
  to introduce no new hardware access, wait, or watchdog-relevant timing
  change versus the pre-migration code.

### Known hardware characteristic (Particle TAN009)

Particle's own Technical Advisory Note TAN009 ("SARA-R410 Intermittent
Increased Connection Time") confirms "elongated initial connection time
after a power on" as a documented characteristic of exactly this modem
family (SARA-R410M, Boron BRN402/BRN404), most observed on
battery-powered devices with periodic connect/sleep cycling — matching
this fleet's operating pattern. The specific historical root cause
(carrier-level network change) was patched in Device OS 2.3.1/3.3.1/4.0.1/5.1.1,
well before this project's 6.4.1, but the note establishes this modem
family has a real history of variable cold-start timing as a hardware/
carrier characteristic, independent of application firmware.

## What remains open

- The exact synchronous operation that blocks a full 60s loop traversal
  during the `bc:21/stage:"sleep"` fault is not yet identified — only that
  it's pre-existing and present on `main`. Worth a dedicated trace of
  everything between loop entry and `System.sleep()` on the regular ULP
  path across both branches.
- The `bc:18/stage:"connectivity"` faults' true stall location is
  genuinely ambiguous from telemetry alone (could be early state-entry
  bookkeeping, not necessarily NCP I/O) — would need either better
  breadcrumb instrumentation (a fresh breadcrumb write at loop-stage
  entry, not just the stage label) or serial-log correlation from the
  device itself at the moment of a future fault.
- Confirm whether `ENABLE_DIAGNOSTICS_PUBLISH_MODE` was actually compiled
  in for the binary that produced these specific faults (repository state
  alone can't prove this either way).

## Follow-up: AB1805 wake-reason capture and serial-wait tradeoffs

Fold-in per explicit follow-up request. Still investigation/design only —
no implementation.

### 1. AB1805 wake-reason capture — current state and proposed design

`ab1805.getWakeReason()` is already called in `setup()`
(`Generalized-Core-Counter.cpp:994`), but only inside
`if (retainedHibernatePending)` — i.e. only on the boot immediately after
a hibernate attempt, never gated on `System.resetReason()` itself. Traced
directly and confirmed: a genuine watchdog fault that isn't immediately
following a hibernate attempt never reaches this call at all, and even
when it does fire, the wake reason only reaches a local
`Log.info("HibernateWake: pending=1...")` line unless the stricter
`RESET_REASON_POWER_MANAGEMENT` + `ALARM` conditions are also met — it
never reaches the cloud-published status in the fallback case.

- **`updateWakeReason()` freshness** (confirmed by reading
  `AB1805_RK.cpp:23` directly): `AB1805::setup()` already calls
  `updateWakeReason()` internally on chip detection, and the app calls
  `ab1805.setup()` early in `setup()` (line 965) — so the *existing*
  HIBERNATE-path `getWakeReason()` read is trustworthy today. It is
  **not** called anywhere on the ULP or STOP wake-return paths, which is
  exactly the library's documented "required after STOP mode" case — so
  a `getWakeReason()` call added to either of those paths would need its
  own `updateWakeReason()` first (best placement: right after the sleep
  cascade succeeds, before `resumeWDT()`).
- **Proposed design** (confirmed feasible): when `reason == RESET_REASON_PIN_RESET`,
  call `ab1805.updateWakeReason()` (capturing its bool success/fail
  result — the existing `AB1805::setup()` call discards this today) then
  `ab1805.getWakeReason()`, placed right after `ab1805.setup()` at line
  965, before `setWDT()`. Classify the boot as AB1805-confirmed-watchdog
  only when the result is `WATCHDOG`; treat `UNKNOWN` or a failed update
  as inconclusive, not as "AB1805 didn't fire." The existing watchdog
  persistence/forensics-publish logic (currently at lines 836-844 and
  955-960) runs *before* AB1805 init today and would need to move after
  this new classification to act on it — the publish queue is already up
  by line 955, so this is a reordering, not a new dependency.
  Recommend reusing the existing `watchdogResetCount`/`lastWatchdogBreadcrumb`/
  `lastWatchdogUptimeMs`/`lastWatchdogResetReasonData` fields for both
  Device-OS-detected and AB1805-confirmed watchdog events (same
  underlying failure, no need to duplicate), plus one new small persisted
  `lastWatchdogSource` enum (`DEVICE_OS` / `AB1805_PIN`) so later boots
  can tell which path detected it — this needs to be *appended* to
  `SysData`, per this project's persistent-layout append-only contract.

### 2. What the serial-wait tradeoff actually costs — traced precisely, not assumed

Full boot/log ordering traced line-by-line: `HibernateWake:` logs
**after** both existing serial-wait points (the compile-time
`ALLOW_BLOCKING_SERIAL_WAITS` wait and the `serialConnected`-gated wait),
not before. So on a cycle with either wait active, the line is not lost
to the race. It's only lost when *neither* wait is active (the correct,
field-safe soak configuration) and no monitor happens to already be
attached. Device OS's own reset-reason log fires earlier still (before
either application wait) but is moot either way: it logs at `TRACE`, and
this firmware's logger config explicitly filters the `"system"` category
to `WARN` (`Particle_Functions.cpp:27`, confirmed directly) — that line
is never emitted under the current configuration regardless of timing.

### 3. Proposed default "brief wait if already connected" — feasible, confirmed low-cost

`Serial.isConnected()` just queries whether the host has the USB CDC port
open — it needs `Serial.begin()` to have already run, but that's already
handled: the global `SerialLogHandler` (`Particle_Functions.cpp:23`)
calls `Serial.begin()` in its constructor, which runs before `setup()`
via Device OS's global-constructor init, and Particle's own docs confirm
an explicit `Serial.begin()` isn't needed on top of `SerialLogHandler`.
So the ordering constraint is already solved — no new `Serial.begin()`
needed.

**Proposed design**: a new, always-on (not `ALLOW_BLOCKING_SERIAL_WAITS`-gated)
check at the very start of `setup()`, before any application log: if
`Serial.isConnected()` is true, delay for the existing short
`ConnectivityPolicy::DEBUG_SERIAL_POST_CONNECT_DELAY_MS` (500ms) to let
an already-attached monitor settle; if false, proceed immediately with
zero cost. This is a distinct mechanism from `ALLOW_BLOCKING_SERIAL_WAITS`
(which forces a wait hoping something connects) and should stay separate
rather than modify that flag's logic — different field-safety semantics
(this one costs nothing when nothing's attached, by design). Correctly
scoped limitation: this cannot recover Device OS's own pre-`setup()`
messages (moot here per item 2), and won't wait for a monitor attached
*after* the check runs — it only helps a monitor already open in
follow/reconnect mode.

### 4. How often does `setup()` actually run — direct measurement, not assumption

`"status"` is published exactly once per `setup()` call
(`publishStartupStatus()`, `Generalized-Core-Counter.cpp:1028`), so
counting that event directly measures reboot frequency. Pulled 7-day
telemetry across three devices:

| Device | Branch | `setup()` calls / 7 days | Rate |
|---|---|---:|---|
| Boron-Dev-09 | this branch (bench soak) | 22 | ~3.1/day |
| Boron-Dev-11 | `main` | 2 | ~0.3/day |
| Boron-Dev-14 | `main` | 4 | ~0.6/day |

Even at the high end (the actively-exercised bench unit), this is nowhere
near a hot path. A few extra I2C transactions (item 1) or a 500ms
conditional delay only when a monitor is already attached (item 3) is
negligible overhead at 0.3-3 invocations per day — cost-benefit clearly
favors both additions.

## Recommendation for next step

Given the sleep-entry fault (`bc:21/stage:"sleep"`) is confirmed
cross-branch and cross-device, it is a **pre-existing issue** independent
of this investigation's original scope (diagnostics-publish-mode /
Phase 1 PowerManager) and could reasonably be scoped as its own Work
Order, focused on identifying the actual blocking operation in the
generic sleep-entry path — not something to fold into the diagnostics-
publish-mode work. The `bc:18/connectivity` pattern needs better
instrumentation before a root cause can even be proposed.

The AB1805 wake-reason disambiguation (item 1 above) and the default
brief-serial-wait (item 3 above) are both well-evidenced, low-cost, and
ready to be scoped into that same future Work Order — they directly
improve the odds of getting unambiguous data the next time either fault
pattern recurs, which is exactly what's currently missing.

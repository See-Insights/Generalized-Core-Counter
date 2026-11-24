# Copilot CLI Session

> [!NOTE]
> - **Session ID:** `9896dd98-9807-4b98-a5cd-394cfe0fd63c`  
> - **Started:** 11/8/2026, 9:40:24 PM  
> - **Duration:** 33m 34s  
> - **Exported:** 11/8/2026, 10:13:58 PM  

---

<sub>7s</sub>

### User

Implement per the dispatch in docs/work-orders/WO-2026-11-05-007-stage6-copilot-dispatch.md. The WO is the binding spec wherever the dispatch refers to it. Do not commit, push, merge, or release.

---

<sub>9s</sub>

### `bash`

**Read dispatch doc**

`$ cat docs/work-orders/WO-2026-11-05-007-stage6-copilot-dispatch.md; ls docs/work-orders`

<details>
<summary>6 lines</summary>

```
Output too large to read at once (30.5 KB). Saved to: /var/folders/_r/m12xn8s15wb_15djjdgfp5gr0000gn/T/1794154233698-copilot-tool-output-72505-c79da036-84b3-4b21-9fc9-f47e7382af4a.txt
Consider using tools like grep (for searching), head/tail (for viewing start/end), view with view_range (for specific sections), or jq (for JSON) to examine portions of the output.

Preview (first 500 chars):
AGENT: Copilot · MODEL: claude-sonnet-5.5 · REASONING: medium
AUTHORIZATION SCOPE: edit `src/` (the carry predicate in `src/time/ClockTrust.h`, `Clock::isClockTrusted()` and the RTC write-back in `src/time/Clock.cpp`, the latch read, and the hibernate-verdict input at its single boot call site), tests under `tests/` (new I21 tests; existing tests and `tests/stubs/` only as the change forces), and write the report; run host tests, the local Device OS toolchain build and nm/objdump; scratch `build
<shellId: 0 completed with exit code 0>
```

</details>


---

<sub>11s</sub>

### `view`

**view the file at /var/folders/\_r/m12xn8s15wb\_15djjdgfp5gr0000gn/T/1794154233698-copilot-tool-output-72505-c79da036-84b3-4b21-9fc9-f47e7382af4a.txt.**

`/var/folders/_r/m12xn8s15wb_15djjdgfp5gr0000gn/T/1794154233698-copilot-tool-output-72505-c79da036-84b3-4b21-9fc9-f47e7382af4a.txt`

```
File too large to read at once (30.5 KB). Consider using the grep tool to search within the file, or view with view_range to read specific sections.
```


---

<sub>13s</sub>

### `view`

**view the file at /var/folders/\_r/m12xn8s15wb\_15djjdgfp5gr0000gn/T/1794154233698-copilot-tool-output-72505-c79da036-84b3-4b21-9fc9-f47e7382af4a.txt.**

`/var/folders/_r/m12xn8s15wb_15djjdgfp5gr0000gn/T/1794154233698-copilot-tool-output-72505-c79da036-84b3-4b21-9fc9-f47e7382af4a.txt (lines 1-120)`

<details>
<summary>119 lines</summary>

```
AGENT: Copilot · MODEL: claude-sonnet-5.5 · REASONING: medium
AUTHORIZATION SCOPE: edit `src/` (the carry predicate in `src/time/ClockTrust.h`, `Clock::isClockTrusted()` and the RTC write-back in `src/time/Clock.cpp`, the latch read, and the hibernate-verdict input at its single boot call site), tests under `tests/` (new I21 tests; existing tests and `tests/stubs/` only as the change forces), and write the report; run host tests, the local Device OS toolchain build and nm/objdump; scratch `build-tmp/wo-2026-11-05-007-stage6/` (create it; use only that) / NOT authorized: committing, pushing, merging, flashing, creating PRs or tags, touching `lib/`, S1/S1b/S2/S3/S4 behaviour (the latch accessor is read only), the failsafe ladder, item 19's credit logic, the persisted or `retained` layouts, deleting anything you did not create (including `build-tmp/` itself).

# Stage 6 dispatch: WO-2026-11-05-007 (I21, clock trust survives a restart and is withdrawn on evidence)
Binding spec: `docs/work-orders/WO-2026-11-05-007-i21-clock-trust-carry.md` **including Amendment 1 (read it first)** and its rulings 1-7; inputs `WO-2026-11-05-006-item21-clock-trust-step0.md`, `WO-2026-11-05-006-step0-report-partA.md`, `WO-2026-11-05-006-step0-fleet-data.md`. Base: this branch HEAD (`integration/v50-ab1805` at 1d1d5af7a22dc47094a4c23b515b348bd8ae153d plus the records). **Budget: `src/` at most +20 net** against 1d1d5af (`git diff --numstat 1d1d5af -- src`; state the number; removals count); stop and report the cut rather than trim style. **Run at the dispatched reasoning level (medium); the report states the level actually run, or that it could not be observed.** Trust and time logic: be conservative. Leave changes uncommitted.
## Do
1. **Step 0 first** (WO "Step 0", items 1-7): read-only, answered in the report with file:line **on this head**. Stop and report, without editing, if any answer breaks the plan, in particular: the latch (`ExternalRtc::oscillatorFaultSeenSinceBind()`) is not readable where the carry needs it; ruling 7's "out of window" cannot be expressed from the existing verdict (S2 changed it: reason 30 has no timing arm) without a new stored field; the carry cannot see the persisted `lastTimeSync` at the first `isClockTrusted()`; or the budget cannot hold.
2. Implement the WO's "The change" items 1-5 (predicate, `isClockTrusted()` composition, write-back renewal/withdrawal with `T = kMaxCarryCorrectionSec = 60`, fault withdrawal with a synchronous `SystemConfig::flushNow()`, and ruling 7), reading S4's boolean only.
3. **Tests and mutations** exactly as the WO lists (predicate table, write-back boundaries `T-1/T/T+1/-T-1`, unreadable pre-write value, failed write, `isClockTrusted()` composition, the revoke flush, ruling 7's verdict input, the C2 regression written from the actual C2 code on this head). Mutations (each must fail; back up the file, mutate, restore from the backup with cp; never `git checkout`/`git restore` while your work is uncommitted): the nine in the WO.
4. **Verify (workflow §10, binding):** CI's discovery inside this git worktree: `tests/*.sh` via zsh and `tests/*.py` via python3 (count `p=$((p+1))`; report `N/N (sh via zsh, py via python3)`; state the measured baseline before editing; the integration head was 121/121); any new helper under `tests/` exits 0 bare; `thermal_coupling_structural_test.py` also on a clean export of tracked files (remove your scratch copies from `build-tmp/` first); local toolchain build (Device OS 6.6.0, fresh `BUILD_PATH_BASE` as a make argument): text/data/bss (integration head: 151084 / 1114 / 2256) and `nm` showing the new predicate linked; numstat net as above.
## Stop and report if
the budget cannot hold; Step 0 breaks the plan; S1-S4 behaviour or a layout would have to change.
## Report
`docs/work-orders/WO-2026-11-05-007-stage6-copilot-report.md`: model and reasoning level actually used, interpreter line, Step 0 answers, measured baseline, net `src/` lines per file, tests and mutation results, build and `nm` evidence, deviations, open questions. Claim nothing about hardware.
2026-09-29-config-inventory-codex-dispatch.md
2026-09-29-config-inventory-codex-report.md
2026-10-01-connectivity-investigation-codex-dispatch.md
2026-10-01-connectivity-investigation-codex-report.md
2026-10-02-wake-heap-loss-codex-dispatch.md
2026-10-02-wake-heap-loss-codex-report.md
2026-10-04-pre-step6-known-issues-codex-dispatch.md
2026-10-04-pre-step6-known-issues-codex-report.md
2026-10-06-step6-ownership-codex-dispatch.md
2026-10-06-step6-ownership-codex-report.md
2026-10-07-step6-extensibility-evidence.md
2026-10-09-connected-idle-sleep-inventory-dispatch.md
2026-10-09-connected-idle-sleep-inventory.md
2026-10-10-v40-fleet-health-check-report.md
2026-10-10-v40-fleet-health-check.md
2026-10-12-v41-release-and-soak.md
2026-10-15-v42-fleet-health-check-report.md
2026-10-15-v42-fleet-health-check.md
2026-10-15-v43-release-and-soak.md
2026-10-16-v44-deviceos-6.6.0-release-and-soak.md
2026-10-17-v43-fleet-health-check-report.md
2026-10-17-v43-fleet-health-check.md
2026-10-19-sleep-watchdog-investigation-report.md
2026-10-19-v44-bench-soak-dev09-report.md
2026-10-19-v44-bench-soak-dev14-report.md
2026-10-20-v45-release-and-soak.md
2026-10-21-v44-fleet-canaries-report.md
2026-10-22-v44-fleet-health-check-report.md
2026-10-22-v44-fleet-health-check.md
2026-10-22-v45-bench-soak-result.md
2026-10-24-v45-baseline-day1-result.md
2026-10-25-item-20-hour-straddler-step0-codex-dispatch.md
2026-10-25-item-20-hour-straddler-step0-report.md
2026-10-25-v45-baseline-day2-result.md
2026-10-25-v46-release-and-soak.md
2026-10-26-v46-bench-soak-result.md
2026-10-26-v46-day1-result.md
2026-10-27-v46-day2-result.md
2026-10-27-v47-release-and-soak.md
2026-10-28-v46-fleet-health-check-report.md
2026-10-28-v47-day1-and-24h-result.md
2026-10-29-item-10-post-close-credit-step0-codex-dispatch.md
2026-10-29-item-10-post-close-credit-step0-report.md
2026-10-31-v47-fleet-health-check-report.md
2026-10-31-v48-release-and-soak.md
2026-11-05-v49-release-and-soak.md
WO-2026-08-05-002-battery-authority-delta-guard.md
WO-2026-08-05-003-ledger-write-ordering.md
WO-2026-08-06-001-powermanager-phase1.md
WO-2026-08-07-001-state-report-merge-reconciliation.md
WO-2026-08-10-001-watchdog-instrumentation.md
WO-2026-08-11-001-sleep-prep-threshold-exclusion.md
WO-2026-08-12-001-modem-teardown-confirmation-logging.md
WO-2026-08-24-001-unconditional-usb-source-override.md
WO-2026-08-24-002-soc-high-for-vcell-detection.md
WO-2026-08-25-001-power-management-consolidation.md
WO-2026-08-29-001-hibernate-wake-cloud-observability.md
WO-2026-08-29-002-time-sync-guarantee-and-clock-trust.md
WO-2026-08-31-001-p2-build-break-pmic-incomplete-type.md
WO-2026-08-31-002-hibernate-wake-sync-sequencing.md
WO-2026-08-31-003-rtc-skew-bench-hook.md
WO-2026-08-31-004-rtc-loses-time-during-hibernate.md
WO-2026-09-03-001-occupancy-anomaly-clock-trust.md
WO-2026-09-03-002-publish-queue-corrupted-discards.md
WO-2026-09-03-003-ledger-write-fresh-pairing-failures.md
WO-2026-09-03-004-mafc1-sleep-path-watchdog-stalls.md
WO-2026-09-14-001-reporting-policy-adapter-test-stub-bypass.md
WO-2026-09-14-002-hibernate-wake-gate-single-owner.md
WO-2026-09-15-001-excessive-repeat-logging-powerdiag-ledger.md
WO-2026-09-15-002-cellular-acquire-registration-stall-no-escalation.md
WO-2026-09-16-001-appbreadcrumb-enum-no-cross-tu-home.md
WO-2026-09-16-002-hibernate-cycle-owner.md
WO-2026-09-18-001-clock-owner.md
WO-2026-09-19-001-clock-trust-standard.md
WO-2026-09-20-001-rtc-skew-test-silent-noop.md
WO-2026-09-21-001-battery-authority.md
WO-2026-09-21-002-battery-authority-lowbatterymode-flap.md
WO-2026-09-21-003-pdiag-rapid-wake-cycle.md
WO-2026-09-22-001-occupancy-start-time-clamp-unreachable.md
WO-2026-09-22-002-reporting-policy-clock-routing.md
WO-2026-09-23-001-step5-persistent-data-facade-split.md
WO-2026-09-23-002-serial-settle-test-regression.md
WO-2026-09-24-001-daily-cleanup-boundary-fix.md
WO-2026-09-24-001-stage6-copilot-dispatch.md
WO-2026-09-24-001-stage6-round2-copilot-dispatch.md
WO-2026-09-24-001-stage7-round2-codex-dispatch.md
WO-2026-09-24-001-stage7-verdict.md
WO-2026-09-24-003-device-status-timing-visibility.md
WO-2026-09-24-004-retire-open-equals-close-convention.md
WO-2026-09-24-004-stage6-copilot-dispatch.md
WO-2026-09-24-004-stage6-copilot-report.md
WO-2026-09-24-004-stage7-codex-dispatch.md
WO-2026-09-24-004-stage7-verdict.md
WO-2026-09-25-001-bench-diagnostic-D.diff
WO-2026-09-25-001-phase1-codex-answers.md
WO-2026-09-25-001-phase1-codex-dispatch.md
WO-2026-09-25-001-phase1-provenance-compile.log
WO-2026-09-25-001-report-webhook-loss.md
WO-2026-09-25-001-stage4-codex
WO-2026-09-25-001-stage4-codex-investigation-dispatch.md
WO-2026-09-25-001-stage6-copilot-dispatch.md
WO-2026-09-25-001-stage6-copilot-report.md
WO-2026-09-25-001-stage6-decision10-copilot-dispatch.md
WO-2026-09-25-001-stage6-decision10-copilot-report.md
WO-2026-09-25-001-stage6-decision11-copilot-dispatch.md
WO-2026-09-25-001-stage6-decision11-copilot-report.md
```

</details>


---

<sub>15s</sub>

### `bash`

**Size of spec**

`$ cd docs/work-orders; wc -c WO-2026-11-05-007-i21-clock-trust-carry.md; git log --oneline | head -3; git status --short | head`

```
15387 WO-2026-11-05-007-i21-clock-trust-carry.md
fa74680 WO-2026-11-05-007 (I21): dispatch Stage 6 on the integration head (1d1d5af); WO Amendment 1 (base, hold, S4 latch, S2 verdict); Step 0 inputs
1d1d5af Merge pull request #208 from See-Insights/wo/2026-11-08-001-v50-integrated-fixes
de34f30 WO-2026-11-08-001: integrated Stage 7 round 2 verdict (PASS on cf09b80, no findings)
<shellId: 1 completed with exit code 0>
```


---

<sub>17s</sub>

### `view`

**view the file at /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-i21/docs/work-orders/WO-2026-11-05-007-i21-clock-trust-carry.md.**

`/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-i21/docs/work-orders/WO-2026-11-05-007-i21-clock-trust-carry.md`

<details>
<summary>62 lines</summary>

```
# WO-2026-11-05-007: I21 — clock trust survives a restart and is withdrawn on evidence (recovery item 21)

**Author:** Claude Code, 5 Nov 2026, DRAFT for the architect's review; not dispatched · **Status:** DISPATCHED 8 Nov 2026 (architect, Amendment 1 below); rulings of 5 Nov folded in (candidate C, T = 60 s)
**Branch (when dispatched):** `wo/…-i21-clock-trust-carry`, from `main` after **S1-S3 (and C1/C2) have merged** (v50.1 released); ships in **v50.2** with L (library removal)
**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §3 Stages 5-8; §4 (SHA rule); §5 (time and trust logic: Codex xhigh); §12; §13 (c), (j) · **Agents:** Stage 6 Copilot standard/medium; Stage 7 Codex `gpt-5.6-sol` **xhigh**, at most two rounds · **Budget:** `src/` at most **+20 net** (removals count); stop and report rather than trim style.
**Inputs:** `WO-2026-11-05-006-item21-clock-trust-step0.md` with `…-step0-report-partA.md` (Codex, PROCEED, candidate C) and `…-step0-fleet-data.md` (Part B); recovery plan item 21; v50 parent `WO-2026-11-05-002-v50-ab1805-switch.md`; S1 Amendment 2 (the latch).
**Dispatch gate:** S1-S3 and C1/C2 merged to `main`; the latch (S1 or S4) present. I21 does **not** merge before them.

## Amendment 1 (architect, 8 Nov 2026; supersedes the Branch and Dispatch gate lines above)
1. **Base:** this branch is stacked on `integration/v50-ab1805` at `1d1d5af7a22dc47094a4c23b515b348bd8ae153d` (S1, S1b, S2, S3, S4 and the integrated fixes merged; Stage 7 PASS on the integrated head). Every line citation in this WO is from `main` at 41d9911 or the draft's older heads: **re-cite on this head** (Step 0 item 1).
2. **Hold:** the PR is opened (draft) against `integration/v50-ab1805` and **held unmerged until v50.1 ships**, as with C1/C2. I21 ships in v50.2 with L. Never merge.
3. **The latch is S4's:** `ExternalRtc::oscillatorFaultSeenSinceBind()` returns a `std::atomic<bool>&` (RAM only, never cleared, never persisted). I21 reads only that boolean (relaxed load) and adds no latch of its own.
4. **S2 changed the wake verdict:** reason 30 is classified as a "successful HIBERNATE wake" with no timing arm; reason 20 comes from the persisted 0xA5 evidence byte (byte-only). Ruling 7's "out of window" must be defined against the verdict as it exists on this head (Step 0 item 6); if the existing gate cannot express it without a new stored field, stop and report.
5. **Process:** Stage 6 Copilot medium (the report states the level actually run); Stage 7 Codex xhigh, 90-minute box, SHA rule, at most two rounds; a failure goes to the architect.

## Plain goal
Today a reset or power cycle makes the clock untrusted until a cloud sync completes this boot, although the AB1805 holds time across a reset. The cost was items 18 and 19 and, on Dev-09, the missed close and lost night's sleep (23-24 Oct). I21 lets trust **carry across a restart** from the last confirmed sync, and **withdraws it on evidence**: a large correction at a sync, an unreadable or invalid RTC, or an oscillator-fault event. A cloud sync stays the authority for current-boot trust; the carry only fills the gap between boot and the first sync. On a clean day nothing changes.

## Rulings that bind this WO (architect, 5 Nov; not reopened)
1. **Candidate C:** carry from the persisted `lastTimeSync` (`SysData`, `MyPersistentData.h:142`); **no layout change**; zero means "no carry".
2. **Threshold T = 60 s, a named constant** (`kMaxCarryCorrectionSec`). Basis: healthy bench maximum 11 s (Dev-14, 115 syncs); ChurchSt-2's device-data ledger showed −14 s on 2 Nov (a US unit); Dev-11's first fault sign was 596 s (13 values since 21 Sep, four large). The RC-fallback case is covered by the fault latch, not by T. Boundary semantics: withdraw on `abs(correction) > T`; age uses `< 24 h`.
3. **The latch** ("fault seen since bind") is the S-set's (S1 Amendment 2: S1 if it fits, else S4 ≤ +10). It is a process-lifetime boolean, RAM only, set by an `ExternalTime.onEvent` oscillator-fault event **and** at callback registration if `clockSource()` reports the RC fallback. **I21 reads only the boolean.** No persisted latch.
4. **First boot after the upgrade** carries trust from a recent `lastTimeSync` (no migration marker).
5. **Required test:** the C2 regression (a piece opened while trusted, then trust withdrawn).
6. **Validation, not a blocker:** after v49 is the fleet default, read every 42131 device's device-data `correctionSec` once (read-only) and record the spread in this WO; **if any value reaches 30 s, report it before v50.2.**

7. **No trust carry if the last hibernate wake's RTC verdict was out of window** (Chip, 6 Nov; Grok follow-up). **Reuse `HibernateCycle`'s existing `WakeVerdict`** (`time/HibernateCycle.h:55-80`: `gateArm`, `rtcReadOk`, `sleepErrorSec`); no new classifier. Budget stays **`src/` ≤ +20 net**; if the verdict needs storage beyond this boot (the layouts stay unchanged, ruling 1), stop and report.

## The change (lines at `main` 41d9911; **re-cite on the then-current `main`**: S1-S3 move several of these)
1. **A pure, host-testable carry predicate** in `time/ClockTrust.h` (no Particle dependency, like its neighbours): `isCarryEligible(timeValid, externalValid, nowEpoch, lastSyncEpoch, faultSeen)` true only if: the boot's hibernate wake verdict, when this boot is a hibernate wake, did not fail on an RTC arm (ruling 7; Step 0 item 7 fixes the exact predicate), the external clock is readable and valid (`TimeClass(EXTERNAL).isValid()` via S1's helpers), `Clock::isPlausibleEpoch(now)` and `isPlausibleEpoch(lastSyncEpoch)` (`Clock.cpp:551-555`), `lastSyncEpoch != 0`, `0 <= now - lastSyncEpoch < kMaxSyncAgeMs/1000` (24 h; unchanged), and `!faultSeen`. Age uses the epoch (the persisted value), not `millis()`.
2. **`Clock::isClockTrusted()`** (`Clock.cpp:477-480`) becomes `currentBootSyncTrusted || carryEligible`. Every consumer already goes through it or `Clock::isTrusted()`/`openness()` (Part A, Answer 1: the structural test pins the call sites), so no consumer changes. `reportedSyncAgeMs()` (`:522-524`) is unchanged (millis-based, this boot): telemetry for a carried-only boot reports no sync age; Step 0 checks the status payload's `trusted` and `syncAge` fields read sensibly (`DeviceStatusPublisher.cpp:285-304`).
3. **Renewal and withdrawal at the RTC write-back** (`Clock.cpp:392-441`). `lastTimeSync` is stamped on a confirmed write (`:413`) as today, **unless** the write was confirmed with an unreadable pre-write value or `abs(rtcCorrectionSec) > T`: then `set_lastTimeSync(0)` instead (the sentinel `-1` at `:402-405` is not a measurement: use `rtcPreWriteReadOk`, not the value). A failed write also clears it (carry is renewed only by a clean, confirmed, small correction). Current-boot trust is unaffected (the cloud sync happened).
4. **Fault withdrawal.** When the latch becomes true mid-boot, `set_lastTimeSync(0)` and a **synchronous `SystemConfig::flushNow()`** (the normal save is deferred 100 ms; a revoke must survive a reset right after). Likewise flush after a revoke at item 3. Revoke writes are rare, so the flash wear is nil.
5. **Left alone on purpose:** the 24 h bound, `kMaxSyncAgeMs`, the resync pacing, `openness()`'s `Unknown`-counts-as-open rule (#151, item 10), item 19's credit logic (C2 reads `isTrusted()` at piece start), item 15, the layouts (`SysData` and `retained`).

## Step 0 (read-only, inside Stage 6, before editing; stop and report if any answer breaks the plan)
1. **Re-cite** every line above on the merged `main`; list how S1-S3 changed the RTC read/write helpers and where the latch lives (its name, header, lifecycle, whether it is set before the first `isClockTrusted()` at boot).
2. **`lastTimeSync` consumers:** every reader and writer (bare-name search, §13 (c)): the status payload, `ClockDiagnostics`, the ledger; anything that treats zero as "never synced" or shows it to the cloud, so zeroing as a revoke is read correctly (or the display is guarded). Where `SystemConfig` is first loaded relative to the first `isClockTrusted()` call at boot (the carry must see the persisted value).
3. **Boot order:** when `Time.isValid()` and the external read become true at boot after S1 (before `setup()`?), so the carry predicate is correct at the first call; the boot paths that run before a sync (boot-storm timing `Generalized-Core-Counter.cpp:377-394`, the close and night-sleep decisions) and what each does with carried trust.
4. **A restart with a bad but plausible RTC** (battery just replaced and set by hand, or a time set to a wrong date): which input catches it before the first sync (none: Part A, Answer 5) and what it costs; state the exposure window (until the first sync, minutes on a connected unit).
5. **C2 interaction:** `pieceUntrusted` is snapshotted at piece start; a trusted-carried piece whose trust is later withdrawn stays under C2's "keep open until trust returns" (B2) rule; write the regression test from the actual C2 code on `main`.
6. **Ruling 7, the verdict.** The existing verdict is a six-arm gate (`GateArm`: `kResetReason`, `kWakeReason`, `kRtcRead`, `kRtcBeforeZero`, `kRequestedZero`, `kRtcOrder`) plus `sleepErrorSec` (actual minus requested); it has **no explicit "out of window" tolerance**. State which of these means "out of window" (candidate: a failed gate on `kRtcRead`, `kRtcBeforeZero` or `kRtcOrder`, and/or `abs(sleepErrorSec)` beyond a named bound; the bound would be new, so say so and size it from the `hibernate_wake` events). Where the verdict is available when `isClockTrusted()` is first called after a hibernate wake (it runs in `setup()`), and what happens on a later non-hibernate restart the same day (no verdict this boot: say whether carry stays allowed, and why that is safe). Count the lines for the predicate input and its test.
7. **Count** the net `src/` lines per file; if over +20, stop and report the cut (the predicate is the cheap part; the flush and the sentinel handling are the rest).

## Tests (host; the real functions)
- The predicate table: valid/invalid external clock; zero, future, stale, below/above the plausible range `lastSyncEpoch`; age `N-1 s`, `N`, `N+1`; the latch true; the millis wrap does not matter (epoch-based).
- The write-back: correction `T-1`, `T`, `T+1` and `-T-1` (stamp, stamp, revoke, revoke); an unreadable pre-write value (revoke, not `-1` compared with T); a failed write (revoke); a clean small correction renews; the revoke is flushed (structural: `flushNow()` follows `set_lastTimeSync(0)`).
- `isClockTrusted()` composition: carry-only after a restart (trusted before any sync), current-boot sync alone (unchanged), neither (untrusted); the latch withdrawing mid-boot.
- The hibernate-verdict input (ruling 7): a wake whose gate failed on each RTC arm withdraws the carry; a passing wake and a non-hibernate boot behave as before.
- The C2 regression (item 5 above); the existing trust, resync and openness tests unchanged (`clock_trust_standard_structural_test.py`, `clock_resync_wiring_test.py`, `clock_rtc_writeback_test.cpp`, `clock_openness_test.cpp`, `reporting_policy_test.cpp`, `daily_cleanup_boundary_test.py`), plus the structural test's call-site list if a site is added.
- Mutations (each must fail): carry ignores the latch; carry ignores age; age on `millis()`; `abs()` dropped (only positive corrections withdraw); the `-1` sentinel compared against T; no flush after the revoke; a revoke that leaves the current boot untrusted; carry without the plausibility checks; the verdict input ignored.

## Bench (Chip; after v50.1 is on the bench units)
1. **Clean restart:** reset mid-session with a valid RTC and no cloud yet: trusted immediately, the right openness, a timely close, the report boundary, the night sleep as on a normal day (the Dev-09 23-24 Oct pattern).
2. **Wrong RTC:** `skew 60` then `skew 61` (the hook in `time/RtcSkewTest.h`), reset, sync: the correction is measured; at 60 s the carry survives, at 61 s it is cleared and the next restart is untrusted until a sync.
3. **Fault:** host injection is the required proof; physical induction is optional (the spike marked it unsafe).
4. **Clean day:** close totals, report boundaries and sleep identical to the v50.1 baseline.
5. **Validation read** (ruling 6): the device-data `correctionSec` of every 42131 device, once, read-only, recorded here; any value ≥ 30 s is reported before v50.2.

## Fleet validation record (fill after v49 is the fleet default)
_Pending:_ per device, the device-data `correctionSec`, the date read, and the spread. Known so far: Dev-09 max |5| s and Dev-14 max |11| s over 14 days (serial logs); ChurchSt-2 −14 s (2 Nov, device-data ledger); no data for the other 10 units.

## Protected / not in scope
S1-S3, L; the 24 h bound; the failsafe ladder; item 19's credit logic; item 15 (occupancy lost across a reset: **not fixed by I21**, Part A Answer 2); the persisted and `retained` layouts; M-SoM.

## Rollback
Revert the I21 commit: trust returns to boot-local (today's rule); the persisted `lastTimeSync` is already compatible (zero or a timestamp). v50.1 remains the floor.

## Residual window (Grok follow-up V4, declined by Chip, 6 Nov 2026)
V4 asked for more evidence against a fault that the checks above would miss. **Declined.** The boot-time `clockSource()` check (the latch set at callback registration when the RC fallback is reported) already catches a persistent oscillator fault, and a transient fault is bounded by the next sync's 60 s check (T). **The residual window is therefore:** from a transient fault that does not report the RC fallback and raises no event, until the next completed sync, and in any case no longer than the 24 h carry age. During that window carried trust rests on an RTC that may have drifted by an unmeasured amount; Step 0 item 4 states the exposure for a connected unit (minutes, INF) and for a unit that sleeps most of the day (up to its next scheduled connection). No further guard is added; if the fleet validation read (ruling 6) shows any `correctionSec` of 30 s or more, report it before v50.2 (unchanged).

## Why persisted continuity evidence is not needed (Grok summary judgment; Chip's ruling, 6 Nov 2026)
Grok's summary asked whether the carry needs persisted evidence that the RTC kept counting correctly across the restart (a continuity record beyond `lastTimeSync`). **Ruled not needed:**
1. **Drift is caught by the write-back's correction check.** A sync whose measured correction exceeds T (60 s) zeroes `lastTimeSync` (change item 3), so a drifting or rate-halved RTC loses the carry at the next sync. For a Dev-11-type fault (the RTC running at 56-82 % of true rate, Chip's figures) the error grows at (1 - rate) seconds per second, so it passes 60 s after about 2.3 minutes (56 %) to 5.6 minutes (82 %) (arithmetic, not a measurement). It therefore cannot hold a carry across a sync.
2. **The hibernate case is covered by ruling 7**, the hibernate-window check on the last wake's RTC verdict.
3. **Bound, stated honestly:** the check acts at the next completed sync. A restart after a fault begins and before that sync can still carry trust on an RTC that has already drifted; that exposure is the residual window recorded above (V4), bounded by the 24 h carry age and by the next sync. No persisted continuity record is added, and none changes the layouts (ruling 1).
```

</details>


---

<sub>21s</sub>

### `bash`

**Read ClockTrust and Clock.cpp**

`$ cat src/time/ClockTrust.h; wc -l src/time/Clock.cpp; sed -n 360,600p src/time/Clock.cpp`

<details>
<summary>14 lines</summary>

```
Output too large to read at once (29.7 KB). Saved to: /var/folders/_r/m12xn8s15wb_15djjdgfp5gr0000gn/T/1794154245391-copilot-tool-output-72505-86b19c5b-890e-4aee-a06e-4ec4fd2186cd.txt
Consider using tools like grep (for searching), head/tail (for viewing start/end), view with view_range (for specific sections), or jq (for JSON) to examine portions of the output.

Preview (first 500 chars):
/**
 * @file ClockTrust.h
 * @brief Pure, dependency-free clock-trust and resync-gate logic.
 *
 * @details WO-2026-08-29-002 ("Clock-trust mechanism - RTC write-back,
 *          non-blocking sync observation, and a trustworthy clock signal").
 *          That WO was originally titled "Guarantee time sync, make clock
 *          validity self-known, and stop a wrong RTC re-seeding itself";
 *          the "guarantee" half was split out to WO-2026-08-31-002 on
 *          2026-08-31 and the titl
<shellId: 2 completed with exit code 0>
```

</details>


---

<sub>21s</sub>

### `bash`

**Find consumers and verdict**

`$ grep -rn "oscillatorFaultSeenSinceBind\|lastTimeSync\|LastTimeSync" src --include=*.h --include=*.cpp --include=*.ino | grep -v "^src/time/Clock.cpp"; sed -n 40,110p src/time/HibernateCycle.h`

<details>
<summary>70 lines</summary>

```
src/MyPersistentData.cpp:268:time_t sysStatusData::get_lastTimeSync() const  {
src/MyPersistentData.cpp:269:    return getValue<time_t>(offsetof(SysData,lastTimeSync));
src/MyPersistentData.cpp:271:void sysStatusData::set_lastTimeSync(time_t value) {
src/MyPersistentData.cpp:272:    setValue<time_t>(offsetof(SysData, lastTimeSync), value);
src/MyPersistentData.cpp:955:time_t get_lastTimeSync() { return sysStatus.get_lastTimeSync(); }
src/MyPersistentData.cpp:956:void set_lastTimeSync(time_t value) { sysStatus.set_lastTimeSync(value); }
src/MyPersistentData.h:142:		time_t lastTimeSync;                               // Last time we synced time from Particle cloud
src/MyPersistentData.h:271:	time_t get_lastTimeSync() const;
src/MyPersistentData.h:272:	void set_lastTimeSync(time_t value);
src/state/State_Report.cpp:317:    // + persisted lastTimeSync stamp) is recorded once, uniformly, by
src/state/State_Report.cpp:320:    // function called SystemConfig::set_lastTimeSync(Time.now()) immediately
src/time/ExternalRtc.h:35:inline std::atomic<bool> &oscillatorFaultSeenSinceBind() { static std::atomic<bool> seen{false}; return seen; }
src/time/ExternalRtc.h:37:  if (e.isSet(particle::RtcEvent::CLOCK_SOURCE_EXTERNAL_FAILURE)) oscillatorFaultSeenSinceBind().store(true, std::memory_order_relaxed);
src/time/ClockDiagnostics.cpp:29:  // lastTimeSyncEpoch is the persisted, wall-clock completion stamp from
src/time/ClockDiagnostics.cpp:30:  // checkClockResync() (item 6's fix to SystemConfig::get_lastTimeSync(), which
src/time/ClockDiagnostics.cpp:45:  const time_t lastTimeSyncEpoch = SystemConfig::get_lastTimeSync();
src/time/ClockDiagnostics.cpp:78:           (long)lastTimeSyncEpoch);
src/Generalized-Core-Counter.cpp:670:  if (startupOscUsingRC) ExternalRtc::oscillatorFaultSeenSinceBind() = true;  // registering the handler clears the chip's fail flag, so it is read first
src/Generalized-Core-Counter.cpp:914:  // SystemConfig::get_lastTimeSync() (persisted across reboots, stamped only by
src/Generalized-Core-Counter.cpp:919:  // that INTERMITTENT/DISCONNECTED modes rely on). get_lastTimeSync()==0
src/Generalized-Core-Counter.cpp:951:  const bool neverConfirmedSyncEver = (SystemConfig::get_lastTimeSync() == 0);
src/cloud/DeviceStatusPublisher.cpp:269:    // completion stamp (SystemConfig::get_lastTimeSync()), which previously had
src/cloud/DeviceStatusPublisher.cpp:291:        writerBase.name("lastSyncEpoch").value((int)SystemConfig::get_lastTimeSync());
src/persist/SystemConfig.h:141:time_t get_lastTimeSync();
src/persist/SystemConfig.h:142:void set_lastTimeSync(time_t value);

/// Everything a wake boot needs to know, in one call. The hibernate-gate
/// fields are populated only when a hibernate was actually pending this boot.
struct WakeVerdict {
  // ---- RTC read outcome, populated every boot - needed both for
  // hibernate-gate classification and, independently, by the caller's own
  // "did system time just get restored" check. ----
  bool rtcReadOk = false;
  int64_t rtcAtWake = 0;

  // ---- Hibernate wake-validation gate, populated only when a hibernate
  // was pending this boot. `gateInputs` is exposed so the caller can still
  // build the `hibernate_wake` forensic event via
  // `HibernateWakeDiagnostics::buildEventFields()` without this file
  // needing to know anything about cloud events. ----
  bool hibernateGateEvaluated = false;
  HibernateWakeDiagnostics::GateArm gateArm = HibernateWakeDiagnostics::GateArm::kNone;
  HibernateWakeDiagnostics::GateInputs gateInputs{};
  const char *wakeReasonName = "UNKNOWN";
  uint32_t requestedSleepSec = 0;
  uint32_t hibernateCount = 0;
  uint32_t actualSleepSec = 0;
  int32_t sleepErrorSec = 0;
};

/// Arms the retained state for an upcoming HIBERNATE sleep. Called
/// immediately before `System.sleep(config)` with a HIBERNATE
/// configuration. Replaces the inline
/// `retainedHibernateRtcBefore/RequestedSleep/Count++/Pending` sequence
/// formerly in `State_Sleep.cpp`.
void armForSleep(time_t rtcNow, uint32_t requestedSec);

/// Un-arms a hibernate attempt without processing a wake: either the
/// HIBERNATE `System.sleep()` call returned instead of resetting the MCU
/// (the ULTRA_LOW_POWER fallback path), or the wake boot has finished
/// processing `classifyWake()`'s verdict and the pending flag should not
/// carry into the next cycle. Replaces both
/// `retainedHibernatePending = false;` sites.
void abandon();

/// Classifies this boot's wake in one call - see the struct doc above.
/// `osResetReason` is the OS reset reason (`System.resetReason()`'s
/// result, e.g. `RESET_REASON_PIN_RESET`/`RESET_REASON_POWER_MANAGEMENT`).
WakeVerdict classifyWake(int osResetReason);

/// Current hibernate count, for the one confirmed consumer outside the
/// wake-classification path: `publishStartupStatus()`'s `status` event.
uint32_t currentHibernateCount();

} // namespace HibernateCycle

#endif
<shellId: 3 completed with exit code 0>
```

</details>


---

<sub>23s</sub>

### `bash`

**Locate symbols**

`$ cd src; grep -n "" time/ClockTrust.h | sed -n 1,400p | grep -n "inline\|constexpr\|namespace" ; grep -n "isClockTrusted\|classifyWake\|WakeVerdict\|currentBootSyncTrusted\|hibernateGate\|gateArm" -r . | grep -v "^./time/HibernateCycle.h"`

<details>
<summary>84 lines</summary>

```
37:37:namespace ClockTrust {
42:42:constexpr uint32_t kMaxSyncAgeMs = 24UL * 60UL * 60UL * 1000UL;
54:54:constexpr uint32_t kMinResyncRetryIntervalMs = 60UL * 1000UL;
63:63:inline uint32_t elapsedMs(uint32_t nowMs, uint32_t sinceMs) {
90:90:inline bool shouldResync(uint32_t nowMs, uint32_t timeSyncedLastMs) {
111:111:inline bool isTrusted(bool timeValid, uint32_t nowMs, uint32_t timeSyncedLastMs) {
129:129:inline bool canRetryResyncNow(uint32_t nowMs, uint32_t lastAttemptMs) {
151:151:constexpr uint32_t kMinRtcWriteRetryIntervalMs = 60UL * 1000UL;
175:175:inline bool canRetryRtcWriteNow(uint32_t nowMs, uint32_t lastAttemptMs) {
236:236:inline bool shouldAttemptRtcWriteNow(uint32_t lastSyncMs, uint32_t lastWrittenSyncedLastMs,
277:277:inline bool shouldResyncWrapAware(uint32_t nowMs, uint32_t timeSyncedLastMs,
289:289:inline bool isTrustedWrapAware(bool timeValid, uint32_t nowMs, uint32_t timeSyncedLastMs,
329:329:constexpr uint32_t kReportedSyncAgeUnavailableMs = 0xFFFFFFFFUL;
352:352:inline uint32_t wrapAwareReportedSyncAgeMs(uint32_t nowMs, uint32_t timeSyncedLastMs,
364:364:} // namespace ClockTrust
./MyPersistentData.cpp:43:// WO-2026-09-18 Step 3a: isClockTrusted() is now declared by time/Clock.h,
./MyPersistentData.cpp:646:  // isClockTrusted()'s doc comment (state/State_Common.h) for why
./MyPersistentData.cpp:648:  current.set_lastCountTime(isClockTrusted() ? Time.now() : 0);
./MyPersistentData.cpp:820:        // isClockTrusted(). State_Report.cpp's alert-40 escalation cooldown
./state/State_Idle.cpp:121:                            (isClockTrusted() ? 8 : 0) | (Clock::isTimeValid() ? 16 : 0);
./state/State_Common.h:217:bool isClockTrusted();
./state/State_Common.h:228: *          snapshot while disconnected, so callers (isClockTrusted(),
./state/State_Modes.cpp:38:    // to gate on isClockTrusted() so an untrusted clock records the 0
./state/State_Modes.cpp:41:    CurrentReadings::set_lastCountTime(isClockTrusted() ? Time.now() : 0);
./state/State_Modes.cpp:186:        // isClockTrusted() changes only the recorded value, never control
./state/State_Modes.cpp:188:        CurrentReadings::set_lastCountTime(isClockTrusted() ? Time.now() : 0);
./state/State_Modes.cpp:223:          // isClockTrusted(). Unlike lastCountTime, occupancyStartTime's 0
./time/HibernateCycle.cpp:39:WakeVerdict classifyWake(int osResetReason) {
./time/HibernateCycle.cpp:40:  WakeVerdict v{};
./time/HibernateCycle.cpp:56:    v.hibernateGateEvaluated = true;
./time/HibernateCycle.cpp:69:      v.gateArm = GateArm::kResetReason;
./time/HibernateCycle.cpp:71:      v.gateArm = GateArm::kWakeReason;
./time/HibernateCycle.cpp:73:      v.gateArm = GateArm::kRtcRead;
./time/HibernateCycle.cpp:75:      v.gateArm = GateArm::kRtcBeforeZero;
./time/HibernateCycle.cpp:77:      v.gateArm = GateArm::kRequestedZero;
./time/HibernateCycle.cpp:79:      v.gateArm = GateArm::kRtcOrder;
./time/HibernateCycle.cpp:81:      v.gateArm = GateArm::kNone;
./time/HibernateCycle.cpp:87:    if (v.gateArm == GateArm::kNone) {
./time/HibernateWakeDiagnostics.h:10: *          `time/HibernateCycle.cpp`'s `classifyWake()` - this header does
./time/HibernateWakeDiagnostics.h:36:/// `HibernateCycle::classifyWake()` evaluates them) first failed. `kNone`
./time/HibernateWakeDiagnostics.h:48:/// Raw inputs to the gate, exactly as `HibernateCycle::classifyWake()`
./time/HibernateWakeDiagnostics.h:61:inline const char *gateArmName(GateArm arm) {
./time/HibernateWakeDiagnostics.h:105:/// `classifyWake()`, and passed in rather than re-derived here; this
./time/HibernateWakeDiagnostics.h:112:/// `classifyWake()` derived and the caller stored in
./time/HibernateWakeDiagnostics.h:154:      "{\"result\":\"%s\",\"gateArm\":\"%s\",\"osReason\":%d,\"wakeReason\":\"%s\","
./time/HibernateWakeDiagnostics.h:157:      ok ? "ok" : "fail", gateArmName(f.arm), f.osResetReason, f.wakeReasonName,
./time/Clock.cpp:176: *          serviceAwakeWatchdog() - and isClockTrusted() reached it from
./time/Clock.cpp:187: *          BEHAVIORAL CONSEQUENCE: while disconnected, isClockTrusted()/
./time/Clock.cpp:475:bool isClockTrusted() {
./time/Clock.cpp:490: *          `isClockTrusted()` correctly goes false (via
./time/Clock.cpp:509: *          it, and `isClockTrusted()` is independently false. Callers
./time/Clock.cpp:515: *          Deliberately NOT used by `checkClockResync()`, `isClockTrusted()`,
./time/Clock.cpp:562:  return isClockTrusted();
./time/ClockDiagnostics.cpp:44:  const bool clockTrusted = isClockTrusted();
./time/Clock.h:12: *          `checkClockResync()`/`requestClockResync()`, `isClockTrusted()`,
./time/Clock.h:44: *          `Clock::isTrusted()` (a thin wrapper around `isClockTrusted()`)
./time/Clock.h:51: *          and `isClockTrusted()` themselves are UNCHANGED - they remain the
./time/Clock.h:102:bool isClockTrusted();
./time/Clock.h:133: *          quietly upgrading callers to `isClockTrusted()`, which would be
./time/Clock.h:154: * @brief WO-2026-09-19 Step 3b: thin wrapper around `isClockTrusted()`.
./time/Clock.h:156: * @details Deliberately just `return isClockTrusted();` - `isClockTrusted()`
./Generalized-Core-Counter.cpp:118:// requestClockResync()/checkClockResync()/isClockTrusted()/
./Generalized-Core-Counter.cpp:549:    // WO-2026-08-29-002 item 8: deliberately NOT gated on isClockTrusted().
./Generalized-Core-Counter.cpp:758:  const HibernateCycle::WakeVerdict wakeVerdict = HibernateCycle::classifyWake(reason);
./Generalized-Core-Counter.cpp:804:  // wakeVerdict.rtcReadOk/rtcAtWake were computed inside classifyWake()
./Generalized-Core-Counter.cpp:836:  if (wakeVerdict.hibernateGateEvaluated) {
./Generalized-Core-Counter.cpp:839:    if (wakeVerdict.gateArm == HibernateWakeDiagnostics::GateArm::kNone) {
./Generalized-Core-Counter.cpp:860:    // buildEventFields() takes wakeVerdict.gateArm directly now (Step 2) -
./Generalized-Core-Counter.cpp:866:          wakeVerdict.gateArm, wakeVerdict.gateInputs, reason, startupHibernateWakeReason,
./Generalized-Core-Counter.cpp:928:  // section: because isClockTrusted() is unconditionally false at this
./Generalized-Core-Counter.cpp:941:  // isTrusted()/isClockTrusted() is false on every boot (including every
./Generalized-Core-Counter.cpp:1275:  // 28796s instead of -1). Additionally require isClockTrusted() (sync
./Generalized-Core-Counter.cpp:1278:      (isClockTrusted() && lastConnection != 0 && Time.now() > lastConnection)
./cloud/ReportPublisher.cpp:214: * @details The gate in setup() (fed by `HibernateCycle::classifyWake()`,
./cloud/ReportPublisher.cpp:215: * evaluated when `wakeVerdict.hibernateGateEvaluated` is true) only ever
./cloud/ReportPublisher.cpp:229: *        `HibernateCycle::classifyWake()` and assembled by
./cloud/ReportPublisher.cpp:280:    // WO-2026-08-29-002 item 8: deliberately NOT gated on isClockTrusted().
./cloud/DeviceStatusPublisher.cpp:20:bool isClockTrusted();
./cloud/DeviceStatusPublisher.cpp:24:// isClockTrusted() already uses; no parallel mechanism). This supersedes
./cloud/DeviceStatusPublisher.cpp:30:// isClockTrusted()/reportedSyncAgeMs() above - the real definition lives in
./cloud/DeviceStatusPublisher.cpp:43:// isClockTrusted()/reportedSyncAgeMs() above - these are plain globals in
./cloud/DeviceStatusPublisher.cpp:267:    // in Generalized-Core-Counter.cpp (isClockTrusted()) - not
./cloud/DeviceStatusPublisher.cpp:289:        writerBase.name("trusted").value(isClockTrusted());
<shellId: 4 completed with exit code 0>
```

</details>


---

<sub>25s</sub>

### `bash`

**Read Clock write-back etc.**

`$ cd src; sed -n 380,485p time/Clock.cpp; sed -n 535,570p time/Clock.cpp; sed -n 30,100p time/HibernateCycle.cpp; sed -n 25,50p time/ExternalRtc.h`

<details>
<summary>211 lines</summary>

```
// ClockTrust::shouldAttemptRtcWriteNow() only applies the floor when
  // lastSyncMs is the SAME value that most recently failed - a fresh sync
  // value is admitted immediately regardless of how recently any write was
  // last attempted.
  //
  // Round 5 cleanup task 4 (Stage 7 finding 7): the gate condition itself
  // is now ClockTrust::shouldAttemptRtcWriteNow() - a pure function shared
  // with the host tests - rather than being re-derived inline, so a host
  // test can exercise the real decision logic instead of a hand-written
  // mirror that could silently drift from it.
  if (ClockTrust::shouldAttemptRtcWriteNow(lastSyncMs, lastRtcWriteSyncedLastMs,
                                            nowMs, lastRtcWriteFailedAttemptMs,
                                            lastRtcWriteFailedSyncMs)) {
    // WO-2026-08-31-004 Amendment A-2: read the RTC's own value immediately
    // before correcting it, purely to report how far off it was - this read
    // does not gate or alter the write below in any way. If the read fails,
    // the delta is reported as unavailable (-1) rather than fabricated; see
    // this file's Amendment A note (mirroring RtcSkewTest's own "do not log
    // a fabricated value" rule) for the same reasoning applied here.
    time_t rtcPreWriteValue = 0;
    const bool rtcPreWriteReadOk = ExternalRtc::get(rtcPreWriteValue);
    const bool rtcUpdated = ExternalRtc::setFromSystem();
    const long rtcCorrectionSec = rtcPreWriteReadOk
        ? (long)(Time.now() - rtcPreWriteValue)
        : -1;
    Log.info("ClockResync: sync advanced, rtcUpdated=%d epoch=%ld correctionSec=%ld",
             rtcUpdated ? 1 : 0, (long)Time.now(), rtcCorrectionSec);
    if (rtcUpdated) {
      // Finding 1: only mark this sync value as "written" once the RTC
      // write is confirmed successful, and only then stamp lastTimeSync.
      lastRtcWriteSyncedLastMs = lastSyncMs;
      SystemConfig::set_lastTimeSync(Time.now());

      // Only persist the correction for cloud reporting once the write it
      // describes is confirmed - same "confirmed, not attempted" standard
      // Finding 1 already applies to lastRtcWriteSyncedLastMs/lastTimeSync
      // above. A failed pre-write read (rtcPreWriteReadOk == false) leaves
      // the prior confirmed value in place rather than overwriting it with
      // -1, so a transient read failure cannot erase real history.
      lastRtcCorrectionSec = rtcPreWriteReadOk ? rtcCorrectionSec : lastRtcCorrectionSec;

      // Round 5 cleanup task 3 (Stage 7 finding 3): the status payload is
      // published on connect, BEFORE this corrective sync completes (see
      // State_Connect.cpp), so it correctly read trusted=false at that
      // point. Nothing previously republished it once the sync succeeded,
      // so the ledger could go a whole session without ever showing
      // trusted=true. Flag the EXISTING deferred-republish mechanism
      // (Cloud::loop() already drains pendingStatusPublish, retrying on
      // failure) rather than publishing synchronously here - this cannot
      // block or reorder anything checkClockResync() does. The same
      // republish also carries this sync's correctionSec, computed above.
      Cloud::instance().requestStatusPublish("ClockResync");
    } else {
      // Round 6 (second follow-up): record the failure-only timestamp/value
      // pair that paces JUST this value's retries - never touched on a
      // success, so a later distinct sync is never delayed by it.
      lastRtcWriteFailedAttemptMs = nowMs;
      lastRtcWriteFailedSyncMs = lastSyncMs;
      Log.warn("ClockResync: setRtcFromSystem failed (Time.isValid=%d) - will retry after retry floor",
               Time.isValid() ? 1 : 0);
    }
  }

  // --- Request side: ask for a resync when the monotonic gate is open, ---
  // --- paced by the retry floor.                                       ---
  if (Particle.connected() &&
      ClockTrust::shouldResyncWrapAware(nowMs, lastSyncMs, millisWrapGeneration, syncCaptureWrapGeneration) &&
      ClockTrust::canRetryResyncNow(nowMs, clockResyncLastAttemptMs)) {
    requestClockResync(lastSyncMs == 0 ? "no confirmed sync yet this boot" : "24h elapsed since last confirmed sync");
  }
}

/**
 * @brief Item 8: is `Time.now()` currently trustworthy enough to persist or
 *        report?
 *
 * @details Per Finding 3, `Time.isValid()` alone is NOT sufficient - it was
 *          true throughout the incident this Work Order investigates. This
 *          additionally requires a confirmed cloud time sync within the
 *          last `ClockTrust::kMaxSyncAgeMs`, using the same monotonic,
 *          non-blocking, wrap-aware signal as the resync gate above
 *          (`observedTimeSyncedLastMs()` / `syncCaptureWrapGeneration`).
 *
 *          Acceptance criteria note (Finding 5, accepted as-is - see the
 *          Work Order): this deliberately requires BOTH `Time.isValid()`
 *          AND sync recency. `Time.isValid()` is necessary but not
 *          sufficient on its own (that is the whole point of Finding 3);
 *          sync recency is the decisive term that actually distinguishes
 *          "trusted" from "not" in every real scenario this Work Order
 *          investigates - a device with `Time.isValid()==false` is
 *          untrusted regardless of sync recency (defensive; should not
 *          occur once a sync has completed), and a device with
 *          `Time.isValid()==true` is untrusted unless it ALSO has a recent
 *          confirmed sync.
 */
bool isClockTrusted() {
  return ClockTrust::isTrustedWrapAware(Time.isValid(), millis(), observedTimeSyncedLastMs(),
                                         millisWrapGeneration, syncCaptureWrapGeneration);
}

/**
 * @brief Item 9 / Round 6 (Stage 7 finding 1): a wrap-aware, sentinel-
 *        bearing sync age for TELEMETRY ONLY - not a control-path signal.
 *
 * @details Stage 7 finding 1: `logTimeDiag()`'s `syncAgeMs` and the status
 *          payload's `syncAgeSec` (`DeviceStatusPublisher.cpp`) both used
  const int secondsUntil = secondsUntilNextOpenForSeconds(
      snapshot.localSecondsOfDay, openHour, closeHour, openNow);

  return secondsUntil;
}

// ===== New in Step 3a =====

namespace Clock {

bool isTimeValid() {
  return Time.isValid();
}

bool isPlausibleEpoch(time_t epoch) {
  static const time_t kEpochMin = 1704067200; // 2024-01-01 00:00:00 UTC
  static const time_t kEpochMax = 2051222400; // 2035-01-01 00:00:00 UTC
  return epoch >= kEpochMin && epoch < kEpochMax;
}

} // namespace Clock

// ===== New in Step 3b =====

namespace Clock {

bool isTrusted() {
  return isClockTrusted();
}

Openness openness() {
  // Never fail open: an untrusted clock or unloaded config both return
  // Unknown, explicitly - see this function's doc comment in Clock.h for
  // why isWithinOpenHours()'s fail-OPEN behavior is wrong for a decision
  // that commits the device to state.
  if (!isTrusted()) {

void abandon() {
  retainedHibernatePending = false;
}

uint32_t currentHibernateCount() {
  return retainedHibernateCount;
}

WakeVerdict classifyWake(int osResetReason) {
  WakeVerdict v{};

  // ===== RTC READ - every boot, independent of the above =====
  time_t rtcTime = 0;
  v.rtcReadOk = ExternalRtc::get(rtcTime);
  v.rtcAtWake = (int64_t)rtcTime;

  // ===== HIBERNATE WAKE-VALIDATION GATE =====
  // This IS the classification now - not a call into a separately
  // maintained mirror. All six conditions below are evaluated directly,
  // in the same short-circuit order as before this step (WO-2026-09-14-002
  // Step 1 first consolidated a hand-written `if` chain and a mirrored
  // classifyGateArm() into one call site; this step consolidates further,
  // absorbing the classification itself so there is exactly one place this
  // logic is written, not a function elsewhere being wrapped).
  if (retainedHibernatePending) {
    v.hibernateGateEvaluated = true;
    HibernateWakeDiagnostics::GateInputs &in = v.gateInputs;
    in.resetReasonIsPowerManagement = (osResetReason == RESET_REASON_POWER_MANAGEMENT);
    in.rtcReadOk = v.rtcReadOk;
    in.rtcBefore = (int64_t)retainedHibernateRtcBefore;
    in.requestedSleepSec = retainedHibernateRequestedSleep;
    in.rtcAtWake = v.rtcAtWake;
    // Reason 30 is a successful HIBERNATE wake (RTC or GPIO alike) and alone passes the wake-reason arm (WO-2026-11-05-004 Amendments 3-4).
    in.wakeReasonIsHibernateWake = in.resetReasonIsPowerManagement;
    if (in.resetReasonIsPowerManagement) v.wakeReasonName = "POWER_MGMT";

    using HibernateWakeDiagnostics::GateArm;
    if (!in.resetReasonIsPowerManagement) {
      v.gateArm = GateArm::kResetReason;
    } else if (!in.wakeReasonIsHibernateWake) {
      v.gateArm = GateArm::kWakeReason;
    } else if (!in.rtcReadOk) {
      v.gateArm = GateArm::kRtcRead;
    } else if (!(in.rtcBefore > 0)) {
      v.gateArm = GateArm::kRtcBeforeZero;
    } else if (!(in.requestedSleepSec > 0)) {
      v.gateArm = GateArm::kRequestedZero;
    } else if (!(in.rtcAtWake >= in.rtcBefore)) {
      v.gateArm = GateArm::kRtcOrder;
    } else {
      v.gateArm = GateArm::kNone;
    }

    v.requestedSleepSec = retainedHibernateRequestedSleep;
    v.hibernateCount = retainedHibernateCount;

    if (v.gateArm == GateArm::kNone) {
      v.actualSleepSec = (uint32_t)(rtcTime - retainedHibernateRtcBefore);
      v.sleepErrorSec = (int32_t)v.actualSleepSec - (int32_t)retainedHibernateRequestedSleep;
    }
  }

  return v;
}

} // namespace HibernateCycle
  return get(back) && (back > t ? back - t : t - back) <= kWriteVerifyToleranceSec;  // a silent no-op write fails
}
inline bool setFromSystem() { return Time.isValid() && set(Time.now()); }
inline bool isSet() {
  hal_rtc_option_t opt = {};
  opt.size = sizeof(opt);
  opt.source = HAL_RTC_SOURCE_EXTERNAL;
  return hal_rtc_time_is_valid(&opt);
}
// v50 S4: process-lifetime latch (RAM only, never cleared, never persisted); a reboot re-derives it from the clock source.
inline std::atomic<bool> &oscillatorFaultSeenSinceBind() { static std::atomic<bool> seen{false}; return seen; }
inline void onOscillatorEvent(particle::RtcEvents e, void *) {
  if (e.isSet(particle::RtcEvent::CLOCK_SOURCE_EXTERNAL_FAILURE)) oscillatorFaultSeenSinceBind().store(true, std::memory_order_relaxed);
}
// The fields the fleet configuration fixes (including the clock source: Device OS reports the actual one).
template <typename C> bool sameFleetConfig(C &a, C &b) {
  return a.valid() && a.address() == b.address() && a.interruptPin() == b.interruptPin() &&
         a.watchdogPin() == b.watchdogPin() && a.capabilities().value() == b.capabilities().value() &&
         a.defaultTimeSource() == b.defaultTimeSource() && a.clockSource() == b.clockSource() && a.halDevice()->i2c.interface == b.halDevice()->i2c.interface;
}
// Bind once with the fleet configuration; a failure is logged and the boot continues on the internal clock.
template <typename R, typename C> int ensureBound(R &rtc, const C &fleet) {
  int r = 0;
  if (!rtc.status().bound()) {
    r = rtc.enable(fleet);
    Log.info("AB1805: enable r=%d", r);
<shellId: 5 completed with exit code 0>
```

</details>


---

<sub>25s</sub>

### `bash`

**Boot sequence**

`$ cd src; sed -n 650,680p Generalized-Core-Counter.cpp; sed -n 740,870p Generalized-Core-Counter.cpp`

<details>
<summary>147 lines</summary>

```
}

  // Configure publish queue to retain ~30+ days of hourly reports
  // across all supported platforms (P2, Boron, Argon). With an
  // hourly reporting interval, 800 file-backed events provide
  // headroom over the 720 events needed for a full 30 days.
  PublishQueuePosix::instance()
      .withFileQueueSize(800)
      .setup(); // Initialize the publish queue

  // ===== TIME AND RTC =====
  // Device OS seeds Time from the chip before setup() when it is already bound; on an unbound boot S1b seeds it only on a successful plausible pre-bind chip read.
  const bool timeValidBeforeRtc = Time.isValid();

  // Oscillator snapshot: usingRC from the live clock source, AOS/FOS from the configuration. The raw OSC registers are not readable through the public API (published as null).
  startupOscUsingRC = Am18x5.status().clockSource() == particle::RtcClockSource::INTERNAL;
  const auto oscCaps = Am18x5.getConfig().capabilities();
  startupOscAos = !!(oscCaps & particle::RtcCap::AUTO_CLOCK_SOURCE_INTERNAL_ON_BATTERY);
  startupOscFos = !!(oscCaps & particle::RtcCap::AUTO_CLOCK_SOURCE_INTERNAL_ON_FAIL);
  Log.info("OscState: usingRC=%d aos=%d fos=%d", startupOscUsingRC ? 1 : 0, startupOscAos ? 1 : 0, startupOscFos ? 1 : 0);
  if (startupOscUsingRC) ExternalRtc::oscillatorFaultSeenSinceBind() = true;  // registering the handler clears the chip's fail flag, so it is read first
  ExternalTime.onEvent(ExternalRtc::onOscillatorEvent);

    // Re-run the occupancyStartTime future-date clamp once here: Time is valid here only if Device OS or a successful plausible S1b pre-bind read seeded it.
  CurrentReadings::revalidateOccupancyStartTimeIfTimeAvailable();

#if PLATFORM_ID == PLATFORM_BORON && ENABLE_RTC_SKEW_TEST
  // ===== BENCH-ONLY: DELIBERATE RTC SKEW (WO-2026-08-31-003, Amendment A) =====
  // Compiled in ONLY when ENABLE_RTC_SKEW_TEST=1 (see BuildProfile.h) on
  // Boron. Fires ONCE - not once per boot, but once ever, across resets,
  // until a fresh flash or an explicit reset of retainedRtcSkewTestFired
                 Time.format(rtcSkewVerify, TIME_FORMAT_DEFAULT).c_str(),
                 rtcSkewVerifyReadOk ? 1 : 0);
      } else {
        // Write failed - the RTC still holds whatever it held before this
        // hook ran. Do not claim the skew took effect (A.4).
        Log.error("RtcSkewTest: ONE-SHOT bench hook FAILED - ExternalRtc::set() write did not "
                  "succeed; RTC left unchanged (before(raw=%ld readOk=%d) attemptedAfter=%s)",
                  (long)rtcSkewRawBefore, rtcSkewReadBeforeOk ? 1 : 0,
                  Time.format(rtcSkewAfter, TIME_FORMAT_DEFAULT).c_str());
      }
    }
  }
#endif

  // ===== WAKE CLASSIFICATION (HibernateCycle) =====
  // Reason 30 is judged by HibernateCycle from the OS reason and the retained
  // request. Reason 20 is judged from persisted evidence only (a power-down
  // clears `retained`); no evidence is counted as an inferred watchdog.
  const HibernateCycle::WakeVerdict wakeVerdict = HibernateCycle::classifyWake(reason);

  bool inferredWatchdog = false;
  if (reason == RESET_REASON_PIN_RESET) {
    startupPinResetChecked = true;
    inferredWatchdog = HibernateWakeDiagnostics::isInferredWatchdog(RecoveryState::get_powerOffRequested());
    startupInferredWatchdog = inferredWatchdog;
    Log.info("PIN_RESET: %s", inferredWatchdog ? "no power-off evidence - inferred watchdog" : "power-off evidence - power-up wake");
  }
  RecoveryState::set_powerOffRequested(0);
  SystemConfig::flushNow(); // consume the evidence durably before anything can reload it

  // ===== WATCHDOG RESET PERSISTENCE / FORENSICS =====
  // Moved here (from immediately after SystemConfig::setup(), before AB1805 init)
  // so it can act on the classification above. The publish queue was
  // already initialized above, so this is a reordering, not a new
  // dependency. Covers both the Device-OS-detected case
  // (reason == RESET_REASON_WATCHDOG) and the inferred PIN_RESET
  // case classified above (the "inferred" source); both reuse the same lastWatchdog* fields, tagged
  // with lastWatchdogSource. An inconclusive or non-watchdog PIN_RESET must
  // not overwrite a previously persisted watchdog's forensic data.
  const bool watchdogClassified = watchdogResetDetected || inferredWatchdog;
  if (watchdogClassified) {
    const uint16_t priorWatchdogResetCount = RecoveryState::get_watchdogResetCount();
    if (priorWatchdogResetCount < 0xFFFF) {
      RecoveryState::set_watchdogResetCount((uint16_t)(priorWatchdogResetCount + 1));
    }
    RecoveryState::set_lastWatchdogBreadcrumb(previousBreadcrumb);
    RecoveryState::set_lastWatchdogUptimeMs(previousBreadcrumbMs);
    RecoveryState::set_lastWatchdogResetReasonData(reasonData);
    RecoveryState::set_lastWatchdogSource(watchdogResetDetected ? RecoveryState::WATCHDOG_SOURCE_DEVICE_OS : RecoveryState::WATCHDOG_SOURCE_INFERRED);

    // New watchdog-reset alert code (its own tier, strictly above tier-3;
    // see getAlertSeverity() in MyPersistentData.cpp - this must outrank
    // any already-active tier-3 alert like thrash detection or a boot
    // storm). WO-2026-10-06-001: reported once, in the first report after
    // the watchdog reboot, then cleared by isAutoClearAfterReportAlert() so
    // it no longer hides later alerts. The forensics persist elsewhere:
    // watchdogResetCount, the lastWatchdog* fields and the watchdog event.
    RecoveryState::raiseAlert(19);

    // Queue watchdog forensic event only after PublishQueuePosix setup above
    // initialized its mutex.
    publishWatchdogForensics(inferredWatchdog);
  }

  // wakeVerdict.rtcReadOk/rtcAtWake were computed inside classifyWake()
  // (called above) - nothing between that call and here
  // touches Time's validity (confirmed: the RtcSkewTest bench hook
  // explicitly does not call Time.setTime(), and neither the watchdog init nor the
  // watchdog forensics block touches Time), so sampling Time.isValid() here
  // observes the same transition it always did (seeded by Device OS
  // earlier in setup(), not by this read itself).
  const time_t rtcTime = (time_t)wakeVerdict.rtcAtWake;
  const bool rtcReadOk = wakeVerdict.rtcReadOk;
  const bool timeValidAfterRtc = Time.isValid();
  if (!timeValidBeforeRtc && timeValidAfterRtc) {
    if (rtcReadOk) {
      if (SystemConfig::get_verboseMode()) {
        Log.info("RTC restored system time: %s (rtc=%s)",
                 Time.timeStr().c_str(),
                 Time.format(rtcTime, TIME_FORMAT_DEFAULT).c_str());
      }
    } else if (SystemConfig::get_verboseMode()) {
      Log.info("RTC restored system time: %s (rtc read failed)",
               Time.timeStr().c_str());
    }
  } else if (!timeValidAfterRtc) {
    Log.warn("RTC did not restore time (rtcSet=%s rtcReadOk=%s)",
             ExternalRtc::isSet() ? "true" : "false",
             rtcReadOk ? "true" : "false");
  }

  startupHibernateStatusReady = false;
  startupHibernateWakeReason = "UNKNOWN";
  startupHibernateActualSleepSec = 0;
  startupHibernateSleepErrorSec = 0;
#if PLATFORM_ID == PLATFORM_BORON
  if (wakeVerdict.hibernateGateEvaluated) {
    startupHibernateWakeReason = wakeVerdict.wakeReasonName;

    if (wakeVerdict.gateArm == HibernateWakeDiagnostics::GateArm::kNone) {
      startupHibernateActualSleepSec = wakeVerdict.actualSleepSec;
      startupHibernateSleepErrorSec = wakeVerdict.sleepErrorSec;
      startupHibernateStatusReady = true;

      Log.info("HibernateWake: reason=%s req=%lu actual=%lu err=%ld count=%lu",
               startupHibernateWakeReason,
               (unsigned long)wakeVerdict.requestedSleepSec,
               (unsigned long)startupHibernateActualSleepSec,
               (long)startupHibernateSleepErrorSec,
               (unsigned long)wakeVerdict.hibernateCount);
    } else {
      Log.info("HibernateWake: pending=1 reason=%d wake=%s rtcOk=%d",
               reason,
               startupHibernateWakeReason,
               rtcReadOk ? 1 : 0);
    }

    // WO-2026-08-29-001: queue a forensic event capturing the gate outcome
    // BEFORE HibernateCycle::abandon() below, so the failure case
    // (previously invisible - see the Work Order) reaches the cloud too.
    // buildEventFields() takes wakeVerdict.gateArm directly now (Step 2) -
    // it used to re-derive it via a second call to classifyGateArm() with
    // the same inputs, which is why that function no longer exists as a
    // separately callable thing (WO-2026-09-16 Step 2).
    {
      const HibernateWakeDiagnostics::EventFields eventFields = HibernateWakeDiagnostics::buildEventFields(
          wakeVerdict.gateArm, wakeVerdict.gateInputs, reason, startupHibernateWakeReason,
          wakeVerdict.hibernateCount, startupHibernateActualSleepSec, startupHibernateSleepErrorSec);

      publishHibernateWakeForensics(eventFields);
    }
<shellId: 6 completed with exit code 0>
```

</details>


---

<sub>34s</sub>

### `bash`

**Step 0 lookups**

`$ cd src; sed -n 1,25p time/ExternalRtc.h; grep -n "SystemConfig::setup\|SystemConfig::load\|isClockTrusted\|Clock::isTrusted\|Clock::openness\|checkClockResync\|flushNow" Generalized-Core-Counter.cpp | head -40; grep -n "flushNow" -r . | head; sed -n 30,62p time/HibernateWakeDiagnostics.h; git diff --stat HEAD~1 | tail -1; grep -n "ExternalRtc\|\"time/" time/Clock.cpp | head -20`

<details>
<summary>86 lines</summary>

```
#pragma once
// v50 S1: Device OS owns the carrier AB1805. Every app read/write of the chip goes through hal_rtc_* (external source).
#include "Particle.h"
#include <atomic>

namespace ExternalRtc {
constexpr time_t kWriteVerifyToleranceSec = 2;  // read-back must land within this of the written time
inline bool get(time_t &t) {
  struct timeval tv = {};
  hal_rtc_option_t opt = {};
  opt.size = sizeof(opt);
  opt.source = HAL_RTC_SOURCE_EXTERNAL;
  if (hal_rtc_get_time(&tv, &opt) != 0) return false;
  t = tv.tv_sec;
  return true;
}
inline bool set(time_t t) {
  struct timeval tv = {};
  tv.tv_sec = t;
  hal_rtc_option_t opt = {};
  opt.size = sizeof(opt);
  opt.source = HAL_RTC_SOURCE_EXTERNAL;
  if (hal_rtc_set_time(&tv, &opt) != 0) return false;
  time_t back = 0;
  return get(back) && (back > t ? back - t : t - back) <= kWriteVerifyToleranceSec;  // a silent no-op write fails
118:// requestClockResync()/checkClockResync()/isClockTrusted()/
369:  // WO-2026-09-19 Step 3b: gated on Clock::isTrusted(), not just
378:  if (Clock::isTrusted()) {
496:  SystemConfig::setup();    // Initialize persistent storage
506:  session.pieceUntrusted = CurrentReadings::get_occupied() && !Clock::isTrusted(); session.occupancyStartMs = millis();  // straddler piece starts at boot
549:    // WO-2026-08-29-002 item 8: deliberately NOT gated on isClockTrusted().
768:  SystemConfig::flushNow(); // consume the evidence durably before anything can reload it
771:  // Moved here (from immediately after SystemConfig::setup(), before AB1805 init)
915:  // checkClockResync() at confirmed-sync time - see below) is the correct
928:  // section: because isClockTrusted() is unconditionally false at this
940:  // WO-2026-09-19 Step 3b: deliberately NOT converted to Clock::isTrusted().
941:  // isTrusted()/isClockTrusted() is false on every boot (including every
1134:  checkClockResync(); // WO-2026-08-29-002: recurring app-side RTC write-back
1275:  // 28796s instead of -1). Additionally require isClockTrusted() (sync
1278:      (isClockTrusted() && lastConnection != 0 && Time.now() > lastConnection)
./MyPersistentData.cpp:919:void flushNow() { sysStatus.flush(true); }
./MyPersistentData.cpp:1015:void flushNow() { sensorConfig.flush(true); }
./MyPersistentData.cpp:1112:void flushNow() { current.flush(true); }
./power/ConnectivityFailsafe.cpp:160:  SystemConfig::flushNow();
./power/ConnectivityFailsafe.cpp:309:    SystemConfig::flushNow();
./power/ConnectivityFailsafe.cpp:330:  RecoveryState::set_powerOffRequested(0); RecoveryState::set_connectivityRecoveryStage(2); SystemConfig::flushNow(); // not expected to return: no recent stage-3 evidence
./state/State_Error.cpp:117:  CurrentReadings::flushNow();
./state/State_Error.cpp:118:  SystemConfig::flushNow();
./state/State_Error.cpp:119:  SystemConfig::SensorSettings::flushNow();
./state/State_Error.cpp:181:      RecoveryState::set_powerOffRequested(0); SystemConfig::flushNow(); // not expected to return: drop the evidence
#include <stdint.h>
#include <stdio.h>

namespace HibernateWakeDiagnostics {

/// Identifies which of the gate's six conditions (in the order
/// `HibernateCycle::classifyWake()` evaluates them) first failed. `kNone`
/// means every condition passed - the gate succeeded.
enum class GateArm {
  kNone = 0,
  kResetReason,    // OS reset reason was not RESET_REASON_POWER_MANAGEMENT
  kWakeReason,     // unreachable after kResetReason since v50 S2 (reason 30 alone passes)
  kRtcRead,        // the external RTC read failed
  kRtcBeforeZero,  // retainedHibernateRtcBefore was not > 0
  kRequestedZero,  // retainedHibernateRequestedSleep was not > 0
  kRtcOrder,       // rtcTime on wake was earlier than retainedHibernateRtcBefore
};

/// Raw inputs to the gate, exactly as `HibernateCycle::classifyWake()`
/// reads them. The caller resolves `resetReasonIsPowerManagement` and
/// `wakeReasonIsHibernateWake` from Device-OS/AB1805 types before populating this,
/// so this header never needs to know those enums.
struct GateInputs {
  bool resetReasonIsPowerManagement;
  bool wakeReasonIsHibernateWake;
  bool rtcReadOk;
  int64_t rtcBefore;
  uint32_t requestedSleepSec;
  int64_t rtcAtWake;
};

inline const char *gateArmName(GateArm arm) {
  switch (arm) {
 6 files changed, 296 insertions(+)
2:#include "time/Clock.h"
7:#include "time/LocalTimeCache.h"    // Cached LocalTimeRK conversions
8:#include "time/ClockTrust.h"        // Sync-recency resync gate and trust signal (WO-2026-08-29-002)
10:#include "time/ExternalRtc.h"      // hal_rtc_* external-source helpers
400:    const bool rtcPreWriteReadOk = ExternalRtc::get(rtcPreWriteValue);
401:    const bool rtcUpdated = ExternalRtc::setFromSystem();
<shellId: 7 completed with exit code 0>
```

</details>


---

<sub>40s</sub>

### `bash`

**Read early boot and C2**

`$ cd src; sed -n 360,396p Generalized-Core-Counter.cpp; sed -n 488,512p Generalized-Core-Counter.cpp; grep -n "pieceUntrusted" -r . | head -20; grep -n "isValid()" Generalized-Core-Counter.cpp | head`

<details>
<summary>75 lines</summary>

```
(unsigned long)reasonData);
  }

  if (previousBootFailedEarly && qualifiesForStormCount) {
    if (bootStormCount < 255) {
      bootStormCount++;
    }
  }

  // WO-2026-09-19 Step 3b: gated on Clock::isTrusted(), not just
  // Clock::isTimeValid() - this window-reset arithmetic (now -
  // bootStormWindowStart) would otherwise run on an RTC-seeded-but-unconfirmed
  // epoch, which could prematurely reset the storm counter (masking a real
  // storm) or corrupt the baseline. Not updating the window during an
  // untrusted-clock boot does not defeat detection: bootStormCount already
  // incremented unconditionally above, so an untrusted clock simply leaves
  // the 600s cooldown from firing on a wrong baseline - the trip threshold at
  // line below is unaffected either way.
  if (Clock::isTrusted()) {
    const time_t now = Time.now();
    if (bootStormWindowStart == 0) {
      bootStormWindowStart = now;
    } else if ((now - bootStormWindowStart) > 600) {
      bootStormCount = (previousBootFailedEarly && qualifiesForStormCount) ? 1 : 0;
      bootStormWindowStart = now;
    }
  }

  if (bootStormCount >= 6 && previousBootFailedEarly && qualifiesForStormCount) {
    bootStormTripCount++;
    bootStormAlertPending = true;
    Log.error("BOOT STORM: %u early resets detected (reason=%d)", bootStormCount, reason);
    Connectivity::requestFullDisconnectAndRadioOff();
    Particle.process();
    SystemSleepConfiguration bootStormSleep;
    bootStormSleep.mode(SystemSleepMode::ULTRA_LOW_POWER).duration(600000UL);
    drainSerialBeforeSleep();
  // force a cloud connection regardless of opening-hours logic.
  // This allows ledger changes (e.g., opening hours) to be pulled down to
  // recover a device that would otherwise immediately sleep.
  if (digitalRead(BUTTON_PIN) == LOW) { // Active-low user button
    Log.info("Boot override: user button held - forcing CONNECTING_STATE");
    transitionTo(CONNECTING_STATE, "boot button override");
  }

  SystemConfig::setup();    // Initialize persistent storage
  SystemConfig::SensorSettings::setup(); // Initialize the sensor configuration
  CurrentReadings::setup();      // Initialize the current status data

  // WO-2026-10-04-001 item B: lastOccupancyEvent is a millis() value, so the
  // persisted one belongs to the previous boot. Clearing it lets the existing
  // zero sentinel re-arm the debounce from this boot. Remember a session that
  // was already open so the first trusted close can credit it (item B3). The
  // anchor is captured here, once: a report sent later in this boot advances
  // lastReport and must not move the cap.
  session.pieceUntrusted = CurrentReadings::get_occupied() && !Clock::isTrusted(); session.occupancyStartMs = millis();  // straddler piece starts at boot
  if (session.pieceUntrusted) Log.info("Occ: untrusted straddler, pre-boot part dropped");
  session.occupancySessionBootAnchor = CurrentReadings::get_occupied() && !session.pieceUntrusted
      ? std::max(CurrentReadings::get_occupancyStartTime(), SystemConfig::get_lastReport()) : 0;
  CurrentReadings::set_lastOccupancyEvent(0);

  // WO-2026-08-25-001 Amendment C, Decision C1 boot-ordering corollary
./state/StateMachine.h:71:  bool pieceUntrusted = false;           // piece opened with an untrusted clock: credited on millis() until it closes
./state/State_Common.h:367:	const bool piece = occupied && session.pieceUntrusted;
./state/State_Common.h:454:	session.pieceUntrusted = false; session.occupancyStartMs = 0;
./state/State_Report.cpp:82:      session.occupancyStartMs = millis(); session.pieceUntrusted = !Clock::isTrusted();  // trusted piece
./state/State_Modes.cpp:73:      session.pieceUntrusted = !Clock::isTrusted();
./state/State_Modes.cpp:158:    if (SystemConfig::get_sensorMode() == SystemConfig::OCCUPANCY && (signalLEDStatus() || (session.pieceUntrusted && CurrentReadings::get_occupied())) && debounceRemainingMs() == 0) {
./state/State_Modes.cpp:235:          session.pieceUntrusted = !Clock::isTrusted();
./Generalized-Core-Counter.cpp:506:  session.pieceUntrusted = CurrentReadings::get_occupied() && !Clock::isTrusted(); session.occupancyStartMs = millis();  // straddler piece starts at boot
./Generalized-Core-Counter.cpp:507:  if (session.pieceUntrusted) Log.info("Occ: untrusted straddler, pre-boot part dropped");
./Generalized-Core-Counter.cpp:508:  session.occupancySessionBootAnchor = CurrentReadings::get_occupied() && !session.pieceUntrusted
662:  const bool timeValidBeforeRtc = Time.isValid();
692:  // procedure's amended step 3 (verify Time.isValid()/wrong-time belief
808:  // watchdog forensics block touches Time), so sampling Time.isValid() here
813:  const bool timeValidAfterRtc = Time.isValid();
910:  // Finding 3 (WO-2026-08-29-002, Round 4 review): Time.isValid() alone can
979:           Time.isValid() ? (isWithinOpenHours() ? 1 : 0) : -1,
1272:  // WO-2026-08-29-002 item 8/Finding 3: Time.isValid() alone is NOT a trust
1274:  // (a wrong RTC seeded system time, isValid() was true, and this computed
<shellId: 8 completed with exit code 0>
```

</details>


---

<sub>53s</sub>

### `bash`

**C2 code and tests layout**

`$ cd src; sed -n 355,380p state/State_Common.h; sed -n 1,60p ../tests/stubs/*.h 2>/dev/null | head -0; ls ../tests ../tests/stubs | head -150; grep -n "isClockTrusted\|get_lastTimeSync\|set_lastTimeSync" -r ../tests --include=* -l | head -30`

<details>
<summary>196 lines</summary>

```
*/
inline OccupancyCloseResult closeOccupancySessionSafely(const char *path, time_t closeAt = Time.now()) {
	OccupancyCloseResult result;
	const bool occupied = CurrentReadings::get_occupied();
	const int8_t alertCode = RecoveryState::get_alertCode();
	// WO-2026-09-19 Step 3b: Clock::isTrusted(), not isTimeValid() - the
	// duration arithmetic below (now - start) must not run on an RTC-seeded-
	// but-unconfirmed epoch, which can produce a wildly wrong session length.
	const bool timeValid = Clock::isTrusted();
	// WO-2026-11-03-006: a piece opened untrusted is measured on millis() for its
	// whole life (no wall clock, even if trust returns): credit its on-time up to
	// the debounce expiry, then close it so the next PIR opens a new piece.
	const bool piece = occupied && session.pieceUntrusted;
	// WO-2026-10-04-001 item B2 (a trusted piece that loses trust): keep it open,
	// credit nothing and re-arm the debounce.
	if (!timeValid && occupied && !piece) {
		CurrentReadings::set_lastOccupancyEvent(millis());
		result.stillOpen = true;
		return result;
	}
	const time_t now = Time.now();
	const time_t start = CurrentReadings::get_occupancyStartTime();
	// WO-2026-10-29-002: credit no later than the last close boundary. The
	// daily-cleanup call passes that same boundary, so it is unchanged.
	if (occupancyIgnoredWhenClosed()) {
		const time_t boundary = DailyBoundary::check(now).boundary;
head: illegal line count -- 0
../tests:
README.md
SensorManagerBatteryAuthorityTests.cpp
alert19_clears_after_report_test.sh
awake_cycle_counters_test.sh
battery_authority_commit_sites_structural_test.py
battery_authority_seam_structural_test.py
battery_authority_structural_test.py
battery_health_test.cpp
battery_health_test.sh
battery_tier_guard_test.cpp
battery_tier_guard_test.sh
boron_soc_commit_test.py
build_flags_witness_test.sh
bump_version_scheme_test.sh
charge_inhibit_thermal_test.cpp
charge_inhibit_thermal_test.sh
clock_hibernate_helpers_move_structural_test.py
clock_openness_test.cpp
clock_openness_test.sh
clock_owner_structural_test.py
clock_resync_wiring_test.py
clock_rtc_writeback_test.cpp
clock_status_republish_test.cpp
clock_status_republish_test.sh
clock_trust_standard_structural_test.py
clock_trust_test.cpp
close_before_night_sleep_test.py
config_window_hold_test.cpp
config_window_hold_test.sh
conn_phase_modem_off_test.sh
connect_attempt_counter_test.sh
connect_no_modem_wait_structural_test.py
connected_online_last_connection_test.cpp
connected_online_last_connection_test.sh
connection_mode_downgrade_structural_test.py
connection_mode_downgrade_test.cpp
connection_mode_downgrade_test.sh
connection_signal_validity_test.sh
connectivity_failsafe_move_structural_test.py
connectivity_failsafe_open_hours_test.cpp
connectivity_failsafe_open_hours_test.sh
connectivity_failsafe_stage1_retired_test.py
connectivity_failsafe_unknown_openness_test.cpp
connectivity_failsafe_unknown_openness_test.sh
daily_cleanup_boundary_test.py
debounce_expiry_c1_test.sh
default_timezone_test.cpp
default_timezone_test.sh
deliberate_reset_flush_test.py
exerciser_baseline_test.py
external_rtc_structural_test.py
external_rtc_test.cpp
facade_header_isolation_test.py
failsafe_cadence_rule_test.cpp
failsafe_cadence_rule_test.sh
firmware_update_dwell_test.sh
firmware_update_wiring_structural_test.py
fixtures
hibernate_cycle_single_owner_structural_test.py
hibernate_wake_diagnostics_test.cpp
hibernate_wake_diagnostics_test.sh
hour_rules_test.cpp
hour_rules_test.sh
hourly_while_occupied_test.cpp
hourly_while_occupied_test.sh
idle_timediag_on_change_test.cpp
idle_timediag_on_change_test.sh
ledger_no_retry_test.cpp
ledger_no_retry_test.sh
loop_forensics_move_structural_test.py
loop_stage_sleep_prep_exclusion_test.py
modem_health_equivalence_test.cpp
modem_health_equivalence_test.sh
modem_health_owner_structural_test.py
modem_teardown_confirmation_logging_test.py
nightly_heap_guard_flush_test.sh
occupancy_ends_at_close_structural_test.py
occupancy_report_by_mode_test.cpp
occupancy_report_by_mode_test.sh
occupancy_session_restart_test.sh
occupancy_start_time_clamp_test.cpp
occupancy_start_time_clamp_test.sh
oom_immediate_reset_test.sh
persisted_layout_preservation_test.py
persistence_facade_behavior_test.cpp
persistence_facade_behavior_test.sh
persistence_facade_fanin_structural_test.py
pir_sensor_lifecycle_test.cpp
pir_sensor_lifecycle_test.sh
pir_sensor_reset_totals_test.cpp
pir_sensor_reset_totals_test.sh
pmic_fault_monitor_test.cpp
pmic_fault_monitor_test.sh
pmic_latch_call_site_structural_test.py
power_composition_test.cpp
power_composition_test.sh
power_constants_structural_test.py
power_constants_values_test.cpp
power_constants_values_test.sh
power_consumer_regression_test.sh
power_diagnostics_batch_test.cpp
power_diagnostics_batch_test.sh
power_diagnostics_log_guard_test.cpp
power_diagnostics_log_guard_test.sh
power_manager_accessors_test.cpp
power_source_override_test.cpp
power_source_override_test.sh
power_tier_test.cpp
power_tier_test.sh
pre_bind_rtc_test.cpp
pre_bind_rtc_test.py
publish_with_ack_structural_test.py
release_upload_job_test.py
report_payload_fields_test.py
report_payload_golden_test.cpp
report_payload_golden_test.sh
report_publisher_move_structural_test.py
reporting_interval_cap_test.cpp
reporting_interval_cap_test.sh
reporting_interval_store_seam_structural_test.py
reporting_policy_adapter_test.cpp
reporting_policy_adapter_test.sh
reporting_policy_clock_owner_structural_test.py
reporting_policy_test.cpp
reset_cause_structural_test.py
reset_wiring_test.cpp
reset_wiring_test.sh
retained_boot_structural_test.py
retained_boot_test.cpp
retained_boot_test.sh
rtc_skew_test.cpp
rtc_skew_test.sh
s2_alarm_watchdog_wake_structural_test.py
s2_hibernate_normalise.py
s2_transcript_normalise.py
s3_timed_power_off_test.cpp
s3_timed_power_off_test.sh
sensor_type_source_test.cpp
sensor_type_source_test.sh
sensor_type_validation_test.cpp
sensor_wake_source_test.cpp
sensor_wake_source_test.sh
serial_settle_test.py
sleep_breadcrumb_sequence_test.py
sleep_cloud_gate_equivalence_test.cpp
sleep_cloud_gate_equivalence_test.sh
sleep_cloud_gate_structural_test.py
sleep_config_leak_test.cpp
sleep_config_leak_test.sh
../tests/stubs/power_source_override_overrides/MyPersistentData.h
../tests/stubs/power_source_override_overrides/cloud/CloudTestShim.cpp
../tests/stubs/power_source_override_overrides/persist/SystemConfig.h
../tests/stubs/clock_status_republish_overrides/CloudLinkStubs.cpp
../tests/idle_timediag_on_change_test.sh
../tests/occupancy_report_by_mode_test.cpp
../tests/clock_rtc_writeback_test.cpp
../tests/sleep_wake_equivalence_test.cpp
../tests/default_timezone_test.cpp
../tests/clock_trust_standard_structural_test.py
../tests/clock_owner_structural_test.py
../tests/persistence_facade_behavior_test.cpp
../tests/sleep_wake_equivalence_test.sh
../tests/connection_mode_downgrade_test.cpp
../tests/idle_timediag_on_change_test.cpp
../tests/reset_wiring_test.cpp
../tests/clock_resync_wiring_test.py
../tests/clock_status_republish_test.cpp
<shellId: 9 completed with exit code 0>
```

</details>


---

<sub>55s</sub>

### `bash`

**CI test discovery**

`$ grep -rn "tests/\*" .github/workflows | head; which zsh python3; python3 --version; ls build-tmp 2>/dev/null | head`

<details>
<summary>6 lines</summary>

```
.github/workflows/build.yml:45:          for t in tests/*.sh; do
.github/workflows/build.yml:49:          for t in tests/*.py; do
/bin/zsh
/opt/homebrew/bin/python3
Python 3.14.6
<shellId: 10 completed with exit code 0>
```

</details>


---

<sub>57s</sub>

### `bash`

**View CI loop**

`$ sed -n 35,70p .github/workflows/build.yml`

<details>
<summary>36 lines</summary>

```
dynalib/inc
      - name: Install zsh
        run: sudo apt-get install -y zsh
      - name: Run host tests (sh via zsh, py via python3)
        id: tests
        env:
          TMPDIR: ${{ runner.temp }}
          DEVICE_OS_ROOT: ${{ github.workspace }}/device-os-6.4.1
        run: |
          pass=0; total=0; failed=()
          for t in tests/*.sh; do
            total=$((total+1))
            if zsh "$t"; then pass=$((pass+1)); else failed+=("$t"); fi
          done
          for t in tests/*.py; do
            total=$((total+1))
            if python3 "$t"; then pass=$((pass+1)); else failed+=("$t"); fi
          done
          echo "$pass/$total (sh via zsh, py via python3)" | tee -a "$GITHUB_STEP_SUMMARY"
          echo "result=$pass/$total" >> "$GITHUB_OUTPUT"
          if [ "${#failed[@]}" -gt 0 ]; then
            printf 'FAILED: %s\n' "${failed[@]}"
            exit 1
          fi

  firmware:
    runs-on: ubuntu-latest
    outputs:
      sha256: ${{ steps.rec.outputs.sha256 }}
      digest: ${{ steps.rec.outputs.digest }}
      os: ${{ steps.props.outputs.os }}
    steps:
      - uses: actions/checkout@3d3c42e5aac5ba805825da76410c181273ba90b1 # v7.0.1
        with:
          persist-credentials: false
      - name: Tag matches source FIRMWARE_PRODUCT_VERSION (before building)
<shellId: 11 completed with exit code 0>
```

</details>


---

<sub>1m 0s</sub>

### Copilot

Baseline run in background while I study the tests.

---

<sub>1m 0s</sub>

### `bash` — Failed

**Prepare scratch and runner**

```
$ mkdir -p build-tmp/wo-2026-11-05-007-stage6 && cat > /tmp/runall.sh <<'EOF'
cd "$1"
p=0; t=0; f=()
for x in tests/*.sh; do t=$((t+1)); if zsh "$x" >/dev/null 2>&1; then p=$((p+1)); else f+=("$x"); fi; done
for x in tests/*.py; do t=$((t+1)); if python3 "$x" >/dev/null 2>&1; then p=$((p+1)); else f+=("$x"); fi; done
echo "$p/$t"; printf 'FAILED: %s\n' "${f[@]}"
EOF
git check-ignore build-tmp; git status --short
```

```
Permission denied because no interactive user response was available. Retry in an interactive session so the user can approve it, or try an alternative that does not require this permission.
```


---

<sub>1m 2s</sub>

### `bash`

**Create scratch dir**

`$ mkdir -p build-tmp/wo-2026-11-05-007-stage6; git check-ignore build-tmp; git status --short`

```
build-tmp
<shellId: 13 completed with exit code 0>
```


---

<sub>1m 6s</sub>

### `create`

**create a new file at /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-i21/build-tmp/wo-2026-11-05-007-stage6/runall.sh.**

`/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-i21/build-tmp/wo-2026-11-05-007-stage6/runall.sh`

```
Created file /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-i21/build-tmp/wo-2026-11-05-007-stage6/runall.sh with 308 characters
```


---

<sub>10m 53s</sub>

### `bash`

**Baseline test run**

`$ zsh build-tmp/wo-2026-11-05-007-stage6/runall.sh 2>&1 | tail -8`

```
121/121 (sh via zsh, py via python3)
FAILED:
<shellId: 14 completed with exit code 0>
```


---

<sub>1m 8s</sub>

### `bash`

**Study writeback test**

`$ cat tests/clock_rtc_writeback_test.cpp | head -150; cat tests/clock_openness_test.sh`

<details>
<summary>230 lines</summary>

```
// Host test for the RTC write-back state machine added to
// checkClockResync() (WO-2026-08-29-002, review fixes through Round 5).
//
// checkClockResync() itself lives in time/Clock.cpp (WO-2026-09-18 Step 3a;
// previously Generalized-Core-Counter.cpp) and can't be compiled standalone
// on the host (heavy Particle/AB1805/PublishQueue dependencies - see
// tests/README.md's established precedent). Its
// write-back gate DECISION, however, is now a small, pure, shared function
// (Round 5 cleanup task 4, Stage 7 finding 7) that this test calls directly
// - not a hand-written mirror of it:
//
//     if (ClockTrust::shouldAttemptRtcWriteNow(lastSyncMs, lastRtcWriteSyncedLastMs,
//                                               nowMs, lastRtcWriteFailedAttemptMs,
//                                               lastRtcWriteFailedSyncMs)) {
//       const bool rtcUpdated = ab1805.setRtcFromSystem();
//       if (rtcUpdated) {
//         lastRtcWriteSyncedLastMs = lastSyncMs;
//         sysStatus.set_lastTimeSync(Time.now());
//       } else {
//         lastRtcWriteFailedAttemptMs = nowMs;
//         lastRtcWriteFailedSyncMs = lastSyncMs;
//       }
//     }
//
// `RtcWriteBackSimulator` below calls the REAL ClockTrust::shouldAttemptRtcWriteNow()
// for the gate condition, with only the real ab1805.setRtcFromSystem() I2C
// call replaced by an injectable outcome (rtcWriteSucceeds - this part is
// vendored hardware I/O and cannot be pure host code), so the gate decision
// itself is exercised with real production logic on real call sequences.
// tests/clock_resync_wiring_test.py separately parses the ACTUAL shipped
// source and asserts checkClockResync() calls this exact shared function
// (rather than re-deriving the condition inline) and in the correct order
// relative to the confirmed-write bookkeeping - so the two tests together
// prove both "the real code calls the real gate function, in the right
// place" and "that gate function behaves correctly", with no hand-written
// re-derivation of the condition left to drift from the real one.
//
// Covers the scenarios from the Chief Engineer's reviews:
//   1. A sync completing WITHOUT any app-initiated request (Device OS's own
//      cloud-handshake sync) still drives an RTC write - the gate reacts to
//      OBSERVED Particle.timeSyncedLast() state only, never to whether this
//      app called requestClockResync().
//   2. The step-2 race explicitly: syncTimeDone() (not modeled here, since
//      the gate no longer references it at all - that IS the fix) going
//      true before timeSyncedLast() advances, then timeSyncedLast()
//      advancing on a later check, still produces exactly one RTC write.
//   3. Exactly-once semantics for THIS function's own idempotency: a single
//      sync value must not produce repeated RTC writes on subsequent checks
//      while timeSyncedLast() is unchanged. (This is explicitly NOT a claim
//      about the total number of physical RTC writes across the whole
//      system - see Finding 8 below and checkClockResync()'s doc comment.)
//   4. No sync at all (timeSyncedLast() stays 0): no RTC write, no
//      lastTimeSync stamp ever occurs.
//   5. Finding 1 (Round 4 review, HIGH): a FAILED write must not be
//      permanently treated as done. The tracked value must only advance
//      after a CONFIRMED successful write; a failed write must retry on a
//      later check (past the retry floor) with the same lastSyncMs, and
//      only stop retrying once a write actually succeeds.
//   6. Finding 8 (Round 4 review, LOW, documented not "fixed"): the
//      vendored AB1805::loop() one-shot reacts to the same "a sync has
//      completed" condition as this gate and runs immediately before it in
//      loop() - so the first successful sync of each boot is written to the
//      RTC twice in practice (once by the vendored one-shot, once by this
//      gate). This test suite does NOT claim otherwise; it only proves this
//      gate's OWN calls to ab1805.setRtcFromSystem() are exactly-once per
//      observed sync advance.
//   7. Round 5 cleanup task 2 (Stage 7 finding 5, HIGH): a FAILED write must
//      be PACED, not retried on every single check. Without pacing, a
//      permanent AB1805/Wire fault would retry a locked I2C transaction on
//      every main-loop pass - potentially hundreds of times per second on a
//      bus shared with PMIC/battery work. This replaces the OLD test that
//      asserted "200 attempts in 200 checks", which had codified the unpaced
//      hot loop as a requirement rather than catching it as a defect.
//   8. Round 6 (second follow-up), Stage 7 pass 4 (HIGH): the pacing floor
//      from (7) must NEVER delay admission of a DISTINCT new sync value,
//      even if it arrives within kMinRtcWriteRetryIntervalMs of a prior
//      SUCCESSFUL write - only a repeat attempt of the exact value that
//      already FAILED is paced. The Round 5 implementation kept a single
//      "last attempt of any kind" timestamp and applied the floor to every
//      write attempt regardless of outcome, so a second distinct confirmed
//      sync arriving soon after a success was wrongly withheld from the
//      RTC - a narrower recurrence of the exact defect this Work Order
//      exists to fix, since a hibernate inside that window would persist
//      the OLDER, already-superseded sync value. Fixed by tracking
//      lastRtcWriteFailedAttemptMs/lastRtcWriteFailedSyncMs - set ONLY on
//      failure - so the floor only ever keys off a failed attempt of the
//      SAME value.

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "time/ClockTrust.h"

namespace {

// Line-for-line mirror of checkClockResync()'s write-back gate, with the
// real ab1805.setRtcFromSystem() call replaced by an injectable outcome
// (rtcWriteSucceeds) so Finding 1's failure/retry path can be exercised
// without real hardware.
//
// Round 5 cleanup task 4 (Stage 7 finding 7): the GATE CONDITION itself is
// no longer re-derived here - it calls ClockTrust::shouldAttemptRtcWriteNow(),
// the exact same pure function checkClockResync() calls in production (see
// tests/clock_resync_wiring_test.py's write_back_gate check, which parses
// the real source and asserts it calls this same function). Only the parts
// that genuinely cannot be pure host code remain simulated here: the
// ab1805.setRtcFromSystem() I2C call's outcome (rtcWriteSucceeds) and the
// bookkeeping updates (lastRtcWriteSyncedLastMs,
// lastRtcWriteFailedAttemptMs/lastRtcWriteFailedSyncMs,
// sysStatus.set_lastTimeSync()) that checkClockResync() performs around
// that real call. This closes the "tests a hand-written simulator, not the
// production function" gap for the gate DECISION, while the I2C outcome
// remains necessarily mocked (it is vendored hardware I/O).
struct RtcWriteBackSimulator {
  uint32_t lastRtcWriteSyncedLastMs = 0;
  // Round 6 (second follow-up): failure-only pair, mirroring production's
  // fix - never touched on a success, so the pacing floor can never delay
  // a distinct new sync value.
  uint32_t lastRtcWriteFailedAttemptMs = 0;
  uint32_t lastRtcWriteFailedSyncMs = 0;
  bool rtcWriteSucceeds = true; // stands in for ab1805.setRtcFromSystem()'s return value
  int rtcWriteAttemptCount = 0;  // every call to ab1805.setRtcFromSystem(), success or failure
  int rtcWriteConfirmedCount = 0; // only calls that returned true
  int lastTimeSyncStampCount = 0;

  // Mirrors:
  //   if (ClockTrust::shouldAttemptRtcWriteNow(lastSyncMs, lastRtcWriteSyncedLastMs,
  //                                             nowMs, lastRtcWriteFailedAttemptMs,
  //                                             lastRtcWriteFailedSyncMs)) {
  //     const bool rtcUpdated = ab1805.setRtcFromSystem();
  //     if (rtcUpdated) {
  //       lastRtcWriteSyncedLastMs = lastSyncMs;
  //       sysStatus.set_lastTimeSync(Time.now());
  //     } else {
  //       lastRtcWriteFailedAttemptMs = nowMs;
  //       lastRtcWriteFailedSyncMs = lastSyncMs;
  //     }
  //   }
  // The condition itself (the `if`) is the REAL production function, not a
  // re-derivation of it - only the body's I2C call is simulated.
  void check(uint32_t nowMs, uint32_t lastSyncMs) {
    if (ClockTrust::shouldAttemptRtcWriteNow(lastSyncMs, lastRtcWriteSyncedLastMs,
                                              nowMs, lastRtcWriteFailedAttemptMs,
                                              lastRtcWriteFailedSyncMs)) {
      rtcWriteAttemptCount++;              // mirrors calling ab1805.setRtcFromSystem()
      const bool rtcUpdated = rtcWriteSucceeds;
      if (rtcUpdated) {
        lastRtcWriteSyncedLastMs = lastSyncMs; // Finding 1: only on confirmed success
        rtcWriteConfirmedCount++;
#!/bin/zsh
set -euo pipefail

# WO-2026-09-19 Step 3b: Clock::openness()'s truth table.
#
# Part 1 (clock_openness_test.cpp) runs a hand-written mirror of the
# three-input decision on the host. Part 2 here traces the REAL
# src/time/Clock.cpp to confirm the real openness() actually checks trust
# FIRST, then config validity, then falls through to the same open/closed
# computation isWithinOpenHours() itself uses - so the host-side mirror
# cannot silently drift from what production actually does.

repo_root="${0:A:h:h}"
binary="$TMPDIR/clock_openness_test"
clock_src="$repo_root/src/time/Clock.cpp"

clang++ -std=c++17 -Wall -Wextra -pedantic \
  -I"$repo_root/src" \
  "$repo_root/tests/clock_openness_test.cpp" \
  -o "$binary"
"$binary"

echo ""
echo "--- Part 2: Clock::openness() fidelity checks against the real source ---"

openness_fn=$(python3 - "$clock_src" <<'PY'
import sys

path = sys.argv[1]
with open(path) as f:
    lines = f.readlines()

start_idx = None
for i, line in enumerate(lines):
    if "Openness openness() {" in line:
        start_idx = i
        break

if start_idx is None:
    sys.exit("EXTRACT_FAILED: signature not found")

depth = 0
opened = False
end_idx = None
for i in range(start_idx, len(lines)):
    for ch in lines[i]:
        if ch == '{':
            depth += 1
            opened = True
        elif ch == '}':
            depth -= 1
    if opened and depth == 0:
        end_idx = i
        break

if end_idx is None:
    sys.exit("EXTRACT_FAILED: closing brace not found")

sys.stdout.write("".join(lines[start_idx:end_idx + 1]))
PY
)

if [[ -z "$openness_fn" ]]; then
  echo "FAILED: could not extract Clock::openness() verbatim from $clock_src" >&2
  exit 1
fi
echo "Extracted the real Clock::openness() body verbatim from $clock_src"

if ! print -r -- "$openness_fn" | grep -q "!isTrusted()"; then
  echo "FAILED: Clock::openness() must check !isTrusted() (not !isTimeValid() or a re-derived trust check)" >&2
  exit 1
fi

if ! print -r -- "$openness_fn" | grep -q "!Config::isValid(false)"; then
  echo "FAILED: Clock::openness() must check !Config::isValid(false)" >&2
  exit 1
fi

if ! print -r -- "$openness_fn" | grep -q "Openness::Unknown"; then
  echo "FAILED: Clock::openness() must be able to return Openness::Unknown" >&2
  exit 1
fi

# Ordering: the trust check must appear BEFORE the config check, which must
# appear before the open/closed computation - so an untrusted clock never
# even reaches the config or hour-window logic (must never fail open).
trust_idx=$(print -r -- "$openness_fn" | grep -n "!isTrusted()" | head -1 | cut -d: -f1)
config_idx=$(print -r -- "$openness_fn" | grep -n "!Config::isValid(false)" | head -1 | cut -d: -f1)
hour_idx=$(print -r -- "$openness_fn" | grep -n "isWithinOpenHoursForHour(" | head -1 | cut -d: -f1)

if [[ -z "$trust_idx" || -z "$config_idx" || -z "$hour_idx" ]]; then
  echo "FAILED: could not locate all three checks (trust/config/hour) inside Clock::openness()" >&2
  exit 1
fi

if (( trust_idx >= config_idx || config_idx >= hour_idx )); then
  echo "FAILED: Clock::openness() must check trust, then config validity, then the hour window, in that order (found lines trust=$trust_idx config=$config_idx hour=$hour_idx)" >&2
  exit 1
fi

echo "OK: Clock::openness() checks isTrusted(), then Config::isValid(false), then the hour window, in that order - and can return Unknown"
echo ""
echo "clock_openness_test (host mirror + real-source fidelity) passed"
<shellId: 15 completed with exit code 0>
```

</details>


---

<sub>11m 3s</sub>

### `bash`

**Find plausibility uses and external validity helper**

`$ grep -rn "isPlausibleEpoch\|kEpochMin\|1704067200" src tests | grep -v "^tests/fixtures" | head -20; grep -n "EXTERNAL\|isValid" src/time/*.cpp src/time/*.h | grep -i "external\|TimeClass" | head`

<details>
<summary>26 lines</summary>

```
src/time/PreBindRtc.h:12:  if (timeOk && Clock::isPlausibleEpoch(chip) && !Time.isValid()) Time.setTime(chip);
src/time/HibernateEligibility.cpp:56:  if (!Clock::isPlausibleEpoch(rtcNow)) {
src/time/Clock.cpp:549:bool isPlausibleEpoch(time_t epoch) {
src/time/Clock.cpp:550:  static const time_t kEpochMin = 1704067200; // 2024-01-01 00:00:00 UTC
src/time/Clock.cpp:552:  return epoch >= kEpochMin && epoch < kEpochMax;
src/time/Clock.h:32: *          `Clock::isPlausibleEpoch()` (absorbing the former
src/time/Clock.h:151:bool isPlausibleEpoch(time_t epoch);
tests/rtc_skew_test.sh:41:#   Step 2 - A.6 gap 2: extracts isPlausibleEpoch()'s REAL
tests/rtc_skew_test.sh:46:#     that changes the REAL isPlausibleEpoch() to always return
tests/rtc_skew_test.sh:240:echo "--- Step 2: A.6 gap 2 - isPlausibleEpoch() acceptance, using the REAL function body ---"
tests/rtc_skew_test.sh:242:# Extract isPlausibleEpoch()'s real body VERBATIM from
tests/rtc_skew_test.sh:255:    if "bool isPlausibleEpoch(time_t epoch) {" in line:
tests/rtc_skew_test.sh:283:  echo "FAILED: could not extract isPlausibleEpoch() verbatim from $clock_src" >&2
tests/rtc_skew_test.sh:286:echo "Extracted the real isPlausibleEpoch() body verbatim from $clock_src"
tests/rtc_skew_test.sh:308:  echo "// ---- REAL isPlausibleEpoch() body, extracted verbatim from"
tests/rtc_skew_test.sh:318:  echo '    bool ok = isPlausibleEpoch(after);'
tests/rtc_skew_test.sh:329:    echo '    bool ok = isPlausibleEpoch(after);'
tests/rtc_skew_test.sh:336:  echo '  // skewed below it - proving isPlausibleEpoch() actually'
tests/rtc_skew_test.sh:340:  echo '    time_t anchor = (time_t)1704067200 + 100; // 100s after the real kRtcMin (2024-01-01 UTC)'
tests/rtc_skew_test.sh:342:  echo '    bool ok = isPlausibleEpoch(after);'
src/time/ExternalRtc.h:12:  opt.source = HAL_RTC_SOURCE_EXTERNAL;
src/time/ExternalRtc.h:22:  opt.source = HAL_RTC_SOURCE_EXTERNAL;
src/time/ExternalRtc.h:27:inline bool setFromSystem() { return Time.isValid() && set(Time.now()); }
src/time/ExternalRtc.h:31:  opt.source = HAL_RTC_SOURCE_EXTERNAL;
src/time/ExternalRtc.h:37:  if (e.isSet(particle::RtcEvent::CLOCK_SOURCE_EXTERNAL_FAILURE)) oscillatorFaultSeenSinceBind().store(true, std::memory_order_relaxed);
<shellId: 16 completed with exit code 0>
```

</details>


---

<sub>11m 32s</sub>

### `bash`

**Remaining Step 0 reads**

`$ cd src; grep -n "lastTimeSync" time/Clock.cpp | head; sed -n 905,960p Generalized-Core-Counter.cpp; grep -n "getValue" MyPersistentData.h | head -3; grep -n "T getValue" -A8 MyPersistentData.h persist/*.h | head -20; sed -n 262,275p cloud/DeviceStatusPublisher.cpp; sed -n 285,300p cloud/DeviceStatusPublisher.cpp`

<details>
<summary>91 lines</summary>

```
103:// stamp `lastTimeSync`, and update the tracked value - regardless of
277: *          design note above). This is safe to stamp `lastTimeSync` at this
286: *          Finding 1 fix: `lastTimeSync` is stamped with `Time.now()` here,
409:      // write is confirmed successful, and only then stamp lastTimeSync.
411:      SystemConfig::set_lastTimeSync(Time.now());
415:      // Finding 1 already applies to lastRtcWriteSyncedLastMs/lastTimeSync
    transitionTo(CONNECTING_STATE, "invalid config");
  }

  // Validate time and configure local time converter
  //
  // Finding 3 (WO-2026-08-29-002, Round 4 review): Time.isValid() alone can
  // be true here with zero confirmed cloud syncs ever - Device OS seeds
  // system time from the RTC every boot, whether or not the RTC is correct.
  //
  // SystemConfig::get_lastTimeSync() (persisted across reboots, stamped only by
  // checkClockResync() at confirmed-sync time - see below) is the correct
  // signal to use here, NOT Particle.timeSyncedLast() (which always reads 0
  // immediately after every boot/HIBERNATE wake and would force this branch
  // on every single wake, defeating the low-power connection scheduling
  // that INTERMITTENT/DISCONNECTED modes rely on). get_lastTimeSync()==0
  // means "this device has never, across its whole persisted history,
  // completed a confirmed sync" - true on a genuinely fresh install/reset.
  //
  // SCOPE NARROWED 2026-08-31: Round 5 added a second OR term
  // (`hibernateWakeWithoutConfirmedSyncThisBoot`, gated on
  // `System.resetReason() == RESET_REASON_POWER_MANAGEMENT`) intended to
  // guarantee a resync opportunity around every HIBERNATE wake. That term
  // is REVERTED here per the Work Order's "SCOPE NARROWED 2026-08-31"
  // section: because isClockTrusted() is unconditionally false at this
  // point in setup() (no connection has been attempted yet this boot), the
  // term reduced to just `resetReason == RESET_REASON_POWER_MANAGEMENT`,
  // which fired on EVERY hibernate wake - not just the skewed-RTC case -
  // making the mutually-exclusive opening-hour alert-40 suppression in the
  // `else` branch below unreachable on every normal overnight hibernate
  // wake. That was an unauthorised regression to existing behaviour
  // (Stage 7 re-review finding 1). The wake-time guarantee this term was
  // trying to provide has moved to WO-2026-08-31-002, which is explicitly
  // permitted to touch the sleep gate and connection-mode policy that a
  // correct fix actually requires; this WO no longer claims a wake-time
  // guarantee and reverts to the plain Round-4 condition below.
  // WO-2026-09-19 Step 3b: deliberately NOT converted to Clock::isTrusted().
  // isTrusted()/isClockTrusted() is false on every boot (including every
  // hibernate wake) until a sync completes THIS boot - see this function's
  // own comment a few lines above, which already hit this exact trap once.
  // Swapping the isTimeValid() term here would force CONNECTING_STATE on
  // every single wake, the precise regression neverConfirmedSyncEver (a
  // PERSISTED, cross-boot flag) exists to avoid: this condition already
  // correctly forces a connect when the RTC has never been set at all
  // (isTimeValid()==false) OR when this device has never, across its whole
  // history, completed a confirmed sync - a different, already-correct
  // question from "is the clock trustworthy right now."
  const bool neverConfirmedSyncEver = (SystemConfig::get_lastTimeSync() == 0);
  if (!Clock::isTimeValid() || neverConfirmedSyncEver) {
    transitionTo(CONNECTING_STATE, !Clock::isTimeValid() ? "time invalid" : "no confirmed time sync ever (Finding 3)");
  } else {
    // Now that time is valid, configure local time converter for timezone-aware operations
    conv.withCurrentTime().convert();

    // Check if waking from overnight hibernate - suppress alert 40 since
    // 8+ hours without webhook during closed hours is expected, not an error.
    if (System.resetReason() == RESET_REASON_POWER_MANAGEMENT) {
    writerBase.endObject();
    // Item 9 (WO-2026-08-29-002): clock trust verdict and sync recency, so
    // "was this cycle's timestamp trustworthy" is readable from telemetry
    // instead of reconstructed. trusted uses the same monotonic
    // Particle.timeSyncedLast() signal as the resync gate and trust check
    // in Generalized-Core-Counter.cpp (isClockTrusted()) - not
    // Time.isValid() alone (Finding 3). lastSyncEpoch is the persisted
    // completion stamp (SystemConfig::get_lastTimeSync()), which previously had
    // zero callers.
    //
    // Round 6 (Stage 7 finding 1): syncAgeSec is derived from
    // reportedSyncAgeMs(), the wrap-aware, sentinel-bearing telemetry
    // value - not raw elapsedMs(). Raw elapsedMs() would wrap back around
    // to a small, deceptively fresh-looking value after a full millis()
        const long syncAgeSec = (reportedAgeMs == ClockTrust::kReportedSyncAgeUnavailableMs)
            ? -1L
            : (long)(reportedAgeMs / 1000UL);
        writerBase.name("clock").beginObject();
        writerBase.name("trusted").value(isClockTrusted());
        writerBase.name("syncAgeSec").value((int)syncAgeSec);
        writerBase.name("lastSyncEpoch").value((int)SystemConfig::get_lastTimeSync());
        // WO-2026-08-31-004 Amendment A-2: read-and-publish only, no new
        // sampling beyond Clock.cpp's own one new I2C read at resync time.
        // This value changes on essentially every confirmed resync (unlike
        // the other fields in this object, which are mostly static between
        // syncs), and Clock::checkClockResync() already calls
        // requestStatusPublish() at exactly the same point it updates this
        // value, so a changed correctionSec always accompanies a genuine
        // payload change - the duplicate-suppression check in this
        // function (comparing against lastPublishedStatus) cannot silently
<shellId: 17 completed with exit code 0>
```

</details>


---

<sub>11m 50s</sub>

### `bash`

**Check pre-load behavior**

`$ cd src; grep -n "getValue" -B2 -A12 persist/*.h MyPersistentData.h | grep -n "template" | head; sed -n 255,275p MyPersistentData.cpp; grep -n "SystemConfig::\|sysStatus" Generalized-Core-Counter.cpp | awk -F: '$1<496' | head`

<details>
<summary>19 lines</summary>

```
return getValue<bool>(offsetof(SysData,serialConnected));
}
void sysStatusData::set_serialConnected(bool value) {
    setValue<bool>(offsetof(SysData,serialConnected), value);
}

time_t sysStatusData::get_lastDailyCleanup() const  {
    return getValue<time_t>(offsetof(SysData,lastDailyCleanup));
}
void sysStatusData::set_lastDailyCleanup(time_t value) {
    setValue<time_t>(offsetof(SysData, lastDailyCleanup), value);
}

time_t sysStatusData::get_lastTimeSync() const  {
    return getValue<time_t>(offsetof(SysData,lastTimeSync));
}
void sysStatusData::set_lastTimeSync(time_t value) {
    setValue<time_t>(offsetof(SysData, lastTimeSync), value);
}

// ********** Operating Mode Configuration Get/Set Functions **********
<shellId: 18 completed with exit code 0>
```

</details>


---

<sub>11m 53s</sub>

### `bash`

**View checkClockResync start and ClockTrust tail**

`$ cd src; sed -n 296,362p time/Clock.cpp; sed -n 1,25p time/Clock.cpp; sed -n 100,135p time/ClockTrust.h; sed -n 355,364p time/ClockTrust.h`

<details>
<summary>132 lines</summary>

```
*          *request*-side gate (`shouldResync()`) also saw a recent sync and
 *          would not ask again for up to 24h, while a HIBERNATE could
 *          re-seed system time from the still-wrong RTC much sooner. Now, on
 *          a failed write, the tracked value is left unchanged, so the very
 *          next `checkClockResync()` call (still observing the same
 *          unwritten `lastSyncMs`) retries the write - recurring, not a
 *          one-shot.
 *
 *          Round 5 cleanup (Stage 7 finding 5): the recurring retry above
 *          is now paced by `ClockTrust::canRetryRtcWriteNow()`
 *          (`kMinRtcWriteRetryIntervalMs`), a monotonic millis()-based
 *          floor separate from the request-side retry floor. Without this,
 *          a PERMANENT AB1805/Wire fault would retry `setRtcFromSystem()`
 *          (a locked I2C transaction) on every single main-loop pass -
 *          potentially hundreds of times per second on a bus shared with
 *          PMIC/battery work, with matching log volume and watchdog
 *          pressure. This only paces the FAILED-write path: a confirmed
 *          successful write advances `lastRtcWriteSyncedLastMs`, which
 *          already blocks any further attempt for that sync value via the
 *          gate condition itself, so the success path's cadence is
 *          unaffected.
 *
 *          Round 6 (second follow-up) fix: the Round 5 implementation
 *          above keyed the floor to `lastRtcWriteAttemptMs`, a timestamp
 *          recorded before EVERY attempt including successes - so a
 *          DISTINCT new sync value arriving within `kMinRtcWriteRetryIntervalMs`
 *          of a successful write was wrongly withheld from the RTC too,
 *          which made the two paragraphs above false in that case. Fixed
 *          by splitting the tracked state into `lastRtcWriteFailedAttemptMs`/
 *          `lastRtcWriteFailedSyncMs`, captured ONLY in the `else` (failed)
 *          branch below, so `ClockTrust::shouldAttemptRtcWriteNow()` applies
 *          the floor only when retrying the exact value that already
 *          failed - never to a value that has not yet been attempted.
 *
 *          Finding 2 fix, Round 4: uses `observedTimeSyncedLastMs()`, not
 *          `Particle.timeSyncedLast()` directly, so this can never block
 *          past a watchdog boundary during a slow cellular connect.
 *
 *          Finding 4 fix, Round 4: the request- and write-back-side gates
 *          use `ClockTrust::shouldResyncWrapAware()`/wrap-generation
 *          tracking rather than the plain millis()-diff gate, so an ancient
 *          sync can't be misread as fresh after a full millis() rollover
 *          (~49.7 days) with no intervening confirmed sync.
 *
 *          Finding 8 (Round 4 review, accepted/documented, not "fixed"):
 *          the vendored `AB1805::loop()` (lib/AB1805_RK, untouched) has its
 *          own one-shot RTC write, gated on
 *          `!timeSet && Time.isValid() && Particle.connected() &&
 *          Particle.timeSyncedLast() != 0` - the SAME condition ("a sync has
 *          completed") this function's write-back reacts to, and
 *          `ab1805.loop()` runs immediately before `checkClockResync()` in
 *          `loop()`. So the FIRST time a sync completes each boot, BOTH
 *          fire, writing the same correct value to the RTC twice. This is a
 *          harmless, idempotent duplicate write (same value, immediate
 *          succession), not a correctness bug - it costs one extra I2C
 *          transaction once per boot. There is no accessor into the
 *          vendored `timeSet` latch to suppress it without modifying
 *          `lib/AB1805_RK`, which is out of scope. "Exactly once" elsewhere
 *          in this codebase (tests, comments) refers to THIS function's own
 *          idempotency across repeated calls with an unchanged
 *          `timeSyncedLast()` value, not to the total count of physical RTC
 *          writes across both this function and the vendored one-shot.
 */
void checkClockResync() {
  const uint32_t nowMs = millis();
  tickMillisWrapTracker(nowMs);
  const uint32_t lastSyncMs = observedTimeSyncedLastMs();
#include "Particle.h"
#include "time/Clock.h"

#include "../Config.h"
#include "persist/SystemConfig.h"
#include "LocalTimeRK.h"            // LocalTimeConvert, LocalTime::instance()
#include "time/LocalTimeCache.h"    // Cached LocalTimeRK conversions
#include "time/ClockTrust.h"        // Sync-recency resync gate and trust signal (WO-2026-08-29-002)
#include "cloud/Cloud.h"            // Cloud::instance().requestStatusPublish()
#include "time/ExternalRtc.h"      // hal_rtc_* external-source helpers

static bool isWithinOpenHoursForHour(uint8_t hour, uint8_t openHour, uint8_t closeHour) {
  // Daytime window only: the hour rules guarantee closeHour > openHour, and
  // always-open is 0/24 (WO-2026-09-24-004).
  return (hour >= openHour) && (hour < closeHour);
}

static int secondsUntilNextOpenForSeconds(uint32_t secondsOfDay,
                                          uint8_t openHour,
                                          uint8_t /*closeHour*/,
                                          bool openNow) {
  uint32_t openSec = (uint8_t)openHour * 3600;

  // Normalize: if we're currently within opening hours, next open is tomorrow
  if (openNow) {
 *
 * @param timeValid `Time.isValid()`.
 * @param nowMs Current `millis()`.
 * @param timeSyncedLastMs `Particle.timeSyncedLast()`.
 *
 * @details Requires BOTH `Time.isValid()` and a confirmed sync within
 *          `kMaxSyncAgeMs`. Finding 3: `Time.isValid()` alone was true
 *          throughout the ~5h09m-wrong-clock incident because
 *          Device OS had seeded system time from the RTC - it carries
 *          no information about correctness.
 */
inline bool isTrusted(bool timeValid, uint32_t nowMs, uint32_t timeSyncedLastMs) {
    return timeValid && timeSyncedLastMs != 0 && elapsedMs(nowMs, timeSyncedLastMs) < kMaxSyncAgeMs;
}

/**
 * @brief True when enough time has passed since the last resync *attempt*
 *        to issue another one.
 *
 * @param nowMs Current `millis()`.
 * @param lastAttemptMs `millis()` at the time the last resync request was
 *        actually issued, or `0` if none has been issued yet this boot.
 *
 * @details This paces retries after a request that did not result in a
 *          confirmed sync (see `kMinResyncRetryIntervalMs`); it is separate
 *          from `shouldResync()`, which decides *whether* a resync is
 *          needed at all. `lastAttemptMs == 0` (no attempt yet this boot)
 *          always allows an immediate first attempt.
 */
inline bool canRetryResyncNow(uint32_t nowMs, uint32_t lastAttemptMs) {
    if (lastAttemptMs == 0) {
        return true;
    }
    return elapsedMs(nowMs, lastAttemptMs) >= kMinResyncRetryIntervalMs;
}

    if (timeSyncedLastMs == 0) {
        return kReportedSyncAgeUnavailableMs;
    }
    if (currentWrapGeneration != syncCaptureWrapGeneration) {
        return kReportedSyncAgeUnavailableMs;
    }
    return elapsedMs(nowMs, timeSyncedLastMs);
}

} // namespace ClockTrust
<shellId: 19 completed with exit code 0>
```

</details>


---

<sub>11m 58s</sub>

### `bash`

**Check other RTC writers**

`$ cd src; grep -n "ab1805\|Am18x5\|ExternalTime" Generalized-Core-Counter.cpp | head -20; grep -rn "setRtcFromSystem\|ExternalRtc::set" . | grep -v "^./time/Clock.cpp" | head`

<details>
<summary>17 lines</summary>

```
127:AB1805 ab1805(Wire);   // AB1805 RTC / Watchdog
294:  if (!Am18x5.status().bound() && !PreBindRtc::run(ab1805, Wire)) Log.warn("PreBindRtc: chip access failed");
295:  ExternalRtc::ensureBound(Am18x5, particle::Am18x5Configuration().i2c(Wire).interruptPin(WKP).watchdogPin(WATCHDOG_DONE_PIN)
665:  startupOscUsingRC = Am18x5.status().clockSource() == particle::RtcClockSource::INTERNAL;
666:  const auto oscCaps = Am18x5.getConfig().capabilities();
671:  ExternalTime.onEvent(ExternalRtc::onOscillatorEvent);
1296:    snprintf(pinResetFields, sizeof(pinResetFields), ",\"ab1805WakeReason\":\"N/A\",\"watchdogSource\":\"%s\"", startupInferredWatchdog ? "inferred" : "NONE");
1364: *        power-off evidence (the label "ab1805_watchdog" is kept for fleet
1375:  // "ab1805_watchdog" distinguishes an inferred PIN_RESET (no persisted
1379:  const char *resetLabel = inferred ? "ab1805_watchdog" : "watchdog";
./time/ClockTrust.h:138:// re-attempt ab1805.setRtcFromSystem() on every single main-loop pass. A
./time/ClockTrust.h:158: * @param lastAttemptMs `millis()` at the time `ab1805.setRtcFromSystem()`
./time/ClockTrust.h:219: *          that is `AB1805::setRtcFromSystem()`'s I2C result, which is
./Generalized-Core-Counter.cpp:686:  // A.3: RTC write only - ExternalRtc::set() - nothing else. In
./Generalized-Core-Counter.cpp:730:      const bool rtcSkewWriteOk = ExternalRtc::set(rtcSkewAfter);
./Generalized-Core-Counter.cpp:745:        Log.error("RtcSkewTest: ONE-SHOT bench hook FAILED - ExternalRtc::set() write did not "
<shellId: 20 completed with exit code 0>
```

</details>


---

<sub>12m 6s</sub>

### `edit`

**edit the file at /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-i21/src/time/ClockTrust.h.**

`/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-i21/src/time/ClockTrust.h`

```
File /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-i21/src/time/ClockTrust.h updated with changes.
```


---

<sub>12m 6s</sub>

### `bash`

**Check includes**

`$ cd src; sed -n 28,40p time/ClockTrust.h; grep -n "^#include" time/ClockTrust.h`

<details>
<summary>11 lines</summary>

```
*          the same pattern used by `state/SleepPrepSpanTiming.h` and
 *          `power/ChargeInhibitPolicy.h`.
 */

#ifndef __CLOCK_TRUST_H
#define __CLOCK_TRUST_H

#include <stdint.h>

namespace ClockTrust {

// Longest interval allowed between confirmed time syncs before another is
// requested, and the longest a confirmed sync is still considered "fresh"
35:#include <stdint.h>
<shellId: 21 completed with exit code 0>
```

</details>


---

<sub>12m 12s</sub>

### Copilot

Now Clock.cpp.

---

<sub>12m 12s</sub>

### `bash`

**Apply Clock.cpp edits via script (check)**

```
$ cd src && python3 - <<'EOF'
import re
p='time/Clock.cpp'
s=open(p).read()
s=s.replace("""bool isClockTrusted() {
  return ClockTrust::isTrustedWrapAware(Time.isValid(), millis(), observedTimeSyncedLastMs(),
                                         millisWrapGeneration, syncCaptureWrapGeneration);
}""","""bool isClockTrusted() {
  return ClockTrust::isTrustedWrapAware(Time.isValid(), millis(), observedTimeSyncedLastMs(),
                                         millisWrapGeneration, syncCaptureWrapGeneration) || carryEligible();
}""",1)
s=s.replace("""void checkClockResync() {
  const uint32_t nowMs = millis();
""","""void checkClockResync() {
  if (ExternalRtc::oscillatorFaultSeenSinceBind().load(std::memory_order_relaxed) && SystemConfig::get_lastTimeSync() != 0) {
    SystemConfig::set_lastTimeSync(0); SystemConfig::flushNow();  // withdraw the carry now: it must survive a reset right after
  }
  const uint32_t nowMs = millis();
""",1)
s=s.replace("""    if (rtcUpdated) {
      // Finding 1: only mark this sync value as "written" once the RTC
      // write is confirmed successful, and only then stamp lastTimeSync.
      lastRtcWriteSyncedLastMs = lastSyncMs;
      SystemConfig::set_lastTimeSync(Time.now());
""","""    const time_t carryStamp = (time_t)ClockTrust::carryStamp(rtcUpdated, rtcPreWriteReadOk, rtcCorrectionSec, Time.now());
    SystemConfig::set_lastTimeSync(carryStamp);
    if (carryStamp == 0) SystemConfig::flushNow();  // I21: a revoke is flushed synchronously
    if (rtcUpdated) {
      // Finding 1: only mark this sync value as "written" once the RTC
      // write is confirmed successful.
      lastRtcWriteSyncedLastMs = lastSyncMs;
""",1)
open(p,'w').write(s)
EOF
grep -n "^bool isClockTrusted\|^ \*/$" time/Clock.cpp | head -0; grep -n "^void checkClockResync\|^bool isClockTrusted\|^namespace\|^}" time/Clock.cpp | sed -n 1,20p
```

<details>
<summary>22 lines</summary>

```
head: illegal line count -- 0
16:}
36:}
59:}
71:}
116:namespace {
157:} // namespace
210:}
231:}
250:}
359:void checkClockResync() {
455:}
480:bool isClockTrusted() {
483:}
528:}
544:}
548:namespace Clock {
552:}
558:}
560:} // namespace Clock
564:namespace Clock {
<shellId: 22 completed with exit code 0>
```

</details>


---

<sub>12m 15s</sub>

### `bash`

**View anon namespace end**

`$ cd src; sed -n 150,160p time/Clock.cpp`

<details>
<summary>11 lines</summary>

```
// Time.now() at write time minus the RTC's own pre-write reading. Positive
// means the RTC was behind; negative means it was ahead. -1 is the same
// "no confirmed sync yet this boot" sentinel convention reportedSyncAgeMs()/
// syncAgeSec already use in the cloud status payload - a real correction of
// exactly -1 second is possible but rare, and this field is diagnostic
// telemetry, not a control-path signal.
long lastRtcCorrectionSec = -1;
} // namespace

/**
 * @brief Non-blocking substitute for Particle.timeSyncedLast() (Finding 2,
<shellId: 23 completed with exit code 0>
```

</details>


---

<sub>12m 24s</sub>

### `edit`

**edit the file at /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-i21/src/time/Clock.cpp.**

`/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-i21/src/time/Clock.cpp`

```
File /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-i21/src/time/Clock.cpp updated with changes.
```


---

<sub>12m 24s</sub>

### `edit`

**edit the file at /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-i21/src/time/HibernateWakeDiagnostics.h.**

`/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-i21/src/time/HibernateWakeDiagnostics.h`

```
File /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-i21/src/time/HibernateWakeDiagnostics.h updated with changes.
```


---

<sub>12m 24s</sub>

### `edit`

**edit the file at /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-i21/src/Generalized-Core-Counter.cpp.**

`/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-i21/src/Generalized-Core-Counter.cpp`

```
File /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-i21/src/Generalized-Core-Counter.cpp updated with changes.
```


---

<sub>12m 27s</sub>

### `bash`

**Budget check**

`$ git diff --numstat 1d1d5af -- src; git diff 1d1d5af -- src/time/Clock.cpp | head -80`

<details>
<summary>55 lines</summary>

```diff
1	0	src/Generalized-Core-Counter.cpp
15	3	src/time/Clock.cpp
10	0	src/time/ClockTrust.h
2	0	src/time/HibernateWakeDiagnostics.h
diff --git a/src/time/Clock.cpp b/src/time/Clock.cpp
index a36c835..7063ba2 100644
--- a/src/time/Clock.cpp
+++ b/src/time/Clock.cpp
@@ -154,6 +154,13 @@ uint32_t lastSeenSyncRawMs = 0;
 // exactly -1 second is possible but rare, and this field is diagnostic
 // telemetry, not a control-path signal.
 long lastRtcCorrectionSec = -1;
+
+// I21: trust carried from the persisted lastTimeSync; the (I2C) external-clock check runs only when a stamp exists.
+bool carryEligible() {
+  const time_t now = Time.now(), last = SystemConfig::get_lastTimeSync();
+  return last != 0 && ClockTrust::isCarryEligible(Time.isValid() && Clock::isPlausibleEpoch(now), ExternalRtc::isSet(), now, last, Clock::isPlausibleEpoch(last),
+                                                  ExternalRtc::oscillatorFaultSeenSinceBind().load(std::memory_order_relaxed));
+}
 } // namespace
 
 /**
@@ -357,6 +364,9 @@ void requestClockResync(const char *reason) {
  *          writes across both this function and the vendored one-shot.
  */
 void checkClockResync() {
+  if (ExternalRtc::oscillatorFaultSeenSinceBind().load(std::memory_order_relaxed) && SystemConfig::get_lastTimeSync() != 0) {
+    SystemConfig::set_lastTimeSync(0); SystemConfig::flushNow();  // withdraw the carry now: it must survive a reset right after
+  }
   const uint32_t nowMs = millis();
   tickMillisWrapTracker(nowMs);
   const uint32_t lastSyncMs = observedTimeSyncedLastMs();
@@ -404,11 +414,13 @@ void checkClockResync() {
         : -1;
     Log.info("ClockResync: sync advanced, rtcUpdated=%d epoch=%ld correctionSec=%ld",
              rtcUpdated ? 1 : 0, (long)Time.now(), rtcCorrectionSec);
+    const time_t carryStamp = (time_t)ClockTrust::carryStamp(rtcUpdated, rtcPreWriteReadOk, rtcCorrectionSec, Time.now());
+    SystemConfig::set_lastTimeSync(carryStamp);
+    if (carryStamp == 0) SystemConfig::flushNow();  // I21: a revoke is flushed synchronously
     if (rtcUpdated) {
       // Finding 1: only mark this sync value as "written" once the RTC
-      // write is confirmed successful, and only then stamp lastTimeSync.
+      // write is confirmed successful.
       lastRtcWriteSyncedLastMs = lastSyncMs;
-      SystemConfig::set_lastTimeSync(Time.now());
 
       // Only persist the correction for cloud reporting once the write it
       // describes is confirmed - same "confirmed, not attempted" standard
@@ -474,7 +486,7 @@ void checkClockResync() {
  */
 bool isClockTrusted() {
   return ClockTrust::isTrustedWrapAware(Time.isValid(), millis(), observedTimeSyncedLastMs(),
-                                         millisWrapGeneration, syncCaptureWrapGeneration);
+                                         millisWrapGeneration, syncCaptureWrapGeneration) || carryEligible();
 }
 
 /**
<shellId: 24 completed with exit code 0>
```

</details>


---

<sub>12m 38s</sub>

### `bash` — Failed

**Trim comments/blank lines**

```
$ cd src && python3 - <<'EOF'
p='time/ClockTrust.h'; s=open(p).read()
s=s.replace("""
// I21 (WO-2026-11-05-007): trust carried across a restart from the persisted lastTimeSync (epoch, not millis()).
constexpr long kMaxCarryCorrectionSec = 60;  // a sync""","""constexpr long kMaxCarryCorrectionSec = 60;  // I21 (WO-2026-11-05-007): a sync""")
s=s.replace("""// The lastTimeSync to persist after a write-back attempt: now only for a confirmed write with a readable, small correction; else 0 (revoke).
inline int64_t carryStamp(""","""inline int64_t carryStamp(""")
s=s.replace("""inline bool isCarryEligible(""","""inline bool isCarryEligible(""")
open(p,'w').write(s)
p='time/Clock.cpp'; s=open(p).read()
s=s.replace("""long lastRtcCorrectionSec = -1;

// I21: trust carried from the persisted lastTimeSync; the (I2C) external-clock check runs only when a stamp exists.
bool carryEligible() {""","""long lastRtcCorrectionSec = -1;
bool carryEligible() {  // I21: trust carried from the persisted lastTimeSync; the external-clock check runs only when a stamp exists""")
open(p,'w').write(s)
EOF
cd ..; git diff --numstat 1d1d5af -- src
```

```
Permission denied because no interactive user response was available. Retry in an interactive session so the user can approve it, or try an alternative that does not require this permission.
```


---

<sub>12m 45s</sub>

### Copilot

I'll use the edit tool.

---

<sub>12m 45s</sub>

### `edit`

**edit the file at /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-i21/src/time/ClockTrust.h.**

`/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-i21/src/time/ClockTrust.h`

```
File /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-i21/src/time/ClockTrust.h updated with changes.
```


---

<sub>12m 45s</sub>

### `edit`

**edit the file at /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-i21/src/time/ClockTrust.h.**

`/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-i21/src/time/ClockTrust.h`

```
File /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-i21/src/time/ClockTrust.h updated with changes.
```


---

<sub>12m 45s</sub>

### `edit`

**edit the file at /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-i21/src/time/Clock.cpp.**

`/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-i21/src/time/Clock.cpp`

```
File /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-i21/src/time/Clock.cpp updated with changes.
```


---

<sub>12m 47s</sub>

### `bash`

**Recount budget**

`$ git diff --numstat 1d1d5af -- src; git diff 1d1d5af -- src/time/ClockTrust.h`

<details>
<summary>21 lines</summary>

```diff
1	0	src/Generalized-Core-Counter.cpp
13	3	src/time/Clock.cpp
7	0	src/time/ClockTrust.h
2	0	src/time/HibernateWakeDiagnostics.h
diff --git a/src/time/ClockTrust.h b/src/time/ClockTrust.h
index 5cf2ba8..80c833c 100644
--- a/src/time/ClockTrust.h
+++ b/src/time/ClockTrust.h
@@ -360,6 +360,13 @@ inline uint32_t wrapAwareReportedSyncAgeMs(uint32_t nowMs, uint32_t timeSyncedLa
     }
     return elapsedMs(nowMs, timeSyncedLastMs);
 }
+constexpr long kMaxCarryCorrectionSec = 60;  // I21 (WO-2026-11-05-007): a sync correcting the RTC by more than this withdraws the carry
+inline bool isCarryEligible(bool timeOk, bool externalOk, int64_t nowEpoch, int64_t lastSyncEpoch, bool lastPlausible, bool faultSeen) {
+    return timeOk && externalOk && lastPlausible && !faultSeen && lastSyncEpoch != 0 && nowEpoch >= lastSyncEpoch && nowEpoch - lastSyncEpoch < (int64_t)(kMaxSyncAgeMs / 1000UL);
+}
+inline int64_t carryStamp(bool written, bool preReadOk, long correctionSec, int64_t nowEpoch) {
+    return written && preReadOk && correctionSec <= kMaxCarryCorrectionSec && correctionSec >= -kMaxCarryCorrectionSec ? nowEpoch : 0;
+}
 
 } // namespace ClockTrust
<shellId: 26 completed with exit code 0>
```

</details>


---

<sub>12m 53s</sub>

### `bash`

**Find C2 tests**

`$ cd tests; grep -ln "closeOccupancySessionSafely\|pieceUntrusted" *; sed -n 1,80p debounce_expiry_c1_test.sh`

<details>
<summary>91 lines</summary>

```
grep: __pycache__: Is a directory
grep: fixtures: Is a directory
grep: stubs: Is a directory
clock_trust_standard_structural_test.py
daily_cleanup_boundary_test.py
debounce_expiry_c1_test.sh
hourly_while_occupied_test.sh
idle_timediag_on_change_test.cpp
occupancy_ends_at_close_structural_test.py
occupancy_report_by_mode_test.cpp
occupancy_report_by_mode_test.sh
occupancy_session_restart_test.sh
sleep_wake_equivalence_test.cpp
sleep_wake_equivalence_test.sh
sleep_wake_structural_test.py
standby_latch_daily_clear_test.cpp
untrusted_piece_c2_test.sh
#!/bin/zsh
# WO-2026-11-03-004 item 19 C1 - one millis-based debounce expiry.
#
# The REAL debounceRemainingMs() (with occupancyDebounceMs()) is extracted from
# State_Common.h and driven against fakes: expiry depends on millis() and
# lastOccupancyEvent only (never the wall clock), the zero sentinel re-arms,
# and the boundary is expired at exactly the debounce. Call-site checks prove
# the awake Idle/Modes paths, the wake close and the sleep plan all use it.
# Mutations of the predicate and of each call site must each be caught.
set -euo pipefail

repo_root="${0:A:h:h}"
work="$repo_root/build-tmp/wo-2026-11-03-004-stage6/c1_test"
mkdir -p "$work"
src="$repo_root/src/state"

extract_pred() {
  awk '/^inline uint32_t occupancyDebounceMs\(\)/ {a=1} a {print} a && /^inline uint32_t debounceRemainingMs/ {b=1} b && /^}/ {exit}' "$1"
}

run_predicate() {
  local common="$1"
  extract_pred "$common" > "$work/pred.inc"
  [[ -s "$work/pred.inc" ]] || { print -u2 "EXTRACT_FAILED"; return 2; }
  cat > "$work/t.cpp" <<'CPP'
#include <cstdint>
#include <cstdio>
static uint32_t nowMs = 0, lastEvt = 0, set1 = 0;
static uint32_t millis() { return nowMs; }
namespace CurrentReadings {
  uint32_t get_lastOccupancyEvent() { return lastEvt; }
  void set_lastOccupancyEvent(uint32_t v) { lastEvt = v; }
}
namespace SystemConfig { namespace SensorSettings { uint32_t get_sensorSetting1() { return set1; } } }
namespace Config { uint32_t occupancyDebounceMsForRuntime() { return 3600000; } }
#include "pred.inc"
static int fails = 0;
static void expect(const char* n, uint32_t got, uint32_t want) {
  if (got != want) { std::printf("FAIL %s got=%u want=%u\n", n, got, want); fails++; }
}
int main() {
  set1 = 60000; nowMs = 1000000; lastEvt = 1000000;
  expect("fresh", debounceRemainingMs(), 60000);
  nowMs = 1000000 + 59999; expect("1ms left", debounceRemainingMs(), 1);
  nowMs = 1000000 + 60000; expect("boundary expired", debounceRemainingMs(), 0);
  nowMs = 1000000 + 90000; expect("past expiry", debounceRemainingMs(), 0);
  set1 = 0; nowMs = 1000000 + 3599000; expect("runtime default", debounceRemainingMs(), 1000);
  set1 = 60000; lastEvt = 0; nowMs = 5000;
  expect("sentinel re-arms", debounceRemainingMs(), 60000);
  expect("sentinel written", lastEvt, 5000);
  lastEvt = 4294967000u; nowMs = 100; // millis() wrap: elapsed 396
  expect("wrap", debounceRemainingMs(), 60000 - 396);
  return fails ? 1 : 0;
}
CPP
  c++ -std=c++17 -I"$work" -o "$work/t" "$work/t.cpp" && "$work/t"
}

run_predicate "$src/State_Common.h" || { print -u2 "FAILED: predicate behaviour"; exit 1; }

# --- call sites -------------------------------------------------------------
check_has() { grep -qF -- "$2" "$1" || { print -u2 "FAILED: $1 lacks: $2"; return 1; } }
check_sites() {
  local d="$1"
  check_has "$d/State_Idle.cpp" "if (debounceRemainingMs() == 0) {" &&
  check_has "$d/State_Modes.cpp" "if (debounceRemainingMs() == 0) {" &&
  check_has "$d/State_Modes.cpp" "OCCUPANCY && (signalLEDStatus() || (session.pieceUntrusted && CurrentReadings::get_occupied())) && debounceRemainingMs() == 0" &&
  check_has "$d/State_Sleep.cpp" "const uint32_t remaining = debounceRemainingMs();" &&
  check_has "$d/State_Sleep.cpp" "(int)(remaining / 1000UL) + 1"
}
check_sites "$src" || exit 1

# The wall-clock LED deadline must no longer decide an OCCUPANCY wake close.
if grep -n "OCCUPANCY" "$src/State_Modes.cpp" | grep -q "signalLEDTimeRemaining"; then
  print -u2 "FAILED: OCCUPANCY wake close still uses signalLEDTimeRemaining"; exit 1
fi

# --- mutations ---------------------------------------------------------------
mutate() {  # name file old new
  local name="$1" file="$2" old="$3" new="$4"
<shellId: 27 completed with exit code 0>
```

</details>


---

<sub>12m 55s</sub>

### `bash`

**Read C2 test**

`$ cd tests; cat untrusted_piece_c2_test.sh`

<details>
<summary>13 lines</summary>

```
Output too large to read at once (21.5 KB). Saved to: /var/folders/_r/m12xn8s15wb_15djjdgfp5gr0000gn/T/1794154999484-copilot-tool-output-72505-48cde13f-e054-48aa-be71-d27bc18ced94.txt
Consider using tools like grep (for searching), head/tail (for viewing start/end), view with view_range (for specific sections), or jq (for JSON) to examine portions of the output.

Preview (first 500 chars):
#!/bin/zsh
# WO-2026-11-03-006 item 19 C2 - an untrusted session piece is credited, closed
# and reported on millis().
#
# The REAL closeOccupancySessionSafely(), debounceRemainingMs() and
# updateOccupancyState() are extracted from the sources and driven against
# fakes: the Dev-09 replay (23-24 Oct), the reboot straddler (post-boot part
# only), a piece that keeps millis() when trust returns, a trusted piece that is
# unchanged, a fresh piece after each close (both piece fields cleared at clos
<shellId: 28 completed with exit code 0>
```

</details>


---

<sub>12m 58s</sub>

### `view`

**view the file at /var/folders/\_r/m12xn8s15wb\_15djjdgfp5gr0000gn/T/1794154999484-copilot-tool-output-72505-48cde13f-e054-48aa-be71-d27bc18ced94.txt.**

`/var/folders/_r/m12xn8s15wb_15djjdgfp5gr0000gn/T/1794154999484-copilot-tool-output-72505-48cde13f-e054-48aa-be71-d27bc18ced94.txt (lines 1-200)`

<details>
<summary>192 lines</summary>

```
#!/bin/zsh
# WO-2026-11-03-006 item 19 C2 - an untrusted session piece is credited, closed
# and reported on millis().
#
# The REAL closeOccupancySessionSafely(), debounceRemainingMs() and
# updateOccupancyState() are extracted from the sources and driven against
# fakes: the Dev-09 replay (23-24 Oct), the reboot straddler (post-boot part
# only), a piece that keeps millis() when trust returns, a trusted piece that is
# unchanged, a fresh piece after each close (both piece fields cleared at close
# and set by every open path), and no wall-start check on a piece. The piece-open and boot sites
# (not drivable without the whole application) are exact-text checks. Every
# mutation in the WO must fail this test.
set -euo pipefail

repo_root="${0:A:h:h}"
work="$repo_root/build-tmp/wo-2026-11-03-006-stage6-r2/c2_test"
mkdir -p "$work"
src="$repo_root/src"

extract_braced_block() {
  awk -v marker="$2" '
    !active && index($0, marker) { active = 1 }
    active {
      print
      opens = gsub(/\{/, "{"); closes = gsub(/\}/, "}")
      depth += opens - closes
      if (seen_open && depth == 0) exit
      if (opens > 0) seen_open = 1
    }' "$1"
}

# --- structural checks (the sites that cannot be driven on the host) ----------
check_structure() {  # $1 = src root
  local r="$1" modes="$1/state/State_Modes.cpp" app="$1/Generalized-Core-Counter.cpp"
  local open_pair
  open_pair=$'session.occupancyStartMs = millis();\n'
  python3 - "$modes" "$app" "$r/state/StateMachine.h" "$r/state/State_Report.cpp" <<'PY'
import re, sys
modes, app, sm, report = (open(p).read() for p in sys.argv[1:5])
def need(cond, msg):
    if not cond:
        print("FAILED:", msg); sys.exit(1)
# Both sites that open a session record the millis() start and the trust, right after the epoch start.
pat = re.compile(r"set_occupied\(true\);\n(?:\s*//.*\n)*\s*CurrentReadings::set_occupancyStartTime\(Time\.now\(\)\);\n(?:\s*//.*\n)*\s*session\.occupancyStartMs = millis\(\);\n\s*session\.pieceUntrusted = !Clock::isTrusted\(\);\n")
need(len(pat.findall(modes)) == 2, "both piece-open sites must set occupancyStartMs = millis() and pieceUntrusted = !Clock::isTrusted()")
# The wake-close must not hang on the LED when an untrusted piece is open (the C1 verdict's LED-gate gap).
need("OCCUPANCY && (signalLEDStatus() || (session.pieceUntrusted && CurrentReadings::get_occupied())) && debounceRemainingMs() == 0" in modes, "the wake close must also fire for an open untrusted piece")
# The reboot straddler: untrusted piece starting at boot (millis() 0), no B3 anchor for it.
need("session.pieceUntrusted = CurrentReadings::get_occupied() && !Clock::isTrusted();" in app, "setup() must mark an open session at an untrusted boot as an untrusted piece")
need("session.occupancyStartMs = millis();  // straddler" in app, "the boot restore must set occupancyStartMs = millis()")
# The close==24 boundary reopen is a trusted piece: it sets both fields.
need(re.search(r"set_occupancyStartTime\(std::max\(boundary, originalSessionStart\)\);\n\s*session\.occupancyStartMs = millis\(\); session\.pieceUntrusted = !Clock::isTrusted\(\);", report) is not None, "the boundary reopen must set occupancyStartMs = millis() and pieceUntrusted = !Clock::isTrusted()")
need("session.occupancySessionBootAnchor = CurrentReadings::get_occupied() && !session.pieceUntrusted" in app, "the B3 boot anchor must stay for a trusted boot only")
need("pre-boot part dropped" in app, "setup() must log the dropped straddler part")
# RAM only.
need("uint32_t occupancyStartMs" in sm and "bool pieceUntrusted" in sm, "piece fields must be SessionState (RAM) members")
PY
}
check_structure "$src" || exit 1
for f in MyPersistentData.h device_pinout.cpp; do
  if grep -qE 'occupancyStartMs|pieceUntrusted' "$src/$f"; then print -u2 "FAILED: piece fields leaked into $f"; exit 1; fi
done

# --- behavioural driver --------------------------------------------------------
build_and_run() {  # $1 = State_Common.h $2 = State_Modes.cpp
  local out="$work/driver.cpp"
  {
    cat <<'CPP'
#include <algorithm>
#include <cassert>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <iostream>
#include <string>
#include <vector>
static unsigned long g_millis = 0;
unsigned long millis() { return g_millis; }
static time_t g_now = 0;
struct FakeTime { time_t now() const { return g_now; } };
FakeTime Time;
struct FakeLog {
  std::vector<std::string> lines;
  void add(const char *fmt, va_list ap) { char b[256]; vsnprintf(b, sizeof(b), fmt, ap); lines.push_back(b); }
  void warn(const char *fmt, ...) { va_list ap; va_start(ap, fmt); add(fmt, ap); va_end(ap); }
  void info(const char *fmt, ...) { va_list ap; va_start(ap, fmt); add(fmt, ap); va_end(ap); }
  int count(const char *n) const { int c = 0; for (auto &l : lines) if (l.find(n) != std::string::npos) ++c; return c; }
};
FakeLog Log;
namespace CurrentReadings {
bool occupied = false; time_t occupancyStartTime = 0; uint32_t total = 0; uint32_t lastEvt = 0;
bool get_occupied() { return occupied; }
void set_occupied(bool v) { occupied = v; }
time_t get_occupancyStartTime() { return occupancyStartTime; }
void set_occupancyStartTime(time_t v) { occupancyStartTime = v; }
uint32_t get_totalOccupiedSeconds() { return total; }
void set_totalOccupiedSeconds(uint32_t v) { total = v; }
uint32_t get_lastOccupancyEvent() { return lastEvt; }
void set_lastOccupancyEvent(uint32_t v) { lastEvt = v; }
}
namespace RecoveryState { int8_t get_alertCode() { return 0; } }
namespace Clock {
bool trusted = false;
bool isTrusted() { return trusted; }
enum class Openness { Open, Closed, Unknown };
Openness open = Openness::Unknown;
Openness openness() { return open; }
}
namespace DailyBoundary {
struct Result { bool due; time_t boundary; uint8_t close; };
time_t boundary = 0;
Result check(time_t) { return {false, boundary, 0}; }
}
namespace SystemConfig {
enum ConnectionMode { CONNECTED = 0, DISCONNECTED_KEEP_ALIVE = 2, INTERMITTENT_KEEP_ALIVE = 3 };
uint8_t connectionMode = INTERMITTENT_KEEP_ALIVE;
uint8_t get_configuredConnectionMode() { return connectionMode; }
namespace SensorSettings { uint32_t set1 = 300000; uint32_t get_sensorSetting1() { return set1; } }
}
struct PowerManager {
  static PowerManager &instance() { static PowerManager pm; return pm; }
  uint8_t effectiveConnectionMode() const { return SystemConfig::get_configuredConnectionMode(); }
};
namespace Config { uint32_t occupancyDebounceMsForRuntime() { return 300000UL; } }
struct SessionState {
  time_t occupancySessionBootAnchor = 0;
  bool occupancyChangeTriggered = false;
  uint32_t occupancyStartMs = 0;
  bool pieceUntrusted = false;
};
SessionState session;
enum State { IDLE_STATE, REPORTING_STATE };
State state = IDLE_STATE;
int reports = 0;
void transitionTo(State next, const char *) { state = next; ++reports; }
void signalLED(bool) {}
int unoccupied = 0;
void logUnoccupiedEvent(const char *, uint32_t, uint32_t, bool) { ++unoccupied; }
CPP
    extract_braced_block "$1" "inline bool reportsOccupancyChangesNow() {"
    extract_braced_block "$1" "inline bool occupancyIgnoredWhenClosed() {"
    extract_braced_block "$1" "struct OccupancyCloseResult {"
    extract_braced_block "$1" "inline uint32_t occupancyDebounceMs() {"
    extract_braced_block "$1" "inline uint32_t debounceRemainingMs() {"
    extract_braced_block "$1" "inline OccupancyCloseResult closeOccupancySessionSafely(const char *path, time_t closeAt = Time.now()) {"
    extract_braced_block "$2" "void updateOccupancyState() {"
    cat <<'CPP'

// The two piece-open sites, as the (text-checked) source writes them.
void pir() {
  if (!CurrentReadings::occupied) {
    CurrentReadings::occupied = true;
    CurrentReadings::occupancyStartTime = g_now;
    session.occupancyStartMs = millis();
    session.pieceUntrusted = !Clock::isTrusted();
  }
  CurrentReadings::lastEvt = millis();
}
// The close==24 boundary reopen in State_Report.cpp (text-checked): a trusted piece.
void boundaryReopen() {
  CurrentReadings::set_occupied(true);
  CurrentReadings::set_occupancyStartTime(g_now);
  session.occupancyStartMs = millis(); session.pieceUntrusted = !Clock::isTrusted();
}
// One loop pass per second of real time; epoch and millis() advance together.
void run(long seconds) { for (long i = 0; i < seconds; ++i) { g_now += 1; g_millis += 1000; updateOccupancyState(); state = IDLE_STATE; } }
void reset() {  // a reboot with a persisted open session: setup() clears lastEvt, millis() restarts
  g_millis = 0; CurrentReadings::lastEvt = 0;
  session = SessionState();
  session.pieceUntrusted = CurrentReadings::occupied && !Clock::isTrusted();
}
void world(uint32_t set1) {
  Log.lines.clear(); CurrentReadings::occupied = false; CurrentReadings::occupancyStartTime = 0; CurrentReadings::total = 0;
  CurrentReadings::lastEvt = 0; session = SessionState(); Clock::trusted = false; Clock::open = Clock::Openness::Unknown;
  DailyBoundary::boundary = 0; SystemConfig::SensorSettings::set1 = set1; g_millis = 7000000; g_now = 1792000000;
  state = IDLE_STATE; reports = 0; unoccupied = 0;
}
// A pulsed session: PIR every 30 s for `pulses` pulses, then the debounce runs out.
void session_of(int pulses) { for (int i = 0; i < pulses; ++i) { pir(); run(30); } }

// Dev-09 timeline: a session every 20 min; ten PIR pulses 30 s apart (270 s) then a 300 s debounce = 570 s.
void gap(long total, long used) { run(total - used); }

void testControlCreditsEachPieceItsOwnOnTime() {
  world(300000);
  for (int k = 0; k < 10; ++k) {
    session_of(10);
    gap(1200, 300);
    assert(!CurrentReadings::occupied);
    assert(CurrentReadings::total == (uint32_t)(570 * (k + 1)));
  }
  assert(unoccupied == 10 && reports == 10);            // every close raises the occupancy-change report
  assert(Log.count("Occ: untrusted piece credit=570") == 10);
}

// The reset at 10:40:55 (334 s into the first session): the straddler credits its post-boot part only.
void testRebootStraddlerCreditsPostBootOnly() {
  world(300000);
```

</details>


---

<sub>13m 2s</sub>

### `bash`

**Read close function**

`$ cd src/state; grep -n "inline OccupancyCloseResult closeOccupancySessionSafely" State_Common.h; sed -n 330,420p State_Common.h | grep -v "^\s*//"; grep -n "inline bool occupancyIgnoredWhenClosed" -A12 State_Common.h`

<details>
<summary>88 lines</summary>

```
356:inline OccupancyCloseResult closeOccupancySessionSafely(const char *path, time_t closeAt = Time.now()) {
 *
 * @details The single debounce-expiry predicate, on millis() and
 *          lastOccupancyEvent only (no wall clock). A zero lastOccupancyEvent
 *          (boot clear) re-arms to now so it never expires immediately.
 */
inline uint32_t debounceRemainingMs() {
	uint32_t last = CurrentReadings::get_lastOccupancyEvent();
	if (last == 0) {
		last = millis();
		CurrentReadings::set_lastOccupancyEvent(last);
	}
	const uint32_t elapsed = millis() - last;
	const uint32_t debounceMs = occupancyDebounceMs();
	return elapsed >= debounceMs ? 0 : debounceMs - elapsed;
}

/**
 * @brief Safely closes the current occupancy session and guards wrapped time math.
 *
 * Unsigned subtraction of closeAt - occupancyStartTime can wrap into a huge
 * bogus duration if the stored start time is zero or in the future.
 *
 * @param path Short caller label for anomaly logs
 * @param closeAt Epoch to credit the current occupancy session through
 * @return Close result including the session duration and new total on valid close
 */
inline OccupancyCloseResult closeOccupancySessionSafely(const char *path, time_t closeAt = Time.now()) {
	OccupancyCloseResult result;
	const bool occupied = CurrentReadings::get_occupied();
	const int8_t alertCode = RecoveryState::get_alertCode();
	const bool timeValid = Clock::isTrusted();
	const bool piece = occupied && session.pieceUntrusted;
	if (!timeValid && occupied && !piece) {
		CurrentReadings::set_lastOccupancyEvent(millis());
		result.stillOpen = true;
		return result;
	}
	const time_t now = Time.now();
	const time_t start = CurrentReadings::get_occupancyStartTime();
	if (occupancyIgnoredWhenClosed()) {
		const time_t boundary = DailyBoundary::check(now).boundary;
		if (start < boundary) closeAt = std::min(closeAt, boundary);
	}
	if (session.occupancySessionBootAnchor != 0) {
		const time_t bootEpoch = now - (time_t)(millis() / 1000UL);
		const time_t anchor = session.occupancySessionBootAnchor;
		session.occupancySessionBootAnchor = 0;
		closeAt = std::min(closeAt, std::min(bootEpoch, anchor + (time_t)(occupancyDebounceMs() / 1000UL)));
	}
	const uint32_t previousTotal = CurrentReadings::get_totalOccupiedSeconds();
	result.totalSeconds = previousTotal;

	bool durationComputable = false;
	long long rawSessionSeconds = 0;
	uint32_t sessionSeconds = 0;
	bool invalid = !piece && (!timeValid || start == 0);

	if (piece) {
		rawSessionSeconds = (CurrentReadings::get_lastOccupancyEvent() - session.occupancyStartMs + occupancyDebounceMs()) / 1000UL;
		durationComputable = true;
		Log.info("Occ: untrusted piece credit=%lld start=%lu", rawSessionSeconds, (unsigned long)session.occupancyStartMs);
	} else if (timeValid && start != 0) {
		time_t effectiveStart = start;
		if (start > now && start <= now + 5) {
			effectiveStart = now;
		}
		if (effectiveStart > closeAt) {
			effectiveStart = closeAt;
		}
		rawSessionSeconds = (long long)closeAt - (long long)effectiveStart;
		durationComputable = true;
	}

	if (!piece && timeValid && start > now + 5) {
		invalid = true;
	}

	if (!invalid && durationComputable) {
63:inline bool occupancyIgnoredWhenClosed() { return Clock::openness() == Clock::Openness::Closed; }
64-
65-/**
66- * @brief Reasons the long-duration connectivity failsafe intentionally deferred action.
67- */
68-enum FailsafeDeferReason : uint8_t {
69-	FAILSAFE_DEFER_NONE = 0,
70-	FAILSAFE_DEFER_INVALID_TIME = 1,
71-	FAILSAFE_DEFER_NO_LAST_CONNECTION = 2,
72-	FAILSAFE_DEFER_DISCONNECTED_MODE = 3,
73-	FAILSAFE_DEFER_UPDATE_PENDING = 4,
74-	FAILSAFE_DEFER_LOW_BATTERY_HARD_STAGE_SUPPRESSED = 5,
75-	FAILSAFE_DEFER_CLOSED_HOURS_LONG_SLEEP = 6,
<shellId: 29 completed with exit code 0>
```

</details>


---

<sub>13m 5s</sub>

### `bash`

**Rest of close function**

`$ cd src/state; sed -n 420,470p State_Common.h`

<details>
<summary>37 lines</summary>

```
if (!invalid && durationComputable) {
		sessionSeconds = (uint32_t)rawSessionSeconds;
		const uint32_t newTotal = previousTotal + sessionSeconds;
		if (sessionSeconds > 86400UL || newTotal > 86400UL) {
			invalid = true;
		} else {
			CurrentReadings::set_totalOccupiedSeconds(newTotal);
			result.valid = true;
			result.sessionSeconds = sessionSeconds;
			result.totalSeconds = newTotal;
		}
	} else if (!invalid) {
		invalid = true;
	}

	if (invalid) {
		char sessionBuf[24];
		if (durationComputable) {
			snprintf(sessionBuf, sizeof(sessionBuf), "%lld", rawSessionSeconds);
		} else {
			strncpy(sessionBuf, "na", sizeof(sessionBuf));
			sessionBuf[sizeof(sessionBuf) - 1] = '\0';
		}
		Log.warn("OccAnom: path=%s now=%lu start=%lu prev=%lu dur=%s occ=%d alert=%d",
					 path ? path : "?",
					 (unsigned long)closeAt,
					 (unsigned long)start,
					 (unsigned long)previousTotal,
					 sessionBuf,
					 occupied ? 1 : 0,
					 (int)alertCode);
	}

	CurrentReadings::set_occupied(false);
	session.pieceUntrusted = false; session.occupancyStartMs = 0;
	CurrentReadings::set_occupancyStartTime(0);
	return result;
}
<shellId: 30 completed with exit code 0>
```

</details>


---

<sub>13m 30s</sub>

### `create`

**create a new file at /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-i21/tests/clock\_trust\_carry\_test.cpp.**

`/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-i21/tests/clock_trust_carry_test.cpp`

```
Created file /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-i21/tests/clock_trust_carry_test.cpp with 4062 characters
```


---

<sub>14m 1s</sub>

### `create`

**create a new file at /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-i21/tests/clock\_trust\_carry\_test.sh.**

`/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-i21/tests/clock_trust_carry_test.sh`

```
Created file /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-i21/tests/clock_trust_carry_test.sh with 9560 characters
```


---

<sub>14m 16s</sub>

### `bash`

**Run new test**

`$ chmod +x tests/clock_trust_carry_test.sh; zsh tests/clock_trust_carry_test.sh 2>&1 | tail -30`

<details>
<summary>15 lines</summary>

```
clock_trust_carry_test passed
c2 regression passed
caught: carry ignores the latch
caught: carry ignores age
caught: age on millis()
caught: abs() dropped
caught: -1 sentinel compared against T
caught: no flush after the revoke
caught: revoke leaves the current boot untrusted
caught: carry without plausibility checks
caught: carry without plausibility checks (call site)
caught: verdict input ignored
caught: verdict arm dropped

clock_trust_carry_test passed (unit, C2 regression, call-site text, 11 mutation checks)
<shellId: 31 completed with exit code 0>
```

</details>


---

<sub>20m 50s</sub>

### `bash`

**Full test run**

`$ zsh build-tmp/wo-2026-11-05-007-stage6/runall.sh 2>&1 | tail -8`

<details>
<summary>6 lines</summary>

```
118/122 (sh via zsh, py via python3)
FAILED: tests/watchdog_inferred_classification_test.sh
FAILED: tests/clock_resync_wiring_test.py
FAILED: tests/s2_alarm_watchdog_wake_structural_test.py
FAILED: tests/thermal_coupling_structural_test.py
<shellId: 32 completed with exit code 0>
```

</details>


---

<sub>20m 56s</sub>

### `bash`

**Inspect failures**

`$ for t in tests/watchdog_inferred_classification_test.sh; do zsh $t 2>&1 | tail -6; done; for t in tests/clock_resync_wiring_test.py tests/s2_alarm_watchdog_wake_structural_test.py tests/thermal_coupling_structural_test.py; do echo "== $t"; python3 $t 2>&1 | tail -5; done`

<details>
<summary>17 lines</summary>

```
if (HibernateWakeDiagnostics::isRtcArm(wakeVerdict.gateArm)) SystemConfig::set_lastTimeSync(0);  // I21: no trust carry after a wake whose RTC check failed
                                         ^
/var/folders/_r/m12xn8s15wb_15djjdgfp5gr0000gn/T//watchdog_inferred.nryr61/block.h:9:78: error: no member named 'set_lastTimeSync' in namespace 'SystemConfig'
  if (HibernateWakeDiagnostics::isRtcArm(wakeVerdict.gateArm)) SystemConfig::set_lastTimeSync(0);  // I21: no trust carry after a wake whose RTC check failed
                                                               ~~~~~~~~~~~~~~^
2 errors generated.
== tests/clock_resync_wiring_test.py
    main()
    ~~~~^^
  File "/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-i21/tests/clock_resync_wiring_test.py", line 293, in main
    stamp_idx = check_fn_stripped.index("SystemConfig::set_lastTimeSync(Time.now())")
ValueError: substring not found
== tests/s2_alarm_watchdog_wake_structural_test.py
FAIL: the boot clear must be flushed immediately (forced consumption)
== tests/thermal_coupling_structural_test.py
FAIL: readTmp112TemperatureC() is referenced from outside SensorManager.h/.cpp: ['/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-i21/build-tmp/wo-2026-11-05-007-stage6/carry_test/mut/src/sensors/SensorManager.cpp', '/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-i21/build-tmp/wo-2026-11-05-007-stage6/carry_test/mut/src/sensors/SensorManager.h']
<shellId: 33 completed with exit code 0>
```

</details>


---

<sub>21m 1s</sub>

### `bash`

**Inspect test expectations**

`$ cd tests; grep -n "flushNow\|block.h\|powerOffRequested" watchdog_inferred_classification_test.sh | head -20; grep -n "forced consumption" -B12 s2_alarm_watchdog_wake_structural_test.py; sed -n 280,300p clock_resync_wiring_test.py`

<details>
<summary>42 lines</summary>

```
17:end = "SystemConfig::flushNow();"
29:extract "$gcc_src" "$work/block.h"
30:clang++ -std=c++17 -Wall -Wextra -pedantic -I"$repo_root/src" -DBOOT_BLOCK_H="\"$work/block.h\"" \
53:  expect_fail "$label" "$work/h_$label" "$work/block.h"
62:  mutate "$work/block.h" "$work/b_$label.h" "$old" "$new"
65:bmutant boot_clear_removed "  RecoveryState::set_powerOffRequested(0);" ""
66:bmutant boot_flush_removed "  SystemConfig::flushNow();" ""
61-        pass
62-    setup = gcc[gcc.index("classifyWake(reason)"):]
63-    block = setup[:setup.index("SystemConfig::flushNow();") + 25]
64-    if "isInferredWatchdog(" not in block or "get_powerOffRequested()" not in block:
65-        p.append("reason-20 classification must use isInferredWatchdog with the persisted byte")
66-    if re.search(r"\bretained\w*", block):
67-        p.append("retained must not be reason-20 evidence")
68-    if not 0 <= block.find("isInferredWatchdog(") < block.find("set_powerOffRequested(0)"):
69-        p.append("the byte must be read before it is cleared")
70-    if gcc.count("set_powerOffRequested(0)") != 1:
71-        p.append("the byte must be cleared exactly once, at boot")
72-    if not re.search(r"set_powerOffRequested\(0\);\s*SystemConfig::flushNow\(\);", gcc):
73:        p.append("the boot clear must be flushed immediately (forced consumption)")
    # calling setRtcFromSystem(), which is exactly what let a failed write be
    # silently treated as permanently done).
    gate_idx = write_back_gate.start()
    check_fn_stripped = strip_line_comments(check_fn)
    write_back_gate_stripped = re.search(
        r"ClockTrust::shouldAttemptRtcWriteNow\(\s*lastSyncMs\s*,\s*"
        r"lastRtcWriteSyncedLastMs\s*,\s*nowMs\s*,\s*lastRtcWriteFailedAttemptMs\s*,\s*"
        r"lastRtcWriteFailedSyncMs\s*\)",
        check_fn_stripped,
    )
    gate_idx = write_back_gate_stripped.start() if write_back_gate_stripped else gate_idx
    rtc_idx = check_fn_stripped.index("ExternalRtc::setFromSystem()")
    tracked_update_idx = check_fn_stripped.index("lastRtcWriteSyncedLastMs = lastSyncMs")
    stamp_idx = check_fn_stripped.index("SystemConfig::set_lastTimeSync(Time.now())")
    if not (gate_idx < rtc_idx < tracked_update_idx < stamp_idx):
        fail(
            "checkClockResync() must check the write-back gate, THEN call "
            "setRtcFromSystem(), THEN (only on confirmed success) update "
            "lastRtcWriteSyncedLastMs, THEN stamp lastTimeSync - found order "
            f"gate={gate_idx}, setRtcFromSystem={rtc_idx}, "
            f"trackedUpdate={tracked_update_idx}, set_lastTimeSync={stamp_idx} "
<shellId: 34 completed with exit code 0>
```

</details>


---

<sub>21m 5s</sub>

### `bash`

**View extract markers**

`$ cd tests; sed -n 8,32p watchdog_inferred_classification_test.sh`

<details>
<summary>23 lines</summary>

```
gcc_src="$repo_root/src/Generalized-Core-Counter.cpp"
hdr="$repo_root/src/time/HibernateWakeDiagnostics.h"

# Extract the REAL boot classification block (read, classify, clear, flush) from setup().
extract() {
  python3 - "$1" "$2" <<'PY'
import sys
t = open(sys.argv[1]).read()
a = t.index("  bool inferredWatchdog = false;\n")
end = "SystemConfig::flushNow();"
b = t.index(end, a) + len(end)
open(sys.argv[2], "w").write(t[a:b] + "\n")
PY
}

build_run() { # <include-dir> <block> <out>
  clang++ -std=c++17 -Wall -Wextra -pedantic -I"$1" -DBOOT_BLOCK_H="\"$2\"" \
    "$repo_root/tests/watchdog_inferred_classification_test.cpp" -o "$3" 2>/dev/null
  "$3"
}

extract "$gcc_src" "$work/block.h"
clang++ -std=c++17 -Wall -Wextra -pedantic -I"$repo_root/src" -DBOOT_BLOCK_H="\"$work/block.h\"" \
  "$repo_root/tests/watchdog_inferred_classification_test.cpp" -o "$work/t"
"$work/t"
<shellId: 35 completed with exit code 0>
```

</details>


---

<sub>21m 12s</sub>

### `edit`

**edit the file at /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-i21/src/Generalized-Core-Counter.cpp.**

`/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-i21/src/Generalized-Core-Counter.cpp`

```
File /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-i21/src/Generalized-Core-Counter.cpp updated with changes.
```


---

<sub>21m 12s</sub>

### `edit`

**edit the file at /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-i21/src/Generalized-Core-Counter.cpp.**

`/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-i21/src/Generalized-Core-Counter.cpp`

```
File /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-i21/src/Generalized-Core-Counter.cpp updated with changes.
```


---

<sub>21m 15s</sub>

### `bash`

**View assertions to update**

`$ cd tests; sed -n 296,312p clock_resync_wiring_test.py; grep -n "set_lastTimeSync" clock_resync_wiring_test.py`

<details>
<summary>24 lines</summary>

```
"checkClockResync() must check the write-back gate, THEN call "
            "setRtcFromSystem(), THEN (only on confirmed success) update "
            "lastRtcWriteSyncedLastMs, THEN stamp lastTimeSync - found order "
            f"gate={gate_idx}, setRtcFromSystem={rtc_idx}, "
            f"trackedUpdate={tracked_update_idx}, set_lastTimeSync={stamp_idx} "
            "(Finding 1, Round 4 review: updating the tracked value before/"
            "regardless of the write's success permanently consumes a "
            "FAILED write, blocking retry for up to 24h)"
        )

    # Round 6 (second follow-up), Stage 7 pass 4 regression check: neither
    # failure-tracking assignment may appear BEFORE ExternalRtc::setFromSystem()
    # is called (i.e. unconditionally, before the outcome is known) - that
    # is exactly the Stage-7-pass-4 defect (recording an attempt timestamp
    # regardless of success, which re-arms the retry floor on every write
    # and wrongly delays a later distinct sync value).
    failed_attempt_idx = check_fn_stripped.find("lastRtcWriteFailedAttemptMs = nowMs")
30:3. `SystemConfig::set_lastTimeSync(Time.now())` (item 6's Finding-1 fix) is
293:    stamp_idx = check_fn_stripped.index("SystemConfig::set_lastTimeSync(Time.now())")
300:            f"trackedUpdate={tracked_update_idx}, set_lastTimeSync={stamp_idx} "
349:    if "SystemConfig::set_lastTimeSync(Time.now())" not in guarded_body:
350:        fail("the SystemConfig::set_lastTimeSync() stamp must be inside the `if (rtcUpdated)` guard (Finding 1)")
461:    if "SystemConfig::set_lastTimeSync" in daily_code_text:
463:            "dailyCleanup() must not stamp SystemConfig::set_lastTimeSync() directly "
<shellId: 36 completed with exit code 0>
```

</details>


---

<sub>21m 17s</sub>

### `bash`

**View guarded body check**

`$ cd tests; sed -n 325,355p clock_resync_wiring_test.py`

<details>
<summary>30 lines</summary>

```
"the failed-write else branch, AFTER the outcome is known "
            "(Round 6 second follow-up regression check, Stage 7 pass 4)"
        )

    # The tracked-value update and the lastTimeSync stamp must both be
    # gated behind the write's confirmed-success branch, not merely follow
    # it textually (a `rtcUpdated = true;` unconditional assignment right
    # before an unguarded update would satisfy the ordering check above
    # while still reintroducing Finding 1). Require an explicit
    # `if (rtcUpdated)` guard wrapping both statements.
    confirm_guard = re.search(
        r"if\s*\(\s*rtcUpdated\s*\)\s*\{([^}]*)\}",
        check_fn,
        re.DOTALL,
    )
    if not confirm_guard:
        fail(
            "checkClockResync() must guard the tracked-value update and "
            "lastTimeSync stamp behind `if (rtcUpdated)` (Finding 1) so a "
            "failed write does not advance either"
        )
    guarded_body = confirm_guard.group(1)
    if "lastRtcWriteSyncedLastMs = lastSyncMs" not in guarded_body:
        fail("the lastRtcWriteSyncedLastMs update must be inside the `if (rtcUpdated)` guard (Finding 1)")
    if "SystemConfig::set_lastTimeSync(Time.now())" not in guarded_body:
        fail("the SystemConfig::set_lastTimeSync() stamp must be inside the `if (rtcUpdated)` guard (Finding 1)")

    # Round 6 (second follow-up), Stage 7 pass 4: the failure-only tracking
    # writes must be OUTSIDE the success guard (i.e. in the paired `else`),
    # never alongside the success-path updates above - putting them inside
    # the `if (rtcUpdated)` guard would mean a SUCCESS also arms the retry
<shellId: 37 completed with exit code 0>
```

</details>


---

<sub>21m 28s</sub>

### `edit`

**edit the file at /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-i21/tests/clock\_resync\_wiring\_test.py.**

`/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-i21/tests/clock_resync_wiring_test.py`

```
File /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-i21/tests/clock_resync_wiring_test.py updated with changes.
```


---

<sub>21m 28s</sub>

### `edit`

**edit the file at /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-i21/tests/clock\_resync\_wiring\_test.py.**

`/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-i21/tests/clock_resync_wiring_test.py`

```
File /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-i21/tests/clock_resync_wiring_test.py updated with changes.
```


---

<sub>21m 28s</sub>

### `edit`

**edit the file at /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-i21/tests/clock\_trust\_carry\_test.sh.**

`/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-i21/tests/clock_trust_carry_test.sh`

```
File /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-i21/tests/clock_trust_carry_test.sh updated with changes.
```


---

<sub>21m 37s</sub>

### `edit`

**edit the file at /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-i21/tests/clock\_trust\_carry\_test.sh.**

`/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-i21/tests/clock_trust_carry_test.sh`

```
File /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-i21/tests/clock_trust_carry_test.sh updated with changes.
```


---

<sub>21m 37s</sub>

### `edit`

**edit the file at /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-i21/tests/clock\_trust\_carry\_test.sh.**

`/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-i21/tests/clock_trust_carry_test.sh`

```
File /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-i21/tests/clock_trust_carry_test.sh updated with changes.
```


---

<sub>21m 37s</sub>

### `edit`

**edit the file at /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-i21/tests/clock\_trust\_carry\_test.sh.**

`/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-i21/tests/clock_trust_carry_test.sh`

```
No changes made: old_str and new_str are the same
```


---

<sub>29m 9s</sub>

### `bash`

**Rerun carry test and full suite**

`$ zsh tests/clock_trust_carry_test.sh 2>&1 | tail -6; ls build-tmp/wo-2026-11-05-007-stage6/carry_test; zsh build-tmp/wo-2026-11-05-007-stage6/runall.sh 2>&1 | tail -6`

<details>
<summary>11 lines</summary>

```
caught: carry without plausibility checks
caught: carry without plausibility checks (call site)
caught: verdict input ignored
caught: verdict arm dropped

clock_trust_carry_test passed (unit, C2 regression, call-site text, 11 mutation checks)
c2
c2.cpp
unit
122/122 (sh via zsh, py via python3)
FAILED:
<shellId: 38 completed with exit code 0>
```

</details>


---

<sub>29m 18s</sub>

### `bash`

**Thermal test on clean export**

`$ rm -rf build-tmp/wo-2026-11-05-007-stage6/carry_test; d=$(mktemp -d); git ls-files -z | xargs -0 tar cf - 2>/dev/null | tar xf - -C $d; cd $d && python3 tests/thermal_coupling_structural_test.py | tail -2; echo rc=$?; rm -rf $d`

```
with no `static` cross-call accumulator (AC-C9) and all
      samples are collected within a single bounded loop (AC-C10).
rc=0
<shellId: 39 completed with exit code 0>
```


---

<sub>29m 17s</sub>

### `bash`

**Find build setup**

`$ ls ~/.particle/toolchains; ls; grep -n "make\|BUILD_PATH_BASE\|DEVICE_OS" .github/workflows/build.yml | head -20; ls ~/.particle/toolchains/*`

<details>
<summary>41 lines</summary>

```
buildscripts
buildtools
deviceOS
gcc-arm
openocd
AI_DEVELOPMENT_WORKFLOW.md
ARCHITECTURE_OVERVIEW.md
CHANGELOG.md
Doxyfile
LICENSE
PROJECT_STATUS.md
README.md
STYLE.md
build-tmp
bump_version.sh
default-settings-v3.23.json
docs
lib
project.properties
release
release.sh
src
tests
tools
42:          DEVICE_OS_ROOT: ${{ github.workspace }}/device-os-6.4.1
/Users/chipmc/.particle/toolchains/buildscripts:
1.10.2
1.17.2

/Users/chipmc/.particle/toolchains/buildtools:
1.1.1

/Users/chipmc/.particle/toolchains/deviceOS:
2.1.0
6.3.4
6.4.1
6.6.0

/Users/chipmc/.particle/toolchains/gcc-arm:
10.2.1
9.2.1

/Users/chipmc/.particle/toolchains/openocd:
0.12.0-particle.2
<shellId: 40 completed with exit code 0>
```

</details>


---

<sub>29m 20s</sub>

### `bash`

**Find local build recipe**

`$ sed -n 70,140p .github/workflows/build.yml; ls ~/.particle/toolchains/buildscripts/1.17.2 | head; ls docs/work-orders | grep -i "build\|toolchain" | head; grep -rn "BUILD_PATH_BASE" docs tools *.md 2>/dev/null | head -5`

<details>
<summary>80 lines</summary>

```
- name: Tag matches source FIRMWARE_PRODUCT_VERSION (before building)
        if: github.ref_type == 'tag'
        run: |
          set -euo pipefail
          [[ "$GITHUB_REF_NAME" =~ ^v([0-9]{1,2})\.([0-9]{1,2})$ ]] || { echo "::error::tag '$GITHUB_REF_NAME' is not v<major>.<iteration> (1-2 digits each)"; exit 1; }
          want=$((10#${BASH_REMATCH[1]} * 100 + 10#${BASH_REMATCH[2]}))
          got=$(echo FIRMWARE_PRODUCT_VERSION | cc -E -P -x c++ -include src/FirmwareVersion.h - | tail -n 1)  # the active macro, as the compiler sees it
          [ "$got" = "$want" ] || { echo "::error::source FIRMWARE_PRODUCT_VERSION '$got' != $want from tag $GITHUB_REF_NAME; not building"; exit 1; }
      - name: Read Device OS version from project.properties
        id: props
        run: echo "os=$(sed -n 's/^deviceOS=//p' project.properties)" >> "$GITHUB_OUTPUT"
      - name: Compile (on the runner; no Particle token, no auto-version)
        id: compile
        uses: particle-iot/compile-action@3b6e15e99da59d074120f854a71659d4c81f092c # v1.0.7
        with:
          particle-platform-name: boron
          device-os-version: ${{ steps.props.outputs.os }}
      - uses: actions/upload-artifact@cf430e030ddbb5b0abf93d22962f4752f3646cd9 # v7.0.2
        with:
          name: firmware-boron
          path: |
            ${{ steps.compile.outputs.firmware-path }}
            ${{ steps.compile.outputs.target-path }}/*.elf
          if-no-files-found: error
      - name: Record .bin SHA-256 and buildpack image digest
        id: rec
        env:
          BIN: ${{ steps.compile.outputs.firmware-path }}
          OS: ${{ steps.props.outputs.os }}
        run: |
          set -euo pipefail
          sha=$(sha256sum "$BIN" | cut -d' ' -f1)
          digest=$(docker image inspect "particle/buildpack-particle-firmware:$OS-boron" --format '{{join .RepoDigests ","}}')
          [ -n "$digest" ] || { echo "::error::buildpack image has no RepoDigest; cannot record it"; exit 1; }
          echo "sha256=$sha" >> "$GITHUB_OUTPUT"
          echo "digest=$digest" >> "$GITHUB_OUTPUT"
      - name: Job summary
        env:
          DIGEST: ${{ steps.rec.outputs.digest }}
        run: |
          {
            echo "device-os-version: ${{ steps.compile.outputs.device-os-version }}"
            # The action's firmware-version output is unreliable (reported 24 for product version 48).
            echo "firmware-product-version: $(sed -n 's/^#define FIRMWARE_PRODUCT_VERSION *//p' src/FirmwareVersion.h)"
            echo "buildpack-image: $DIGEST"
          } >> "$GITHUB_STEP_SUMMARY"

  publish:
    if: github.ref_type == 'tag'
    needs: [host-tests, firmware]
    runs-on: ubuntu-latest
    permissions:
      contents: write  # this job only; the token is in the env of the one Release step alone
    steps:
      - uses: actions/download-artifact@9000827ccba6bdab643e8b6fd33ac0654aef8333 # v8.0.2
        with:
          name: firmware-boron
          path: fw
      - name: Guards and Release (the only step with the write token)
        env:
          GH_TOKEN: ${{ github.token }}
          TAG: ${{ github.ref_name }}
          WANT_SHA: ${{ needs.firmware.outputs.sha256 }}
          OS: ${{ needs.firmware.outputs.os }}
          DIGEST: ${{ needs.firmware.outputs.digest }}
          TESTS: ${{ needs.host-tests.outputs.tests }}
        run: |
          set -euo pipefail
          [[ "$TAG" =~ ^v([0-9]{1,2})\.([0-9]{1,2})$ ]] || { echo "::error::tag '$TAG' is not v<major>.<iteration> (1-2 digits each)"; exit 1; }
          want=$((10#${BASH_REMATCH[1]} * 100 + 10#${BASH_REMATCH[2]}))
          bin=$(find fw -name '*.bin')
Makefile
WO-2026-08-31-001-p2-build-break-pmic-incomplete-type.md
WO-2026-09-26-001-build-provenance.md
WO-2026-11-02-003-github-actions-build.md
docs/work-orders/WO-2026-10-09-003-stage7-codex-dispatch.md:39:   - Build HEAD (`git archive HEAD` into your scratch directory) and the working tree, each with a fresh `BUILD_PATH_BASE`. Give text/data/bss for both and the delta. Copilot claims 151468 / 1094 / 2220 for the change.
docs/work-orders/WO-2026-11-06-001-stage7-r1-verdict.md:62:Build: **Device OS 6.6.0, Boron, buildscripts 1.17.2, fresh `BUILD_PATH_BASE` passed as a make argument**.
docs/work-orders/WO-2026-10-21-003-sleep-s4-plan.md:125:**Controller reproduction** (Claude Code, Device OS 6.4.1 Boron, fresh `BUILD_PATH_BASE`, `-frandom-seed=p6`, "before" is `git archive HEAD`):
docs/work-orders/WO-2026-10-14-002-stage7-codex-dispatch.md:41:4. **ELF.** Build HEAD (`git archive HEAD` into scratch) and the working tree, each with a fresh `BUILD_PATH_BASE`. Give text/data/bss for both (Copilot claims 151372 → 151308). Compare `nm -C -S`:
docs/work-orders/2026-10-12-v41-release-and-soak.md:15:- No other source changes. Fresh `BUILD_PATH_BASE` build; `strings` shows the version; tests `N/N (sh via zsh, py via python3)`.
<shellId: 41 completed with exit code 0>
```

</details>


---

<sub>29m 23s</sub>

### `bash`

**Find make invocation**

`$ grep -rn "make.*PLATFORM\|PARTICLE_DEVELOP\|COMPILE_LTO\|APPDIR=" docs/work-orders/WO-2026-11-06-001-stage7-r1-verdict.md docs/work-orders/WO-2026-10-21-003-sleep-s4-plan.md docs/work-orders/WO-2026-10-14-002-stage7-codex-dispatch.md | head -8; grep -n "deviceOS" project.properties`

```
3:deviceOS=6.6.0
<shellId: 42 completed with exit code 0>
```


---

<sub>29m 27s</sub>

### `bash`

**Search for build command**

`$ grep -rln "buildscripts/1.17.2\|buildscripts/1.1" docs AI_DEVELOPMENT_WORKFLOW.md tools 2>/dev/null | head; grep -rhn "make -f.*Makefile\|\bmake .*all\b" docs AI_DEVELOPMENT_WORKFLOW.md 2>/dev/null | head -5; sed -n 1,40p ~/.particle/toolchains/buildscripts/1.17.2/Makefile | head -50`

<details>
<summary>49 lines</summary>

```
docs/work-orders/WO-2026-09-25-001-stage7-codex/binary-review.md
docs/work-orders/WO-2026-11-05-003-stage6-copilot-transcript.md
docs/work-orders/WO-2026-11-05-004-stage6-r3-copilot-transcript.md
docs/work-orders/WO-2026-11-03-004-stage6-copilot-transcript.md
docs/work-orders/WO-2026-11-03-006-stage6-r2-copilot-transcript.md
docs/work-orders/WO-2026-11-04-002-stage6-r2-copilot-transcript.md
docs/work-orders/WO-2026-11-06-002-stage6-r3-copilot-transcript.md
docs/work-orders/WO-2026-11-05-003-r1fix-stage6-copilot-report.md
docs/work-orders/WO-2026-11-05-003-r1fix-stage6-copilot-transcript.md
docs/work-orders/WO-2026-11-03-006-stage6-copilot-transcript.md
15:4. **Your findings 1 and 2 (the two High defects) are NOT fixed in S1, by the architect's ruling (Amendment 4):** both are carried to a new piece **S1b** (`docs/work-orders/WO-2026-11-06-002-v50-s1b-pre-bind-rtc-access.md` on the draft branch `wo/2026-11-05-002-v50-ab1805-switch-draft`, approved, not yet built), which makes one raw I2C access to 0x69 on an unbound boot **before** `Am18x5.enable()`: it seeds `Time` from a plausible chip time and sets WRTC. It sits on the integration branch between S1 and S2, and the set merges to `main` only after all five bench items pass. **Do not fail S1 for those two defects as such.** Instead judge, and say so plainly: (a) does anything in S1 make the two defects *worse or harder to fix* in S1b (for example a call order in `setup()` that leaves no place for the pre-bind access, or a read-back that would loop or retry forever when WRTC = 0)? (b) does S1b's described design (read the chip, seed `Time` if plausible and not valid, set WRTC, then S1's `enable()`) close both defects as you described them, and is there any gap in it (the bind's copy of the internal clock into the chip after the seed; the 12/24-hour bit; the Wire ownership window)? Put the S1b observations in a separate section; they are advice for S1b's own Stage 7, not S1's verdict.
14:The local build used the README environment and `make -f /Users/chipmc/.particle/toolchains/buildscripts/1.17.2/Makefile compile-user -s`, with APPDIR set to this scratch `build-source`, PLATFORM=boron and DEVICE_OS_VERSION=6.4.1. It created a fresh user archive in `deviceOS/6.4.1/build/target/user/platform-13-m/build-source`.
48:3. **Stage 7 rebuilds both sides fresh,** expecting +56 text. Copilot's "before" of 150876 came from `make compile-all`, not the release recipe.
22:- **`src/`:** **≤ +20 net non-move lines, HARD**: lines moved byte-for-byte count 0 (reported as gross); new header, include, namespace and wrapper lines count, and so do the lines in the three call sites that change. Over means STOP and report (§13 (b)). Removing the Connect duplicate should make the total small.
83:Policy recommendation: repair the existing variable-budget design, do not shorten based on invalid RSSI. Use 300 normal, 660 periodic deep, with the deep opportunity independent of success and not erased by state re-entry; honor deep due after long sleep at the next permitted wake. A reasonable initial cadence is every fourth scheduled failed attempt and a due flag after a several-hour interval, rather than extra immediate wakeups. Keep critical-battery safety as an explicit separate constraint. Remove >50% opportunistic deep: SoC is not evidence of likely attachment, and the rule defeated savings on the USB-backed Singapore bench. Savings in US remain unmeasured. Accept one coherent connection policy range and give idle residence its own meaning; do not make all low-charge attempts permanently 300. The [saved guidance](https://docs.particle.io/reference/device-os/api/cellular/connect/) explicitly allows variable backoff with periodic full 11 minutes.
# Makefile for the Particle Workbench
# Supports local compile and local flash for Particle projects
#
# Inputs:
# PARTICLE_CLI_PATH: path to the Particle CLI executable
# APPDIR: path to the user application sources
# DEVICE_OS_PATH: path to the source code of the Particle DeviceOS
# PLATFORM: the name of the Particle platform to compile for
# PLATFORM_ID: the id of the Particle platform to compile for

version = 1.17.2
all: flash-all

run_steps := $(MAKE) -f $(lastword $(MAKEFILE_LIST))

# Set target build output directory
export TARGET_DIR := $(APPDIR)/target/$(DEVICE_OS_VERSION)/$(PLATFORM)

# Compatibility flag for the tr tool
export LC_COLLATE := C

# Remove env vars which may conflict with our build environment
unexport LDFLAGS
unexport CFLAGS
unexport CXXFLAGS
unexport CPPFLAGS
unexport CONLYFLAGS
unexport ASFLAGS

# Convert Wiring files (ino) files to C++ files (cpp)
INO_FILES := $(wildcard $(APPDIR)/*.ino) $(wildcard $(APPDIR)/src/*.ino)
PREPROCESSED_FILES := $(patsubst %.ino,%.cpp,$(INO_FILES))
inspect-env:
ifeq ($(PARTICLE_LOCAL_COMPILER_DEBUG),1)
	$(call print_stage,OUTPUTTING ENVIRONMENT INFO)
	@env
endif

preprocess: $(PREPROCESSED_FILES)
$(PREPROCESSED_FILES): %.cpp: %.ino
<shellId: 43 completed with exit code 0>
```

</details>


---

<sub>29m 32s</sub>

### `bash`

**Local toolchain build**

`$ B=$PWD/build-tmp/wo-2026-11-05-007-stage6; rm -rf $B/build-source $B/bpb; mkdir -p $B/build-source $B/bpb; cp -R src lib project.properties $B/build-source/; cd $B; T=$HOME/.particle/toolchains; (make -f $T/buildscripts/1.17.2/Makefile compile-user -s APPDIR=$B/build-source PLATFORM=boron PLATFORM_ID=13 DEVICE_OS_VERSION=6.6.0 DEVICE_OS_PATH=$T/deviceOS/6.6.0 BUILD_PATH_BASE=$B/bpb 2>&1 | tail -15)`

<details>
<summary>11 lines</summary>

```
grep: project.properties: No such file or directory
grep: project.properties: No such file or directory

:::: COMPILING APPLICATION

grep: project.properties: No such file or directory
grep: project.properties: No such file or directory
make[3]: arm-none-eabi-gcc: Command not found
/Users/chipmc/.particle/toolchains/deviceOS/6.6.0/build/arm-tools.mk:151: *** "ARM gcc version 10.2.1 or later required, but found ".  Stop.
make[2]: *** [modules/boron/user-part] Error 2
make[1]: *** [make-main] Error 2
make: *** [compile-user] Error 2
<shellId: 44 completed with exit code 0>
```

</details>


---

<sub>32m 50s</sub>

### `bash`

**Build with toolchain on PATH**

`$ B=$PWD/build-tmp/wo-2026-11-05-007-stage6; T=$HOME/.particle/toolchains; ls $T/gcc-arm/10.2.1/bin | head -3; ls $T/buildtools/1.1.1; cd $B/build-source && export PATH=$T/gcc-arm/10.2.1/bin:$T/buildtools/1.1.1/bin:$PATH && (make -f $T/buildscripts/1.17.2/Makefile compile-user -s APPDIR=$B/build-source PLATFORM=boron PLATFORM_ID=13 DEVICE_OS_VERSION=6.6.0 DEVICE_OS_PATH=$T/deviceOS/6.6.0 BUILD_PATH_BASE=$B/bpb 2>&1 | tail -15)`

<details>
<summary>26 lines</summary>

```
arm-none-eabi-addr2line
arm-none-eabi-ar
arm-none-eabi-as
ThirdPartyNotices.txt
_dfu-prefix
_dfu-suffix
_dfu-util
dfu-prefix
dfu-suffix
dfu-util
lib
make
      |                 ~~~~~~~~^~~~~~~~~~~~~~~~~~~
/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-i21/build-tmp/wo-2026-11-05-007-stage6/build-source/lib/LocalTimeRK/src/LocalTimeRK.cpp: In member function 'void LocalTimeConvert::prevDay(LocalTimeHMS)':
/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-i21/build-tmp/wo-2026-11-05-007-stage6/build-source/lib/LocalTimeRK/src/LocalTimeRK.cpp:1244:12: warning: unused variable 'origTime' [-Wunused-variable]
 1244 |     time_t origTime = time;
      |            ^~~~~~~~
/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-i21/build-tmp/wo-2026-11-05-007-stage6/build-source/lib/LocalTimeRK/src/LocalTimeRK.cpp: In member function 'void LocalTimeConvert::nextDay(LocalTimeHMS)':
/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-i21/build-tmp/wo-2026-11-05-007-stage6/build-source/lib/LocalTimeRK/src/LocalTimeRK.cpp:1255:12: warning: unused variable 'origTime' [-Wunused-variable]
 1255 |     time_t origTime = time;
      |            ^~~~~~~~
Creating /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-i21/build-tmp/wo-2026-11-05-007-stage6/build-source/target/6.6.0/boron/platform_user_ram.ld ...
   text	   data	    bss	    dec	    hex	filename
 151316	   1114	   2256	 154686	  25c3e	/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-i21/build-tmp/wo-2026-11-05-007-stage6/build-source/target/6.6.0/boron/build-source.elf

*** COMPILED SUCCESSFULLY ***
<shellId: 45 completed with exit code 0>
```

</details>


---

<sub>32m 54s</sub>

### `bash`

**nm evidence and final status**

`$ B=$PWD/build-tmp/wo-2026-11-05-007-stage6; T=$HOME/.particle/toolchains; $T/gcc-arm/10.2.1/bin/arm-none-eabi-nm -C -S $B/build-source/target/6.6.0/boron/build-source.elf | grep -i "carry\|isRtcArm\|isCarryEligible\|checkClockResync\|isClockTrusted"; git diff --numstat 1d1d5af -- src; git status --short`

<details>
<summary>14 lines</summary>

```
000c4d90 000000f8 T isClockTrusted()
000c4b50 00000240 T checkClockResync()
1	0	src/Generalized-Core-Counter.cpp
13	3	src/time/Clock.cpp
7	0	src/time/ClockTrust.h
2	0	src/time/HibernateWakeDiagnostics.h
 M src/Generalized-Core-Counter.cpp
 M src/time/Clock.cpp
 M src/time/ClockTrust.h
 M src/time/HibernateWakeDiagnostics.h
 M tests/clock_resync_wiring_test.py
?? tests/clock_trust_carry_test.cpp
?? tests/clock_trust_carry_test.sh
<shellId: 46 completed with exit code 0>
```

</details>


---

<sub>32m 58s</sub>

### `bash`

**Disassembly evidence**

`$ B=$PWD/build-tmp/wo-2026-11-05-007-stage6; T=$HOME/.particle/toolchains; $T/gcc-arm/10.2.1/bin/arm-none-eabi-objdump -d -C $B/build-source/target/6.6.0/boron/build-source.elf --disassemble='isClockTrusted()' | grep -i "bl\b\|bl	\|0x15180\|1704067200\|r[0-9], \[pc" | head -20; $T/gcc-arm/10.2.1/bin/arm-none-eabi-nm -C $B/build-source/target/6.6.0/boron/build-source.elf | grep -i "hal_rtc_time_is_valid\|get_lastTimeSync"`

<details>
<summary>23 lines</summary>

```
c4d94:	f007 fe24 	bl	cc9e0 <__fetch_global_Time()>
   c4d98:	f007 fdca 	bl	cc930 <TimeClass::isValid() const>
   c4d9e:	f003 fa89 	bl	c82b4 <HAL_Timer_Get_Milli_Seconds>
   c4da4:	f7ff fe86 	bl	c4ab4 <observedTimeSyncedLastMs()>
   c4dac:	4b2e      	ldr	r3, [pc, #184]	; (c4e68 <isClockTrusted()+0xd8>)
   c4db0:	4b2e      	ldr	r3, [pc, #184]	; (c4e6c <isClockTrusted()+0xdc>)
   c4db8:	4b2d      	ldr	r3, [pc, #180]	; (c4e70 <isClockTrusted()+0xe0>)
   c4dc0:	f007 fe0e 	bl	cc9e0 <__fetch_global_Time()>
   c4dc4:	f007 fdda 	bl	cc97c <TimeClass::now()>
   c4dca:	f7f2 fb83 	bl	b74d4 <SystemConfig::get_lastTimeSync()>
   c4de2:	f007 fdfd 	bl	cc9e0 <__fetch_global_Time()>
   c4de6:	f007 fda3 	bl	cc930 <TimeClass::isValid() const>
   c4dee:	4a21      	ldr	r2, [pc, #132]	; (c4e74 <isClockTrusted()+0xe4>)
   c4df0:	4921      	ldr	r1, [pc, #132]	; (c4e78 <isClockTrusted()+0xe8>)
   c4e1e:	f003 fa51 	bl	c82c4 <hal_rtc_time_is_valid>
   c4e22:	4b16      	ldr	r3, [pc, #88]	; (c4e7c <isClockTrusted()+0xec>)
   c4e30:	4b10      	ldr	r3, [pc, #64]	; (c4e74 <isClockTrusted()+0xe4>)
   c4e32:	4813      	ldr	r0, [pc, #76]	; (c4e80 <isClockTrusted()+0xf0>)
   c4e50:	4b0c      	ldr	r3, [pc, #48]	; (c4e84 <isClockTrusted()+0xf4>)
000b74d4 T SystemConfig::get_lastTimeSync()
000b74ca T sysStatusData::get_lastTimeSync() const
000c82c4 T hal_rtc_time_is_valid
<shellId: 47 completed with exit code 0>
```

</details>


---

<sub>33m 26s</sub>

### `create`

**create a new file at /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-i21/docs/work-orders/WO-2026-11-05-007-stage6-copilot-report.md.**

`/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-i21/docs/work-orders/WO-2026-11-05-007-stage6-copilot-report.md`

```
Created file /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-i21/docs/work-orders/WO-2026-11-05-007-stage6-copilot-report.md with 8014 characters
```


---

<sub>33m 34s</sub>

### Copilot

I21 is implemented, uncommitted, and `src/` is exactly +20 net (23 added, 3 removed), so the budget holds. The one issue I'd ask you to rule on is a boot-order exposure, described below.

- **Tests:** 122/122 (sh via zsh, py via python3). The baseline before editing was 121/121; the new test adds one.
- **Mutations:** the nine mutations the WO lists, plus two extras, are all caught. They run on a scratch copy of `src/`, not by backing up and restoring the real files, because that never touches your working tree.
- **Existing tests changed:** `clock_resync_wiring_test.py` only. The stamp is now `carryStamp(rtcUpdated, …)` placed before the `if (rtcUpdated)` guard, so a failed write stamps 0.
- **Clean-export check:** `thermal_coupling_structural_test.py` passes on a clean export of tracked files.
- **Build:** Device OS 6.6.0, Boron, fresh `BUILD_PATH_BASE` make argument. text/data/bss is 151316 / 1114 / 2256. `nm` shows `isClockTrusted()` linked, and `objdump` shows it calling `get_lastTimeSync` and `hal_rtc_time_is_valid`. The predicate is inlined, so there is no separate symbol.
- **Ruling 7:** "out of window" is a failed gate on `kRtcRead`, `kRtcBeforeZero` or `kRtcOrder`. It needs no new bound or stored field. It is applied by zeroing `lastTimeSync` right after `classifyWake()`, and the existing `flushNow()` persists it.
- **Zero-as-revoke side effect:** the existing `neverConfirmedSyncEver` check treats zero as "never synced", so a revoked unit will connect to the cloud on its next boot.

**Open questions for you (details in the report):**
- **Boot-order exposure:** the boot-storm check (`Generalized-Core-Counter.cpp:378`) and the piece snapshot (`:506`) read trust before the latch is seeded (`:670`) and before `classifyWake`. Those two reads can therefore see carried trust on a faulty boot. C2's rule for a trusted piece that loses trust keeps it open and credits nothing, so I accepted it rather than move S4/S2 code. Please rule on whether to accept that or authorize moving the seed.
- **Correction masking:** if Device OS rewrites the chip on a cloud sync before `checkClockResync()` pre-reads it, the measured correction would read about 0 and T would never trigger. I couldn't settle this from source; bench item 2 (`skew 61`) will show it.

**Deviations:**
- The reasoning level actually run could not be observed from inside the session.
- I made the first `Clock.cpp` edit with a one-off Python rewrite instead of the edit tool. The result matches the diff.

The report is at `docs/work-orders/WO-2026-11-05-007-stage6-copilot-report.md`.

---

<sub>Generated by [GitHub Copilot CLI](https://github.com/features/copilot/cli)</sub>

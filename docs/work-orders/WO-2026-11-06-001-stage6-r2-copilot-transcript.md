# Copilot CLI Session

> [!NOTE]
> - **Session ID:** `3b9790d2-ae58-47c0-ac98-3b284da5c8cf`  
> - **Started:** 11/8/2026, 9:28:46 AM  
> - **Duration:** 13m 26s  
> - **Exported:** 11/8/2026, 9:42:13 AM  

---

<sub>6s</sub>

### User

Implement per the dispatch in docs/work-orders/WO-2026-11-06-001-stage6-r2-copilot-dispatch.md. The WO is the binding spec wherever the dispatch refers to it. Do not commit, push, merge, or release.

---

<sub>10s</sub>

### `bash`

**Read dispatch**

`$ cat docs/work-orders/WO-2026-11-06-001-stage6-r2-copilot-dispatch.md; ls docs/work-orders`

<details>
<summary>6 lines</summary>

```
Output too large to read at once (28.6 KB). Saved to: /var/folders/_r/m12xn8s15wb_15djjdgfp5gr0000gn/T/1794110337851-copilot-tool-output-80328-cd267a91-ffe3-439e-a0b9-21b01ba78b86.txt
Consider using tools like grep (for searching), head/tail (for viewing start/end), view with view_range (for specific sections), or jq (for JSON) to examine portions of the output.

Preview (first 500 chars):
AGENT: Copilot · MODEL: claude-sonnet-5.5 · REASONING: medium
AUTHORIZATION SCOPE: edit `src/time/ExternalRtc.h` (the latch only), tests under `tests/` (`external_rtc_test.cpp`, `external_rtc_structural_test.py`, `tests/stubs/external_rtc/Particle.h`), and write the report; run host tests, the local Device OS toolchain build and nm; scratch `build-tmp/wo-2026-11-06-001-stage6-r2/` (create it; use only that) / NOT authorized: committing, pushing, merging, flashing, creating PRs or tags, touching 
<shellId: 0 completed with exit code 0>
```

</details>


---

<sub>13s</sub>

### `view`

**view the file at /var/folders/\_r/m12xn8s15wb\_15djjdgfp5gr0000gn/T/1794110337851-copilot-tool-output-80328-cd267a91-ffe3-439e-a0b9-21b01ba78b86.txt.**

`/var/folders/_r/m12xn8s15wb_15djjdgfp5gr0000gn/T/1794110337851-copilot-tool-output-80328-cd267a91-ffe3-439e-a0b9-21b01ba78b86.txt`

```
File too large to read at once (28.6 KB). Consider using the grep tool to search within the file, or view with view_range to read specific sections.
```


---

<sub>15s</sub>

### `view`

**view the file at /var/folders/\_r/m12xn8s15wb\_15djjdgfp5gr0000gn/T/1794110337851-copilot-tool-output-80328-cd267a91-ffe3-439e-a0b9-21b01ba78b86.txt.**

`/var/folders/_r/m12xn8s15wb_15djjdgfp5gr0000gn/T/1794110337851-copilot-tool-output-80328-cd267a91-ffe3-439e-a0b9-21b01ba78b86.txt (lines 1-120)`

<details>
<summary>119 lines</summary>

```
AGENT: Copilot · MODEL: claude-sonnet-5.5 · REASONING: medium
AUTHORIZATION SCOPE: edit `src/time/ExternalRtc.h` (the latch only), tests under `tests/` (`external_rtc_test.cpp`, `external_rtc_structural_test.py`, `tests/stubs/external_rtc/Particle.h`), and write the report; run host tests, the local Device OS toolchain build and nm; scratch `build-tmp/wo-2026-11-06-001-stage6-r2/` (create it; use only that) / NOT authorized: committing, pushing, merging, flashing, creating PRs or tags, touching `lib/`, `Generalized-Core-Counter.cpp` (no second `clockSource()` read: Amendment 2/3), S1-S3, any trust/revoke logic, the payload or layouts, deleting anything you did not create (including `build-tmp/` itself).

# Stage 6 dispatch, round 2 (fixes after Stage 7 round 1): WO-2026-11-06-001 (S4)
Binding spec: `docs/work-orders/WO-2026-11-06-001-v50-s4-oscillator-fault-latch.md` **Amendments 1-3 (read Amendment 3 first)** and Codex's verdict `docs/work-orders/WO-2026-11-06-001-stage7-r1-verdict.md`. Base: this branch HEAD (S4 code at 6a6a57d); your changes go on top, uncommitted. **Budget: `src/` at most +10 net against e21a4a4** (reported +7, expected +8; state the number). **Run at the dispatched reasoning level (medium); the report states the level actually run, or that it could not be observed.**
## Do
1. F1: make the latch `std::atomic<bool>` with relaxed loads/stores (include `<atomic>`; the accessor and the assignment sites keep working; update the host test's `bool&` to `auto&`). Not `volatile`.
2. F3 (tests only): require atomic storage in the structural test; keep the order test (first source read -> initial set -> registration; do NOT require a second read); add the combined-mask runtime vector (`CALIBRATION_FAILURE | CLOCK_SOURCE_EXTERNAL_FAILURE` sets the latch; extend the stub `RtcEvents` with bitwise combination); add mutations: latch initialised true, calibration failure also sets the latch, plain `bool` back, `volatile bool` instead of atomic. Each mutation must fail (back up, mutate, restore from the backup; never `git checkout`/`git restore` while your work is uncommitted).
## Verify (workflow §10, binding)
CI's discovery inside this git worktree: `tests/*.sh` via zsh and `tests/*.py` via python3 (count `p=$((p+1))`; report `N/N (sh via zsh, py via python3)`; S4 round 1: 121/121; any new helper under `tests/` exits 0 bare); `thermal_coupling_structural_test.py` also on a clean export of tracked files (remove scratch copies under `build-tmp/` first); local toolchain build (fresh `BUILD_PATH_BASE` as a make argument; round 1: 151084 / 1114 / 2256); `nm` shows the callback and latch linked; `git diff --numstat e21a4a4 -- src` net <= +10.
## Stop and report if
the budget cannot hold; the atomic needs a change outside `ExternalRtc.h`.
## Report
`docs/work-orders/WO-2026-11-06-001-stage6-r2-copilot-report.md`: model and reasoning level actually used, interpreter line, net `src/` lines, tests and mutation results, build and `nm` evidence, deviations. Claim nothing about hardware.
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

<sub>17s</sub>

### `bash`

**Read verdict and spec**

`$ cd docs/work-orders; ls | grep 11-06-001; cat WO-2026-11-06-001-stage7-r1-verdict.md; grep -n "Amendment 3" -A60 WO-2026-11-06-001-v50-s4-oscillator-fault-latch.md | head -150; cd ../..; git log --oneline | head -3; git status --short`

<details>
<summary>117 lines</summary>

```
WO-2026-11-06-001-stage6-copilot-dispatch.md
WO-2026-11-06-001-stage6-copilot-report.md
WO-2026-11-06-001-stage6-copilot-transcript.md
WO-2026-11-06-001-stage6-r2-copilot-dispatch.md
WO-2026-11-06-001-stage7-r1-codex-dispatch.md
WO-2026-11-06-001-stage7-r1-verdict.md
WO-2026-11-06-001-v50-s4-oscillator-fault-latch.md
UNDER REVIEW: wo/2026-11-06-001-v50-s4-oscillator-fault-latch @ 6a6a57dce125871ede2357e48d42b8c9559a5e0a

**NOT VERIFIED for S4 as scoped.**

| # | Verdict | Evidence |
|---|---|---|
| 1. Latch | **FAIL** | RAM-only, correctly named, never cleared/persisted, but [`ExternalRtc.h:34–36`](/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s4-r1/src/time/ExternalRtc.h:34) uses a plain `bool` across the RTC worker and future I21 application-thread reads. |
| 2. Registration and order | **FAIL** | The required boot-source read and initial set precede registration at [`Generalized-Core-Counter.cpp:658–664`](/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s4-r1/src/Generalized-Core-Counter.cpp:658). However, a fault between lines 658 and 664 is lost. |
| 3. Callback | **PASS** | It sets only for `CLOCK_SOURCE_EXTERNAL_FAILURE`, using `RtcEvents::isSet`, and performs no logging, flush, persistence, or trust logic. Device OS `EnumFlags::isSet` correctly recognizes bit `0x02` in a combined mask. |
| 4. Scope/linkage | **PASS** | Only the two intended `src/` files changed. No payload, retained/persisted layout, Clock/write-back, or S1/S1b/S2/S3 behavior changed. `nm`/objdump confirm linkage and registration. |
| 5. Tests | **FAIL** | All tests pass and all supplied mutants are caught, but the suite permits both production defects above and does not execute a combined event mask. |
| 6. Budget/build | **PASS** | `src/` is +7/−0 net. Fresh Device OS 6.6.0 Boron build passed at **151084 / 1114 / 2256** text/data/bss. |
| 7. Boot/I21 hand-off | **PASS with blockers above** | S1 bind at `Generalized-Core-Counter.cpp:295–296` precedes the snapshot and registration. No additional scoped behavior change found. I21 must consume the latch in its trust decisions and own revocation, persistence/continuity policy, flushing, and any contract change. |

### Findings

1. **High — cross-thread data race**  
   `src/time/ExternalRtc.h:34–36`; Device OS `am18x5.cpp:1407–1472`.

   The worker thread writes the flag while I21 will read it on the application thread. Cortex-M4 byte loads/stores do not tear, and one-loop staleness would be acceptable, but a plain `bool` still constitutes a C++ data race and undefined behavior. `volatile` does not provide C++ inter-thread synchronization and is not an adequate fix.

   Smallest standards-correct fix: include `<atomic>` and change the function-local static/reference to `std::atomic_bool`. The existing assignment syntax remains valid; the host test’s `bool&` should become `auto&`. This adds one net `src/` line, taking the change to **+8**.

2. **Medium — read→registration fault window**  
   `src/Generalized-Core-Counter.cpp:658–664`; Device OS `am18x5.cpp:620–626,630–650`.

   Device OS registration reaches `onOscillatorEvent()` and clears `OF|ACF` at line 646 before enabling the handler. A fault already present at boot is retained correctly: line 658 reads `OMODE`, so RC fallback sets the latch before registration clears OF.

   But if the crystal fails after line 658 reads EXTERNAL and before line 664 registers:

   - FOS switches the chip to RC and sets OF.
   - Registration clears OF without delivering an event.
   - The earlier source snapshot was healthy.
   - The process-lifetime latch remains false.

   Smallest fix: preserve the required pre-registration read, register, then immediately re-read `clockSource()` and set the latch if it is INTERNAL. Registration clears OF/ACF, not the read-only OMODE state, so the second read catches this window. This adds one more line. Together with the atomic fix, the result is **+9 net**, within +10. It requires adjusting Amendment 1’s “rather than adding a second read” wording while retaining ruling 4’s required first-read-before-registration order.

3. **Medium — tests do not protect the missing guarantees**  
   [`external_rtc_test.cpp:121–131`](/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s4-r1/tests/external_rtc_test.cpp:121), [`external_rtc_structural_test.py:56–70`](/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s4-r1/tests/external_rtc_structural_test.py:56).

   Add checks requiring atomic storage and the order `first source read → initial set → registration → second source read`. Add a combined `CALIBRATION_FAILURE | CLOCK_SOURCE_EXTERNAL_FAILURE` runtime vector; the current stub lacks bitwise combination support. These are test-only changes and do not affect the `src/` budget.

### Verification record

- SHA checked first and matched exactly.
- CI discovery: **121/121** — 67 `.sh` via zsh, 54 `.py` via python3.
- Clean-export thermal structural test: passed.
- Normal structural test caught all nine supplied S4 mutations.
- Independent named audit: **7/7 caught**—five supplied mutations plus two new mutations:
  - latch initialized true;
  - calibration failure also sets the latch.
- Scoped `git diff --check e21a4a4 HEAD -- src tests`: passed.
- Tracked worktree remained unchanged.
- Linkage:
  - callback: weak symbol at `0x000b5358`;
  - RAM latch: one-byte BSS symbol at `0x2003d728`;
  - setup passes the callback to `hal_exrtc_event_handler_add`.
- Scratch-only repair probe—atomic plus post-registration recheck—passed the helper/structural test and Device OS build at **151132 / 1114 / 2256**, confirming the **+9** repair compiles.

Model/reasoning: **gpt-5.6-sol, xhigh**.  
Interpreter: **Python 3.14.6; zsh 5.9 (arm64-apple-darwin26.0); GNU Arm Embedded 10.2.1**.  
Build: **Device OS 6.6.0, Boron, buildscripts 1.17.2, fresh `BUILD_PATH_BASE` passed as a make argument**.

No hardware was available or exercised. Physical healthy-crystal/RC-fallback boot behavior and induced oscillator-fault delivery remain **NOT VERIFIED on hardware**.3:**Author:** Claude Code, 6 Nov 2026, DRAFT for the architect's review; not dispatched · **Status:** DISPATCHED 7 Nov 2026 (after S1, S1b, S2, S3 merged; integration head e21a4a4); pre-ruled: S1 Amendment 2 and Amendment 3 ruling 4 send the latch here, at most +10 `src/`
4-**Parent:** `WO-2026-11-05-002-v50-ab1805-switch.md` (including the 6 Nov rulings: the S-set stays on `integration/v50-ab1805` until all five bench items and both 8 h nights pass on its CI binaries) · **Depends on:** S1 (`WO-2026-11-05-003`, the bind and `Am18x5.status()`) · **Consumer:** I21 (`WO-2026-11-05-007`), which reads only the boolean
5-**Branch (when dispatched):** `wo/…-v50-s4-oscillator-fault-latch`, from `integration/v50-ab1805` after S1 has passed Stage 7 and its PR has been merged to the integration branch; S4's PR goes to the integration branch, **never to `main` alone**.
6-**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §3 Stages 5-8; §4 (SHA rule); §5 (time and trust inputs: Codex xhigh); §12; §13 (c), (j) · **Agents:** Stage 6 Copilot standard/medium (re-run at the dispatched level if a run comes back lower on anything larger than a few lines); Stage 7 Codex `gpt-5.6-sol` **xhigh**, at most two rounds · **Budget:** `src/` at most **+10 net** (removals count); stop and report rather than trim style.
7-**Dispatch gate:** S1 reviewed (Stage 7 passed) and merged to the integration branch. S4 does not merge to `main` before the set.
8-
9-## Plain goal
10-I21 will let clock trust carry across a restart, and must withdraw it on evidence. One piece of evidence is "the oscillator faulted since the chip was bound". Device OS reports it as an event; the application needs a single boolean it can read. S4 adds that boolean and nothing else: **a process-lifetime flag in RAM, never persisted** (Chip's ruling: no persisted latch and no persisted clock-source byte). It is S1's budget overflow: S1 implemented the callback and compiled it, but it was about +9 lines over S1's +20, so it was removed and moved here.
11-
12-## Rulings that bind this piece (not reopened)
13-1. **The latch is a boolean, RAM only,** named for what it means (for example `oscillatorFaultSeenSinceBind`), declared where I21 can read it without a new dependency (Step 0 chooses the home; `time/ClockTrust.h` is the pure, host-testable neighbour; the setter lives with the registration).
14-2. **It is set** (a) by an `ExternalTime.onEvent` callback for `RtcEvent::CLOCK_SOURCE_EXTERNAL_FAILURE`, and (b) **at registration** if `Am18x5.status().clockSource()` reports the RC fallback (a persistent fault is caught at boot by the clock source, not by an event). It is **never cleared** in the process; a reboot starts clean and re-derives (b).
15-3. **I21 reads only the boolean.** S4 contains no trust logic, no revoke, no flush: those are I21's.
16-4. **Order (Chip, 6 Nov): `clockSource()` is read BEFORE the handler is registered.** Registering a handler clears the chip's oscillator-fail flag (`am18x5.cpp:646`), so a fault already flagged at boot is seen only through the clock source; read it first, set the flag from it, then register the callback. Step 0 confirms the order from source and records it; it does not reopen it.
17-5. **Persisted continuity evidence is not added** (Chip, 6 Nov; see I21's "Why persisted continuity evidence is not needed" and the residual window).
18-
19-## Facts from S1's Stage 6 Step 0 (Device OS 6.6.0; **Step 0 here re-verifies each with file:line**)
20-- `RtcEvent::CLOCK_SOURCE_EXTERNAL_FAILURE = 0x02` (`exrtc_hal.h:86-88`); the callback is `void(RtcEvents, void*)`; the event is mapped from the chip's XT oscillator-failure condition at `am18x5.cpp:695-715`.
21-- The oscillator-failure interrupt (OFIE) is enabled **only with the external-crystal oscillator** (`am18x5.cpp:595`, `:635`), and **registering a handler clears the OF flag** (`:646`). So the clock source must be read **before** registration (ruling 4), or a fault already flagged is lost.
22-- S1 already publishes `usingRC` from `Am18x5.status().clockSource()` at boot; S4 reuses that read.
23-
24-## The change (re-cite on the then-current integration head)
25-1. The boolean and its accessor (`inline`, no new `.cpp` if it fits).
26-2. After S1's bind in `setup()`: read `clockSource()`; if RC fallback, set the flag; register the `ExternalTime.onEvent` callback whose body only sets the flag (and may log one line, counted in the budget).
27-3. Nothing else. No change to `Clock`, the write-back, the payload, `retained` or persisted layouts.
28-
29-## Step 0 (read-only, inside Stage 6, before editing; stop and report if any answer breaks the plan)
30-1. Re-verify the three Device OS facts above with file:line, including the flag clear at `:646`, which is why the ruled order (read `clockSource()`, then register) is the one that cannot lose a fault; and whether the callback can run before the flag is declared (interrupt versus loop context: is the setter safe to call from the event path, and does the event path run on the application thread?).
31-2. **Where the flag lives** so I21 can read it at the first `isClockTrusted()` call after boot (that call must see the registration-time value): name the header, the lifecycle, and whether `setup()` order guarantees it is set before the first trust decision.
32-3. What a **mid-boot fault** does today to anything that already read the flag (nothing, by design: I21 owns the revoke); confirm no S1/S2/S3 code path consumes the flag.
33-4. **Count** the net `src/` lines; if over +10, stop and report the cut (S1 measured about +9 for the callback and registration; the boolean and the registration-time read are the remainder).
34-
35-## Tests (host; the real functions)
36-Fakes for `Am18x5`/`ExternalTime`: the flag is false after a normal bind (crystal source); true after a bind that reports the RC fallback (registration-time rule); true after a delivered `CLOCK_SOURCE_EXTERNAL_FAILURE` event; stays true after a second event and after the source later reads healthy (never cleared); the callback is registered once and after the bind (structural, in order); no persistence write is made (structural: no `set_`/`flushNow` in S4's code). Mutations (each must fail): registration-time check removed; event ignored; flag cleared by a later read; callback registered before the bind; the handler registered before the `clockSource()` read (ruling 4, a structural order test); flag persisted; the wrong event enum.
37-
38-## Bench (Chip; part of the integrated head's bench, not alone)
39-Host injection is the required proof. Physical induction of a crystal fault is **optional** (the spike marked it unsafe). On the integrated head, confirm on a bound device that the flag reads false on a healthy crystal and (if a device on the RC fallback is available) true at boot, via the existing boot log line S4 adds or the `usingRC` status field.
40-
41-## Protected / not in scope
42-I21 (every trust decision), S1-S3, L; the clock-trust rule, the failsafe, item 19; the persisted and `retained` layouts; M-SoM.
43-
44-## Rollback
45-Revert the S4 commit on the integration branch before the set merges. After the set ships, rollback is to v49.1 (the guard deletes the stored binding).
46-
47-## Amendment 1 (Claude Code, 7 Nov 2026, from the merged integration head e21a4a4)
48-1. **State of the head.** S1 already reads the clock source once at boot: `Generalized-Core-Counter.cpp:657-662` (`startupOscUsingRC = Am18x5.status().clockSource() == particle::RtcClockSource::INTERNAL;`, the `OscState` log, and `DeviceStatusPublisher.cpp:312` publishes `usingRC`). That read is the "read `clockSource()` first" half of ruling 4; **reuse it for the registration-time rule** (set the flag from `startupOscUsingRC`) rather than adding a second read, and register the `ExternalTime.onEvent` callback **after** it. Step 0 re-cites the lines on the head and confirms no `ExternalTime.onEvent` registration exists yet.
49-2. **Budget unchanged: `src/` at most +10 net.** Stop and report the cut if it does not fit.
50-3. **Test-suite rule (workflow §10):** the local suite runs CI's discovery (`tests/*.sh` via zsh, `tests/*.py` via python3), inside a real git worktree (several tests read git history, so an export fails them); any helper under `tests/` exits 0 when run bare; confirm CI green on the PR before "ready". Baseline at the head: the suite count is measured, not inferred (state it).
51-4. **Where the boolean is read:** I21 is not yet implemented, so S4's consumer today is the boot log line and the status payload only if the budget allows without a payload change; do **not** add a payload field (the contract bump is I21's).
52-
53-## Amendment 2 (Chip, 7 Nov 2026, pre-rulings issued during Stage 7 round 1 on 6a6a57d)
54-1. **Cross-thread access:** if Codex flags the plain `bool` (the callback runs on the Am18x5 worker thread, the application thread reads it), the latch becomes **`std::atomic<bool>` with relaxed ordering**, not `volatile`; still `src/` <= +10.
55-2. **A fault between the `clockSource()` read and the handler registration is the accepted residual.** A persistent fault is caught by the RC fallback at the next boot; a transient one is bounded by I21's 60 s check. **I21 needs only the boolean.**
56-If round 1 raises either point, it is closed by these rulings (item 1 is applied as the fix; item 2 is not a finding). Recorded after the Stage 7 round 1 dispatch, which did not carry them.
57-
58:## Amendment 3 (Claude Code, 7 Nov 2026: applying Amendment 2 to the Stage 7 round 1 verdict, NOT VERIFIED on 6a6a57d; `WO-2026-11-06-001-stage7-r1-verdict.md`)
59-- **F1 (High, cross-thread race): fixed as ruled** by Amendment 2 item 1: `std::atomic<bool>` (relaxed), not `volatile`; Codex's estimate +1 line (net +8).
60-- **F2 (Medium, read-to-registration window): closed by Amendment 2 item 2** (accepted residual). **No second `clockSource()` read after registration** (Codex's +1-line option is declined); Amendment 1's "reuse the existing read" stands.
61-- **F3 (Medium, tests): fix, test-only,** minus the second-read order: require atomic storage; add the combined mask runtime vector (`CALIBRATION_FAILURE | CLOCK_SOURCE_EXTERNAL_FAILURE` sets the latch; extend the stub with bitwise combination); add the mutations from Codex's audit (latch initialised true; calibration failure also sets the latch). The order test stays: first source read -> initial set -> registration.
62-Round 2 (the last): Stage 6 round 2 by Copilot (medium), then Stage 7 round 2 (Codex xhigh, 90-minute box, SHA rule); a failure goes to the architect.
4d8af80 WO-2026-11-06-001 (S4): Stage 7 round 1 verdict (NOT VERIFIED on 6a6a57d: atomic latch, window accepted per Amendment 2, tests); Amendment 3; Stage 6 round 2 dispatch
e0afeb1 WO-2026-11-06-001 (S4): Amendment 2 (architect pre-rulings: std::atomic<bool> relaxed if flagged; the read-to-registration window is an accepted residual)
067f6f9 WO-2026-11-06-001 (S4): Stage 7 round 1 Codex dispatch (UNDER REVIEW 6a6a57d; deadline 05:12:09Z)
<shellId: 1 completed with exit code 0>
```

</details>


---

<sub>19s</sub>

### `bash`

**Read sources and tests**

`$ cat src/time/ExternalRtc.h; cat tests/external_rtc_test.cpp tests/external_rtc_structural_test.py; cat tests/stubs/external_rtc/Particle.h; ls tests/stubs/external_rtc; git diff --numstat e21a4a4 -- src`

<details>
<summary>16 lines</summary>

```
Output too large to read at once (21.6 KB). Saved to: /var/folders/_r/m12xn8s15wb_15djjdgfp5gr0000gn/T/1794110346234-copilot-tool-output-80328-f218e99e-d7d4-4d99-840e-9e77c6283e66.txt
Consider using tools like grep (for searching), head/tail (for viewing start/end), view with view_range (for specific sections), or jq (for JSON) to examine portions of the output.

Preview (first 500 chars):
#pragma once
// v50 S1: Device OS owns the carrier AB1805. Every app read/write of the chip goes through hal_rtc_* (external source).
#include "Particle.h"

namespace ExternalRtc {
constexpr time_t kWriteVerifyToleranceSec = 2;  // read-back must land within this of the written time
inline bool get(time_t &t) {
  struct timeval tv = {};
  hal_rtc_option_t opt = {};
  opt.size = sizeof(opt);
  opt.source = HAL_RTC_SOURCE_EXTERNAL;
  if (hal_rtc_get_time(&tv, &opt) != 0) return false;
  t = tv.tv_
<shellId: 2 completed with exit code 0>
```

</details>


---

<sub>22s</sub>

### `bash`

**Header tail and stub**

`$ sed -n 25,50p src/time/ExternalRtc.h; echo ----; cat tests/stubs/external_rtc/Particle.h; ls tests/stubs/external_rtc; git diff --numstat e21a4a4 -- src`

<details>
<summary>124 lines</summary>

```
}
inline bool setFromSystem() { return Time.isValid() && set(Time.now()); }
inline bool isSet() {
  hal_rtc_option_t opt = {};
  opt.size = sizeof(opt);
  opt.source = HAL_RTC_SOURCE_EXTERNAL;
  return hal_rtc_time_is_valid(&opt);
}
// v50 S4: process-lifetime latch (RAM only, never cleared, never persisted); a reboot re-derives it from the clock source.
inline bool &oscillatorFaultSeenSinceBind() { static bool seen = false; return seen; }
inline void onOscillatorEvent(particle::RtcEvents e, void *) {
  if (e.isSet(particle::RtcEvent::CLOCK_SOURCE_EXTERNAL_FAILURE)) oscillatorFaultSeenSinceBind() = true;
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
  } else {
----
#pragma once
// Host fake of the Device OS surface src/time/ExternalRtc.h uses.
#include <cstdio>
#include <cstdint>
#include <ctime>
#include <sys/time.h>

enum hal_rtc_source_t { HAL_RTC_SOURCE_INTERNAL = 0, HAL_RTC_SOURCE_EXTERNAL = 1 };
struct hal_rtc_option_t {
  uint16_t size;
  int source;
};

struct FakeHal {
  int getResult = 0, setResult = 0;
  time_t chipTime = 0;
  int lastSource = -1;
  int lastUsec = -1;
  int sets = 0;
  bool valid = false;
  bool noStick = false;      // the write returns success but the chip keeps its time
  time_t writeSkew = 0;      // the chip stores written time + this
  int readbackResult = 0;    // get error once a set has happened (the read-back)
  bool afterSet = false;
};
inline FakeHal &fakeHal() { static FakeHal h; return h; }

inline int hal_rtc_get_time(struct timeval *tv, const hal_rtc_option_t *o) {
  fakeHal().lastSource = o ? o->source : -1;
  if (fakeHal().getResult) return fakeHal().getResult;
  if (fakeHal().afterSet && fakeHal().readbackResult) return fakeHal().readbackResult;
  tv->tv_sec = fakeHal().chipTime;
  tv->tv_usec = 0;
  return 0;
}
inline int hal_rtc_set_time(const struct timeval *tv, const hal_rtc_option_t *o) {
  fakeHal().lastSource = o ? o->source : -1;
  fakeHal().lastUsec = (int)tv->tv_usec;
  fakeHal().sets++;
  if (fakeHal().setResult) return fakeHal().setResult;
  fakeHal().afterSet = true;
  if (!fakeHal().noStick) fakeHal().chipTime = tv->tv_sec + fakeHal().writeSkew;
  return 0;
}
inline bool hal_rtc_time_is_valid(const hal_rtc_option_t *o) {
  fakeHal().lastSource = o ? o->source : -1;
  return fakeHal().valid;
}

struct FakeTime {
  bool valid = true;
  time_t now_ = 1800000000;
  bool isValid() { return valid; }
  time_t now() { return now_; }
};
inline FakeTime &fakeTime() { static FakeTime t; return t; }
#define Time fakeTime()

struct FakeLog {
  int infos = 0, warns = 0;
  template <typename... A> void info(const char *, A...) { infos++; }
  template <typename... A> void warn(const char *, A...) { warns++; }
};
inline FakeLog &fakeLog() { static FakeLog l; return l; }
#define Log fakeLog()

namespace particle {
enum class RtcClockSource : uint8_t { NONE = 0, INTERNAL = 1, EXTERNAL = 2 };
struct HalDevice { struct { int interface; } i2c; };
struct Caps { uint32_t v = 0; uint32_t value() const { return v; } };
struct Am18x5Configuration {
  bool valid_ = false;
  uint8_t addr = 0x69;
  int intPin = -1, wdtPin = -1, wire = 0, tsrc = 0;
  uint32_t caps = 0;
  RtcClockSource src = RtcClockSource::INTERNAL;
  HalDevice dev{};
  Am18x5Configuration &i2c(int w) { wire = w; dev.i2c.interface = w; valid_ = true; return *this; }
  Am18x5Configuration &interruptPin(int p) { intPin = p; return *this; }
  Am18x5Configuration &watchdogPin(int p) { wdtPin = p; return *this; }
  Am18x5Configuration &capabilities(uint32_t c) { caps = c; return *this; }
  Am18x5Configuration &clockSource(RtcClockSource c) { src = c; return *this; }
  RtcClockSource clockSource() const { return src; }
  bool valid() const { return valid_; }
  uint8_t address() const { return addr; }
  int interruptPin() const { return intPin; }
  int watchdogPin() const { return wdtPin; }
  Caps capabilities() const { Caps c; c.v = caps; return c; }
  int defaultTimeSource() const { return tsrc; }
  const HalDevice *halDevice() const { return &dev; }
};
enum class RtcEvent : uint32_t { NONE = 0, CALIBRATION_FAILURE = 1, CLOCK_SOURCE_EXTERNAL_FAILURE = 2 };
struct RtcEvents {
  uint32_t v;
  RtcEvents(RtcEvent e) : v((uint32_t)e) {}
  bool isSet(RtcEvents f) const { return (v & f.v) == f.v; }
};
struct Status { bool b; bool bound() const { return b; } };
}  // namespace particle
Particle.h
2	0	src/Generalized-Core-Counter.cpp
5	0	src/time/ExternalRtc.h
<shellId: 3 completed with exit code 0>
```

</details>


---

<sub>21s</sub>

### `bash`

**Tests**

`$ cat tests/external_rtc_test.cpp | sed -n 100,200p; echo ------; cat tests/external_rtc_structural_test.py`

<details>
<summary>176 lines</summary>

```
FakeRtc bad;
  bad.enableResult = -7;
  CHECK(ExternalRtc::ensureBound(bad, fleet()) == -7 && bad.enables == 1);  // failure is returned, never fatal

  FakeRtc missingCap;
  missingCap.bound_ = true;
  missingCap.stored = fleet().capabilities(0);
  CHECK(ExternalRtc::ensureBound(missingCap, fleet()) == 0 && missingCap.enables == 1);

  FakeRtc internalRc;  // stored config identical except it runs from the internal RC
  internalRc.bound_ = true;
  internalRc.stored = fleet().clockSource(particle::RtcClockSource::INTERNAL);
  warns = log.warns;
  CHECK(ExternalRtc::ensureBound(internalRc, fleet()) == 0 && internalRc.enables == 1 && log.warns == warns + 1);
  CHECK(internalRc.stored.clockSource() == particle::RtcClockSource::EXTERNAL);

  particle::Am18x5Configuration c1 = fleet(), c2 = fleet();
  CHECK(ExternalRtc::sameFleetConfig(c1, c2));
  c2.clockSource(particle::RtcClockSource::INTERNAL);
  CHECK(!ExternalRtc::sameFleetConfig(c1, c2) && !ExternalRtc::sameFleetConfig(c2, c1));

  // S4 latch: false at start; only the external-failure event sets it; never cleared (the registration-time half is structural).
  bool &seen = ExternalRtc::oscillatorFaultSeenSinceBind();
  CHECK(!seen);
  ExternalRtc::onOscillatorEvent(particle::RtcEvent::CALIBRATION_FAILURE, nullptr);
  ExternalRtc::onOscillatorEvent(particle::RtcEvent::NONE, nullptr);
  CHECK(!seen);
  ExternalRtc::onOscillatorEvent(particle::RtcEvent::CLOCK_SOURCE_EXTERNAL_FAILURE, nullptr);
  CHECK(seen);
  ExternalRtc::onOscillatorEvent(particle::RtcEvent::CLOCK_SOURCE_EXTERNAL_FAILURE, nullptr);
  ExternalRtc::onOscillatorEvent(particle::RtcEvent::NONE, nullptr);
  CHECK(seen && ExternalRtc::oscillatorFaultSeenSinceBind());

  if (failures) return 1;
  std::printf("external_rtc_test: all checks passed\n");
  return 0;
}
------
#!/usr/bin/env python3
"""v50 S1: structural checks on src/ plus the ExternalRtc helper/bind host test and its mutations.

Each mutation is applied to a copy of the real source in a temp dir; the check must then FAIL.
Run from anywhere; zsh not required.
"""
import os, re, subprocess, sys, tempfile, shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SRC = ROOT / "src"
MAIN = SRC / "Generalized-Core-Counter.cpp"
HDR = SRC / "time" / "ExternalRtc.h"
TEST = ROOT / "tests" / "external_rtc_test.cpp"
STUB = ROOT / "tests" / "stubs" / "external_rtc"


def strip(code):
    code = re.sub(r"/\*.*?\*/", "", code, flags=re.S)
    return re.sub(r"//[^\n]*", "", code)


def structural(main_text, hdr_text, resetcause_text, publisher_text):
    errs = []
    m = strip(main_text)
    if "ab1805.withFOUT" in m or "ab1805.setup()" in m:
        errs.append("ab1805.setup() is still called")
    if "RESET_CAUSE_EXRTC_BINDING_DELETE" in m or "disable(" in m and "Am18x5" in m:
        errs.append("the v49 binding-delete guard is still present")
    sm = re.search(r"inline bool set\(time_t t\) \{(.*?)\n\}", strip(hdr_text), re.S)
    body = sm.group(1) if sm else ""
    if "hal_rtc_set_time" not in body or "get(back)" not in body or body.index("hal_rtc_set_time") > body.index("get(back)"):
        errs.append("set() does not read the chip back after hal_rtc_set_time")
    if "ExternalRtc::ensureBound(Am18x5" not in m:
        errs.append("setup() does not call ExternalRtc::ensureBound(Am18x5, ...)")
    if not re.search(r"\.i2c\(Wire\)\.interruptPin\(WKP\)\.watchdogPin\(WATCHDOG_DONE_PIN\)", m):
        errs.append("fleet config is not Wire/WKP/D5")
    if ".capabilities(particle::RtcCap::AUTO_CLOCK_SOURCE_INTERNAL_ON_FAIL)" not in m:
        errs.append("fallback capability missing from the fleet config")
    if ".clockSource(particle::RtcClockSource::EXTERNAL)" not in m:
        errs.append("clock source EXTERNAL missing from the fleet config")
    if m.index("ensureBound") > m.index("ensureRetainedLoopForensicsInitialized();"):
        errs.append("bind must come before the first Time-dependent setup work")
    if "RESET_CAUSE_EXRTC_BINDING_DELETE" in strip(resetcause_text):
        errs.append("reset cause 8 is still declared")
    if "kLedgerSchemaVersion = 3" not in publisher_text:
        errs.append("kLedgerSchemaVersion is not 3")
    if not re.search(r'name\("oscStatus"\)\.nullValue\(\)', publisher_text) or not re.search(r'name\("oscCtrl"\)\.nullValue\(\)', publisher_text):
        errs.append("oscStatus/oscCtrl are not null")
    if "startupOscStatusReg" in publisher_text or "startupOscStatusReg" in m:
        errs.append("raw osc register global still present")
    for f in ("time/Clock.cpp", "time/HibernateEligibility.cpp"):
        t = strip((SRC / f).read_text())
        if re.search(r"ab1805\.(getRtcAsTime|setRtcFromSystem|isRTCSet|setRtcFromTime)", t):
            errs.append(f"{f} still reads/writes the RTC through ab1805")
    # v50 S4: the oscillator-fault latch (registration-time rule, registration order, RAM only, never cleared).
    reg = "ExternalTime.onEvent(ExternalRtc::onOscillatorEvent)"
    setrc = re.search(r"if \(startupOscUsingRC\) ExternalRtc::oscillatorFaultSeenSinceBind\(\) = true;", m)
    read = re.search(r"startupOscUsingRC = Am18x5\.status\(\)\.clockSource\(\) == particle::RtcClockSource::INTERNAL;", m)
    if m.count(reg) != 1:
        errs.append("the oscillator-event callback is not registered exactly once")
    elif not (read and setrc and m.index("ensureBound") < read.start() < setrc.start() < m.index(reg)):
        errs.append("order must be: bind, clockSource() read, flag set from it, then register the callback")
    h = strip(hdr_text)
    if not re.search(r"if \(e\.isSet\(particle::RtcEvent::CLOCK_SOURCE_EXTERNAL_FAILURE\)\) oscillatorFaultSeenSinceBind\(\) = true;", h):
        errs.append("the callback does not set the latch on CLOCK_SOURCE_EXTERNAL_FAILURE")
    for name, text in (("main", m), ("header", h), ("publisher", strip(publisher_text))):
        for line in text.splitlines():
            if "oscillatorFaultSeenSinceBind" in line and re.search(r"oscillatorFaultSeenSinceBind\(\) = false|set_|flushNow|retained|persist|EEPROM|\.put\(", line):
                errs.append(f"{name}: the latch is cleared or persisted: {line.strip()}")
    return errs


def build_and_run(hdr_text):
    with tempfile.TemporaryDirectory() as d:
        (Path(d) / "time").mkdir()
        (Path(d) / "time" / "ExternalRtc.h").write_text(hdr_text)
        exe = Path(d) / "t"
        r = subprocess.run(["c++", "-std=c++17", "-Wall", "-Wextra", "-I", str(STUB), "-I", d, str(TEST), "-o", str(exe)],
                           capture_output=True, text=True)
        if r.returncode:
            return False, r.stderr
        r = subprocess.run([str(exe)], capture_output=True, text=True)
        return r.returncode == 0, r.stdout


def main():
    hdr = HDR.read_text()
    errs = structural(MAIN.read_text(), hdr, (SRC / "ResetCause.h").read_text(), (SRC / "cloud" / "DeviceStatusPublisher.cpp").read_text())
    if errs:
        for e in errs:
            print("FAIL:", e)
        sys.exit(1)
    print("OK: structural checks")
    ok, out = build_and_run(hdr)
    if not ok:
        print("FAIL: host test\n" + out)
        sys.exit(1)
    print("OK: host helper/bind test")

    def rep(a, b):
        assert a in hdr, a
        return hdr.replace(a, b, 1)

    hmut = {
        "enable-unconditionally": rep("if (!rtc.status().bound()) {", "if (true) {"),
        "skip-bound-check": rep("if (!sameFleetConfig(stored, want)) {", "if (false) {"),
        "reset-on-enable-failure": rep("r = rtc.enable(fleet);\n    Log.info", "r = rtc.enable(fleet); if (r) std::exit(1);\n    Log.info"),
        "write-via-internal-source": hdr.replace("HAL_RTC_SOURCE_EXTERNAL", "HAL_RTC_SOURCE_INTERNAL"),
        "no-readback": rep("return get(back) && (back > t ? back - t : t - back) <= kWriteVerifyToleranceSec;", "return true;"),
        "threshold-removed": rep("(back > t ? back - t : t - back) <= kWriteVerifyToleranceSec", "true"),
        "readback-error-ignored": rep("time_t back = 0;\n  return get(back) &&", "time_t back = t;\n  get(back); return"),
        "wrong-sign": rep("(back > t ? back - t : t - back) <= kWriteVerifyToleranceSec", "back - t <= kWriteVerifyToleranceSec"),
        "ignore-write-error": rep("if (hal_rtc_set_time(&tv, &opt) != 0) return false;", "hal_rtc_set_time(&tv, &opt);"),
        "ignore-fallback-capability": rep("a.capabilities().value() == b.capabilities().value()", "true"),
        "ignore-clock-source": rep("a.clockSource() == b.clockSource()", "true"),
        "wrong-clock-source-value": rep("a.clockSource() == b.clockSource()", "a.clockSource() == particle::RtcClockSource::INTERNAL"),
        "ignore-pin": rep("a.watchdogPin() == b.watchdogPin()", "true"),
        "nonzero-usec": rep("tv.tv_sec = t;", "tv.tv_sec = t; tv.tv_usec = 5;"),
        "latch-event-ignored": rep("if (e.isSet(", "if (false && e.isSet("),
        "latch-wrong-event-enum": rep("e.isSet(particle::RtcEvent::CLOCK_SOURCE_EXTERNAL_FAILURE)", "e.isSet(particle::RtcEvent::CALIBRATION_FAILURE)"),
        "latch-cleared-by-later-event": rep("oscillatorFaultSeenSinceBind() = true;\n}", "oscillatorFaultSeenSinceBind() = true; else oscillatorFaultSeenSinceBind() = false;\n}"),
    }
    bad = 0
    for name, text in hmut.items():
        ok, _ = build_and_run(text)
        print(("OK" if not ok else "FAIL"), f"mutation {name} is caught")
        bad += ok
    main_text, rc, pub = MAIN.read_text(), (SRC / "ResetCause.h").read_text(), (SRC / "cloud" / "DeviceStatusPublisher.cpp").read_text()
    smut = {
        "readback-before-set": (main_text, rc, pub, hdr.replace("hal_rtc_set_time(&tv, &opt) != 0) return false;\n  time_t back = 0;\n  return get(back)", "time_t back = 0; get(back);\n  if (hal_rtc_set_time(&tv, &opt) != 0) return false;\n  return true && get(back)", 1)),
        "setup-left-in": (main_text.replace("ExternalRtc::ensureBound(Am18x5", "ab1805.withFOUT(WKP).setup(); ExternalRtc::ensureBound(Am18x5", 1), rc, pub),
        "guard-kept": (main_text + "\nRESET_CAUSE_EXRTC_BINDING_DELETE", rc, pub),
        "wrong-pin": (main_text.replace("interruptPin(WKP)", "interruptPin(D4)", 1), rc, pub),
        "no-fallback-cap": (main_text.replace(".capabilities(particle::RtcCap::AUTO_CLOCK_SOURCE_INTERNAL_ON_FAIL)", ".capabilities(particle::RtcCap::NONE)", 1), rc, pub),
        "no-clock-source": (main_text.replace(".clockSource(particle::RtcClockSource::EXTERNAL)", "", 1), rc, pub),
        "cause8-kept": (main_text, rc + "\nRESET_CAUSE_EXRTC_BINDING_DELETE = 8,", pub),
        "latch-registration-time-check-removed": (main_text.replace("if (startupOscUsingRC) ExternalRtc::oscillatorFaultSeenSinceBind() = true;", "", 1), rc, pub),
        "latch-cleared-by-later-read": (main_text.replace("if (startupOscUsingRC) ExternalRtc::oscillatorFaultSeenSinceBind() = true;", "ExternalRtc::oscillatorFaultSeenSinceBind() = startupOscUsingRC;", 1), rc, pub),
        "latch-callback-before-bind": (main_text.replace("ExternalTime.onEvent(ExternalRtc::onOscillatorEvent);", "", 1).replace("ExternalRtc::ensureBound(Am18x5", "ExternalTime.onEvent(ExternalRtc::onOscillatorEvent); ExternalRtc::ensureBound(Am18x5", 1), rc, pub),
        "latch-callback-before-clocksource-read": (main_text.replace("ExternalTime.onEvent(ExternalRtc::onOscillatorEvent);", "", 1).replace("startupOscUsingRC = Am18x5.status()", "ExternalTime.onEvent(ExternalRtc::onOscillatorEvent); startupOscUsingRC = Am18x5.status()", 1), rc, pub),
        "latch-registered-twice": (main_text.replace("ExternalTime.onEvent(ExternalRtc::onOscillatorEvent);", "ExternalTime.onEvent(ExternalRtc::onOscillatorEvent); ExternalTime.onEvent(ExternalRtc::onOscillatorEvent);", 1), rc, pub),
        "latch-persisted": (main_text + "\ncurrent.set_oscFault(ExternalRtc::oscillatorFaultSeenSinceBind());", rc, pub),
        "schema-2": (main_text, rc, pub.replace("kLedgerSchemaVersion = 3", "kLedgerSchemaVersion = 2")),
    }
    for name, tup in smut.items():
        a, b, c = tup[:3]
        caught = bool(structural(a, tup[3] if len(tup) > 3 else hdr, b, c))
        print(("OK" if caught else "FAIL"), f"mutation {name} is caught")
        bad += not caught
    if bad:
        sys.exit(1)
    print("external_rtc_structural_test: all checks passed")


main()
<shellId: 4 completed with exit code 0>
```

</details>


---

<sub>30s</sub>

### `bash`

**Apply atomic latch**

```
$ python3 - <<'E'
import re
p='src/time/ExternalRtc.h'
s=open(p).read()
s=s.replace('#include "Particle.h"\n','#include "Particle.h"\n#include <atomic>\n',1)
s=s.replace('inline bool &oscillatorFaultSeenSinceBind() { static bool seen = false; return seen; }','inline std::atomic<bool> &oscillatorFaultSeenSinceBind() { static std::atomic<bool> seen{false}; return seen; }')
s=s.replace('oscillatorFaultSeenSinceBind() = true;\n}','oscillatorFaultSeenSinceBind().store(true, std::memory_order_relaxed);\n}')
open(p,'w').write(s)
E
git diff -- src | head -40; git diff --numstat e21a4a4 -- src
```

<details>
<summary>26 lines</summary>

```diff
diff --git a/src/time/ExternalRtc.h b/src/time/ExternalRtc.h
index b3ae9f6..e2c2a05 100644
--- a/src/time/ExternalRtc.h
+++ b/src/time/ExternalRtc.h
@@ -1,6 +1,7 @@
 #pragma once
 // v50 S1: Device OS owns the carrier AB1805. Every app read/write of the chip goes through hal_rtc_* (external source).
 #include "Particle.h"
+#include <atomic>
 
 namespace ExternalRtc {
 constexpr time_t kWriteVerifyToleranceSec = 2;  // read-back must land within this of the written time
@@ -31,9 +32,9 @@ inline bool isSet() {
   return hal_rtc_time_is_valid(&opt);
 }
 // v50 S4: process-lifetime latch (RAM only, never cleared, never persisted); a reboot re-derives it from the clock source.
-inline bool &oscillatorFaultSeenSinceBind() { static bool seen = false; return seen; }
+inline std::atomic<bool> &oscillatorFaultSeenSinceBind() { static std::atomic<bool> seen{false}; return seen; }
 inline void onOscillatorEvent(particle::RtcEvents e, void *) {
-  if (e.isSet(particle::RtcEvent::CLOCK_SOURCE_EXTERNAL_FAILURE)) oscillatorFaultSeenSinceBind() = true;
+  if (e.isSet(particle::RtcEvent::CLOCK_SOURCE_EXTERNAL_FAILURE)) oscillatorFaultSeenSinceBind().store(true, std::memory_order_relaxed);
 }
 // The fields the fleet configuration fixes (including the clock source: Device OS reports the actual one).
 template <typename C> bool sameFleetConfig(C &a, C &b) {
2	0	src/Generalized-Core-Counter.cpp
6	0	src/time/ExternalRtc.h
<shellId: 5 completed with exit code 0>
```

</details>


---

<sub>36s</sub>

### `bash`

**Edit stub and test**

```
$ python3 - <<'E'
p='tests/stubs/external_rtc/Particle.h'
s=open(p).read()
s=s.replace("struct Status {","inline RtcEvents operator|(RtcEvents a, RtcEvents b) { RtcEvents r(RtcEvent::NONE); r.v = a.v | b.v; return r; }\nstruct Status {",1)
open(p,'w').write(s)
p='tests/external_rtc_test.cpp'
s=open(p).read()
s=s.replace("  bool &seen = ExternalRtc::oscillatorFaultSeenSinceBind();\n  CHECK(!seen);","  auto &seen = ExternalRtc::oscillatorFaultSeenSinceBind();\n  CHECK(!seen.load());",1)
s=s.replace("  CHECK(!seen);\n  ExternalRtc::onOscillatorEvent(particle::RtcEvent::CLOCK","  CHECK(!seen.load());\n  ExternalRtc::onOscillatorEvent(particle::RtcEvent::CLOCK",1)
s=s.replace("  CHECK(seen);\n","  CHECK(seen.load());\n",1)
s=s.replace("CHECK(seen && ExternalRtc::oscillatorFaultSeenSinceBind());","""CHECK(seen.load() && ExternalRtc::oscillatorFaultSeenSinceBind().load());
  CHECK(std::is_same<decltype(seen), std::atomic<bool> &>::value);
""",1)
s=s.rstrip()
s=s.replace("\n  if (failures) return 1;","""
  // Combined mask: calibration failure alone must not set it; combined with the external failure it must.
  // (Same process-lifetime latch is already true here, so check via a fresh copy of the logic's inputs.)
  if (failures) return 1;""",1) if False else s
open(p,'w').write(s+"\n")
E
grep -n "include\|seen" tests/external_rtc_test.cpp | head -30
```

<details>
<summary>9 lines</summary>

```
3:#include "time/ExternalRtc.h"
4:#include <cstdlib>
122:  auto &seen = ExternalRtc::oscillatorFaultSeenSinceBind();
123:  CHECK(!seen.load());
126:  CHECK(!seen.load());
128:  CHECK(seen.load());
131:  CHECK(seen.load() && ExternalRtc::oscillatorFaultSeenSinceBind().load());
132:  CHECK(std::is_same<decltype(seen), std::atomic<bool> &>::value);
<shellId: 6 completed with exit code 0>
```

</details>


---

<sub>42s</sub>

### `bash`

**Add combined vector**

```
$ python3 - <<'E'
p='tests/external_rtc_test.cpp'
s=open(p).read()
s=s.replace('#include <cstdlib>','#include <cstdlib>\n#include <type_traits>',1)
s=s.replace("  CHECK(seen.load() && ExternalRtc::oscillatorFaultSeenSinceBind().load());\n","""  CHECK(seen.load() && ExternalRtc::oscillatorFaultSeenSinceBind().load());

  seen.store(false);  // test-only reset to exercise the combined mask from a clean latch
  ExternalRtc::onOscillatorEvent(particle::RtcEvent::CALIBRATION_FAILURE | particle::RtcEvent::CLOCK_SOURCE_EXTERNAL_FAILURE, nullptr);
  CHECK(seen.load());
""",1)
open(p,'w').write(s)

p='tests/external_rtc_structural_test.py'
s=open(p).read()
s=s.replace('''if not re.search(r"if \\(e\\.isSet(particle::RtcEvent::CLOCK_SOURCE_EXTERNAL_FAILURE)\\) oscillatorFaultSeenSinceBind\\(\\) = true;", h):''','XX')
open(p,'w').write(s)
E
grep -n "isSet\|XX" tests/external_rtc_structural_test.py
```

```
65:    if not re.search(r"if \(e\.isSet\(particle::RtcEvent::CLOCK_SOURCE_EXTERNAL_FAILURE\)\) oscillatorFaultSeenSinceBind\(\) = true;", h):
120:        "latch-event-ignored": rep("if (e.isSet(", "if (false && e.isSet("),
121:        "latch-wrong-event-enum": rep("e.isSet(particle::RtcEvent::CLOCK_SOURCE_EXTERNAL_FAILURE)", "e.isSet(particle::RtcEvent::CALIBRATION_FAILURE)"),
<shellId: 7 completed with exit code 0>
```


---

<sub>53s</sub>

### `bash`

**Update structural test and run**

```
$ python3 - <<'E'
p='tests/external_rtc_structural_test.py'
s=open(p).read()
s=s.replace('''oscillatorFaultSeenSinceBind\\(\\) = true;", h):''','''oscillatorFaultSeenSinceBind\\(\\)\\.store\\(true, std::memory_order_relaxed\\);", h):''',1)
s=s.replace('''        errs.append("the callback does not set the latch on CLOCK_SOURCE_EXTERNAL_FAILURE")
''','''        errs.append("the callback does not set the latch on CLOCK_SOURCE_EXTERNAL_FAILURE (relaxed atomic store)")
    if "#include <atomic>" not in hdr_text or not re.search(r"std::atomic<bool> &oscillatorFaultSeenSinceBind\\(\\) \\{ static std::atomic<bool> \\w+\\{false\\};", h):
        errs.append("the latch is not std::atomic<bool> initialised false")
''',1)
s=s.replace('|persist|EEPROM','|store\\(false|persist|EEPROM',1) if False else s
s=s.replace('oscillatorFaultSeenSinceBind\\(\\) = false|set_','oscillatorFaultSeenSinceBind\\(\\) = false|store\\(false|set_',1)
s=s.replace('''        "latch-cleared-by-later-event": rep("oscillatorFaultSeenSinceBind() = true;\\n}", "oscillatorFaultSeenSinceBind() = true; else oscillatorFaultSeenSinceBind() = false;\\n}"),
''','''        "latch-cleared-by-later-event": rep("std::memory_order_relaxed);\\n}", "std::memory_order_relaxed); else oscillatorFaultSeenSinceBind().store(false);\\n}"),
        "latch-initialised-true": rep("seen{false}", "seen{true}"),
        "latch-calibration-failure-also-sets": rep("if (e.isSet(particle::RtcEvent::CLOCK_SOURCE_EXTERNAL_FAILURE))", "if (e.isSet(particle::RtcEvent::CLOCK_SOURCE_EXTERNAL_FAILURE) || e.isSet(particle::RtcEvent::CALIBRATION_FAILURE))"),
        "latch-plain-bool": rep("std::atomic<bool> &oscillatorFaultSeenSinceBind() { static std::atomic<bool> seen{false}; return seen; }", "bool &oscillatorFaultSeenSinceBind() { static bool seen = false; return seen; }").replace(".store(true, std::memory_order_relaxed)", " = true"),
        "latch-volatile-bool": rep("std::atomic<bool> &oscillatorFaultSeenSinceBind() { static std::atomic<bool> seen{false}; return seen; }", "volatile bool &oscillatorFaultSeenSinceBind() { static volatile bool seen = false; return seen; }").replace(".store(true, std::memory_order_relaxed)", " = true"),
''',1)
open(p,'w').write(s)
E
grep -n "latch-cleared-by-later-event\|latch-plain" tests/external_rtc_structural_test.py | cut -c1-80; python3 tests/external_rtc_structural_test.py | grep -v "^OK"
```

<details>
<summary>17 lines</summary>

```
124:        "latch-cleared-by-later-event": rep("std::memory_order_relaxed);\n}"
127:        "latch-plain-bool": rep("std::atomic<bool> &oscillatorFaultSeenSince
FAIL: host test
/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s4/tests/external_rtc_test.cpp:135:74: error: invalid operands to binary expression ('particle::RtcEvent' and 'particle::RtcEvent')
  ExternalRtc::onOscillatorEvent(particle::RtcEvent::CALIBRATION_FAILURE | particle::RtcEvent::CLOCK_SOURCE_EXTERNAL_FAILURE, nullptr);
                                 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ^ ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s4/tests/external_rtc_test.cpp:137:38: error: too many arguments provided to function-like macro invocation
  CHECK(std::is_same<decltype(seen), std::atomic<bool> &>::value);
                                     ^
/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s4/tests/external_rtc_test.cpp:8:9: note: macro 'CHECK' defined here
#define CHECK(c) do { if (!(c)) { std::printf("FAIL line %d: %s\n", __LINE__, #c); failures++; } } while (0)
        ^
/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s4/tests/external_rtc_test.cpp:137:3: error: use of undeclared identifier 'CHECK'
  CHECK(std::is_same<decltype(seen), std::atomic<bool> &>::value);
  ^
3 errors generated.
<shellId: 8 completed with exit code 0>
```

</details>


---

<sub>57s</sub>

### Copilot

Stub operator must take RtcEvent operands; fix macro commas too.

---

<sub>1m 22s</sub>

### `bash`

**Fix and rerun**

```
$ python3 - <<'E'
p='tests/stubs/external_rtc/Particle.h'
s=open(p).read()
s=s.replace("inline RtcEvents operator|(RtcEvents a, RtcEvents b) { RtcEvents r(RtcEvent::NONE); r.v = a.v | b.v; return r; }","inline RtcEvents operator|(RtcEvents a, RtcEvents b) { RtcEvents r(RtcEvent::NONE); r.v = a.v | b.v; return r; }\ninline RtcEvents operator|(RtcEvent a, RtcEvent b) { return RtcEvents(a) | RtcEvents(b); }")
open(p,'w').write(s)
p='tests/external_rtc_test.cpp'
s=open(p).read()
s=s.replace("CHECK(std::is_same<decltype(seen), std::atomic<bool> &>::value);","CHECK((std::is_same<decltype(seen), std::atomic<bool> &>::value));")
open(p,'w').write(s)
E
python3 tests/external_rtc_structural_test.py | grep -v "^OK"; python3 tests/external_rtc_structural_test.py | grep -c "latch"
```

```
external_rtc_structural_test: all checks passed
13
<shellId: 9 completed with exit code 0>
```


---

<sub>9m 15s</sub>

### `bash`

**Run full test suite**

```
$ p=0;n=0; for f in tests/*.sh; do n=$((n+1)); zsh $f >/dev/null 2>&1 && p=$((p+1)) || echo FAIL $f; done; for f in tests/*.py; do n=$((n+1)); python3 $f >/dev/null 2>&1 </dev/null && p=$((p+1)) || echo FAIL $f; done; echo $p/$n; python3 --version; zsh --version
ls build-tmp 2>/dev/null | head; git status --short
```

<details>
<summary>11 lines</summary>

```
121/121
Python 3.14.6
zsh 5.9 (arm64-apple-darwin26.0)
wo-2026-11-03-004-stage6
wo-2026-11-03-006-stage6-r2
wo-2026-11-06-001-stage6
 M src/time/ExternalRtc.h
 M tests/external_rtc_structural_test.py
 M tests/external_rtc_test.cpp
 M tests/stubs/external_rtc/Particle.h
<shellId: 10 completed with exit code 0>
```

</details>


---

<sub>9m 21s</sub>

### `bash`

**Thermal clean export; find build recipe**

```
$ S=build-tmp/wo-2026-11-06-001-stage6-r2; mkdir -p $S/export && git archive HEAD | tar -x -C $S/export && (cd $S/export && python3 tests/thermal_coupling_structural_test.py </dev/null | tail -2; echo rc=$?); rm -rf $S/export
grep -n -i "make\|toolchain\|BUILD_PATH" docs/work-orders/WO-2026-11-06-001-stage6-copilot-report.md | head -20; ls /Users/chipmc/.particle/toolchains
```

<details>
<summary>11 lines</summary>

```
with no `static` cross-call accumulator (AC-C9) and all
      samples are collected within a single bounded loop (AC-C10).
rc=0
6:## Step 0 (Device OS 6.6.0, `~/.particle/toolchains/deviceOS/6.6.0`)
31:- Build (Device OS 6.6.0, Boron, gcc-arm 10.2.1, buildscripts 1.17.2, fresh `BUILD_PATH_BASE` as make argument): rc 0. **text/data/bss 151084 / 1114 / 2256** (S3 head 150924 / 1114 / 2240: +160 / 0 / +16).
buildscripts
buildtools
deviceOS
gcc-arm
openocd
<shellId: 11 completed with exit code 0>
```

</details>


---

<sub>9m 24s</sub>

### `bash`

**Find build command**

`$ grep -n "make " docs/work-orders/WO-2026-11-06-001-stage6-copilot-transcript.md | head -5; ls /Users/chipmc/.particle/toolchains/{deviceOS,buildscripts,gcc-arm,buildtools}`

<details>
<summary>19 lines</summary>

```
81:4. **Verify (workflow §10, binding):** CI's discovery inside this git worktree: `tests/*.sh` via zsh and `tests/*.py` via python3 (count `p=$((p+1))`; report `N/N (sh via zsh, py via python3)`; the S3 head ran 121/121 and S4 adds its tests: state the measured baseline you saw before editing); any new helper under `tests/` exits 0 bare; `thermal_coupling_structural_test.py` also on a clean export of tracked files (remove your scratch copies from `build-tmp/` first); local toolchain build (Device OS 6.6.0, fresh `BUILD_PATH_BASE` as a make argument): text/data/bss (head: S3 merged, 150924 / 1114 / 2240) and `nm` showing the callback linked; `git diff --numstat e21a4a4 -- src` net <= +10 (state the number).
1857:`$ grep -rhn "make .*APPDIR\|make -f\|PLATFORM=boron\|TARGET_FILE" docs/work-orders/WO-2026-11-05-00*.md docs/work-orders/*s1*report* 2>/dev/null | head -8; ls docs/work-orders | grep -i "11-0"; ls project.properties`
1863:990:make -f '/Users/chipmc/.particle/toolchains/buildscripts/1.17.2/Makefile' compile-user -s
1864:1058:DEVICE_OS_PATH="$T/deviceOS/6.6.0" APPDIR="$PWD" PLATFORM=boron DEVICE_OS_VERSION=6.6.0 GCC_ARM_PATH="$T/gcc-arm/10.2.1/bin/" PATH="$PATH:$T/gcc-arm/10.2.1/bin" BUILD_PATH_BASE="$PWD/$B/bpb" make -f $T/buildscripts/1.17.2/Makefile compile-user -s > $B/build.log 2>&1; echo rc=$?; tail -5 $B/build.log; find $B -name "*.elf"
1865:1278:DEVICE_OS_PATH="$T/deviceOS/6.6.0" APPDIR="$PWD" PLATFORM=boron DEVICE_OS_VERSION=6.6.0 GCC_ARM_PATH="$T/gcc-arm/10.2.1/bin/" PATH="$PATH:$T/gcc-arm/10.2.1/bin" make -f $T/buildscripts/1.17.2/Makefile compile-user -s BUILD_PATH_BASE="$PWD/$B/bpb" > $B/build.log 2>&1; echo rc=$?; tail -5 $B/build.log|cut -c1-300; find $B target -name "*.elf" -newer src/time/ExternalRtc.h
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
<shellId: 12 completed with exit code 0>
```

</details>


---

<sub>13m 8s</sub>

### `bash`

**Local toolchain build**

`$ T=/Users/chipmc/.particle/toolchains; B=build-tmp/wo-2026-11-06-001-stage6-r2; mkdir -p $B; DEVICE_OS_PATH="$T/deviceOS/6.6.0" APPDIR="$PWD" PLATFORM=boron DEVICE_OS_VERSION=6.6.0 GCC_ARM_PATH="$T/gcc-arm/10.2.1/bin/" PATH="$PATH:$T/gcc-arm/10.2.1/bin" make -f $T/buildscripts/1.17.2/Makefile compile-user -s BUILD_PATH_BASE="$PWD/$B/bpb" > $B/build.log 2>&1; echo rc=$?; tail -6 $B/build.log | cut -c1-200; find $B target -name "*.elf" -newer src/time/ExternalRtc.h 2>/dev/null`

<details>
<summary>7 lines</summary>

```
rc=0
Creating /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s4/target/6.6.0/boron/platform_user_ram.ld 
   text	   data	    bss	    dec	    hex	filename
 151092	   1114	   2256	 154462	  25b5e	/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s4/target/6.

*** COMPILED SUCCESSFULLY ***

target/6.6.0/boron/wt-s4.elf
<shellId: 13 completed with exit code 0>
```

</details>


---

<sub>13m 12s</sub>

### `bash`

**nm linkage check**

`$ T=/Users/chipmc/.particle/toolchains; $T/gcc-arm/10.2.1/bin/arm-none-eabi-nm -C target/6.6.0/boron/wt-s4.elf | grep -i "oscillator"; git status --short; git check-ignore target build-tmp`

<details>
<summary>9 lines</summary>

```
000b53cc W ExternalRtc::onOscillatorEvent(particle::EnumFlags<particle::RtcEvent, void>, void*)
2003d728 V ExternalRtc::oscillatorFaultSeenSinceBind()::seen
 M src/time/ExternalRtc.h
 M tests/external_rtc_structural_test.py
 M tests/external_rtc_test.cpp
 M tests/stubs/external_rtc/Particle.h
target
build-tmp
<shellId: 14 completed with exit code 0>
```

</details>


---

<sub>13m 22s</sub>

### Copilot

Write the report.

---

<sub>13m 22s</sub>

### `create`

**create a new file at /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s4/docs/work-orders/WO-2026-11-06-001-stage6-r2-copilot-report.md.**

`/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s4/docs/work-orders/WO-2026-11-06-001-stage6-r2-copilot-report.md`

```
Created file /private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s4/docs/work-orders/WO-2026-11-06-001-stage6-r2-copilot-report.md with 2525 characters
```


---

<sub>13m 26s</sub>

### Copilot

I implemented the round-2 fixes and everything passes. Nothing is committed, pushed or merged. The report is at `docs/work-orders/WO-2026-11-06-001-stage6-r2-copilot-report.md`.

- **Latch:** `ExternalRtc.h` now holds the latch as `std::atomic<bool>`, and the callback stores with relaxed ordering. Net `src/` is +8 against e21a4a4, inside the +10 budget.
- **Tests:**
  - The host test now asserts the atomic type and has a combined-mask vector (`CALIBRATION_FAILURE | CLOCK_SOURCE_EXTERNAL_FAILURE` sets the latch).
  - The stub has bitwise `|` for events.
  - The structural test requires atomic storage and keeps the order test with no second read.
  - Mutations for latch initialised true, calibration failure also setting it, plain `bool` and `volatile bool` are all caught.
- **Suite:** 121/121 (sh via zsh, py via python3). The thermal structural test also passes on a clean `git archive HEAD` export.
- **Build:** Device OS 6.6.0 Boron build succeeds at 151092 / 1114 / 2256 (round 1: 151084 / 1114 / 2256). `nm` shows both the callback and the latch linked.

**Deviation:** The registration-time site in `Generalized-Core-Counter.cpp` still does `= true`. On an atomic that is a sequentially consistent store, which is stronger than relaxed and still correct. Making it relaxed would need an edit to that file, which this round doesn't authorize.

I couldn't observe the reasoning level from inside the session; it was dispatched as medium.

---

<sub>Generated by [GitHub Copilot CLI](https://github.com/features/copilot/cli)</sub>

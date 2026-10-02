# WO-2026-10-02-001: v32, connectivity recovery and field visibility

**Goal, in plain language:** a device that can't reach the cloud gets back within about 3 hours instead of 18, and every report shows enough about memory and sleep to diagnose the next problem.

**Status:** Stage 7 **VERIFIED WITH NOTES** (round 2, 2026-10-02): C VERIFIED in round 2; A, B, D and E VERIFIED in round 1. **At USER GATE 2** (Stage 8: Chip).

**Workflow:** Per `AI_DEVELOPMENT_WORKFLOW.md`: Stage 5 → Stage 6 (Copilot) → Stage 7 (Codex), §5 model routing, §12 guardrails (plain goal, size budgets, two-round rule, facts checked against their source, budget vs actual in the closing record).

**Branch:** `wo/2026-10-02-001-recovery-visibility`, from `main` at `71f955e` (v31 merged, PR #54). Every file:line below was checked against `71f955e` on 2026-10-02.

**Version:** `v32-RecoveryVisibility`, product version 32. Product version 32 was not in the product's firmware list at 13:52Z on 1 Oct (the v31.1-BenchOTA build was never uploaded). Chip confirms in the console before release.

**Sources:** `docs/work-orders/2026-10-01-connectivity-investigation-codex-report.md`; Claude Code's 2026-10-01 read-only investigations (soak update, heap/heap-guard, connection-stage mapping), recorded in project memory.

## Background (step 0, read-only, 2026-10-02)

**Two US devices were silent for about 18 hours and came back only through the failsafe's full reset:**

| Device | Silence (last contact → recovery) | How it ended |
|---|---|---|
| MAFC-1 (e00fce686548d46c4b45e380) | 30 Sep 23:01Z → 1 Oct 17:22Z | v28 status 17:22:29Z: 140 / 0, breadcrumb 10 (`CONNECTIVITY_FAILSAFE_HARD`), 38.3 h uptime |
| Court3 (e00fce686e1a157c27984295) | 30 Sep 21:59Z → 1 Oct 16:00Z | v30 status 16:00:32Z: 140 / 0, breadcrumb 10, 21.3 h uptime, `failsafeStage` 2 |

**Reports with a payload stamp inside the silence: none were made during it.** Each device has one report whose stamp falls inside the window, and each was made after the reset (`rs` 1):
- **MAFC-1:** the catch-up close, stamped 21:59:59 EDT on 30 Sep (boundary − 1) and published 18:00:07Z on 1 Oct.
- **Court3:** the first report of its post-reset boot, stamped 12:00:08 EDT (16:00:08Z) and published 16:00:34Z.

**So the normal report cycle wasn't running during either silence.**

**Occupancy during the silence:**
- **MAFC-1: none recorded.** dd was 163 before and after. The session open at 18:59:54 EDT was lost in the reset.
- **Court3: +321 minutes recorded** (dd 57 → 378), and it survived the reset. So Court3's main loop kept counting while no reports went out.

On Court3, the failsafe's stage 1 (radio reset, at 12 h) didn't restore the connection; stage 2's full reset (at 18 h) did, within a minute. PCKL1 on 30 Sep was another case of a running device not reporting after its close. It recovered after about 1 h through an unexplained 140 / 0 reset with breadcrumb 18.

## Change

**Total `src/` budget: about 45 lines, with B reducing it.** Each item's budget is below. Going over any budget means stop and report. One Copilot round; a second needs Chip (two-round rule).

### A. Recover within about 3 hours without the cloud. Budget ≤ 8

The failsafe (`connectivityFailsafeSupervisor()`, `Generalized-Core-Counter.cpp:2536`, called at the top of every `loop()` pass at `:1602`) measures `connectionAgeSec = now − lastConnection` (`:2585–2587`).

**Stage 5 decision 1 (open hours only):**
1. `CONNECTIVITY_FAILSAFE_STALE_SEC` goes from 12 h to **3 h** (`ConnectivityPolicy.h:114`). It's the threshold for the first action, which after B is the full reset.
2. **The age counts open hours only:**
   - The supervisor acts only while `Clock::openness() == Open`.
   - It measures the age from the **later** of `lastConnection` and **the start of today's open period**. Expose the existing `localTodayAt()` (`DailyBoundary.cpp:9`, currently in an anonymous namespace) through `DailyBoundary.h`, e.g. `DailyBoundary::todayAt(hour)`, rather than adding a new helper.
   - **Consequence (recorded):** the 3 hours restart at each opening. A device stuck from 19:00 is reset at about 22:00. One stuck from 20:00 reaches the close with 2 h counted and is reset at about 09:00.
3. **Why this rule:** with wall-clock time, every device would hit 3 h on its 06:00 wake. The overnight gap since the 22:00 close is 7–8 h (Trail02 7 h). The supervisor runs at the top of `loop()` before the report starts connecting, and its only in-progress guard, `activeConnectAttemptWithinBudget()` (`State_Connect.cpp:165`), is true only inside `CONNECTING_STATE`. So every device would reset every morning. The 3 h matches the existing Ubidots 3-hour loss-of-communication alert.
4. **Unchanged:**
   - `COOLDOWN` (6 h, `:115`) and `JITTER` (30 min, `:116`).
   - The low-battery block in the supervisor (`lowBatteryHardActionBlocked`).
   - The clear on a successful cloud connection (`State_Connect.cpp:538–539`, `set_lastConnection` and `clearConnectivityFailsafeRecovery("cloud-ok")`).
5. **Stage 3** (AB1805 deep power-down) today fires `COOLDOWN` + jitter (6 h + up to 30 min) after stage 2. That stays the same relative to stage 2. It also now requires the open-hours age of at least 3 h, so a stage 2 late in the day pushes stage 3 into the next open period.
6. **Test mode keeps short timings** (`ConnectivityPolicy.h:109–112`): stale **5 min**, cooldown **15 min**, jitter **0**, closed-hours sleep cap 60 s. The open-hours rule applies in test mode too, so **the bench run must happen during open hours**. Keep the test-mode prediction code in `ConnectivityFailsafeTest.cpp:162–171` consistent with the new rule.

### B. Stop doing a step that doesn't help. Net negative

Remove stage 1, the radio reset (`Generalized-Core-Counter.cpp:2637–2646`, the `if (nextStage == 1)` block: `Particle.disconnect()` + `Cellular.off()` via `Connectivity::requestFullDisconnectAndRadioOff()`, then `transitionTo(CONNECTING_STATE, "failsafe stage 1")`).
- **Stage numbers keep their meaning:** with no stage recorded (0), the next action is **stage 2**. A stage 1 persisted by older firmware still progresses to 2. `failsafeStage` values never newly take 1. **Stage 1 is retired**, recorded here.
- **Remove what becomes unused:** `BREADCRUMB_CONNECTIVITY_FAILSAFE` and alert 45 (`CONNECTIVITY_FAILSAFE_ALERT`), if nothing else uses them. Report each.
- **Also in B (needed for the bench; 2 lines, outside the net-negative count, reported separately):** fix the failsafe test-mode build, which doesn't compile from the repo.
  - `ConnectivityFailsafeTest.cpp:3`: `#include "Config.h"` → `#include "../Config.h"`. The bare form collides with Device OS's own `Config.h`.
  - `ConnectivityFailsafeTest.h`: add `#include "cloud/BatteryBackoffPolicy.h"`, for `BatteryTier`.
  - Found on 2026-10-01: the v31 bench image needed exactly these two lines.

### C. ConnSummary tells "modem off" from "searching". Budget ≤ 8

`classifyConnAcquirePhase()` (`State_Connect.cpp:48–60`; enum `:26`, labels `:31–46`) returns `CELLULAR_ACQUIRE` whenever `!Cellular.ready()` (sampled at `:323–326`).
- Add a value, **`MODEM_OFF`**: returned when `!Cellular.ready()` **and** `!Cellular.isOn()`. When `Cellular.isOn()` is true, the value is `CELLULAR_ACQUIRE` as now.
- `Cellular.isOn()` is true only when the interface is powered **and** the modem responds (Device OS 6.4.1 `system_network_manager.cpp:1046`).
- Sample it next to `Cellular.ready()`, under `#if Wiring_Cellular`.
- **Diagnostics only.** `MODEM_OFF` time counts into `connPhaseCellMs` (`addPhaseElapsed`, `:338–341`), so `ConnSummary`'s fields are unchanged and only `last=` and `ConnDiag phase=` gain the new label.
- **No decision may read it.** Today only `CLOUD_ACQUIRE` drives a decision (`:334`, `:398`).
- (`NETWORK_ACQUIRE` can't occur on Boron, because `Network.ready()` equals `Cellular.ready()` there. Recorded, unchanged.)

### D. Each report shows memory. Budget ≤ 10

Add **`fh`** (`System.freeMemory()`) and **`lfb`** (largest free heap block) to the report payload.
- `lfb` comes from `HAL_Core_Runtime_Info()`: a `runtime_info_t` with `.size = sizeof(runtime_info_t)`, reading `largest_free_block_heap`.
- That call is exported to user firmware (`hal/inc/hal_dynalib_core.h:68`, `core_hal.h:175–178`) and filled on Boron from `pvPortLargestFreeBlock()` (`hal/src/nRF52840/core_hal.c:954–955`). So it's simple and safe on 6.4.1, and no fallback is needed.
- Both report formats: occupancy (`Generalized-Core-Counter.cpp:1998`) and counting (`:2013`).

### E. Each report shows whether cycles end in sleep. Budget ≤ 12

**Stage 5 decision 2: the definitions.** Both are RAM counters (not retained), reset at every boot. A night HIBERNATE is a boot, so it resets them too.
- **`cyc`, awake periods since boot:** 1 at boot, plus 1 after **every return** from a `System.sleep()` call, whether it succeeded or failed.
- **`slp`, awake periods that ended in a sleep:** plus 1 for each `System.sleep()` that **succeeded** (the result has no error).
- **The call sites:** `State_Sleep.cpp:1148` (HIBERNATE, which returns only on failure), `:1400`, `:1451`, `:1465`.
- **So:**
  - `cyc ≥ slp` always;
  - `cyc − slp − 1` = sleep calls that failed;
  - **the same `cyc` in two reports means the device didn't sleep between them.**
- Add both to both report formats.

### Ubidots payload rule (Stage 5 decision 3)

Ubidots will create a variable for each new top-level key; that's accepted. **Every new field is a JSON number, unquoted** (`"fh":61234`, never `"fh":"61234"`). Any text must go in a context attached to a numeric variable, as `key1` does on battery. No new key may carry a string.

**The payload buffer** is `char data[256]` (`Generalized-Core-Counter.cpp:1964`). The longest report in the archive on 1 Oct was 153 characters, and the four fields add about 48. State the before and after maximum sizes.

## Permitted files

`src/power/ConnectivityPolicy.h`, `src/Generalized-Core-Counter.cpp`, `src/time/DailyBoundary.h/.cpp` (the export only), `src/diagnostics/ConnectivityFailsafeTest.h/.cpp`, `src/state/State_Connect.cpp` (C only), `src/state/State_Sleep.cpp` (E's counters at the sleep calls only), at most one small header for the counters if needed, the version files via `bump_version.sh`, and `tests/`.

## Protected

- Connection budgets and the deep-attempt rule.
- Cloud-recovery stages.
- v31's update-state logic.
- The six reset-cause codes and their meanings.
- The status payload.
- `lib/`.
- The `WITH_ACK` publish path (`tests/publish_with_ack_structural_test.py` must pass unchanged).

## Non-goals

- A loop-level "no progress in this stage" check (design given 2026-10-01; later).
- The heap-loss source (suspects recorded; D gives the data first).
- Enabling hibernate on US devices.
- The webhook or Ubidots configuration.
- A periodic heap field in the status event.

## Acceptance (Stage 7, narrow, each item against its goal)

- **A:** host checks:
  - with no successful cloud connection, during open hours, the full reset (code 2) fires once the open-hours age reaches 3 h, **not before**;
  - **the 06:00 wake after a normal close does not reset**;
  - closed hours never act;
  - a successful connection clears the timer;
  - stage 3 stays `COOLDOWN` + jitter after stage 2.
  - **Mutations:** restoring 12 h fails a test; removing the open-hours base fails the morning-wake test.
- **B:** a structural test that no radio-reset path remains in the supervisor and that the first action is stage 2. `failsafeStage` never newly takes 1. The test-mode build compiles locally (`-DCONNECTIVITY_FAILSAFE_TEST_MODE=1`).
- **C:** with `Cellular.isOn()` false and not ready, the summary shows `MODEM_OFF`; with it true, `CELLULAR_ACQUIRE`. No decision path reads `MODEM_OFF`.
- **D and E:** in both payload formats, `fh`, `lfb`, `cyc` and `slp` appear as **unquoted JSON numbers**, and no new key carries a string. The payload stays within its buffer (before and after maximum sizes stated). `cyc ≥ slp` always.
- **Everywhere:**
  - the suite passes (sh via zsh, py via python3) and the `WITH_ACK` structural test is green;
  - local ARM build and linkage;
  - the total `src/` diff is about 45 lines;
  - budget versus actual goes in the closing table.

## Bench (after the flash, Dev-09; Chip)

1. **The failsafe:** the v32 test-mode build, **during open hours**, with the antenna removed. Expect no stage-1 line, then `Failsafe: stage=2 action=system-reset` after the open-hours age reaches 5 min. Then a status with 140 / code 2.
2. **The payload:** one report in AWS carrying numeric `fh`, `lfb`, `cyc` and `slp`. In the Particle integration log, the webhook still returns success. In Ubidots, the four new variables appear with numeric values.
3. **ConnSummary:** if a modem-off case occurs, `last=MODEM_OFF` appears.

### Bench results (Dev-09, e00fce68399ee6244a963935, 2026-10-02; read from the AWS archive by Claude Code)

**Overall: PASS** (updated 04:20Z on 2 Oct, Claude Code). Items 1 and 2 hold: the report after a sleep shows `cyc` 2 and `slp` 1. The Ubidots check is deferred, and `MODEM_OFF` wasn't observed (accepted: Stage 7 proved C).

**1. The failsafe (v32-FailsafeTest, antenna removed, open hours): PASS.** Published in one burst at 03:56Z, when Dev-09 reconnected:

| Published | version | Reset / data | failsafeTest / stage | Previous boot's breadcrumb (uptime) |
|---|---|---|---|---|
| 03:56:34.810Z | v32-FailsafeTest | 20 / 0 (USB flash) | 1 / 0 | 4 (19,900 s) |
| 03:56:43.722Z | v32-FailsafeTest | **140 / 2** | 1 / **2** | **10** (377 s) |
| 03:56:47.231Z | v32-FailsafeTest | **140 / 2** | 1 / 2 | **10** (879 s) |

- **No stage 1:** every action was the full reset with code 2.
- **The first reset** came about 6.3 min into the boot, inside the expected window of 5 to about 16 min after the last connection (about 03:33Z).
- **The second reset was again stage 2** (not stage 3), so the failsafe was cleared by a successful connection in that boot. That fits the socket drop at 03:47:45Z, and the reset came about 5.5 min after it.
- **The three boot reports** carried numeric `fh` 89,784–89,800, `lfb` 89,728–89,784, `cyc` 1 and `slp` 0.

**2. The payload (v32-RecoveryVisibility, USB flash): PASS.**
- **Status published 04:03:27.391Z:** `version` **v32-RecoveryVisibility**, `resetReason` 20 (USB flash), `resetReasonData` 0, **`failsafeTest` 0**, `fw_version` 32.
- **Reports since the flash (checked 04:09Z): one,** stamped 12:00:13 SGT and published 04:03:29Z:
  - `occupancy` 0, `dailyoccupancy` 171, `fh` **89800**, `lfb` **89728**, `cyc` **1**, `slp` **0**;
  - the raw payload has all four as **unquoted JSON numbers**, and `key1` is still the only string;
  - `cyc ≥ slp` holds.
- **The report after a sleep,** stamped 12:08:32 SGT and published 04:19:27Z (an occupancy-change report, delivered when Dev-09 reconnected):
  - `occupancy` 0, `dailyoccupancy` 178, `fh` **76856**, `lfb` **75648**, `cyc` **2**, `slp` **1**, all unquoted numbers.
  - **Both counters rose** (1 → 2 and 0 → 1), so one awake period ended in a successful sleep; `cyc ≥ slp`.
  - No report shows an unchanged `cyc`.
- **First memory data from a release build:**
  - `fh` 89,800 → 76,856 (−12,944 B) and `lfb` 89,728 → 75,648 (−14,080 B) between the boot report and the first report after connecting.
  - **That's a one-time step, not a rate.** The boot report is made before the cloud session exists. The drop matches the about 13 KB the Singapore serial logs showed the cloud session taking early in each boot (about 90 KB at boot, about 77 KB at the first connection).
  - A change per hour needs reports over the following hours. That's the first job for these fields in the field.
- **Ubidots:** **deferred until after the v33 fleet rollout** (Chip, 2026-10-02), so the bench doesn't wait on this check. One webhook serves every device, and older firmware doesn't send these fields.

**3. ConnSummary `MODEM_OFF`:** not observed yet. Dev-09's serial isn't forwarded to AWS while it's on the Mac.

## Approval record

- [x] Stage 5: Chip and the architect, 2026-10-02, in the opening dispatch (goal, items A–E, budgets, Stage 7 checks, bench, routing: one Copilot round `claude-opus-5` medium, one narrow Stage 7 Codex `gpt-6-astra` high; not authorized: commits, flashing, Ubidots or webhook changes, AWS writes).
- [x] Stage 5 decisions (Chip, 2026-10-02, after Claude Code's checks): (1) A counts open hours only (wall time would reset every device each morning); (2) E as defined above; (3) Ubidots auto-creates the new variables, so every new field must be an unquoted JSON number and Stage 7 checks the serialized payload.
- [x] Stage 6 round 1: Copilot `claude-opus-5` medium. A +6, B −13 (plus the 2-line test-build fix), C +7, D +5, E +12; total +18 (budget about 45). Suite 56/56 (sh via zsh, py via python3). 8/8 mutations caught. Release 150612 / 1090 / 2204; the test-mode build compiles (152180 / 1090 / 2204). Payload maximum 228 / 217 of 256. Deviations: alert 45 kept for the legacy clear path; `nextStage >= 2` left always-true in two places; the 24-hour-site edge case; `daily_cleanup_boundary_test.py` updated for the rename; a ternary used instead of `std::max`. Deleted only its own scratch directory (archive unchanged, 120,584 files). Report: `WO-2026-10-02-001-stage6-copilot-report.md`.
- [x] Stage 7 round 1: Codex `gpt-6-astra` high, **NOT VERIFIED (item C only)**. A, B, D, E, suite, builds, linkage, identity and budgets all VERIFIED.
  - **C finding:** a change between `MODEM_OFF` and `CELLULAR_ACQUIRE` resets `phaseStartMs` (`State_Connect.cpp:368`), which feeds cloud recovery (`:407`). Compiled reproduction: modem off at 0 s, powered at 50 s, cloud acquisition at 70 s. In v31, recovery stage 1 fires at 70 s; with the change it doesn't, because the elapsed value is only 20 s. Accounting is unchanged (70,000 ms in `connPhaseCellMs` either way).
  - **Smallest fix:** treat `MODEM_OFF` and `CELLULAR_ACQUIRE` as the same phase for timing boundaries, while keeping the raw phase for the label. Stays at +7.
  - **Rulings:** keeping alert 45 is correct.
  - **The 24-hour-site consequence is reachable:** validators accept equal open and close hours from 0 to 23. With open = close = 6, recovery first becomes eligible at 09:00. **With equal hours of 21, 22 or 23, three hours never accumulate, so this failsafe is disabled.** No deployed equal-hours device is known.
  - Working tree byte-identical. Verdict: `WO-2026-10-02-001-stage7-verdict.md`.
- [x] Stage 5 decisions after Stage 7 round 1 (Chip, 2026-10-02):
  1. **Round 2 of 2, item C only:** Codex's fix. `MODEM_OFF` and `CELLULAR_ACQUIRE` count as the same phase for timing and differ only in the label, which is what "diagnostics only" meant. Codex's reproduction is added to C's test, followed by a narrow Stage 7 on C. A, B, D and E are frozen. **If round 2 comes back NOT VERIFIED, drop C from v32 and ship A, B, D and E. No third round.**
  2. **Equal open/close hours:** recorded and accepted. No device uses them, and they're covered by WO-2026-09-24-004 (retiring the open == close convention); a line was added there.
- [x] Stage 6 round 2 (item C only): Copilot `claude-opus-5` medium. `MODEM_OFF` and `CELLULAR_ACQUIRE` are one timing phase (a one-line mapping in the phase-change condition), and `lastConnPhase` is assigned after the block so the raw label is kept. C stays at +7. Test additions: Codex's reproduction (stage 1 at 70 s), v31 equivalence over mixed sequences, the label check, and an automated mutation run on a copy. Suite 56/56. Release 150628 / 1090 / 2204. Claude Code checked: the 18 frozen files (A, B, D, E) are hash-identical, and the archive is unchanged. Report: `WO-2026-10-02-001-stage6-r2-copilot-report.md`.
- [x] Stage 7 round 2: Codex `gpt-6-astra` high. **C VERIFIED; v32 overall VERIFIED WITH NOTES.** Independent reproduction: stage 1 at 70,000 ms, as in v31 (stage 2 at 190,000 ms). v31 equivalence: 59,140 sequences and 1,134,670 ticks against `71f955e`, with recovery times, all three accounting counters and summaries identical. `last=MODEM_OFF` / `phase=MODEM_OFF` shown. The mutation is caught. Suite 56/56 (28 sh via zsh, 28 py via python3). Release 150628 / 1090 / 2204 (`.bin` 151,722). Note carried: equal open/close hours of 21–23 disable the failsafe (accepted; WO-2026-09-24-004). Working tree byte-identical. Verdict: `WO-2026-10-02-001-stage7-r2-verdict.md`.
- [x] Stage 8 (Chip, 2026-10-02): accepted. Commit v32 on its branch (by Claude Code on Chip's instruction), with the WO-2026-09-24-004 rewrite as a separate commit. Product version 32 confirmed free. Bench images: `v32-RecoveryVisibility` (release) and `v32-FailsafeTest` (test mode); no OTA image (v32 doesn't touch update logic, and v31's item A was proven on a full download).
- [ ] Bench on Dev-09 (failsafe test build during open hours; payload fields in AWS and Ubidots; `MODEM_OFF` if it occurs).

## Budget versus actual (closing record)

Per `AI_DEVELOPMENT_WORKFLOW.md` §12 guardrail 3. Actuals are from the Stage 7 verdicts (`WO-2026-10-02-001-stage7-verdict.md` for A, B, D and E; `WO-2026-10-02-001-stage7-r2-verdict.md` for C and the total). "not recorded" means the verdict has no figure.

| Item | Budget | Raised to (reason) | Actual net `src/` lines | Tests |
|---|---|---|---|---|
| A | ≤ 8 | — | +6 | not recorded |
| B | net negative (plus the 2-line test-build fix) | — | −13; build fix +1 net (2 lines touched) | not recorded |
| C | ≤ 8 | — | +7 (round 2; round 1 also +7) | not recorded |
| D | ≤ 10 | — | +5 | not recorded |
| E | ≤ 12 | — | +12 | not recorded |
| Total `src/` | about 45 | — | +18 | not recorded |

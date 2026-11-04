# Copilot CLI Session

> [!NOTE]
> - **Session ID:** `188dc936-04e7-48c9-a844-2064d42b2552`  
> - **Started:** 11/4/2026, 9:17:52 AM  
> - **Duration:** 29m 21s  
> - **Exported:** 11/4/2026, 9:47:14 AM  

---

<sub>6s</sub>

### User

Implement per the dispatch in docs/work-orders/WO-2026-11-03-006-stage6-copilot-dispatch.md. The WO is the binding spec wherever the dispatch refers to it. Do not commit, push, merge, or release.

---

<sub>8s</sub>

### `view`

**view the file at /private/tmp/gcc-step0/c2/docs/work-orders/WO-2026-11-03-006-stage6-copilot-dispatch.md.**

/private/tmp/gcc-step0/c2/docs/work-orders/WO-2026-11-03-006-stage6-copilot-dispatch.md

<details>
<summary>10 lines</summary>

```
AGENT: Copilot · MODEL: claude-sonnet-5.5 · REASONING: medium
AUTHORIZATION SCOPE: edit `src/` files named in the WO, tests under `tests/` (expectations that encode the old untrusted merge only, listed at Step 0), write the report; scratch `build-tmp/wo-2026-11-03-006-stage6/` / NOT authorized: committing, pushing, merging, flashing, creating PRs, changing the persisted or `retained` layout, touching the trusted close path or credit arithmetic beyond what the WO names, deleting anything you didn't create (including `build-tmp/`).

# Stage 6 dispatch — WO-2026-11-03-006 (item 19 C2: untrusted split, credit and report on millis)
Binding spec: `docs/work-orders/WO-2026-11-03-006-item19-c2-untrusted-split.md` **including Amendment 1**, `docs/work-orders/WO-2026-11-03-001-step0-report.md` (§6 C, §7, §8, §10-§13), and C1 (`WO-2026-11-03-004-item19-c1-millis-expiry.md`, already merged into this branch: use its `debounceRemainingMs()`). **Budget: `src/` ≤ +16 net.** Leave changes uncommitted.
Do Step 0 first (re-read every cited line on this base, list the B2/B3 and report-by-mode expectations that must change, confirm `DailyBoundary::check()` is trust-gated, confirm the boot path for a persisted open session while untrusted; stop and report if the design cannot be done in +16). Then implement, test (host suite via zsh for `.sh`, python3 for `.py`: `N/N (sh via zsh, py via python3)`) including the Dev-09 replay (96-98 min credited for 97.21 real; the reboot straddler credits its post-boot part only, about 236 s), the mutations in the WO, the local toolchain build (Device OS 6.6.0, fresh `BUILD_PATH_BASE`) and `nm` linkage of the untrusted credit path. Write `docs/work-orders/WO-2026-11-03-006-stage6-copilot-report.md`. The Dev-14 exerciser bench is Chip's later step: do not claim it.

## Added 4 Nov (controller): inputs from C1's Stage 7 round 1 (PASS on 5d22b1b9; `WO-2026-11-03-004-stage7-r1-verdict.md`, already in this branch)
- **The LED-gate gap (C1 verdict note 6): a C2 acceptance blocker.** In OCCUPANCY mode, with the session still occupied, the LED low and the debounce expired, the wake-close is gated on `signalLEDStatus()` (`src/state/State_Modes.cpp:~156`), so no close happens and a PIR wake extends the existing session; v48 has the same gate. C2 cannot claim "split every untrusted expiry" while it exists. **In Step 0, trace when the condition is reachable** (sleep preparation switches the LED off, `State_Sleep.cpp:~505-517`; after a reset) **and say whether the C2 untrusted close can be reached without the LED gate** (the awake Idle path, the sleep-plan wake that returns to `updateOccupancyState`). If C2 can handle it inside +16 (for example by closing an untrusted piece on the predicate alone, leaving the trusted path and the S-6 fixture untouched), include it and state the change; if not, **stop and report** with the line count. Do not change the trusted wake path or `tests/sleep_wake_transcript.txt`.
- C1 note 5: `updateOccupancyState` now closes at exact equality (`>=`) and keeps milliseconds; replay timelines with those semantics.
- C1 note 9: the earlier "flaky" `alert19_clears_after_report_test.sh` was a runner artifact (`test && ((p++)) || fail` in zsh). Count tests with `p=$((p+1))`, and report the real total (108 on C1's head, plus any new tests).
- The Step 0 report is now in the tree at `docs/work-orders/WO-2026-11-03-001-step0-report.md`.
```

</details>


---

<sub>10s</sub>

### `bash`

**cd /private/tmp/gcc-step0/c2 && git status --short | head; git log --oneline | head -5; ls docs/work-orders; wc -c docs/work-orders/WO-2026-11-03-00***

$ cd /private/tmp/gcc-step0/c2 && git status --short | head; git log --oneline | head -5; ls docs/work-orders; wc -c docs/work-orders/WO-2026-11-03-00*

<details>
<summary>8 lines</summary>

```
Output too large to read at once (20.3 KB). Saved to: /var/folders/_r/m12xn8s15wb_15djjdgfp5gr0000gn/T/1793755082975-copilot-tool-output-66605-988c1ee8-42ad-49cb-bf57-917e3159bf5a.txt
Consider using tools like grep (for searching), head/tail (for viewing start/end), view with view_range (for specific sections), or jq (for JSON) to examine portions of the output.

Preview (first 500 chars):
2703390 WO-2026-11-03-006: merge C1 (WO-004, Stage 7 PASS) into the C2 branch; Step 0 report copy; dispatch addendum (LED-gate gap, C1 notes)
80d07c2 Merge remote-tracking branch 'origin/wo/2026-11-03-004-item19-c1-millis-expiry' into wo/2026-11-03-006-item19-c2-untrusted-split
d48dc27 WO-2026-11-03-004: Stage 7 r1 dispatch and verdict (PASS on 5d22b1b9; xhigh, 108/108)
5d22b1b WO-2026-11-03-004 C1: one millis debounce expiry (debounceRemainingMs) for the awake path, the sleep plan and the wake 
<shellId: 0 completed with exit code 0>
```

</details>


---

<sub>11s</sub>

### `bash`

**cd /private/tmp/gcc-step0/c2 && ls docs/work-orders | grep 2026-11-03; wc -c docs/work-orders/WO-2026-11-03-006-item19-c2-untrusted-split.md docs/work-orders/WO-2026-11-03-001-step0-report.md**

$ cd /private/tmp/gcc-step0/c2 && ls docs/work-orders | grep 2026-11-03; wc -c docs/work-orders/WO-2026-11-03-006-item19-c2-untrusted-split.md docs/work-orders/WO-2026-11-03-001-step0-report.md

<details>
<summary>13 lines</summary>

```
WO-2026-11-03-001-step0-report.md
WO-2026-11-03-004-item19-c1-millis-expiry.md
WO-2026-11-03-004-stage6-copilot-dispatch.md
WO-2026-11-03-004-stage6-copilot-report.md
WO-2026-11-03-004-stage6-copilot-transcript.md
WO-2026-11-03-004-stage7-r1-codex-dispatch.md
WO-2026-11-03-004-stage7-r1-verdict.md
WO-2026-11-03-006-item19-c2-untrusted-split.md
WO-2026-11-03-006-stage6-copilot-dispatch.md
    8075 docs/work-orders/WO-2026-11-03-006-item19-c2-untrusted-split.md
   16230 docs/work-orders/WO-2026-11-03-001-step0-report.md
   24305 total
<shellId: 1 completed with exit code 0>
```

</details>


---

<sub>13s</sub>

### `view`

**view the file at /private/tmp/gcc-step0/c2/docs/work-orders/WO-2026-11-03-006-item19-c2-untrusted-split.md.**

/private/tmp/gcc-step0/c2/docs/work-orders/WO-2026-11-03-006-item19-c2-untrusted-split.md

<details>
<summary>43 lines</summary>

```
# WO-2026-11-03-006: item 19 C2 — split, credit and report each untrusted session piece on millis()

**Author:** Claude Code, 3 Nov 2026, DRAFT for the architect's review · **Status:** DRAFT; not dispatched
**Branch:** `wo/2026-11-03-006-item19-c2-untrusted-split`, from `main` **after C1 (WO-2026-11-03-004) has merged**
**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §3 Stages 5-8; §4 (SHA rule); §5 (time and boundaries: Codex xhigh); §12; §13 (c), (j)
**Target:** **v50**, with C1, the AB1805 native-driver switch and item 21. C1 may merge first but does not release without C2.
**Budget:** `src/` at most **+16 net** (removals count). Stop and report rather than trim style.
**Agents:** Stage 6 Copilot standard, medium. Stage 7 Codex `gpt-5.6-sol`, **xhigh**, `UNDER REVIEW` header, `-stage7-r<N>` files, at most two rounds.

## Plain goal
While the clock is untrusted, an open session is kept open, nothing is credited and the debounce is re-armed (B2), so the separate sessions of a pulsed site merge into one; the first trusted close then credits the whole wall span, gaps included (Dev-09, v45, 23 Oct: 205 min credited, about 97 real). With C1 the debounce expiry is one `millis()` predicate. **C2 uses it:** at each untrusted expiry, credit the piece's measured on-time, close it, and let the next PIR open a new piece. The wall clock only assigns the day.

## Architect ruling (3 Nov): design C, accept bounded loss
Item-19 Step 0 (`WO-2026-11-03-001-step0-report.md` §6 C, §7, §8, §11-§13). **A reboot straddler credits post-boot `millis()` only** (no one-debounce fallback): at most one partial session is lost per reboot while untrusted. Item 15 (occupancy lost across a reset) stays separate; item 21 (trust across restart) is a v50 companion, not a dependency.

## The change (re-check every line at Step 0; numbers from main `21804e3`)
1. **RAM `SessionState` gets a monotonic piece start** (`occupancyStartMs`) and a flag `pieceUntrusted`, both set when a piece opens (`State_Modes.cpp:68-108`, `:231-276`) from `millis()` and the current trust. **RAM only: no persisted or retained field changes.**
2. **Untrusted expiry** (`closeOccupancySessionSafely`, `State_Common.h:341-353`): replace the B2 re-arm. When C1's predicate says expired and the piece is untrusted: credit `(lastOccupancyEvent + debounceMs) - occupancyStartMs` (ms, rounded to minutes with the existing rounding rule), add it to the daily total through the existing credit path (`State_Common.h:397-405`), clear `occupied`, and report the occupancy change exactly as a trusted close does. The next PIR opens a new piece.
3. **A piece opened untrusted stays on `millis()` for its whole life,** even if trust returns before it closes (no mixing of an untrusted wall start with a trusted wall close, the Dev-09 205-minute path). A piece opened trusted is unchanged (the wall path, B3 cap and all). Trusted clean days must credit exactly as v48 does.
4. **Reboot straddler** (`Generalized-Core-Counter.cpp:497-505`): when the device boots with a persisted open session and the clock is untrusted, the session is treated as an untrusted piece whose `occupancyStartMs` is boot (`millis()` 0). Only post-boot on-time is credited; the pre-boot portion is dropped; the B3 boot cap stays for the trusted case.
5. **Day assignment:** a piece is credited to the day of its close under the existing openness rule: `Unknown` openness counts as today (WO-2026-10-29-002); `Closed` is unchanged. A piece open at the boundary is closed at the boundary by the existing boundary path (item 10) and credited its measured on-time up to that point.
6. **Report path:** an untrusted close raises the existing occupancy-change report (the same call a trusted close makes); nothing new is added to the payload. One log line per untrusted credit (`Occ: untrusted piece credit=<s> start=<ms>`), within budget.

## Protected / not in scope
The trusted close and credit arithmetic; B3 and the boot cap for trusted sessions; the report format and payload; the persisted layout and `retained` RAM; item 15; item 21; the boundary close logic; the failsafe.

## Tests (host; real helpers)
- **Dev-09 replay:** the 23-24 Oct timeline (sessions 570 s every 20 min, untrusted, a reset at the 10:40:55-equivalent): credited total 96-98 min for 97.21 real (not 205); the reboot straddler credits its post-boot part (about 236 s) and nothing more.
- **Trusted unchanged:** the full existing B2/B3 harness (`tests/occupancy_session_restart_test.sh`) and `occupancy_report_by_mode_test.cpp`, with only the expectations that encode the old untrusted merge changed (listed at Step 0 and checked, not regenerated blindly).
- Piece opened untrusted, trust returns mid-piece: credits on `millis()`, not the wall span.
- Boundary while untrusted-open: closed at the boundary, on-time credited, next piece starts clean.
- **Mutations (each must fail):** keep the B2 re-arm; credit wall span; credit from the persisted wall start; add a one-debounce fallback at boot; start a trusted piece on `millis()`; drop the report call; reuse the old piece's start for the next piece.
- `nm`: the untrusted credit path linked with production callers.

## Acceptance and bench (Codex xhigh; the Step 0 report §8)
Dev-14 (exerciser: 570 s session every 20 min, no cloud, RTC-seeded `Time` with no confirmed sync this boot), a reset at the 10:40:55-equivalent mid-session, cloud and trust restored after the close boundary. Capture `Occ`, `Sleep`, `WakeReturn`, `ClockResync`, the close report and `dailyoccupancy`. Pass: control 96-98 credited min for 97.21 real, each clean piece 570 s ± wake overhead; fault pass closes in every 630 s gap, never one 20-minute session, reports 96-98 min (not 205); the reset straddler credits about 236 s (proving post-boot only; about 536 s would mean a fallback crept in).

## Rollback
Revert the PR; v48/C1 behavior returns; no data to migrate (nothing persisted changes).

## Open questions for the architect
1. A piece opened untrusted that is still open when trust returns stays on `millis()` (point 3). Confirm, or prefer closing it at the first trusted instant and starting a trusted piece.
2. Does every untrusted piece close raise an occupancy-change report (more reports on a pulsed site while untrusted), or only the first and the last? Recommendation: every close, as a trusted close does, so counts stay consistent.
3. Do you want a counter or log field for "straddlers dropped" (one per reboot while untrusted) so the loss is visible in the field? It costs lines; recommendation: the log line only.
4. Day assignment for a piece that spans local midnight while untrusted: the existing boundary path closes it (point 5). Confirm no extra rule.

## Amendment 1 (architect rulings, 3 Nov) — status: APPROVED; dispatch after C1 passes Stage 7
1. **A piece opened untrusted stays on `millis()` until it closes,** even if trust returns before it closes (change-list point 3 stands).
2. **An untrusted close behaves exactly like a trusted close, including the occupancy-change report** (point 6 stands: the same report call, every close).
3. **No counter.** One `Log.info` line when a straddler's pre-boot part is dropped (inside the ≤ +16 budget); the per-credit log line of point 6 stays only if the budget allows, otherwise the straddler line wins.
4. **No extra midnight rule:** credit goes to the **day current at close**. No daily boundary is due while the clock is untrusted, so the boundary path never sees an untrusted-open piece (point 5's boundary sentence is replaced by this rule; Step 0 confirms that `DailyBoundary::check()` is trust-gated).
5. **Sequence:** C2 builds on C1's predicate (`debounceRemainingMs()`). **Dispatch C2 only after C1 (WO-2026-11-03-004) has passed Stage 7** and its branch is merged into this one. C1 may merge when verified; neither releases before v50.
6. Agents and budget unchanged: Copilot standard/medium; Codex `gpt-5.6-sol` xhigh, §4 SHA rule, at most two rounds; `src/` ≤ +16. No PR for the draft; the WO goes with its implementation PR.
```

</details>


---

<sub>13s</sub>

### `view`

**view the file at /private/tmp/gcc-step0/c2/docs/work-orders/WO-2026-11-03-001-step0-report.md.**

/private/tmp/gcc-step0/c2/docs/work-orders/WO-2026-11-03-001-step0-report.md

<details>
<summary>57 lines</summary>

```
# WO-2026-11-03-001 Step 0 — untrusted-clock occupancy merge

Model actually used: Codex `gpt-5.6-sol`; reasoning: high. Base: `21804e358c1e4cf12c35b49ab432c9d7daeb8122`, branch `wo/2026-11-03-001-item19-step0`.

Decision: **STOP for architecture revision.** A no-layout/no-retained `millis()` design cannot both reject unvouched reboot time and guarantee that no real part of a reboot-straddling session is discarded.

Tests: not run; read-only source/history investigation. Interpreter line: n/a. Stage 4 requires concerns, missing evidence, tests, and a proceed/revise recommendation (`AI_DEVELOPMENT_WORKFLOW.md:335-348`).

## 1. Current behavior

**OBSERVATION:** Session start is `Time.now()` in awake and PIR-wake starts (`src/state/State_Modes.cpp:68-72,231-248`). It is an epoch in `/usr/current.dat`, not retained RAM (`src/MyPersistentData.cpp:613,625,631-635`; `src/MyPersistentData.h:650-678`). Last PIR is persisted `millis()` (`State_Modes.cpp:107-108,245-248,267-276`) and cleared on every boot (`src/Generalized-Core-Counter.cpp:497-505`).

**OBSERVATION:** B2 returns on untrusted time, credits zero, preserves occupied/start, and writes a new `millis()` last-event (`src/state/State_Common.h:338-354`). B3 caps the first trusted cross-boot close at `min(closeAt, bootEpoch, bootAnchor+debounce)` (`:363-370`); setup captures `bootAnchor=max(start,lastReport)` in RAM only (`Generalized-Core-Counter.cpp:503-505`; `StateMachine.h:43-70`). Credit is wall-clock `closeAt-effectiveStart` (`State_Common.h:380-405`).

**OBSERVATION:** A due boundary closes at its epoch, publishes, cleans totals, and only a 0/24 site reopens at the boundary (`src/state/State_Report.cpp:37-83,306-327`). No boundary is due while untrusted (`src/time/DailyBoundary.cpp:26-45`). Reset/hibernate/power-cycle reload `/usr/current.dat`, recapture B3, and clear last-PIR. Writes are deferred 250 ms and serviced by the loop (`MyPersistentData.cpp:631-639`; `Generalized-Core-Counter.cpp:1168-1171`; `lib/StorageHelperRK/src/StorageHelperRK.cpp:25-40`).

**INFERENCE:** Once flushed, start/occupied survive reset, hibernate, and full power loss because they are filesystem data. An abrupt reset inside the deferred-save window can retain the preceding file image. HIBERNATE is another boot for these fields.

## 2. Changes since 23 October

**OBSERVATION:** #151/item 18 now lets Unknown reach the failsafe, but stage 3 remains six hours plus jitter after stage 2 and was projected about 14 h 20 min later (`src/power/ConnectivityFailsafe.cpp:219-314`; `docs/RECOVERY_PLAN_2026-09-26.md:263-266`). Item 20 only shortens occupied sleep to the remaining RTC deadline (`src/state/State_Sleep.cpp:607-620`). WO-2026-10-29-002 ignores occupancy only for known Closed; Unknown is unchanged (`src/state/State_Common.h:359-362`; `State_Modes.cpp:63-72,207-213`; `docs/work-orders/WO-2026-10-29-002-closing-record.md:4-6`).

**OBSERVATION:** Paper replay: 10:35:21Z–14:00:00Z is 12,279 s = 204.65 min. At 47.5% duty, real occupancy is 5,832.5 s = 97.21 min. The report rose 345→550 = 205 min, about 107.79 min excess (`docs/RECOVERY_PLAN_2026-09-26.md:260,269-272`). Boundary 14:00 preceded both the 00:53 next-day boot and its anchor cap, so `min(...)` selected the boundary.

**INFERENCE:** These changes shorten some outages, remove post-close counting only after trust returns, and improve wake timing, but none changes B2. The same merge and first-trusted-close over-credit remain.

## 3. What `millis()` can vouch for

**OBSERVATION:** Unsigned elapsed `millis()` is the project’s monotonic idiom (`src/time/ClockTrust.h:56-65`). Device OS adds STOP/ULP slept time back into `millis()` (`docs/work-orders/WO-2026-10-04-001-pre-step6-fixes.md:68-71`). Every boot clears the previous boot’s last-event (`Generalized-Core-Counter.cpp:497-505`).

**INFERENCE:** It vouches only for elapsed powered time in one boot. For a reboot-straddler, a new RAM start can credit only post-boot elapsed; adding one debounce is a bounded estimate, while crediting the pre-boot portion requires wall/RTC evidence. “Nothing,” “post-boot only,” and “post-boot plus ≤debounce” are policies, not facts recoverable from `millis()`.

## 4. Debounce on `millis()` while untrusted

**OBSERVATION:** Awake expiry already uses `millis()-lastEvent` (`src/state/State_Idle.cpp:48-61`; `State_Modes.cpp:128-148`). Sleep planning and wake-close instead use retained wall-clock `ledOffTime` (`src/device_pinout.cpp:94-138`; `State_Sleep.cpp:607-620`; `State_Modes.cpp:166-185`). ULP already has PIR GPIO plus timer wake (`State_Sleep.cpp:760-780`).

**INFERENCE:** Within a boot, calculate remaining time as `debounceMs-(millis()-lastEvent)`, retain `min(remaining+margin, report wake)`, and make the wake path use the same millis-expiry predicate before processing PIR. Wake sources do not change. Item 20’s RTC-remaining branch should be replaced/reused, not retained as a second deadline. The report path changes only if an untrusted close may add credit and trigger the existing occupancy-change report.

## 5. B2 history

**OBSERVATION:** Before B2, an untrusted close cleared `occupied` and start, losing the real session and emitting a false unoccupied transition. B2 preserved it and retried after one debounce (`docs/work-orders/WO-2026-10-04-001-pre-step6-fixes.md:58-81`; `tests/occupancy_session_restart_test.sh:11-25,381-455`). The accepted history disclosed 36,000 s excess in a ten-hour same-boot trust lapse (`docs/work-orders/WO-2026-10-04-001-stage7-verdict.md:59`).

**INFERENCE:** A millis close preserves B2’s guarantee only for a session wholly observed in that boot. Without cross-reset evidence it can preserve the open flag but cannot guarantee all real pre-reset duration.

## 6. Candidate designs

**OBSERVATION:** Existing coverage includes the real-helper B2/B3 harness (`tests/occupancy_session_restart_test.sh:27-30,294-542`), mode/report harness (`tests/occupancy_report_by_mode_test.cpp:304-451`), sleep-plan equivalence (`tests/sleep_plan_equivalence_test.sh:18-20,194-231`), and closed-boundary structural test (`tests/occupancy_ends_at_close_structural_test.py:15-53`). All candidates can use RAM-only `SessionState`; none requires a persisted/retained layout change.

**INFERENCE — A, measured powered-on time only:** Keep B2 logically open; at first trusted close credit only elapsed since a per-boot monotonic start. Dev-09’s last boot, 00:53:00→01:00:07, gives about 427 s = 7.12 min, losing about 90.1 real min; same-boot gaps still merge. Trusted clean days stay on the current wall path. Worst case loses every pre-boot real minute and overcredits gaps within the final boot. Estimate +12…18 `src`; change B2/B3 expectations and add reset/report cases.

**INFERENCE — B, cap by current-boot millis plus debounce:** Preserve wall credit but cap recovery at latest-boot elapsed plus one debounce. Dev-09 caps near 7.12+5 = 12.12 min, losing about 85.1 real min. Trusted clean behavior remains unchanged. Worst case loses earlier-boot occupancy beyond one debounce and includes final-boot gaps; excess beyond vouched uptime is bounded by one debounce. Estimate +18…24; if it reaches 20, split RAM clock wiring (≤10) from cap/reset behavior (≤14).

**INFERENCE — C, split every untrusted expiry:** At each millis deadline, add `deadline-startMs`, close, and let the next PIR open a new piece. Complete Dev-09 pieces total about 97.21 min. The 10:35:21→10:40:55 reset fragment is 334 s: post-boot-only loses 334 s, yielding ≈91.64 min; a one-debounce fallback recovers 300 s, yielding ≈96.64 min, but still discards 34 s and is not vouched. Trusted clean behavior stays unchanged. Complete same-boot pieces have only processing/rounding error; the reset piece and multi-day assignment remain ambiguous. Estimate +22…32: shared monotonic start/expiry and sleep plan (≤16), then untrusted split/credit/reporting (≤16).

## 7. Items 21, 10, and 15

**OBSERVATION:** Item 21’s sequence is after #151 and item 19 (`docs/RECOVERY_PLAN_2026-09-26.md:288-293`). Item 10 is already v48 and intentionally leaves Unknown unchanged (`WO-2026-10-29-002-closing-record.md:1-6`). Item 15 documents unresolved reset losses of roughly 2–3 h (`RECOVERY_PLAN_2026-09-26.md:244-247`).

**INFERENCE:** Item 21 must not land first: it reduces frequency but cannot define behavior when trust is genuinely withdrawn. Item 10 is the boundary constraint already in force. Item 15 must land before or with any no-discard item-19 architecture; otherwise the reboot case defeats acceptance. If bounded loss is accepted, item 19 can retain B3 and item 15 can remain separate.

## 8. Bench plan

**OBSERVATION:** Use no cloud, valid RTC-seeded `Time` but no confirmed sync this boot, the 570 s/1,200 s exerciser, and a reset at the 10:40:55-equivalent during the session opened at 10:35:21. Restore cloud/trust after the close boundary. Capture `Occ`, `Sleep`, `WakeReturn`, `ClockResync`, the close report, and `dailyoccupancy`.

**INFERENCE:** Control pass is 96–98 credited min for 97.21 real min, with each clean session 570 s ± wake overhead. Fault pass must close during every 630 s gap, never show one session spanning 20 min, and report 96–98 min rather than 205. Grade the reset-straddler separately: 570±few seconds is the no-discard requirement; about 236 s proves post-boot-only; about 536 s proves the one-debounce fallback and its 34 s loss.

## 9. Stop decision

**OBSERVATION:** `millis()` restarts at reboot, while the only cross-boot session start is an untrusted epoch and no persisted elapsed accumulator exists (`src/MyPersistentData.h:675-678`; `Generalized-Core-Counter.cpp:497-505`). Persisted-layout and retained changes are out of scope (`docs/work-orders/WO-2026-11-03-001-item19-untrusted-clock-session-merge-step0.md:37-41`).

**INFERENCE:** **STOP.** A/B discard large real spans; C fixes merging but can still discard the reset fragment or invent up to a debounce. Revise the architecture/acceptance to permit bounded loss, or authorize a cross-reset evidence mechanism with endurance and atomicity analysis. Conditional ranking: C > B > A. The smallest safe investigation is item 15 plus C1/C2, each ≤+20.

## 10. Source map

**OBSERVATION — start:** Wall `occupancyStartTime` is written at awake/PIR-wake starts (`State_Modes.cpp:68-72,231-248`); read for logs, close, boot anchor, and boundary carry (`:110-112`; `State_Common.h:355-388`; `Generalized-Core-Counter.cpp:503-504`; `State_Report.cpp:54-81`); clamped by validation (`MyPersistentData.cpp:682-717`); and cleared by close/daily reset (`State_Common.h:429-430`; `sensors/PIRSensor.cpp:14-22`). It is persisted through `MyPersistentData.h:650-678` and `MyPersistentData.cpp:878-882`; no retained copy exists.

**OBSERVATION — last PIR:** `lastOccupancyEvent` is persisted `millis()` (`MyPersistentData.h:675-677`), written on PIR, zero-sentinel re-arm, and B2 retry (`State_Modes.cpp:107-108,136-142,245-248,276`; `State_Idle.cpp:51-56`; `State_Common.h:350-352`), read by awake debounce (`State_Idle.cpp:51-58`; `State_Modes.cpp:136-146`), and reset at boot/daily reset (`Generalized-Core-Counter.cpp:497-505`; `PIRSensor.cpp:14-22`).

**OBSERVATION — deadline/end:** `ledOffTime` is a retained wall epoch, set/read/cleared by `signalLED*` (`device_pinout.cpp:94-143`); full power loss clears retained RAM (`src/time/RtcSkewTest.h:107-120`). Close time is not stored: default is wall `Time.now()` or the explicit boundary (`State_Common.h:338,355`; `State_Report.cpp:37-60`). Exact credit is computed at `State_Common.h:388` and added at `:397-405`.

**INFERENCE:** One session currently uses three clocks: persisted wall start, persisted-but-boot-cleared monotonic last-PIR, and retained wall deadline. There is no persisted end record.

## 11. Exact merge path

**OBSERVATION:** PIR opens with wall start and last-event millis (`State_Modes.cpp:68-108` or `:231-276`). Awake expiry occurs from millis (`:128-148`) or wake expiry from LED deadline (`:166-173`). With trust false, B2 overwrites last-event with current millis and returns still-open (`State_Common.h:345-353`); callers suppress unoccupied logging/report transition (`State_Idle.cpp:61-74`; `State_Modes.cpp:148-162,172-184`). Later PIR therefore extends the same occupied session. Once trust is true, boundary reporting closes using the old boundary (`State_Report.cpp:37-60`); line 388 computes `closeAt-effectiveStart`, and lines 397-405 add the whole span.

**INFERENCE:** Re-arming is the merge operation; wall subtraction is the gap-credit operation. Either alone is insufficient to explain the 205 min.

## 12. `millis()` across sleep/reset modes

**OBSERVATION:** STOP and ULP return in the same boot; Device OS restores slept time into `millis()` (`WO-2026-10-04-001-pre-step6-fixes.md:68-71`), and both share the stop-based timer implementation (`WO-2026-10-25-001-stage7-verdict.md:28-32`). Network standby is an option on that same ULP configuration (`State_Sleep.cpp:760-780`), so it does not change the MCU-clock result. GCC notes that hibernate resets uptime while intermittent sleep does not (`docs/FIELD_MEANINGS_REFERENCE.md:79-82`).

**OBSERVATION:** HIBERNATE is reset-class sleep (`State_Sleep.cpp:689-740`). `System.reset` paths include failsafe (`ConnectivityFailsafe.cpp:301-307`), sleep failure (`State_Sleep.cpp:892-913`), and watchdog reset. Successful deep power-down removes power and restarts; if the vendored call fails, it waits and calls `System.reset()` (`lib/AB1805_RK/src/AB1805_RK.cpp:575-590`).

**INFERENCE:** `millis()` remains continuous across STOP/ULP with or without network standby. It restarts at hibernate, planned/failsafe/watchdog reset, successful deep power-down, and cold power cycle. Current code then reloads wall `occupancyStartTime`, sets B3 from it/lastReport, and starts last-PIR millis anew.

## 13. Close and reset cases

**OBSERVATION:** Normal expiry calls the common close; boundary passes an explicit epoch; untrusted close stays open; daily cleanup publishes before zeroing (`State_Common.h:338-431`; `State_Report.cpp:54-83`). Reset between close and night sleep makes trust Unknown; v48’s Closed gate cannot exclude new detections (`WO-2026-10-29-002-closing-record.md:5-6`).

**INFERENCE:** Current behavior: normal/boundary closes credit wall duration, untrusted closes credit zero until later, a retained open session gets B3, and a lost open record gets nothing. A credits latest-boot elapsed; B credits at most latest-boot elapsed+debounce; C credits complete millis pieces and applies its chosen reset policy only to the in-flight piece. A reset after a completed close preserves the total once flushed. A post-close Unknown detection can start a new ambiguous session under every design unless item 19 defines day assignment.

## 14. Item 15 effect

**OBSERVATION:** If persisted occupied/start survive, current B3 can later credit through its cap. If they are lost, no close exists to add the episode, while already closed totals remain (`Generalized-Core-Counter.cpp:503-505`; `State_Common.h:363-405`; `RECOVERY_PLAN_2026-09-26.md:244-247`).

**INFERENCE:** A loses the entire pre-boot/in-flight portion even when open survives. B retains at most one debounce plus post-boot elapsed. C preserves flushed completed pieces but loses or estimates the in-flight piece. Losing the occupied record makes all three credit zero for that piece. Item 15 need not precede a bounded-loss fix, but must precede or ride with any claimed no-discard fix.

## 15. Fix budget

**OBSERVATION:** The hard limit is ≤+20 net `src/` lines per WO (`WO-2026-11-03-001-item19-untrusted-clock-session-merge-step0.md:25,28,32-34`). Tests are outside that source budget.

**INFERENCE:** A fits one WO (+12…18). B is borderline (+18…24) and should split RAM-clock wiring from cap/reset behavior if it reaches 20. C should not be compressed into one WO (+22…32): C1 shared monotonic start/expiry plus sleep plan (≤16), then C2 untrusted credit/split, reset policy, and reporting (≤16). A separate item-15 WO is required if acceptance remains “no real session discarded.”
```

</details>


---

<sub>16s</sub>

### `bash`

**cd /private/tmp/gcc-step0/c2 && sed -n 40,300p src/state/State_Modes.cpp**

$ cd /private/tmp/gcc-step0/c2 && sed -n 40,300p src/state/State_Modes.cpp

<details>
<summary>209 lines</summary>

```
// alone (Finding 3).
    CurrentReadings::set_lastCountTime(isClockTrusted() ? Time.now() : 0);

    // Log the new count once per event
    Log.info("Count detected - Hourly: %d, Daily: %d",
             CurrentReadings::get_hourlyCount(), CurrentReadings::get_dailyCount());

    // Flash the on-module BLUE LED for ~1 second as a
    // visual count indicator (works across sleep cycles)
    signalLED(true, 1000);  // Turn on with 1-second timeout

    // Stay in IDLE_STATE; hourly reporting will publish aggregated counts.
  }
}

/**
 * @brief Handle sensor events in OCCUPANCY mode
 *
 * @details In occupancy mode, first detection marks space as "occupied".
 *          Space remains occupied until debounce timeout expires without new detections.
 *          Tracks total occupied time for reporting.
 *          Used for: room occupancy, parking space detection, resource availability
 */
void handleOccupancyMode() {
  // Check if sensor has new data
  // loop() always runs first so the sensor flag is consumed; a detection after the close is ignored.
  if (SensorManager::instance().loop() && !occupancyIgnoredWhenClosed()) {
    // Sensor detected presence
    if (!CurrentReadings::get_occupied()) {
      // Transition from unoccupied to occupied
      CurrentReadings::set_occupied(true);
      CurrentReadings::set_occupancyStartTime(Time.now());
      
      // V3.23: Set LED with debounce timeout from sensor.setting1
      uint32_t setting1Raw = SystemConfig::SensorSettings::get_sensorSetting1();
      uint32_t debounceSeconds = setting1Raw / 1000;
      Log.info("OCCUPANCY detection: setting1=%lu ms => debounce=%lu sec",
               (unsigned long)setting1Raw, (unsigned long)debounceSeconds);
      if (debounceSeconds == 0) {
        Log.warn("Debounce timeout is 0 - using 60 sec default (check sensor.setting1=%lu)", 
                 (unsigned long)setting1Raw);
        debounceSeconds = 60;  // Minimum 60 second debounce for occupancy
      }
      signalLED(true, debounceSeconds * 1000UL);  // Turn on LED until debounce expires
      const bool reportNow = reportsOccupancyChangesNow();
      logOccupiedEvent("pir", debounceSeconds, reportNow);
      
      // In INTERMITTENT_KEEP_ALIVE and CONNECTED modes, report immediately on occupancy state changes
      // This allows dashboard to show real-time occupancy transitions
      if (reportNow) {
        session.occupancyChangeTriggered = true;
        if (state == IDLE_STATE) {
          transitionTo(REPORTING_STATE, "occupancy change");
        }
      }
    } else {
      // Already occupied - reset LED timeout on new motion
      uint32_t debounceSeconds = SystemConfig::SensorSettings::get_sensorSetting1() / 1000;
      if (debounceSeconds == 0) {
        debounceSeconds = 60;  // Minimum 60 second debounce for occupancy
      }
      signalLED(true, debounceSeconds * 1000UL);  // Reset LED timeout
      if (SystemConfig::get_verboseMode()) {
        logOccupiedEvent("pir", debounceSeconds, false, true);
      }
    }

    // Update last event time (resets debounce timer)
    CurrentReadings::set_lastOccupancyEvent(millis());

    if (SystemConfig::get_verboseMode()) {
      uint32_t occupiedDuration = Time.now() - CurrentReadings::get_occupancyStartTime();
      Log.info("Occupancy event - Duration: %lu seconds", occupiedDuration);
    }
  }

  // Check if we need to update occupancy state (timeout check)
  updateOccupancyState();
}

/**
 * @brief Update occupancy state based on debounce timeout
 *
 * @details If space is occupied and debounce timeout has expired without
 *          new sensor events, mark space as unoccupied.
 *          Accumulates total occupied time for daily reporting.
 *          Triggers immediate reporting on state change for DISCONNECTED_KEEP_ALIVE mode.
 */
void updateOccupancyState() {
  if (!CurrentReadings::get_occupied()) {
    return; // Nothing to do if not occupied
  }

  if (debounceRemainingMs() == 0) {
    const bool reportNow = reportsOccupancyChangesNow();
    const OccupancyCloseResult closeResult = closeOccupancySessionSafely("modes");
    if (closeResult.valid) {
      logUnoccupiedEvent("debounce", closeResult.sessionSeconds, closeResult.totalSeconds, reportNow);
    }

    signalLED(false);  // Turn off LED
    
    // In INTERMITTENT_KEEP_ALIVE and CONNECTED modes, report immediately on occupancy state changes
    // This allows dashboard to show real-time occupancy transitions
    if (reportNow && !closeResult.stillOpen) {
      session.occupancyChangeTriggered = true;
      if (state == IDLE_STATE) {
        transitionTo(REPORTING_STATE, "occupancy cleared");
      }
    }
  }
}

bool processSleepWakeEvent(bool pirWake) {
    // Check if LED timeout expired FIRST (before processing new PIR wake)
    // This ensures we don't immediately undo state changes from PIR processing
    if (SystemConfig::get_sensorMode() == SystemConfig::OCCUPANCY && signalLEDStatus() && debounceRemainingMs() == 0) {
      // Debounce period elapsed without motion (the millis predicate, not the LED's wall deadline)
      const bool reportNow = reportsOccupancyChangesNow();
      const OccupancyCloseResult closeResult = closeOccupancySessionSafely("sleep");
      signalLED(false);  // Turn off LED
      if (closeResult.valid) {
        logUnoccupiedEvent("debounce", closeResult.sessionSeconds, closeResult.totalSeconds, reportNow);
      }
      
      // Match the rest of the occupancy state machine: only KEEP_ALIVE and
      // CONNECTED force an immediate report/connect on occupancy transitions.
      if (reportNow && !closeResult.stillOpen) {
        session.occupancyChangeTriggered = true;
        transitionTo(REPORTING_STATE, "sleep-occupancy-debounce-report");
        return true;
      }
    }

    // Process PIR wake events (after checking LED timeout above)
    // If this wake was caused by the PIR interrupt, synthesize a single
    // detection event so that the motion that woke the device is counted
    // even if the ISR flag did not survive ULTRA_LOW_POWER sleep.
    if (pirWake) {
      if (SystemConfig::get_sensorMode() == SystemConfig::COUNTING) {
        CurrentReadings::set_hourlyCount(CurrentReadings::get_hourlyCount() + 1);
        CurrentReadings::set_dailyCount(CurrentReadings::get_dailyCount() + 1);
        // WO-2026-08-29-002 item 8: lastCountTime has no consumers anywhere
        // in this codebase (write-only telemetry field) - gating on
        // isClockTrusted() changes only the recorded value, never control
        // flow.
        CurrentReadings::set_lastCountTime(isClockTrusted() ? Time.now() : 0);
        signalLED(true, 1000);  // Flash for 1 second
        Log.info("Count detected from PIR wake - Hourly: %d, Daily: %d",
                 CurrentReadings::get_hourlyCount(), CurrentReadings::get_dailyCount());
        if (Serial && Serial.isConnected()) {
          Serial.printf("Count detected from PIR wake - Hourly: %d, Daily: %d\r\n",
                        CurrentReadings::get_hourlyCount(), CurrentReadings::get_dailyCount());
        }
      } else if (SystemConfig::get_sensorMode() == SystemConfig::OCCUPANCY) {
        // Occupancy mode: PIR wakes are expected behavior, not thrashing
        thrashGuard.markProgress("PIR_WAKE_OCCUPANCY");
        if (occupancyIgnoredWhenClosed()) {
          Log.info("OCCUPANCY pir-wake ignored: closed");
          return false;
        }

        // If KEEP_ALIVE was temporarily disabled for low battery, refresh the
        // battery policy on wake so recovered power can restore the intended
        // occupancy behavior before we decide whether to report or sleep again.
        // WO-2026-09-21 Step 4 (corrected same day): evaluate() then commit() -
        // one of the four deliberate policy-application sites.
        if (PowerConfig::get_lowBatteryMode()) {
          const float soc = PowerManager::instance().soc();
          float vcell = 0.0f;
          const SensorManager::VcellSampleState vcellState =
              SensorManager::instance().cachedBatteryVoltageState(vcell);
          const BatteryHealth::SocTrust trust = SensorManager::instance().cachedSocTrust();
          BatteryAuthority::commit(
              BatteryAuthority::evaluate(soc, vcellState, vcell, trust, BatteryAuthority::currentTier()),
              soc);
        }
        
        if (!CurrentReadings::get_occupied()) {
          CurrentReadings::set_occupied(true);
          // WO-2026-08-29-002 item 8: deliberately NOT gated on
          // isClockTrusted(). Unlike lastCountTime, occupancyStartTime's 0
          // value is a load-bearing sentinel for two consumers:
          // MyPersistentData.cpp's validate() treats occupied=true with
          // occupancyStartTime==0 as corrupt state and forces occupied=false
          // (a real state-machine side effect, not just a recorded value),
          // and State_Modes.cpp computes `Time.now() - occupancyStartTime`
          // for a logged duration, which would become a multi-decade garbage
          // span if this were 0 while occupied. Writing Time.now() here even
          // when untrusted preserves existing behavior; see the
          // Implementation Report for the Chief Engineer's review.
          CurrentReadings::set_occupancyStartTime(Time.now());
          // Treat the PIR wake as an occupancy event for debounce purposes.
          // If lastOccupancyEvent is left at 0, the debounce logic that uses
          // millis()-based timing will immediately expire and mark UNOCCUPIED.
          CurrentReadings::set_lastOccupancyEvent(millis());

          // Keep LED on for the debounce window (sensor.setting1 is milliseconds).
          uint32_t debounceMs = SystemConfig::SensorSettings::get_sensorSetting1();
          if (debounceMs == 0) {
            debounceMs = Config::occupancyDebounceMsForRuntime();
          }
          signalLED(true, debounceMs);
          const bool reportNow = reportsOccupancyChangesNow();
          logOccupiedEvent("pir-wake", debounceMs / 1000UL, reportNow);
          
          // Match the rest of the occupancy state machine: only KEEP_ALIVE and
          // CONNECTED force an immediate report/connect on occupancy transitions.
          if (reportNow) {
            session.occupancyChangeTriggered = true;
            setAppBreadcrumb(5);
            transitionTo(REPORTING_STATE, "sleep-pir-occupancy-report");
            return true;
          }
        } else {
          // Already occupied - motion detected during debounce, restart timer
          uint32_t debounceMs = SystemConfig::SensorSettings::get_sensorSetting1();
          if (debounceMs == 0) {
            debounceMs = Config::occupancyDebounceMsForRuntime();
          }
          signalLED(true, debounceMs);  // Restart LED timer
          logOccupiedEvent("pir-wake", debounceMs / 1000UL, false, true);
        }
        CurrentReadings::set_lastOccupancyEvent(millis());
      }
    }
  return false;
}
<shellId: 2 completed with exit code 0>
```

</details>


---

<sub>16s</sub>

### `bash`

**cd /private/tmp/gcc-step0/c2 && sed -n 300,450p src/state/State_Common.h; sed -n 485,515p src/Generalized-Core-Counter.cpp; sed -n 20,60p src/time/DailyBoundary.cpp**

$ cd /private/tmp/gcc-step0/c2 && sed -n 300,450p src/state/State_Common.h; sed -n 485,515p src/Generalized-Core-Counter.cpp; sed -n 20,60p src/time/DailyBoundary.cpp

<details>
<summary>193 lines</summary>

```
reason,
					 (unsigned long)sessionSeconds,
					 (unsigned long)totalSeconds,
					 report ? 1 : 0);
}

/**
 * @brief Result of safely closing an occupancy session.
 */
struct OccupancyCloseResult {
	bool valid = false;
	uint32_t sessionSeconds = 0;
	uint32_t totalSeconds = 0;
	bool stillOpen = false;
};

/**
 * @brief The occupancy debounce period in milliseconds.
 *
 * @details sensor.setting1 when configured, otherwise the runtime default.
 *          This is the single definition used by the debounce checks and by
 *          the cross-boot credit cap in closeOccupancySessionSafely().
 */
inline uint32_t occupancyDebounceMs() {
	const uint32_t configured = SystemConfig::SensorSettings::get_sensorSetting1();
	return configured != 0 ? configured : Config::occupancyDebounceMsForRuntime();
}

/**
 * @brief Milliseconds until the occupancy debounce expires (0 when expired).
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
	// WO-2026-09-19 Step 3b: Clock::isTrusted(), not isTimeValid() - the
	// duration arithmetic below (now - start) must not run on an RTC-seeded-
	// but-unconfirmed epoch, which can produce a wildly wrong session length.
	const bool timeValid = Clock::isTrusted();
	// WO-2026-10-04-001 item B2: an untrusted clock cannot credit a session.
	// Keep it open, credit nothing, and re-arm the debounce so the close is
	// retried one debounce later, instead of discarding the session and
	// signalling an unoccupied transition that never happened.
	if (!timeValid && occupied) {
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
		if (start < boundary) closeAt = std::min(closeAt, boundary);
	}
	// WO-2026-10-04-001 item B3: the first trusted close of a session that was
	// already open at boot credits no later than this boot started, and no
	// later than one debounce past the last evidence the session was open.
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
	bool invalid = !timeValid || start == 0;

	if (timeValid && start != 0) {
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

	if (timeValid && start > now + 5) {
		invalid = true;
	}

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
	CurrentReadings::set_occupancyStartTime(0);
	return result;
}
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
  session.occupancySessionBootAnchor = CurrentReadings::get_occupied()
      ? std::max(CurrentReadings::get_occupancyStartTime(), SystemConfig::get_lastReport()) : 0;
  CurrentReadings::set_lastOccupancyEvent(0);

  // WO-2026-08-25-001 Amendment C, Decision C1 boot-ordering corollary
  // (AC-C5): PowerManager::instance().setup() is deliberately NOT called
  // here anymore. It rebuilds the DCT power configuration via
  // refreshInputProfile() WITHOUT any thermal charge-inhibit information
  // (PowerPlatform.cpp:79-98 constructs it from input-profile detection
  // alone) - calling it this early left a real, measured worst-case window
  // of up to ~30.5 seconds (the configured USB serial wait,
  // ConnectivityPolicy::DEBUG_SERIAL_WAIT_TIMEOUT_MS = 30000ms, plus
  // DEBUG_SERIAL_POST_CONNECT_DELAY_MS = 500ms, both reachable in the field
    converter.atLocalTime(hms);
  }

  return converter.time;
}

Result check(time_t now) {
  bool due = false;
  time_t boundary = 0;
  uint8_t close = 0;

  // WO-2026-09-19 Step 3b: Clock::isTrusted(), not isTimeValid() - the day-
  // boundary logic must not run or stamp state on an untrusted clock, which
  // could permanently consume a missed boundary.
  if (Clock::isTrusted()) {
    close = SystemConfig::get_closeTime();
    boundary = todayAt(close);
    if (now < boundary) {
      boundary -= 86400;
    }

    const time_t lastDailyCleanup = SystemConfig::get_lastDailyCleanup();
    due = (lastDailyCleanup < boundary || lastDailyCleanup > now);
  }

  return {due, boundary, close};
}

} // namespace DailyBoundary
<shellId: 3 completed with exit code 0>
```

</details>


---

<sub>20s</sub>

### `bash`

**cd /private/tmp/gcc-step0/c2 && sed -n 35,80p src/state/StateMachine.h; sed -n 35,90p src/state/State_Idle.cpp; grep -n "closeOccupancySessionSafely\|occupancySessionBootAnchor\|debounceRemainingMs\|stillOpen" -r src | grep -v "^src/state/State_Common.h"; sed -n 495,525p src/state/State_Sleep.cpp; grep -n "signalLEDStatus" -A12 src/device_pinout.cpp | head -50; ls tests | head -80; git diff --stat HEAD~0; git log --stat -1 --format= 5d22b1b | tail -3**

$ cd /private/tmp/gcc-step0/c2 && sed -n 35,80p src/state/StateMachine.h; sed -n 35,90p src/state/State_Idle.cpp; grep -n "closeOccupancySessionSafely\|occupancySessionBootAnchor\|debounceRemainingMs\|stillOpen" -r src | grep -v "^src/state/State_Common.h"; sed -n 495,525p src/state/State_Sleep.cpp; grep -n "signalLEDStatus" -A12 src/device_pinout.cpp | head -50; ls tests | head -80; git diff --stat HEAD~0; git log --stat -1 --format= 5d22b1b | tail -3

<details>
<summary>220 lines</summary>

```
extern volatile bool userSwitchDetected;

/**
 * @brief ISR used for the user/service button input.
 */
void userSwitchISR();

/**
 * @brief RAM-only session flags that coordinate behavior across state handlers.
 *
 * These values are intentionally not persisted. They describe the current wake
 * cycle, in-flight webhook expectations, modem teardown timing, and temporary
 * suppression or diagnostic state.
 */
struct SessionState {
  bool firstConnectionObserved = false;
  bool firstConnectionQueueDrainedLogged = false;
  bool suppressAlert40ThisSession = false;
  bool awaitingWebhookResponse = false;
  bool webhookExpectedOnConnect = false;   // We queued a webhook publish and should start a response window on next cloud connect
  unsigned long webhookAwaitStartMs = 0;  // millis() when the short-term webhook response window started
  bool serviceRequestTriggered = false;   // User/service button requested immediate report/connect
  bool occupancyChangeTriggered = false;  // Report triggered by occupancy state change
  bool returnToSleepAfterReport = false;  // Should return to SLEEPING_STATE after connection
  bool connectAttemptActive = false;      // CONNECTING_STATE currently owns modem recovery
  unsigned long connectAttemptBudgetMs = 0;
  bool modemUnstable = false;             // RAM-only modem instability flag for temporary containment
  bool modemStandbySuppressed = false;    // Tracks whether standby suppression is active/logged
  uint8_t modemUnstableReason = 0;        // 0=none, 1=slow_teardown, 2=connect_timeout
  uint8_t modemConnectTimeoutCount = 0;   // Consecutive connect timeout counter (RAM-only)
  unsigned long lastTeardownEndMs = 0;    // millis() when teardown last completed
  uint32_t lastCloudDisconnectElapsedMs = 0;
  uint32_t lastModemOffElapsedMs = 0;
  uint32_t lastTotalTeardownElapsedMs = 0;
  uint32_t failsafeDeferLogMask = 0;      // One-shot failsafe defer reasons logged this boot
  time_t occupancySessionBootAnchor = 0; // max(start, lastReport) captured at boot for a session open across the restart; 0 = none
};

/// Connection and service timing constants shared across the state machine.
extern const unsigned long stayAwakeLong;
extern const unsigned long resetWait;
extern const unsigned long maxOnlineWorkMs;
extern unsigned long connectedStartMs;
extern SessionState session;

/**
  setLoopStage(LOOP_STAGE_IDLE_PROCESSING);

  const bool enteredIdle = (state != oldState);
  if (enteredIdle) {
    publishStateTransition();
  }

  // Process LED timers for timed flashes (COUNTING mode)
  signalLEDUpdate();

  // Maintain LED state for OCCUPANCY mode and check debounce timeout
  if (SystemConfig::get_sensorMode() == SystemConfig::OCCUPANCY) {
    // Check if debounce timeout expired (no motion for debounce period)
    if (CurrentReadings::get_occupied()) {
      if (debounceRemainingMs() == 0) {
        // Debounce timeout expired - space is now unoccupied
        const bool reportNow = reportsOccupancyChangesNow();
        const OccupancyCloseResult closeResult = closeOccupancySessionSafely("idle");
        signalLED(false);  // Turn off LED
        if (closeResult.valid) {
          logUnoccupiedEvent("debounce", closeResult.sessionSeconds, closeResult.totalSeconds, reportNow);
        }
        
        // In INTERMITTENT_KEEP_ALIVE and CONNECTED modes, report immediately
        // on occupancy transitions. In other modes, occupancy transitions are
        // still tracked, but do not force an immediate report/connect.
        if (reportNow && !closeResult.stillOpen) {
          session.occupancyChangeTriggered = true;
          transitionTo(REPORTING_STATE, "occupancy transition");
          return;
        }
      }
    }
    
    // Maintain LED state based on occupancy
    if (CurrentReadings::get_occupied() && !signalLEDStatus()) {
      signalLED(true);   // Keep LED on while occupied
    } else if (!CurrentReadings::get_occupied() && signalLEDStatus()) {
      signalLED(false);  // Ensure LED off when unoccupied
    }
  }

  // A change latched while not in Idle (main-loop handler, or CONNECTED, which
  // does not sleep while open) goes out from the first Idle pass.
  if (session.occupancyChangeTriggered) {
    transitionTo(REPORTING_STATE, "occupancy change pending");
    return;
  }

  // If configuration changes (for example, device-settings ledger updates)
  // move the park from CLOSED->OPEN while the device is already awake,
  // ensure the sensor stack is enabled. Previously this only happened on
  // wake from sleep, which caused "awake but not counting".
  {
    static bool enableAttemptedThisOpenWindow = false;
    bool openNow = isWithinOpenHours();
src/state/StateMachine.h:70:  time_t occupancySessionBootAnchor = 0; // max(start, lastReport) captured at boot for a session open across the restart; 0 = none
src/state/State_Sleep.cpp:613:      const uint32_t remaining = debounceRemainingMs();  // 0 when expired
src/state/State_Idle.cpp:49:      if (debounceRemainingMs() == 0) {
src/state/State_Idle.cpp:52:        const OccupancyCloseResult closeResult = closeOccupancySessionSafely("idle");
src/state/State_Idle.cpp:61:        if (reportNow && !closeResult.stillOpen) {
src/state/State_Report.cpp:60:      closeOccupancySessionSafely("daily-cleanup", boundary);
src/state/State_Modes.cpp:133:  if (debounceRemainingMs() == 0) {
src/state/State_Modes.cpp:135:    const OccupancyCloseResult closeResult = closeOccupancySessionSafely("modes");
src/state/State_Modes.cpp:144:    if (reportNow && !closeResult.stillOpen) {
src/state/State_Modes.cpp:156:    if (SystemConfig::get_sensorMode() == SystemConfig::OCCUPANCY && signalLEDStatus() && debounceRemainingMs() == 0) {
src/state/State_Modes.cpp:159:      const OccupancyCloseResult closeResult = closeOccupancySessionSafely("sleep");
src/state/State_Modes.cpp:167:      if (reportNow && !closeResult.stillOpen) {
src/Generalized-Core-Counter.cpp:503:  session.occupancySessionBootAnchor = CurrentReadings::get_occupied()
  // verdict that commits the device to the night-sleep branch below. An
  // untrusted clock (Unknown) must NOT commit to an overnight hibernate
  // based on a wrong belief about what time it is - see Clock::openness()'s
  // doc comment. logTimeDiag()'s isOpen= stays sourced from the raw,
  // fail-open isWithinOpenHours() (not parkOpenness) so its openness= field
  // in the same log line can be compared against it - the two are expected
  // to diverge exactly when the clock is untrusted.
  int nightSleepSec = -1;
  const Clock::Openness parkOpenness = Clock::openness();
  logTimeDiag(isWithinOpenHours());
  if (parkOpenness == Clock::Openness::Closed) {
    // WO-2026-09-30-001: never commit to night sleep while the close is due.
    if (DailyBoundary::check(Time.now()).due) {
      transitionTo(REPORTING_STATE, "close due before night sleep");
      return true;
    }

    // Notify sensor layer we are entering full night sleep so sensors and
    // indicator LEDs can be powered down. During daytime naps we keep
    // interrupt-driven sensors (like PIR) powered so they can wake the
    // device from ULTRA_LOW_POWER sleep.
    SensorManager::instance().onEnterSleep();
    signalLED(false);

    // ********** Night sleep (outside opening hours) **********
    nightSleepSec = secondsUntilNextOpen();
    if (nightSleepSec <= 0) {
      nightSleepSec = Config::DEFAULT_REPORT_INTERVAL_SEC;
    }

    // Device OS maximum sleep duration is 546 minutes (~9.1 hours).
141:bool signalLEDStatus() {
142-    return digitalRead(BLUE_LED) == HIGH;
143-}
144-                
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
exerciser_baseline_test.py
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
 tests/sleep_wake_equivalence_test.sh               |    2 +-
 tests/sleep_wake_structural_test.py                |   24 +-
 18 files changed, 7058 insertions(+), 112 deletions(-)
<shellId: 4 completed with exit code 0>
```

</details>


---

<sub>23s</sub>

### `bash`

**cd /private/tmp/gcc-step0/c2 && sed -n 85,140p src/device_pinout.cpp; sed -n 595,640p src/state/State_Sleep.cpp; cat docs/work-orders/WO-2026-11-03-004-stage7-r1-verdict.md | grep -n -i "note\|LED" | head -30**

$ cd /private/tmp/gcc-step0/c2 && sed -n 85,140p src/device_pinout.cpp; sed -n 595,640p src/state/State_Sleep.cpp; cat docs/work-orders/WO-2026-11-03-004-stage7-r1-verdict.md | grep -n -i "note\|LED" | head -30

<details>
<summary>101 lines</summary>

```
pinMode(WAKEUP_PIN, INPUT_PULLUP);    // AB1805 FOUT/nIRQ (open-drain, active-LOW, needs pull-up)
    pinMode(BLUE_LED, OUTPUT);     // On-module status LED
    return true;
}


// ---------------------------------------------------------------------------
// Centralized LED management
// ---------------------------------------------------------------------------
// Use RTC seconds (not millis) so time advances during sleep
retained time_t ledOffTime = 0;  // RTC time when LED should turn off (0 = indefinite)

void signalLED(bool state, uint32_t durationMs) {
    digitalWrite(BLUE_LED, state ? HIGH : LOW);
    
    if (state && durationMs > 0) {
        // Record the deadline in retained RTC time so sleep/wake cycles do not
        // extend the requested on-time.
        if (Time.isValid()) {
            ledOffTime = Time.now() + (durationMs / 1000);
        } else {
            ledOffTime = 0;  // Fallback to indefinite if no valid time
        }
    } else if (state) {
        ledOffTime = 0;
    } else {
        ledOffTime = 0;
    }
}

void signalLEDUpdate() {
    // Only the main loop should clear timeout-driven LED state so the timeout
    // behavior stays deterministic and easy to reason about.
    if (ledOffTime > 0 && Time.isValid() && Time.now() >= ledOffTime) {
        digitalWrite(BLUE_LED, LOW);
        ledOffTime = 0;
    }
}

uint32_t signalLEDTimeRemaining() {
    if (ledOffTime == 0) {
        return 0;
    }
    
    if (!Time.isValid()) {
        return 0;
    }
    
    time_t now = Time.now();
    if (now >= ledOffTime) {
        return 0;
    }
    
    return (uint32_t)(ledOffTime - now);  // Return seconds remaining
}

    if (ledRemaining > 0) {
      Log.info("COUNTING: Deferring sleep - LED remaining=%lu sec", 
               ledRemaining);
      transitionTo(IDLE_STATE, "sleep-counting-led-defer");
      return true;
    }
    // Turn off LED before sleep (should already be off)
    if (signalLEDStatus()) {
      signalLED(false);
    }
  }

  // OCCUPANCY MODE: Calculate sleep duration based on debounce timer or scheduled wake
  if (SystemConfig::get_sensorMode() == SystemConfig::OCCUPANCY) {
    if (CurrentReadings::get_occupied()) {
      // Occupied - wake for debounce timeout check
      uint32_t setting1Raw = SystemConfig::SensorSettings::get_sensorSetting1();
      uint32_t debounceSeconds = (setting1Raw > 0) ? (setting1Raw / 1000) : 60;
      const uint32_t remaining = debounceRemainingMs();  // 0 when expired
      if (remaining > 0) {
        const int untilDeadline = (int)(remaining / 1000UL) + 1;  // wake just after expiry (1 s margin)
        if (untilDeadline < wakeInSeconds) wakeInSeconds = untilDeadline;
      } else if (debounceSeconds < (uint32_t)wakeInSeconds) {
        wakeInSeconds = (int)debounceSeconds;  // no deadline known: the full debounce
      }
      sleepReason = "debounce";
    } else {
      // Unoccupied - use scheduled wake time
      sleepReason = "scheduled";
    }
  }
  plan = {wakeInSeconds, sleepReason, overnightFallbackSleep};
  return false;
}

bool nightlyHeapGuard(bool overnightFallbackSleep) {
  // Before a long overnight
  // ULTRA_LOW_POWER fallback sleep, optionally reset first when heap has
  // drifted low. This gives the next day a clean start without affecting
  // open-hours behavior.
  if (overnightFallbackSleep) {
    const unsigned long freeHeap = (unsigned long)System.freeMemory();

    if (freeHeap <= ConnectivityPolicy::NIGHTLY_HEAP_RESET_THRESHOLD_BYTES) {
      Log.info("ModemTeardown: radioOn=%d point=heap-guard-reset", (int)Connectivity::isRadioPoweredOn());
      Log.warn("Nightly heap guard: freeHeap=%lu <= %lu before overnight ULTRA_LOW_POWER fallback - resetting for clean next-day start",
10:1. **Note — predicate verified.** `debounceRemainingMs()` depends only on `millis()`, `lastOccupancyEvent`, and the configured debounce; zero re-arms to the current boot, and `elapsed >= debounceMs` returns zero (`src/state/State_Common.h:335-343`). Unsigned subtraction correctly handles one `millis()` wrap, provided expiry is evaluated within the 49.7-day modulo interval. A genuinely future nonzero timestamp produces a huge modulo elapsed value and therefore immediate expiry; normal reboot flow prevents that by running persistence setup and then clearing the prior-boot value (`src/Generalized-Core-Counter.cpp:493-505`), after which the sentinel re-arms it. The earlier validation seed at `src/MyPersistentData.cpp:695-698` is therefore cleared before state handling. `updateOccupancyState()` now closes at exact equality (`src/state/State_Modes.cpp:133`), whereas v48 used `>` (`21804e3:src/state/State_Modes.cpp:146`); this intentional one-boundary-step change matches Idle and the binding predicate.
12:2. **Note — all required call sites use the predicate.** Awake Idle uses it at `src/state/State_Idle.cpp:45-53`; the centralized occupancy handler at `src/state/State_Modes.cpp:128-140`; wake-close at `src/state/State_Modes.cpp:153-160`; and the occupied sleep plan at `src/state/State_Sleep.cpp:607-619`. Wake-close is evaluated before the PIR block (`src/state/State_Modes.cpp:153-178`). Before expiry, an occupied PIR re-arms the LED and writes the new event time (`src/state/State_Modes.cpp:254-263`). After expiry, non-immediate-report modes close and then process the PIR; IKA/CONNECTED retain v48’s early report transition/return (`src/state/State_Modes.cpp:165-170`).
14:3. **Note — no remaining wall-clock debounce close criterion.** `ledOffTime` remains a retained wall deadline solely for LED management (`src/device_pinout.cpp:94-138`). Remaining `signalLEDTimeRemaining()` consumers are wake logging (`src/state/State_Sleep.cpp:34-40`) and COUNTING-mode defer/turn-off behavior (`src/state/State_Idle.cpp:241-247`, `src/state/State_Sleep.cpp:592-604,1057-1060`). None calls the occupancy close helper. The daily-boundary close remains wall-clock based, but it is a day-boundary rule, not a debounce criterion (`src/state/State_Report.cpp:37-61`).
16:4. **Note — item 20’s second deadline is removed.** The occupied plan now calculates `floor(remainingMs/1000)+1` and takes the nearer report wake (`src/state/State_Sleep.cpp:613-619`); it retains the specified full-debounce fallback when already expired. Paper replay: ten pulses beginning 12:52:30 finish at 12:57:00, so the 300-second expiry is 13:02:00. If report/connect returns to the plan at 13:00:06, the predicate returns 114,000 ms and schedules 115 seconds, waking about 13:02:01—not another 300 seconds. Trusted and untrusted plans use that same expiry because the predicate contains no trust query; the test crosses both trust states (`tests/sleep_plan_equivalence_test.cpp:215-239`).
18:5. **Note — trusted clean behavior is semantically preserved, but not bit-for-bit at timing edges.** The intended `millis()` rule can close one loop earlier at exact equality. Also, v48’s LED deadline truncated `Time.now()` and `durationMs/1000` to whole seconds (`src/device_pinout.cpp:101-105`), while the new predicate preserves milliseconds. With a whole-second debounce, a PIR at epoch phase `.900` could make v48 judge expiry up to 0.9 seconds earlier; arbitrary non-whole-second settings can approach two seconds. This is the intended consequence of replacing the coarse wall deadline, not an extra deadline or credit change. Untrusted B2 still keeps the session open, credits zero, and re-arms after an expiry attempt (`src/state/State_Common.h:364-371`), as required for C1.
20:6. **Note — LED-gate gap is unchanged from v48, not a C1 regression, but it is a C2 acceptance blocker.** The exact gap is: OCCUPANCY mode, session still occupied, physical LED low, and debounce expired. `signalLEDStatus()==false` prevents wake-close (`src/state/State_Modes.cpp:156`); a PIR wake then extends the existing session (`src/state/State_Modes.cpp:254-263`). V48 had the same LED-status gate (`21804e3:src/state/State_Modes.cpp:169`). The state is reachable when sleep preparation switches the LED off (`src/state/State_Sleep.cpp:505-517`) and after reset. It does not block this frozen C1 scope, but C2 cannot claim “split every untrusted expiry” while that condition remains unaccounted for.
22:7. **Note — credit, reports, boundaries, and layouts are untouched.** `closeOccupancySessionSafely()` is byte-identical to v48; credit remains at `src/state/State_Common.h:390-423`. The report/boundary file, persistence implementation/layout, retained LED owner, boot owner, and `SessionState` are blob-identical to `21804e3`; relevant layouts remain at `src/MyPersistentData.h:650-679`, `src/state/StateMachine.h:49-71`, and `src/device_pinout.cpp:95`. No persisted or retained field was added. Source delta is 24 additions/28 removals, net `-4`, within budget.
24:8. **Note — fixtures and tests verified independently.** S-4 is 98→121 scenarios. Exactly 13 pre-existing scenarios changed: occupied set1 `{0,999,1000,5000,3589000,3590000,3591000,3592000,7200000}`, closed `{0,5000,50000000}`, and unknown-untrusted. Each lost one `CALL signalLEDTimeRemaining`, gained one `CALL get_sensorSetting1`, and retained identical `OUT` lines. Ten old debounce cases became 33 millis/trust cases (`tests/fixtures/sleep_plan_transcript.txt:1-6`; scenario construction at `tests/sleep_plan_equivalence_test.cpp:215-239`). S-6 is byte-identical to main, hash `914e77f7653dbe87ee57ed2a425ef250faf433de`.
30:9. **Note — Stage 6’s “flake” was a runner bug.** Its command used `test && ((p++)) || failure` (`docs/work-orders/WO-2026-11-03-004-stage6-copilot-transcript.md:5857-5863`). In zsh, the first post-increment evaluates to zero and returns failure, falsely naming the first alphabetical test—`alert19_clears_after_report_test.sh`—even though its log passed. The committed tree contains 62 shell and 46 Python tests, not 109. The alert test passed 30/30 repeated runs, and the corrected full run passed **108/108**.
32:10. **Note — production linkage verified.** Fresh Device OS 6.6.0 build succeeded: text/data/bss `151692/1114/2200`. ELF `nm` shows `debounceRemainingMs()` at `0x000c245c`. Objdump shows direct calls from `updateOccupancyState()` at `0x000c2502` and `processSleepWakeEvent(bool)` at `0x000c26c0`. `handleIdleState()` inlines the sentinel/millis arithmetic and calls `occupancyDebounceMs()`; `handleSleepingState()` fully inlines the predicate and debounce lookup around `0x000c350e–0x000c353c`.
34:11. **Note — bench remains pending.** Nothing in the diff claims the :52 straddle test passed; the Stage 6 report explicitly marks it pending (`docs/work-orders/WO-2026-11-03-004-stage6-copilot-report.md:74-76`), matching Amendment 1 (`docs/work-orders/WO-2026-11-03-004-item19-c1-millis-expiry.md:47-51`). The remote PR body was not available under the no-network authorization, so its wording itself is not verified; before merge it must state that Chip’s Dev-09/Dev-14 straddle run is pending.
<shellId: 5 completed with exit code 0>
```

</details>


---

<sub>50s</sub>

### `bash`

**cd /private/tmp/gcc-step0/c2 && sed -n 255,275p docs/RECOVERY_PLAN_2026-09-26.md; grep -rn "236" docs/work-orders/WO-2026-11-03-00[146]*.md | head; grep -n "Clock::\|#include" src/state/State_Modes.cpp | head -30**

$ cd /private/tmp/gcc-step0/c2 && sed -n 255,275p docs/RECOVERY_PLAN_2026-09-26.md; grep -rn "236" docs/work-orders/WO-2026-11-03-00[146]*.md | head; grep -n "Clock::\|#include" src/state/State_Modes.cpp | head -30

<details>
<summary>39 lines</summary>

```
- **The trap:** a restart with no successful connect leaves no confirmed sync this boot, so `Clock::isTrusted()` is false and `Clock::openness()` returns Unknown (`Clock.cpp`, "no confirmed sync this boot"; trust also lapses 24 h after the last sync). Then: **the failsafe never acts**: `connectivityFailsafeSupervisor()` returns unless `Clock::openness() == Clock::Openness::Open` (`ConnectivityFailsafe.cpp:228`, no defer log in release builds), which recreates the circular case the comment at `:190–205` warns against (a wedged radio has, by definition, never completed a sync this boot, so gating the recovery on trust disables it exactly when it is needed); **no night sleep** (`sleepPlan` commits only when openness is Closed, `State_Sleep.cpp` ~505; the sensor and exerciser are never powered down, because `onEnterSleep()` runs only in that branch); **no close** (`DailyBoundary::check()` is never due on an untrusted clock); **sessions are never credited** (`closeOccupancySessionSafely` keeps an open session open while untrusted, `State_Common.h:341–349`).
   - **Evidence, Dev-09, 23 Oct** (S3 `particle-events/2026-10-23/serialLog/e00fce68399ee6244a963935/`, hooks, Particle API): last sync `ClockResync: sync advanced` 07:03:12Z; last hook 07:35:20Z; Singtel registration backoff ("stage 14, next attempt in 13800 s", 10:39:52Z, rising to stage 22 by 23:10Z); MCU restart at **10:40:55Z** (uptime restarts), **observed to be the failsafe's stage-2 reset**: that boot's startup status, held in the offline publish queue for 14 h and delivered at 00:56:42Z on 24 Oct, reads `resetReason 140`, `resetReasonData 2`, `appBreadcrumb 10`, `appBreadcrumbMs 28845416` (the uptime at the reset), `failsafeStage 2`, `failsafeCount 1`, `failsafeLastAction 1792752040` (= 23 Oct 10:40:40Z); then **ten hourly 660 s connect failures** from 11:04Z to 23:11Z, `Sleep->Report reason=sleep-pir-overdue-report` each hour, `occ=1 totalMin=345` unchanged from 11:04Z to 23:00Z, no `Daily boundary reached`, no night sleep, PIR wakes (the exerciser) continuing 12+ h after the 22:00 SGT close, API offline from 07:35:43Z. It was not recovered by Chip's first power-cycle (about 00:08Z on 24 Oct: no cloud contact for 45 min); a second reset at 00:53Z connected after 193 s (see the recovery evidence below). **Not directly confirmed:** zero `TimeDiag:` lines were captured in the day's serial (the forwarder drops lines at each sleep), so `trusted=0`/`openness=2` is inferred from the behavior and the code; the absence of a failsafe action is not independent evidence (stage 3 is due 6 h after the stage-2 reset, 16:41Z at the earliest, after the 14:00Z close).
   - **Precedent:** Dev-14, v42, 15 Oct (see "A modem-off sleep loop after a restart" and item 3): a stage-2 reset at 06:11:08Z, nine failed attempts to 11:14Z, recovered only by Chip's manual power cycle at about 11:33Z; stage 3 (the deep power-down) would have been due no earlier than 12:14Z. Same shape (reset, no connect), so the trap probably also cost that bench device its stage 3.
   - **Candidate fix (about one line):** treat Unknown as Open in that check: `if (Clock::openness() == Clock::Openness::Closed || now <= ageBase) return;` (`ConnectivityFailsafe.cpp:228`). A trusted clock that says Closed still skips the failsafe (the device is asleep at night); an untrusted clock no longer disables the escalation (stage 3, the AB1805 deep power-down, then restarts the modem too). Pin it with the supervisor's host test (two cases: Unknown acts; Closed does not).
   - **To check (fleet):** whether any fleet device has shown a long silence matching this. S3 scan, 20 Sep to 24 Oct, nine US devices: **no non-night hook gap of 4 h or more since 2 Oct** (the open-hours gate, WO-2026-10-02-001 item A, v34 on); the earlier gaps of 9 to 22 h (Court3, Court1, PCKL1, PCKL2, MAFC-1, MAFC-2, 25 Sep to 2 Oct) ended in failsafe resets (reset 140, failsafeStage 2, e.g. Court3 2 Oct 10:02Z data 2, MAFC-1 2 Oct 16:15Z data 2) on pre-gate builds, so they were not this trap. The last failsafe resets on the US fleet were 4 and 5 Oct (Court3, PCKL1); all recovered at once. So the trap is evidenced only on the two bench devices on Singtel.
   - **Recovery evidence, Dev-09, 24 Oct** (S3 `serialLog`, `Ubidots-Sensor-Hook-v1`, `status`, Particle API; 00:43 to 01:10Z): Chip reset the device at **00:53:00Z** (serial uptime restarts to 4,080 ms). **Connect:** `CloudRecover: stage=1 action=particle-reconnect` 00:56:09Z (cellular registration took 192,923 ms), `CloudRecover: success` 00:56:15Z, `ConnSummary: ok elapsed=198607 ... cloudMs=5684 sig=72/19 q=33` and `Failsafe: cleared reason=cloud-ok` 00:56:17Z, `Connect: ok` 00:56:23Z. **Clock sync:** `ClockResync: sync advanced, rtcUpdated=1 epoch=1792803374` at 00:56:25Z; the `device-status` ledger then reads `trusted: true`, `syncAgeSec: 3`, and the API `online: true`, `last_heard` 00:56:33Z. **The queue:** 33 events from the flash-persisted publish queue were delivered 00:56:18 to 00:57:03Z: 24 hourly and session reports (payload stamps 23 Oct 07:44Z to 24 Oct 00:00Z; from 10:40Z on every report reads `occ 1`, `dailyoccupancy 345`, alert 31, `cyc` 14 to 472, `resets` 1) and three statuses. **The missed 23 Oct close** was processed by the first Report after trust, the hourly report at **01:00:07Z** (`ConnSummary: ok elapsed=1461`): a close report stamped **23 Oct 13:59:59Z** (the boundary minus 1 s), `occ 0`, **`dailyoccupancy 550`**, `cyc 3`, `resets 1`, connecttime 198. **The overnight session was credited, with no boot cap:** 550 minus the 345 minutes already held is **205 minutes**, which is exactly the span from the session start (`Occ: state=1` at 10:35:21Z) to the boundary (13:59:59Z; 204.6 min); the B3 boot cap takes the earliest of the boundary, the boot time and the last report plus the debounce, and the boundary (14:00Z) was earliest, so it did not bind. **That credit is inflated:** the exerciser runs a 570 s session every 20 min (about 47.5 % duty), so the real occupied time in that 3 h 25 min window is about 97 minutes; the stuck-open session credited about 108 minutes too many. That belongs with item 10 (credit while awake across a boundary). **The cleanup ran:** the next report, Dev-09's first normal session, has its start report at **01:13:04Z** (`occ 1`, **`dailyoccupancy 0`**, `resets 0`, `cyc 4`; serial `Report: occ=1 totalMin=0`) and its end report at **01:22:34Z** (`occ 0`, `dailyoccupancy 9`, `cyc 14`; serial `totalMin=9`), a start-to-end of 570 s (S3 `Ubidots-Sensor-Hook-v1`, 24 Oct). So the missed close zeroed the daily total and the reset count, and the session after it is normal; from that start report on Dev-09 is the v45 control in the weekend runs.
   - **Other notes from the recovery:** the first power-cycle (00:08Z) did not connect for 45 min, so recovery is not deterministic; the registration may simply have recovered on its own (the modem's long backoff). The status that carried the 140/2 reset reached the cloud 14 h after the reset, so the field never saw the failsafe action while the device was dark.
   - **Weekend runs:** Dev-09's exerciser data are excluded from 23 Oct 10:40Z until its first normal session after the 00:53Z reset on 24 Oct (the first start report after the 01:00:08Z close report); from there Dev-09 is the v45 control beside Dev-14 (v46) in the Saturday and Sunday runs.
   - **The fix (WO-2026-10-24-001, PR #151, ships in v47 with #142):** `ConnectivityFailsafe.cpp:228` skips only a known Closed (Unknown counts as open), and the TEST_MODE prediction follows. Stage 3 stays 6 h of cooldown plus jitter after stage 2 and 3 h of age from the day's opening, so for Dev-09 it would have fired at about 09:00 SGT on 24 Oct, **about 14 h 20 min after the stage-2 reset** (Codex's derivation, Stage 7 check 3), instead of never.
   - **Open decision (architect, 24 Oct): whether to shorten the post-stage-2 cooldown** (and/or the opening-based age base) so that a device that is dark after its own stage-2 reset reaches stage 3 sooner than the next morning. Dev-09 would have waited about 14 h. The cooldown (6 h) and jitter were set to avoid fleet-wide simultaneous power-downs and to give stage 2 time to work; a shorter interval trades that against longer dark periods. Not part of WO-2026-10-24-001; needs its own Step 0 (history of the 6 h value in WO-2026-10-02-001, the fleet's stage-2 recovery rate within 1 h, which the "queued check" under item 3 would measure).
   - **Bench test (deferred to after 31 Oct):** a `CONNECTIVITY_FAILSAFE_TEST_MODE` build (stale threshold 5 min) on Dev-14, with the modem unable to register (hands needed: antenna or SIM out, or a carrier block), showing stage 2 and then, after the cooldown and jitter, stage 3 with the clock untrusted after the reset. Claude Code prepares the build and the expected log lines (`Failsafe: stage=2 action=system-reset`, `Failsafe: stage=3 action=deep-powerdown`, the `ConnectivityFailsafeTest` prediction lines).
   - **Stage 3 may not power the device down (architect, 1 Nov; AB1805 Step 0, `WO-2026-10-31-001-step0-report.md`):** the vendored `deepPowerDown()` sets PWGT with no configuration-key write (`lib/AB1805_RK/src/AB1805_RK.cpp:559-565`), while Device OS writes key 0xA1 right before PWGT (`hal/shared/am18x5.cpp:1050-1051`). The library's own comment says the key auto-clears after a write (`:597-601`). **Whether the chip ignores an unkeyed PWGT write is unproven (INF)**; if it does, stage 3 falls through to `System.reset()` and the device is not powered down. **The TEST_MODE bench above must watch the power rail, the modem and the off-time, not just the log line.**
   - **When:** first, ahead of the other items in this list: a Step 0 for the one-line change and its test, then a WO small enough to ship in v47 or sooner.

19. **An untrusted clock merges separate sessions, and the whole span is credited at once when trust returns** (architect, 2026-10-24; found on Dev-09, v45, with item 18). Priority: **after item 18**: item 18's fix shortens these episodes (the failsafe can act) but does not remove them.
   - **What happens:** while the clock is untrusted, `closeOccupancySessionSafely` keeps an open session open, credits nothing and re-arms the debounce (`State_Common.h:341–349`, item B2 of WO-2026-10-04-001). Each debounce expiry that should have closed a session is postponed, so the separate sessions of a pulsed or intermittent site merge into one open session. When trust returns, the first trusted close credits `close time - start time`: the whole span, **gaps included**, capped only by the boot rule (B3: the earliest of the boundary, the boot time and the last report plus the debounce).
   - **Evidence, Dev-09** (S3 `Ubidots-Sensor-Hook-v1`, `serialLog`, 23 to 24 Oct; see item 18): the session opened at 10:35:21Z (`Occ: state=1 reason=pir`) and stayed open through the 10:40:55Z restart and until the first trusted Report at 01:00:07Z on 24 Oct, which closed it at the boundary. The credit was **205 minutes** (`dailyoccupancy` 345 before, **550** in the close report stamped 23 Oct 13:59:59Z, the 24 Oct 01:00:08Z delivery), the span 10:35:21Z to 14:00Z (204.6 min). The exerciser's pattern (a 570 s session every 20 min, about 47.5 % duty) puts the real occupied time in that window at about **97 minutes**, so the credit is about **+108 minutes** too high. **Which day received it:** the credit went into the **23 Oct total** (the close report for the 23 Oct day, reported as 550 minutes); the 24 Oct day started clean (`dailyoccupancy 0` at the 01:13:04Z start report), so only 23 Oct's total is wrong. It is a fleet-visible number: the daily occupied minutes are the product.
   - **Why the boot cap did not help:** the cap takes the earliest of the boundary, the boot time and the last report plus the debounce; the boundary (14:00Z) was earlier than the 00:53Z boot and the last report, so it did not bind. A recovery after the day's close always credits up to the boundary.
   - **Candidate direction (a Step 0 is needed first):** measure a session's duration with `millis()` within a boot (monotonic, independent of the clock's trust) and use the clock only to assign the day. A session open across a restart or a stretch with no confirmed sync would then be credited its measured on-time (or only what `millis()` can vouch for), not the wall-clock span. Questions for Step 0: what `millis()` can and cannot vouch for across a restart (it resets; the retained occupancy start time is wall-clock); how to credit a session that straddles a reboot; whether the debounce expiry itself can run on `millis()` so sessions are separated even while untrusted; the effect on the B2/B3 tests and on the report path; the history of B2 (WO-2026-10-04-001), which chose "keep it open" to avoid discarding a real session.
   - **Related:** item 10 (occupancy credited after the close while awake counts into the next day) and item 15 (occupancy lost across a reset).
   - **When:** after item 18; Step 0 first (history, then a design that cannot lose a real session).
docs/work-orders/WO-2026-11-03-001-step0-report.md:65:**INFERENCE:** Control pass is 96–98 credited min for 97.21 real min, with each clean session 570 s ± wake overhead. Fault pass must close during every 630 s gap, never show one session spanning 20 min, and report 96–98 min rather than 205. Grade the reset-straddler separately: 570±few seconds is the no-discard requirement; about 236 s proves post-boot-only; about 536 s proves the one-debounce fallback and its 34 s loss.
docs/work-orders/WO-2026-11-03-006-item19-c2-untrusted-split.md:28:- **Dev-09 replay:** the 23-24 Oct timeline (sessions 570 s every 20 min, untrusted, a reset at the 10:40:55-equivalent): credited total 96-98 min for 97.21 real (not 205); the reboot straddler credits its post-boot part (about 236 s) and nothing more.
docs/work-orders/WO-2026-11-03-006-item19-c2-untrusted-split.md:36:Dev-14 (exerciser: 570 s session every 20 min, no cloud, RTC-seeded `Time` with no confirmed sync this boot), a reset at the 10:40:55-equivalent mid-session, cloud and trust restored after the close boundary. Capture `Occ`, `Sleep`, `WakeReturn`, `ClockResync`, the close report and `dailyoccupancy`. Pass: control 96-98 credited min for 97.21 real, each clean piece 570 s ± wake overhead; fault pass closes in every 630 s gap, never one 20-minute session, reports 96-98 min (not 205); the reset straddler credits about 236 s (proving post-boot only; about 536 s would mean a fallback crept in).
docs/work-orders/WO-2026-11-03-006-stage6-copilot-dispatch.md:6:Do Step 0 first (re-read every cited line on this base, list the B2/B3 and report-by-mode expectations that must change, confirm `DailyBoundary::check()` is trust-gated, confirm the boot path for a persisted open session while untrusted; stop and report if the design cannot be done in +16). Then implement, test (host suite via zsh for `.sh`, python3 for `.py`: `N/N (sh via zsh, py via python3)`) including the Dev-09 replay (96-98 min credited for 97.21 real; the reboot straddler credits its post-boot part only, about 236 s), the mutations in the WO, the local toolchain build (Device OS 6.6.0, fresh `BUILD_PATH_BASE`) and `nm` linkage of the untrusted credit path. Write `docs/work-orders/WO-2026-11-03-006-stage6-copilot-report.md`. The Dev-14 exerciser bench is Chip's later step: do not claim it.
1:#include "state/State_Common.h"
2:#include "../Config.h"
3:#include "cloud/Cloud.h"
4:#include "LocalTimeRK.h"
5:#include "persist/CurrentReadings.h"
6:#include "persist/PowerConfig.h"
7:#include "persist/SystemConfig.h"
8:#include "power/BatteryAuthority.h"
9:#include "power/PowerManager.h"
10:#include "PublishQueuePosixRK.h"
11:#include "sensors/SensorManager.h"
12:#include "ThrashGuard.h"
13:#include "device_pinout.h"
14:#include "sensors/SensorDefinitions.h"
<shellId: 6 completed with exit code 0>
```

</details>


---

<sub>1m 35s</sub>

### `bash`

**cd /private/tmp/gcc-step0/c2 && python3 - <<'EOF'
import re
p='src/state/State_Common.h'
s=open(p).read()
old=s[s.index("\t// WO-2026-10-04-001 item B2: an untrusted clock cannot credit"):s.index("\tconst time_t now = Time.now();\n\tconst time_t start")]
new='''\t// WO-2026-11-03-006: a piece opened untrusted is measured on millis() for its
\t// whole life (no wall clock, even if trust returns): credit its on-time up to
\t// the debounce expiry, then close it so the next PIR opens a new piece.
\tconst bool piece = occupied && session.pieceUntrusted;
\t// WO-2026-10-04-001 item B2 (a trusted piece that loses trust): keep it open,
\t// credit nothing and re-arm the debounce.
\tif (!timeValid && occupied && !piece) {
\t\tCurrentReadings::set_lastOccupancyEvent(millis());
\t\tresult.stillOpen = true;
\t\treturn result;
\t}
'''
s=s.replace(old,new)
s=s.replace("\tbool invalid = !timeValid || start == 0;\n\n\tif (timeValid && start != 0) {",
"""\tbool invalid = (!timeValid && !piece) || start == 0;

\tif (piece) {
\t\trawSessionSeconds = (CurrentReadings::get_lastOccupancyEvent() - session.occupancyStartMs + occupancyDebounceMs()) / 1000UL;
\t\tdurationComputable = true;
\t\tLog.info("Occ: untrusted piece credit=%lld start=%lu", rawSessionSeconds, (unsigned long)session.occupancyStartMs);
\t} else if (timeValid && start != 0) {""")
s=s.replace("\tCurrentReadings::set_occupied(false);\n\tCurrentReadings::set_occupancyStartTime(0);\n\treturn result;","\tCurrentReadings::set_occupied(false);\n\tsession.pieceUntrusted = false;\n\tCurrentReadings::set_occupancyStartTime(0);\n\treturn result;")
open(p,'w').write(s)

p='src/state/StateMachine.h'
s=open(p).read()
s=s.replace("  time_t occupancySessionBootAnchor = 0;","  uint32_t occupancyStartMs = 0;         // millis() when the current piece opened (0 = boot for a straddler)\n  bool pieceUntrusted = false;           // piece opened with an untrusted clock: credited on millis() until it closes\n  time_t occupancySessionBootAnchor = 0;",1)
open(p,'w').write(s)

p='src/state/State_Modes.cpp'
s=open(p).read()
s=s.replace("      CurrentReadings::set_occupancyStartTime(Time.now());\n      \n","      CurrentReadings::set_occupancyStartTime(Time.now());\n      session.occupancyStartMs = millis();\n      session.pieceUntrusted = !Clock::isTrusted();\n      \n",1)
s=s.replace("          CurrentReadings::set_occupancyStartTime(Time.now());\n          // Treat","          CurrentReadings::set_occupancyStartTime(Time.now());\n          session.occupancyStartMs = millis();\n          session.pieceUntrusted = !Clock::isTrusted();\n          // Treat",1)
s=s.replace("OCCUPANCY && signalLEDStatus() && debounceRemainingMs() == 0) {","OCCUPANCY &&\n        (signalLEDStatus() || (session.pieceUntrusted && CurrentReadings::get_occupied())) && debounceRemainingMs() == 0) {",1)
open(p,'w').write(s)

p='src/Generalized-Core-Counter.cpp'
s=open(p).read()
s=s.replace("  session.occupancySessionBootAnchor = CurrentReadings::get_occupied()\n","  session.pieceUntrusted = CurrentReadings::get_occupied() && !Clock::isTrusted();  // straddler: millis() start is boot (0)\n  if (session.pieceUntrusted) Log.info(\"Occ: untrusted straddler, pre-boot part dropped\");\n  session.occupancySessionBootAnchor = (CurrentReadings::get_occupied() && !session.pieceUntrusted)\n",1)
open(p,'w').write(s)
EOF
git diff --numstat -- src; git diff -- src | grep -c . ; grep -n "Clock.h\|include" src/Generalized-Core-Counter.cpp | grep -i clock**

$ cd /private/tmp/gcc-step0/c2 && python3 - <<'EOF'
import re
p='src/state/State_Common.h'
s=open(p).read()
old=s[s.index("\t// WO-2026-10-04-001 item B2: an untrusted clock cannot credit"):s.index("\tconst time_t now = Time.now();\n\tconst time_t start")]
new='''\t// WO-2026-11-03-006: a piece opened untrusted is measured on millis() for its
\t// whole life (no wall clock, even if trust returns): credit its on-time up to
\t// the debounce expiry, then close it so the next PIR opens a new piece.
\tconst bool piece = occupied && session.pieceUntrusted;
\t// WO-2026-10-04-001 item B2 (a trusted piece that loses trust): keep it open,
\t// credit nothing and re-arm the debounce.
\tif (!timeValid && occupied && !piece) {
\t\tCurrentReadings::set_lastOccupancyEvent(millis());
\t\tresult.stillOpen = true;
\t\treturn result;
\t}
'''
s=s.replace(old,new)
s=s.replace("\tbool invalid = !timeValid || start == 0;\n\n\tif (timeValid && start != 0) {",
"""\tbool invalid = (!timeValid && !piece) || start == 0;

\tif (piece) {
\t\trawSessionSeconds = (CurrentReadings::get_lastOccupancyEvent() - session.occupancyStartMs + occupancyDebounceMs()) / 1000UL;
\t\tdurationComputable = true;
\t\tLog.info("Occ: untrusted piece credit=%lld start=%lu", rawSessionSeconds, (unsigned long)session.occupancyStartMs);
\t} else if (timeValid && start != 0) {""")
s=s.replace("\tCurrentReadings::set_occupied(false);\n\tCurrentReadings::set_occupancyStartTime(0);\n\treturn result;","\tCurrentReadings::set_occupied(false);\n\tsession.pieceUntrusted = false;\n\tCurrentReadings::set_occupancyStartTime(0);\n\treturn result;")
open(p,'w').write(s)

p='src/state/StateMachine.h'
s=open(p).read()
s=s.replace("  time_t occupancySessionBootAnchor = 0;","  uint32_t occupancyStartMs = 0;         // millis() when the current piece opened (0 = boot for a straddler)\n  bool pieceUntrusted = false;           // piece opened with an untrusted clock: credited on millis() until it closes\n  time_t occupancySessionBootAnchor = 0;",1)
open(p,'w').write(s)

p='src/state/State_Modes.cpp'
s=open(p).read()
s=s.replace("      CurrentReadings::set_occupancyStartTime(Time.now());\n      \n","      CurrentReadings::set_occupancyStartTime(Time.now());\n      session.occupancyStartMs = millis();\n      session.pieceUntrusted = !Clock::isTrusted();\n      \n",1)
s=s.replace("          CurrentReadings::set_occupancyStartTime(Time.now());\n          // Treat","          CurrentReadings::set_occupancyStartTime(Time.now());\n          session.occupancyStartMs = millis();\n          session.pieceUntrusted = !Clock::isTrusted();\n          // Treat",1)
s=s.replace("OCCUPANCY && signalLEDStatus() && debounceRemainingMs() == 0) {","OCCUPANCY &&\n        (signalLEDStatus() || (session.pieceUntrusted && CurrentReadings::get_occupied())) && debounceRemainingMs() == 0) {",1)
open(p,'w').write(s)

p='src/Generalized-Core-Counter.cpp'
s=open(p).read()
s=s.replace("  session.occupancySessionBootAnchor = CurrentReadings::get_occupied()\n","  session.pieceUntrusted = CurrentReadings::get_occupied() && !Clock::isTrusted();  // straddler: millis() start is boot (0)\n  if (session.pieceUntrusted) Log.info(\"Occ: untrusted straddler, pre-boot part dropped\");\n  session.occupancySessionBootAnchor = (CurrentReadings::get_occupied() && !session.pieceUntrusted)\n",1)
open(p,'w').write(s)
EOF
git diff --numstat -- src; git diff -- src | grep -c . ; grep -n "Clock.h\|include" src/Generalized-Core-Counter.cpp | grep -i clock

<details>
<summary>9 lines</summary>

```
3	1	src/Generalized-Core-Counter.cpp
2	0	src/state/StateMachine.h
14	7	src/state/State_Common.h
6	1	src/state/State_Modes.cpp
107
49:#include "time/ClockTrust.h"         // Sync-recency resync gate and trust signal (WO-2026-08-29-002)
53:#include "time/Clock.h"               // Clock owner: open-hours, resync/RTC write-back, trust signal (WO-2026-09-18 Step 3a)
118:// time/Clock.h (WO-2026-09-18 Step 3a) - defined in time/Clock.cpp.
<shellId: 7 completed with exit code 0>
```

</details>


---

<sub>1m 49s</sub>

### `bash`

**cd /private/tmp/gcc-step0/c2 && wc -l tests/occupancy_session_restart_test.sh tests/occupancy_report_by_mode_test.cpp tests/debounce_expiry_c1_test.sh; sed -n 1,120p tests/occupancy_session_restart_test.sh**

$ cd /private/tmp/gcc-step0/c2 && wc -l tests/occupancy_session_restart_test.sh tests/occupancy_report_by_mode_test.cpp tests/debounce_expiry_c1_test.sh; sed -n 1,120p tests/occupancy_session_restart_test.sh

<details>
<summary>116 lines</summary>

```
570 tests/occupancy_session_restart_test.sh
     822 tests/occupancy_report_by_mode_test.cpp
     106 tests/debounce_expiry_c1_test.sh
    1498 total
#!/bin/zsh
# WO-2026-10-04-001 item B - a restart during an occupancy session does not
# lose that session's minutes.
#
# Three defects, one fix:
#
#  B1  `lastOccupancyEvent` is a millis() value but it is PERSISTED, so after a
#      restart the debounce compared this boot's millis() against the previous
#      boot's. setup() now clears it to 0, which the existing zero sentinel in
#      State_Idle/State_Modes re-arms from.
#  B2  `closeOccupancySessionSafely()` treated an untrusted clock as an invalid
#      session and cleared `occupied`/`occupancyStartTime` anyway, throwing the
#      session away and signalling an unoccupied transition that never
#      happened. It now keeps the session open, credits nothing, and re-arms
#      the debounce so the close is retried one debounce later.
#  B3  A session open across a restart was credited from its (pre-restart)
#      start to the close time, including all the time the device was off. The
#      clock is never trusted during setup() (Clock::isTrusted() needs a cloud
#      sync observed this boot), so a RAM-only flag set in setup() makes the
#      FIRST TRUSTED CLOSE of that session cap `closeAt` at
#      min(closeAt, bootEpoch, anchor + debounce), anchor = max(start,
#      lastReport) CAPTURED ONCE IN setup(). Capturing it at boot matters: a
#      report sent later in the boot, before the clock is trusted, advances
#      lastReport and would otherwise move the cap (Stage 7 finding). Over-credit
#      is then bounded by one debounce period however long the device was off.
#
# Behavioural test: the REAL closeOccupancySessionSafely(), occupancyDebounceMs()
# and updateOccupancyState() are extracted verbatim from the sources and driven
# against fakes. Part 1 is the one thing that cannot be driven - setup()'s boot
# clear - and is an exact-text check.
set -euo pipefail

repo_root="${0:A:h:h}"
common_src="$repo_root/src/state/State_Common.h"
modes_src="$repo_root/src/state/State_Modes.cpp"
app_src="$repo_root/src/Generalized-Core-Counter.cpp"
generated="${TMPDIR:-/tmp}/occupancy_session_restart_test.cpp"
binary="${TMPDIR:-/tmp}/occupancy_session_restart_test"

extract_braced_block() {
  local file="$1"
  local marker="$2"
  local out
  out=$(awk -v marker="$marker" '
    !active && index($0, marker) { active = 1 }
    active {
      print
      opens = gsub(/\{/, "{")
      closes = gsub(/\}/, "}")
      depth += opens - closes
      if (seen_open && depth == 0) exit
      if (opens > 0) seen_open = 1
    }
  ' "$file")
  if [[ -z "$out" ]]; then
    print -u2 "EXTRACT_FAILED: $file has no block starting with: $marker"
    exit 1
  fi
  print -r -- "$out"
}

# --- Part 1: the boot clear (B1) and the cross-boot flag (B3) -----------------
#
# setup() runs once, before any state handler, and cannot be driven on the host
# without standing up the whole application. These two statements must be in
# setup(), immediately after the persistent data loads and BEFORE anything can
# read lastOccupancyEvent or close a session.

setup_block=$(extract_braced_block "$app_src" "void setup() {")

for required in \
  "session.occupancySessionBootAnchor = CurrentReadings::get_occupied()" \
  "? std::max(CurrentReadings::get_occupancyStartTime(), SystemConfig::get_lastReport()) : 0;" \
  "CurrentReadings::set_lastOccupancyEvent(0);"
do
  if ! print -r -- "$setup_block" | grep -qF -- "$required"; then
    print -u2 "FAILED: setup() is missing: $required"
    exit 1
  fi
done

# Ordering: both must come after CurrentReadings::setup() (the load) ...
load_at=$(print -r -- "$setup_block" | grep -nF 'CurrentReadings::setup();' | head -1 | cut -d: -f1)
flag_at=$(print -r -- "$setup_block" | grep -nF 'session.occupancySessionBootAnchor = CurrentReadings::get_occupied()' | head -1 | cut -d: -f1)
clear_at=$(print -r -- "$setup_block" | grep -nF 'CurrentReadings::set_lastOccupancyEvent(0);' | head -1 | cut -d: -f1)

if (( load_at >= flag_at )); then
  print -u2 "FAILED: the cross-boot flag is sampled before the persistent data is loaded"
  exit 1
fi
# ... and the flag must be sampled BEFORE the clear, since the clear would
# otherwise be free to change what the flag observes.
if (( flag_at >= clear_at )); then
  print -u2 "FAILED: lastOccupancyEvent is cleared before the open-session flag is sampled"
  exit 1
fi

# The flag is RAM-only: it lives in SessionState, which is explicitly not
# persisted and not retained. No new persisted field is allowed by this WO.
grep -qF 'time_t occupancySessionBootAnchor = 0;' "$repo_root/src/state/StateMachine.h" || {
  print -u2 "FAILED: occupancySessionBootAnchor is not a SessionState (RAM-only) field"; exit 1; }
if grep -qF 'occupancySessionBootAnchor' "$repo_root/src/MyPersistentData.h"; then
  print -u2 "FAILED: occupancySessionBootAnchor must not become a persisted field"
  exit 1
fi
if grep -rqE 'retained[^;]*occupancySessionBootAnchor' "$repo_root/src"; then
  print -u2 "FAILED: occupancySessionBootAnchor must not be retained"
  exit 1
fi

# --- Part 2: drive the real close and the real debounce ----------------------
{
  cat <<'CPP'
#include <algorithm>
#include <cassert>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
<shellId: 8 completed with exit code 0>
```

</details>


---

<sub>2m 18s</sub>

### `bash`

**cd /private/tmp/gcc-step0/c2 && for t in tests/occupancy_session_restart_test.sh tests/occupancy_report_by_mode_test.sh tests/debounce_expiry_c1_test.sh tests/sleep_wake_equivalence_test.sh; do echo "== $t"; zsh $t 2>&1 | tail -8; done**

$ cd /private/tmp/gcc-step0/c2 && for t in tests/occupancy_session_restart_test.sh tests/occupancy_report_by_mode_test.sh tests/debounce_expiry_c1_test.sh tests/sleep_wake_equivalence_test.sh; do echo "== $t"; zsh $t 2>&1 | tail -8; done

<details>
<summary>23 lines</summary>

```
== tests/occupancy_session_restart_test.sh
FAILED: setup() is missing: session.occupancySessionBootAnchor = CurrentReadings::get_occupied()
== tests/occupancy_report_by_mode_test.sh
                               ~~~~~~~ ^
/var/folders/_r/m12xn8s15wb_15djjdgfp5gr0000gn/T//occupancy_report_by_mode.0GECPQ/clean/extracted_modes_wake_event.inc:81:19: error: no member named 'occupancyStartMs' in 'SessionState'
          session.occupancyStartMs = millis();
          ~~~~~~~ ^
/var/folders/_r/m12xn8s15wb_15djjdgfp5gr0000gn/T//occupancy_report_by_mode.0GECPQ/clean/extracted_modes_wake_event.inc:82:19: error: no member named 'pieceUntrusted' in 'SessionState'
          session.pieceUntrusted = !Clock::isTrusted();
          ~~~~~~~ ^
9 errors generated.
== tests/debounce_expiry_c1_test.sh
FAILED: /private/tmp/gcc-step0/c2/src/state/State_Modes.cpp lacks: OCCUPANCY && signalLEDStatus() && debounceRemainingMs() == 0
== tests/sleep_wake_equivalence_test.sh
          ~~~~~~~ ^
/var/folders/_r/m12xn8s15wb_15djjdgfp5gr0000gn/T//sleep_wake_equivalence.4B9u38/cur_event.inc:82:19: error: no member named 'pieceUntrusted' in 'SessionState'
          session.pieceUntrusted = !Clock::isTrusted();
          ~~~~~~~ ^
/var/folders/_r/m12xn8s15wb_15djjdgfp5gr0000gn/T//sleep_wake_equivalence.4B9u38/cur_event.inc:82:44: error: no member named 'isTrusted' in namespace 'Clock'
          session.pieceUntrusted = !Clock::isTrusted();
                                    ~~~~~~~^
4 errors generated.
<shellId: 9 completed with exit code 0>
```

</details>


---

<sub>2m 25s</sub>

### `bash`

**cd /private/tmp/gcc-step0/c2 && git status --short; grep -n "SessionState\|struct Clock\|namespace Clock\|occupancySessionBootAnchor\|session\b" tests/sleep_wake_equivalence_test.sh | head -30; ls tests | grep -i sleep_wake; grep -n "SessionState\|namespace Clock" tests/*.sh tests/*.cpp | grep -v "^tests/sleep_wake_eq" | head -20**

$ cd /private/tmp/gcc-step0/c2 && git status --short; grep -n "SessionState\|struct Clock\|namespace Clock\|occupancySessionBootAnchor\|session\b" tests/sleep_wake_equivalence_test.sh | head -30; ls tests | grep -i sleep_wake; grep -n "SessionState\|namespace Clock" tests/*.sh tests/*.cpp | grep -v "^tests/sleep_wake_eq" | head -20

<details>
<summary>32 lines</summary>

```
M src/Generalized-Core-Counter.cpp
 M src/state/StateMachine.h
 M src/state/State_Common.h
 M src/state/State_Modes.cpp
152:            "OCCEVT occupied reason=pir-wake led=45s report=0 reset=1" "OCCEVT unoccupied reason=debounce session=120s total=4000s report=1" \
153:            "OCCEVT unoccupied reason=debounce session=120s total=4000s report=0" "OUT trans=1 serviceRequest=1" "occChange=1" \
201:        session.occupancyChangeTriggered = true;
226:mutate_and_expect_diff "button wake skips the service request" sleep "    session.serviceRequestTriggered = true;
sleep_wake_equivalence_test.cpp
sleep_wake_equivalence_test.sh
sleep_wake_structural_test.py
tests/occupancy_session_restart_test.sh:98:# The flag is RAM-only: it lives in SessionState, which is explicitly not
tests/occupancy_session_restart_test.sh:101:  print -u2 "FAILED: occupancySessionBootAnchor is not a SessionState (RAM-only) field"; exit 1; }
tests/occupancy_session_restart_test.sh:178:namespace Clock {
tests/occupancy_session_restart_test.sh:185:}  // namespace Clock
tests/occupancy_session_restart_test.sh:221:struct SessionState {
tests/occupancy_session_restart_test.sh:225:SessionState session;
tests/config_window_hold_test.cpp:86:namespace Clock {
tests/config_window_hold_test.cpp:88:} // namespace Clock
tests/connection_mode_downgrade_test.cpp:36:namespace Clock {
tests/connectivity_failsafe_unknown_openness_test.cpp:42:namespace Clock {
tests/connectivity_failsafe_unknown_openness_test.cpp:47:} // namespace Clock
tests/idle_timediag_on_change_test.cpp:25:namespace Clock {
tests/idle_timediag_on_change_test.cpp:31:namespace Clock {
tests/occupancy_report_by_mode_test.cpp:60:namespace Clock {
tests/occupancy_report_by_mode_test.cpp:67:}  // namespace Clock
tests/occupancy_report_by_mode_test.cpp:99:struct SessionState {
tests/occupancy_report_by_mode_test.cpp:106:SessionState session;
tests/occupancy_report_by_mode_test.cpp:282:  session = SessionState();
tests/persistence_facade_behavior_test.cpp:34:namespace Clock {
tests/sleep_cloud_gate_equivalence_test.cpp:32:namespace Clock {
<shellId: 10 completed with exit code 0>
```

</details>


---

<sub>2m 29s</sub>

### `bash`

**cd /private/tmp/gcc-step0/c2 && grep -n "session\|namespace\|isTrusted\|signalLEDStatus\|get_occupied" tests/sleep_wake_equivalence_test.cpp | head -50; sed -n 55,110p tests/occupancy_report_by_mode_test.cpp**

$ cd /private/tmp/gcc-step0/c2 && grep -n "session\|namespace\|isTrusted\|signalLEDStatus\|get_occupied" tests/sleep_wake_equivalence_test.cpp | head -50; sed -n 55,110p tests/occupancy_report_by_mode_test.cpp

<details>
<summary>79 lines</summary>

```
50:namespace Clock {
53:}  // namespace Clock
54:namespace Config {
57:}  // namespace Config
75:  uint32_t sessionSeconds = 0;
83:  r.sessionSeconds = 120;
92:inline void logUnoccupiedEvent(const char *reason, uint32_t sessionSeconds, uint32_t totalSeconds, bool report) {
93:  sim::emit("OCCEVT unoccupied reason=%s session=%lus total=%lus report=%d", reason, (unsigned long)sessionSeconds,
102:inline bool signalLEDStatus() { return ledOnNow; }
114:namespace BatteryAuthority {
129:}  // namespace BatteryAuthority
131:namespace CurrentReadings {
138:bool get_occupied() { return persistedOccupied; }
144:}  // namespace CurrentReadings
145:namespace SystemConfig {
148:namespace SensorSettings {
150:}  // namespace SensorSettings
151:}  // namespace SystemConfig
152:namespace PowerConfig {
154:}  // namespace PowerConfig
156:namespace base {
160:}  // namespace base
162:namespace cur {
165:}  // namespace cur
202:  session = SessionState();
226:         sim::transitions, (int)session.serviceRequestTriggered, (int)session.occupancyChangeTriggered, (unsigned)persistedHourly,
236:  namespace SC = SystemConfig;
312:  // The LED-timeout close that is not valid, or leaves the session open.
314:    WakeScenario &s = mk(std::string("pir occupancy led-expired ") + kModeName[m] + (valid ? " session still open" : " close not valid"), true, false, false);
int initializeCalls = 0;
}  // namespace StorageHelperRK

bool publishDiagnosticSafe(const char *, const char *, PublishFlags) { return true; }
bool isClockTrusted() { return true; }
namespace Clock {
enum class Openness { Open, Closed, Unknown };
bool trusted = true;
Openness opennessValue = Openness::Open;
bool isTimeValid() { return true; }
bool isTrusted() { return trusted; }
Openness openness() { return opennessValue; }
}  // namespace Clock

// WO-2026-10-29-002: the last close boundary, as DailyBoundary::check() reports it.
namespace DailyBoundary {
struct Result {
  bool due;
  time_t boundary;
  uint8_t close;
};
time_t boundaryValue = 0;
Result check(time_t) { return {false, boundaryValue, 0}; }
}  // namespace DailyBoundary

namespace Config {
bool configValid = true;
uint32_t occupancyDebounceMsForRuntime() { return 300000UL; }
bool isValid(bool) { return configValid; }
}  // namespace Config

namespace {
int failures = 0;
}
#define CHECK(cond, msg)                                                       \
  do {                                                                         \
    if (!(cond)) {                                                             \
      std::cerr << "FAIL: " << (msg) << " (" #cond ")\n";                      \
      ++failures;                                                              \
    }                                                                          \
  } while (0)

// ----- Application surface the extracted code collaborates with ---------------

struct SessionState {
  time_t occupancySessionBootAnchor = 0;
  bool occupancyChangeTriggered = false;
  bool serviceRequestTriggered = false;
  bool suppressAlert40ThisSession = false;
  bool awaitingWebhookResponse = false;
};
SessionState session;

enum State { IDLE_STATE, REPORTING_STATE, SLEEPING_STATE, CONNECTING_STATE, ERROR_STATE };
State state = IDLE_STATE;
State oldState = IDLE_STATE;
<shellId: 11 completed with exit code 0>
```

</details>


---

<sub>2m 32s</sub>

### `bash`

**cd /private/tmp/gcc-step0/c2 && sed -n 40,60p tests/sleep_wake_equivalence_test.cpp; grep -n "struct SessionState" -A8 tests/sleep_wake_equivalence_test.cpp tests/*.cpp | head -40; sed -n 1,60p tests/debounce_expiry_c1_test.sh**

$ cd /private/tmp/gcc-step0/c2 && sed -n 40,60p tests/sleep_wake_equivalence_test.cpp; grep -n "struct SessionState" -A8 tests/sleep_wake_equivalence_test.cpp tests/*.cpp | head -40; sed -n 1,60p tests/debounce_expiry_c1_test.sh

<details>
<summary>93 lines</summary>

```
static uint32_t ledRemainingNow = 0;

// ----- Application surface the two functions collaborate with ------------------------------------------------
struct TimeStub {
  time_t now() { return (time_t)10000; }
};
inline TimeStub Time;
inline bool isClockTrusted() { return cur_in.clockTrusted; }
inline bool isWithinOpenHours() { return cur_in.open; }

namespace Clock {
enum class Openness { Open, Closed, Unknown };
inline Openness openness() { return (Openness)cur_in.openness; }
}  // namespace Clock
namespace Config {
inline uint16_t reportingIntervalSecForRuntime() { return 3600; }
inline uint32_t occupancyDebounceMsForRuntime() { return 300000UL; }
}  // namespace Config

// The real predicates of src/state/State_Common.h, over the same fakes.
inline bool reportDueThisInterval() {
tests/occupancy_report_by_mode_test.cpp:99:struct SessionState {
tests/occupancy_report_by_mode_test.cpp-100-  time_t occupancySessionBootAnchor = 0;
tests/occupancy_report_by_mode_test.cpp-101-  bool occupancyChangeTriggered = false;
tests/occupancy_report_by_mode_test.cpp-102-  bool serviceRequestTriggered = false;
tests/occupancy_report_by_mode_test.cpp-103-  bool suppressAlert40ThisSession = false;
tests/occupancy_report_by_mode_test.cpp-104-  bool awaitingWebhookResponse = false;
tests/occupancy_report_by_mode_test.cpp-105-};
tests/occupancy_report_by_mode_test.cpp-106-SessionState session;
tests/occupancy_report_by_mode_test.cpp-107-
--
tests/standby_latch_daily_clear_test.cpp:17:struct SessionState {
tests/standby_latch_daily_clear_test.cpp-18-  bool modemUnstable = false;
tests/standby_latch_daily_clear_test.cpp-19-  bool modemStandbySuppressed = false;
tests/standby_latch_daily_clear_test.cpp-20-  uint8_t modemUnstableReason = 0;
tests/standby_latch_daily_clear_test.cpp-21-  uint8_t modemConnectTimeoutCount = 0;
tests/standby_latch_daily_clear_test.cpp-22-  unsigned long lastTeardownEndMs = 0;
tests/standby_latch_daily_clear_test.cpp-23-};
tests/standby_latch_daily_clear_test.cpp-24-static SessionState session;
tests/standby_latch_daily_clear_test.cpp-25-
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
<shellId: 12 completed with exit code 0>
```

</details>


---

<sub>2m 34s</sub>

### `bash`

**cd /private/tmp/gcc-step0/c2 && sed -n 60,106p tests/debounce_expiry_c1_test.sh; sed -n 25,40p tests/sleep_wake_equivalence_test.cpp; grep -n "occupancy_ends\|State_Modes" tests/sleep_wake_equivalence_test.sh | head**

$ cd /private/tmp/gcc-step0/c2 && sed -n 60,106p tests/debounce_expiry_c1_test.sh; sed -n 25,40p tests/sleep_wake_equivalence_test.cpp; grep -n "occupancy_ends\|State_Modes" tests/sleep_wake_equivalence_test.sh | head

<details>
<summary>67 lines</summary>

```
# --- call sites -------------------------------------------------------------
check_has() { grep -qF -- "$2" "$1" || { print -u2 "FAILED: $1 lacks: $2"; return 1; } }
check_sites() {
  local d="$1"
  check_has "$d/State_Idle.cpp" "if (debounceRemainingMs() == 0) {" &&
  check_has "$d/State_Modes.cpp" "if (debounceRemainingMs() == 0) {" &&
  check_has "$d/State_Modes.cpp" "OCCUPANCY && signalLEDStatus() && debounceRemainingMs() == 0" &&
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
  local m="$work/mut"; rm -rf "$m"; mkdir -p "$m"
  cp "$src/State_Common.h" "$src/State_Idle.cpp" "$src/State_Modes.cpp" "$src/State_Sleep.cpp" "$m/"
  python3 - "$m/$file" "$old" "$new" <<'PY'
import sys
p, old, new = sys.argv[1:]
s = open(p).read()
if s.count(old) != 1:
    print("MUTATION_NOT_APPLIED:", old, s.count(old)); sys.exit(3)
open(p, "w").write(s.replace(old, new))
PY
  local caught=0
  run_predicate "$m/State_Common.h" >/dev/null 2>&1 || caught=1
  check_sites "$m" >/dev/null 2>&1 || caught=1
  if grep -n "OCCUPANCY" "$m/State_Modes.cpp" | grep -q "signalLEDTimeRemaining"; then caught=1; fi
  if (( caught )); then print "caught: $name"; else print -u2 "NOT CAUGHT: $name"; exit 1; fi
}

mutate "no expiry clamp" State_Common.h "return elapsed >= debounceMs ? 0 : debounceMs - elapsed;" "return debounceMs - elapsed;"
mutate "off by the margin" State_Common.h "debounceMs - elapsed;" "debounceMs - elapsed + 1000;"
mutate "sentinel not handled" State_Common.h "if (last == 0) {" "if (false) {"
mutate "wall clock close" State_Modes.cpp "OCCUPANCY && signalLEDStatus() && debounceRemainingMs() == 0" "OCCUPANCY && signalLEDStatus() && signalLEDTimeRemaining() == 0"
mutate "awake idle on subtraction" State_Idle.cpp "if (debounceRemainingMs() == 0) {" "if (millis() - CurrentReadings::get_lastOccupancyEvent() > 1) {"
mutate "plan on RTC deadline" State_Sleep.cpp "const uint32_t remaining = debounceRemainingMs();" "const uint32_t remaining = signalLEDTimeRemaining() * 1000UL;"
mutate "plan margin dropped" State_Sleep.cpp "(int)(remaining / 1000UL) + 1" "(int)(remaining / 1000UL)"

print "debounce_expiry_c1_test: predicate and call sites correct, all mutations detected"
  int openness = 0;  // 0 Open, 1 Closed, 2 Unknown
  bool occupied = false, ledOn = false;
  uint32_t ledRemaining = 0, setting1 = 0;
  long lastReport = 7300;
  uint16_t hourly = 4, daily = 9;
  bool closeValid = true, closeStillOpen = false;
  bool sensorReady = true;
  int vcellState = 0, trust = 0;
};
static WakeScenario cur_in;
static long persistedLastCount = 0, persistedOccStart = 0;
static uint16_t persistedHourly = 0, persistedDaily = 0;
static bool persistedOccupied = false;
static uint32_t persistedLastOccEvent = 0;
static bool ledOnNow = false;
static uint32_t ledRemainingNow = 0;
5:# wake-event processing into processSleepWakeEvent() (State_Modes.cpp) changes no behaviour.
12:# commit named in the fixture header. Mutants of private copies of State_Sleep.cpp and State_Modes.cpp must
60:  # $1 = base|cur, $2 = State_Sleep.cpp, $3 = State_Modes.cpp, $4 = output dir
65:extract cur "$src_root/state/State_Sleep.cpp" "$src_root/state/State_Modes.cpp" "$work"
125:extract cur "$src_root/state/State_Sleep.cpp" "$src_root/state/State_Modes.cpp" "$work/fresh"
163:  [[ "$which" == sleep ]] && file=State_Sleep.cpp || file=State_Modes.cpp
166:  cp "$src_root/state/State_Modes.cpp" "$m/State_Modes.cpp"
179:  python3 "$work/extract.py" cur "$m/State_Sleep.cpp" "$m/State_Modes.cpp" "$m"
<shellId: 13 completed with exit code 0>
```

</details>


---

<sub>2m 38s</sub>

### `bash`

**cd /private/tmp/gcc-step0/c2 && grep -n "SessionState\|include" tests/sleep_wake_equivalence_test.cpp | head -20**

$ cd /private/tmp/gcc-step0/c2 && grep -n "SessionState\|include" tests/sleep_wake_equivalence_test.cpp | head -20

<details>
<summary>12 lines</summary>

```
9:#include <cmath>
10:#include <cstdio>
11:#include <cstring>
12:#include <map>
13:#include <set>
14:#include <string>
15:#include "SleepExecutionFakes.h"
158:#include BASE_REGION_INC
163:#include CUR_EVENT_INC
164:#include CUR_ROUTE_INC
202:  session = SessionState();
<shellId: 14 completed with exit code 0>
```

</details>


---

<sub>2m 40s</sub>

### `bash`

**cd /private/tmp/gcc-step0/c2 && grep -rn "struct SessionState" -B2 -A12 tests/*.h tests/fixtures 2>/dev/null | head -40; grep -rln "SleepExecutionFakes.h" tests | head**

$ cd /private/tmp/gcc-step0/c2 && grep -rn "struct SessionState" -B2 -A12 tests/*.h tests/fixtures 2>/dev/null | head -40; grep -rln "SleepExecutionFakes.h" tests | head

<details>
<summary>9 lines</summary>

```
tests/sleep_plan_equivalence_test.sh
tests/sleep_plan_equivalence_test.cpp
tests/sleep_wake_equivalence_test.cpp
tests/sleep_preflight_equivalence_test.cpp
tests/sleep_cloud_gate_equivalence_test.sh
tests/sleep_execution_equivalence_test.cpp
tests/sleep_execution_equivalence_test.sh
tests/sleep_cloud_gate_equivalence_test.cpp
<shellId: 15 completed with exit code 0>
```

</details>


---

<sub>2m 42s</sub>

### `bash`

**cd /private/tmp/gcc-step0/c2 && f=$(find . -name SleepExecutionFakes.h | head -1); echo $f; grep -n "SessionState" -A25 $f | head -50; grep -n "namespace Clock" -A6 $f**

$ cd /private/tmp/gcc-step0/c2 && f=$(find . -name SleepExecutionFakes.h | head -1); echo $f; grep -n "SessionState" -A25 $f | head -50; grep -n "namespace Clock" -A6 $f

```
./tests/stubs/sleep_teardown/SleepExecutionFakes.h
<shellId: 16 completed with exit code 1>
```


---

<sub>2m 44s</sub>

### `bash`

**cd /private/tmp/gcc-step0/c2 && grep -rn "session" tests/stubs/sleep_teardown/*.h | head; grep -n "SessionState\|session" tests/sleep_wake_equivalence_test.sh | head**

$ cd /private/tmp/gcc-step0/c2 && grep -rn "session" tests/stubs/sleep_teardown/*.h | head; grep -n "SessionState\|session" tests/sleep_wake_equivalence_test.sh | head

```
152:            "OCCEVT occupied reason=pir-wake led=45s report=0 reset=1" "OCCEVT unoccupied reason=debounce session=120s total=4000s report=1" \
153:            "OCCEVT unoccupied reason=debounce session=120s total=4000s report=0" "OUT trans=1 serviceRequest=1" "occChange=1" \
201:        session.occupancyChangeTriggered = true;
226:mutate_and_expect_diff "button wake skips the service request" sleep "    session.serviceRequestTriggered = true;
<shellId: 17 completed with exit code 0>
```


---

<sub>2m 46s</sub>

### `bash`

**cd /private/tmp/gcc-step0/c2 && grep -rn "SessionState" tests/stubs src/state/*.h | head; sed -n 1,40p tests/stubs/sleep_teardown/SleepExecutionFakes.h**

$ cd /private/tmp/gcc-step0/c2 && grep -rn "SessionState" tests/stubs src/state/*.h | head; sed -n 1,40p tests/stubs/sleep_teardown/SleepExecutionFakes.h

<details>
<summary>45 lines</summary>

```
tests/stubs/sleep_teardown/state/State_Common.h:29:struct SessionState {
tests/stubs/sleep_teardown/state/State_Common.h:39:inline SessionState session;
tests/stubs/modem_health/state/State_Common.h:10:struct SessionState {
tests/stubs/modem_health/state/State_Common.h:16:static SessionState session;
src/state/StateMachine.h:49:struct SessionState {
src/state/StateMachine.h:80:extern SessionState session;
#pragma once
// Host fakes for the sleep-execution equivalence harness (WO-2026-10-21-001): what the
// hibernate attempt, the ULP/STOP cascade and the standby decision call. Every call
// that changes the world goes to sim::events; the inputs are scripted in sim::exec.
#include <cstring>
#include <ctime>
#include <utility>
#include "Particle.h"
#include "state/State_Common.h"
#include "ResetCause.h"
#include "ThrashGuard.h"
#include "observability/WakeCycleStats.h"
#include "persist/CurrentReadings.h"
#include "persist/SystemConfig.h"
#include "persist/RecoveryState.h"
#include "power/ConnectivityPolicy.h"

#define PLATFORM_BORON 13
#define PLATFORM_ID PLATFORM_BORON
#define ALLOW_BLOCKING_SERIAL_WAITS 0
#define ENABLE_DIAGNOSTICS_PUBLISH_MODE 1
#ifndef CONNECTIVITY_FAILSAFE_TEST_MODE
#define CONNECTIVITY_FAILSAFE_TEST_MODE 0
#endif
#define Wiring_Watchdog 1
#ifndef ENABLE_SLEEP_TRACE
#define ENABLE_SLEEP_TRACE 0
#endif
#ifndef HAL_PLATFORM_CELLULAR
#define HAL_PLATFORM_CELLULAR 0
#endif

typedef uint16_t pin_t;
constexpr pin_t PIN_INVALID = 0xFFFF, BUTTON_PIN = 4, WAKEUP_PIN = 7;
enum InterruptMode { RISING = 1, FALLING = 2 };
enum PinMode { INPUT = 0, INPUT_PULLUP = 2 };
enum { LOW = 0, HIGH = 1 };
constexpr int NETWORK_INTERFACE_CELLULAR = 3;
enum class SystemSleepMode { ULTRA_LOW_POWER, STOP, HIBERNATE };
enum class SystemSleepNetworkFlag { INACTIVE_STANDBY };
<shellId: 18 completed with exit code 0>
```

</details>


---

<sub>2m 49s</sub>

### `bash`

**cd /private/tmp/gcc-step0/c2 && sed -n 25,42p tests/stubs/sleep_teardown/state/State_Common.h**

$ cd /private/tmp/gcc-step0/c2 && sed -n 25,42p tests/stubs/sleep_teardown/state/State_Common.h

<details>
<summary>17 lines</summary>

```
}
inline void setAppBreadcrumb(uint8_t code) { sim::emit("CRUMB %u", (unsigned)code); }
inline void serviceAwakeWatchdog() { sim::reads.watchdog++; }

struct SessionState {
  unsigned long lastTeardownEndMs = 0;
  uint32_t lastCloudDisconnectElapsedMs = 0;
  uint32_t lastModemOffElapsedMs = 0;
  uint32_t lastTotalTeardownElapsedMs = 0;
  bool modemUnstable = false;
  bool awaitingWebhookResponse = false;
  bool occupancyChangeTriggered = false;
  bool serviceRequestTriggered = false;
};
inline SessionState session;

class PowerManager {
 public:
<shellId: 19 completed with exit code 0>
```

</details>


---

<sub>3m 25s</sub>

### `bash`

**cd /private/tmp/gcc-step0/c2 && python3 - <<'EOF'
def sub(p,o,n):
    s=open(p).read(); assert s.count(o)==1,(p,o); open(p,'w').write(s.replace(o,n))
sub('tests/stubs/sleep_teardown/state/State_Common.h',"  bool serviceRequestTriggered = false;\n};\ninline SessionState session;","  bool serviceRequestTriggered = false;\n  uint32_t occupancyStartMs = 0;\n  bool pieceUntrusted = false;\n};\ninline SessionState session;")
sub('tests/sleep_wake_equivalence_test.cpp',"inline Openness openness() { return (Openness)cur_in.openness; }","inline Openness openness() { return (Openness)cur_in.openness; }\ninline bool isTrusted() { return cur_in.clockTrusted; }")
sub('tests/occupancy_report_by_mode_test.cpp',"  bool awaitingWebhookResponse = false;\n};\nSessionState session;","  bool awaitingWebhookResponse = false;\n  uint32_t occupancyStartMs = 0;\n  bool pieceUntrusted = false;\n};\nSessionState session;")
sub('tests/debounce_expiry_c1_test.sh','"OCCUPANCY && signalLEDStatus() && debounceRemainingMs() == 0" &&','"(signalLEDStatus() || (session.pieceUntrusted && CurrentReadings::get_occupied())) && debounceRemainingMs() == 0" &&')
sub('tests/debounce_expiry_c1_test.sh','mutate "wall clock close" State_Modes.cpp "OCCUPANCY && signalLEDStatus() && debounceRemainingMs() == 0" "OCCUPANCY && signalLEDStatus() && signalLEDTimeRemaining() == 0"','mutate "wall clock close" State_Modes.cpp "pieceUntrusted && CurrentReadings::get_occupied())) && debounceRemainingMs() == 0" "pieceUntrusted && CurrentReadings::get_occupied())) && signalLEDTimeRemaining() == 0"')
EOF
for t in tests/occupancy_report_by_mode_test.sh tests/debounce_expiry_c1_test.sh tests/sleep_wake_equivalence_test.sh; do echo "== $t"; zsh $t 2>&1 | tail -8; done; git status --short**

$ cd /private/tmp/gcc-step0/c2 && python3 - <<'EOF'
def sub(p,o,n):
    s=open(p).read(); assert s.count(o)==1,(p,o); open(p,'w').write(s.replace(o,n))
sub('tests/stubs/sleep_teardown/state/State_Common.h',"  bool serviceRequestTriggered = false;\n};\ninline SessionState session;","  bool serviceRequestTriggered = false;\n  uint32_t occupancyStartMs = 0;\n  bool pieceUntrusted = false;\n};\ninline SessionState session;")
sub('tests/sleep_wake_equivalence_test.cpp',"inline Openness openness() { return (Openness)cur_in.openness; }","inline Openness openness() { return (Openness)cur_in.openness; }\ninline bool isTrusted() { return cur_in.clockTrusted; }")
sub('tests/occupancy_report_by_mode_test.cpp',"  bool awaitingWebhookResponse = false;\n};\nSessionState session;","  bool awaitingWebhookResponse = false;\n  uint32_t occupancyStartMs = 0;\n  bool pieceUntrusted = false;\n};\nSessionState session;")
sub('tests/debounce_expiry_c1_test.sh','"OCCUPANCY && signalLEDStatus() && debounceRemainingMs() == 0" &&','"(signalLEDStatus() || (session.pieceUntrusted && CurrentReadings::get_occupied())) && debounceRemainingMs() == 0" &&')
sub('tests/debounce_expiry_c1_test.sh','mutate "wall clock close" State_Modes.cpp "OCCUPANCY && signalLEDStatus() && debounceRemainingMs() == 0" "OCCUPANCY && signalLEDStatus() && signalLEDTimeRemaining() == 0"','mutate "wall clock close" State_Modes.cpp "pieceUntrusted && CurrentReadings::get_occupied())) && debounceRemainingMs() == 0" "pieceUntrusted && CurrentReadings::get_occupied())) && signalLEDTimeRemaining() == 0"')
EOF
for t in tests/occupancy_report_by_mode_test.sh tests/debounce_expiry_c1_test.sh tests/sleep_wake_equivalence_test.sh; do echo "== $t"; zsh $t 2>&1 | tail -8; done; git status --short

```
== tests/occupancy_report_by_mode_test.sh
<command with shellId: 20 is still running after 30 seconds. The command is still running. You will be automatically notified when it completes; if you need the command to complete end your response with no tool calls to wait for the notification. Use stop_bash to stop it.>
```


---

<sub>4m 24s</sub>

### `read_bash`

**Waiting up to 120 seconds for command output**

<details>
<summary>Arguments</summary>

```json
{
  "shellId": "20",
  "delay": 120
}
```

</details>

<details>
<summary>29 lines</summary>

```
== tests/occupancy_report_by_mode_test.sh
OK (mutation): 'flag take moved below the early exits' caught by /norepeat:/ (17 failed checks, e.g. FAIL: KEEP_ALIVE: norepeat: config invalid then Idle: the flag is taken at entry (!session.occupancyChangeTrig)
OK (mutation): 'sleep check ignores disconnectRequested' caught by /teardown:/ (5 failed checks, e.g. FAIL: KEEP_ALIVE: teardown: the flag does not go to REPORTING on this sleep span (!wentTo("occupancy change pe)
OK (mutation): 'mid-gate exit keeps the gate timer' caught by /midgate:/ (4 failed checks, e.g. FAIL: KEEP_ALIVE: midgate: leaving mid-gate resets the gate timer (cloudGate.cloudSyncStartMs == 0))
OK (mutation): 'sleep check moved below suppress' caught by /soak:/ (17 failed checks, e.g. FAIL: KEEP_ALIVE: soak: the next sleep prep goes to REPORTING for the pending change (state == REPORTING_STATE)
OK (mutation): 'sleep check moved below cloudGatePass' caught by /cloud gate|midgate:/ (7 failed checks, e.g. FAIL: the sleep check sits before the handler's cloudGatePass( call (the cloud gate wait) (SLEEP_CONSUMER_BEFO)
OK (mutation): 'sleep check moved below the teardown' caught by /sits before System.sleep\(\)|teardown:/ (12 failed checks, e.g. FAIL: the sleep check sits before System.sleep() and every sleep-or-suppress decision (SLEEP_CONSUMER_BEFORE_S)
OK (mutation): 'sleep check moved below sleepAndResume' caught by /soak:/ (12 failed checks, e.g. FAIL: KEEP_ALIVE: soak: no sleep-or-suppress decision was reached (!sleepReached && !wentTo("sleep-timer-occup)
MUTATION_FAILED: '    if (SystemConfig::get_sensorMode() == SystemConfig::OCCUPANCY && signalLEDStatus() && debounceRemainingMs() == 0) {' not found in /private/tmp/gcc-step0/c2/src/state/State_Modes.cpp
== tests/debounce_expiry_c1_test.sh
caught: no expiry clamp
caught: off by the margin
caught: sentinel not handled
caught: wall clock close
caught: awake idle on subtraction
caught: plan on RTC deadline
caught: plan margin dropped
debounce_expiry_c1_test: predicate and call sites correct, all mutations detected
== tests/sleep_wake_equivalence_test.sh
MUTATION_NOT_APPLIED: '    if (SystemConfig::get_sensorMode() == SystemConfig::OCCUPANCY && signalLEDStatus() && debounceRemainingMs() == 0) {' x0
 M src/Generalized-Core-Counter.cpp
 M src/state/StateMachine.h
 M src/state/State_Common.h
 M src/state/State_Modes.cpp
 M tests/debounce_expiry_c1_test.sh
 M tests/occupancy_report_by_mode_test.cpp
 M tests/sleep_wake_equivalence_test.cpp
 M tests/stubs/sleep_teardown/state/State_Common.h
<shellId: 20 completed with exit code 0>
```

</details>


---

<sub>4m 29s</sub>

### `bash`

**cd /private/tmp/gcc-step0/c2 && grep -rn "signalLEDStatus() && debounceRemainingMs" tests | cut -c1-220**

$ cd /private/tmp/gcc-step0/c2 && grep -rn "signalLEDStatus() && debounceRemainingMs" tests | cut -c1-220

```
tests/occupancy_report_by_mode_test.sh:488:  "    if (SystemConfig::get_sensorMode() == SystemConfig::OCCUPANCY && signalLEDStatus() && debounceRemainingMs() == 0) {" \
tests/occupancy_report_by_mode_test.sh:498:    if (SystemConfig::get_sensorMode() == SystemConfig::OCCUPANCY && signalLEDStatus() && debounceRemainingMs() == 0) {
tests/sleep_wake_structural_test.py:227:LED_CLOSE_NEW = "signalLEDStatus() && debounceRemainingMs() == 0) {\n      // Debounce period elapsed without motion (the millis predicate, not the LED's wall deadline)"
tests/sleep_wake_equivalence_test.sh:185:LED_CLOSE='    if (SystemConfig::get_sensorMode() == SystemConfig::OCCUPANCY && signalLEDStatus() && debounceRemainingMs() == 0) {'
<shellId: 21 completed with exit code 0>
```


---

<sub>4m 33s</sub>

### `bash`

**cd /private/tmp/gcc-step0/c2 && sed -n 480,510p tests/occupancy_report_by_mode_test.sh; sed -n 220,235p tests/sleep_wake_structural_test.py; sed -n 180,200p tests/sleep_wake_equivalence_test.sh; git diff src/state/State_Modes.cpp | head -30**

$ cd /private/tmp/gcc-step0/c2 && sed -n 480,510p tests/occupancy_report_by_mode_test.sh; sed -n 220,235p tests/sleep_wake_structural_test.py; sed -n 180,200p tests/sleep_wake_equivalence_test.sh; git diff src/state/State_Modes.cpp | head -30

<details>
<summary>96 lines</summary>

```diff
"$sleep_pending" "" \
  "  WakeContext wake;
  if (sleepAndResume(wakeInSeconds, useNetworkStandby, sleepReason, wake)) return;
" "  WakeContext wake;
  if (sleepAndResume(wakeInSeconds, useNetworkStandby, sleepReason, wake)) return;
$sleep_pending"
# WO-2026-10-22-001: the wake's LED-timeout close must run before the PIR event is processed.
mutate_and_expect_failure "PIR event processed before the debounce close" "wake pir\\+end:" "state/State_Modes.cpp" \
  "    if (SystemConfig::get_sensorMode() == SystemConfig::OCCUPANCY && signalLEDStatus() && debounceRemainingMs() == 0) {" \
  "    if (false) {" \
  "        CurrentReadings::set_lastOccupancyEvent(millis());
      }
    }
  return false;
}" \
  "        CurrentReadings::set_lastOccupancyEvent(millis());
      }
    }
    if (SystemConfig::get_sensorMode() == SystemConfig::OCCUPANCY && signalLEDStatus() && debounceRemainingMs() == 0) {
      const bool reportNow = reportsOccupancyChangesNow();
      const OccupancyCloseResult closeResult = closeOccupancySessionSafely(\"sleep\");
      signalLED(false);
      if (reportNow && !closeResult.stillOpen) {
        session.occupancyChangeTriggered = true;
        transitionTo(REPORTING_STATE, \"sleep-occupancy-debounce-report\");
        return true;
      }
    }
  return false;
}"
mutate_and_expect_failure "connected clear restored, entry take removed" "norepeat:" "state/State_Report.cpp" \
    CurrentReadings::set_lastOccupancyEvent(lastEvent);
  }
  uint32_t timeSinceLastEvent = millis() - lastEvent;

  // Check if debounce timeout has expired
  if (timeSinceLastEvent > debounceMs) {"""
LED_CLOSE_OLD = "signalLEDTimeRemaining() == 0 && signalLEDStatus()) {\n      // LED timeout expired - debounce period elapsed without motion"
LED_CLOSE_NEW = "signalLEDStatus() && debounceRemainingMs() == 0) {\n      // Debounce period elapsed without motion (the millis predicate, not the LED's wall deadline)"
if base_modes is not None:
    base_modes = base_modes.replace(DEBOUNCE_OLD, "  if (debounceRemainingMs() == 0) {").replace(LED_CLOSE_OLD, LED_CLOSE_NEW)
cur_modes_head = modes[:modes.index("\nbool processSleepWakeEvent(bool pirWake) {")]
added = [l for l in modes.splitlines(keepends=True) if l not in base_modes.splitlines(keepends=True)]
head_added = [l for l in cur_modes_head.splitlines(keepends=True) if l not in base_modes.splitlines(keepends=True)]
# WO-2026-11-03-004: updateOccupancyState() now asks the one debounce predicate.
if head_added and not all(l.startswith("#include") or l == "  if (debounceRemainingMs() == 0) {\n" for l in head_added):
    fail("State_Modes.cpp changed above the new owner beyond added includes")
  build_run b "$m/new.txt" "$m" > "$m/build.log" 2>&1 || { echo "MUTANT_DID_NOT_BUILD: $label"; cat "$m/build.log"; exit 1; }
  if diff -q "$work/expected.txt" "$m/new.txt" > /dev/null; then echo "MUTANT_SURVIVED: $label"; exit 1; fi
  echo "caught: $label"
}

LED_CLOSE='    if (SystemConfig::get_sensorMode() == SystemConfig::OCCUPANCY && signalLEDStatus() && debounceRemainingMs() == 0) {'
# The PIR event processed before the LED-timeout close (a pair of literal replacements).
mutate_and_expect_diff "PIR event processed before the LED-timeout close" modes \
  "$LED_CLOSE" "    if (false) {" \
  "        CurrentReadings::set_lastOccupancyEvent(millis());
      }
    }
  return false;
}" "        CurrentReadings::set_lastOccupancyEvent(millis());
      }
    }
$LED_CLOSE
      const bool reportNow = reportsOccupancyChangesNow();
      const OccupancyCloseResult closeResult = closeOccupancySessionSafely(\"sleep\");
      signalLED(false);
      if (reportNow && !closeResult.stillOpen) {
diff --git a/src/state/State_Modes.cpp b/src/state/State_Modes.cpp
index ea08112..ff7b3d5 100644
--- a/src/state/State_Modes.cpp
+++ b/src/state/State_Modes.cpp
@@ -69,6 +69,8 @@ void handleOccupancyMode() {
       // Transition from unoccupied to occupied
       CurrentReadings::set_occupied(true);
       CurrentReadings::set_occupancyStartTime(Time.now());
+      session.occupancyStartMs = millis();
+      session.pieceUntrusted = !Clock::isTrusted();
       
       // V3.23: Set LED with debounce timeout from sensor.setting1
       uint32_t setting1Raw = SystemConfig::SensorSettings::get_sensorSetting1();
@@ -153,7 +155,8 @@ void updateOccupancyState() {
 bool processSleepWakeEvent(bool pirWake) {
     // Check if LED timeout expired FIRST (before processing new PIR wake)
     // This ensures we don't immediately undo state changes from PIR processing
-    if (SystemConfig::get_sensorMode() == SystemConfig::OCCUPANCY && signalLEDStatus() && debounceRemainingMs() == 0) {
+    if (SystemConfig::get_sensorMode() == SystemConfig::OCCUPANCY &&
+        (signalLEDStatus() || (session.pieceUntrusted && CurrentReadings::get_occupied())) && debounceRemainingMs() == 0) {
       // Debounce period elapsed without motion (the millis predicate, not the LED's wall deadline)
       const bool reportNow = reportsOccupancyChangesNow();
       const OccupancyCloseResult closeResult = closeOccupancySessionSafely("sleep");
@@ -229,6 +232,8 @@ bool processSleepWakeEvent(bool pirWake) {
           // when untrusted preserves existing behavior; see the
           // Implementation Report for the Chief Engineer's review.
           CurrentReadings::set_occupancyStartTime(Time.now());
+          session.occupancyStartMs = millis();
+          session.pieceUntrusted = !Clock::isTrusted();
           // Treat the PIR wake as an occupancy event for debounce purposes.
<shellId: 22 completed with exit code 0>
```

</details>


---

<sub>4m 52s</sub>

### `bash`

**cd /private/tmp/gcc-step0/c2 && python3 - <<'EOF'
def sub(p,o,n,c=1):
    s=open(p).read(); assert s.count(o)==c,(p,o,s.count(o)); open(p,'w').write(s.replace(o,n))
new_gate="(signalLEDStatus() || (session.pieceUntrusted && CurrentReadings::get_occupied()))"
sub('src/state/State_Modes.cpp',"OCCUPANCY &&\n        (signalLEDStatus() || (session.pieceUntrusted && CurrentReadings::get_occupied())) && debounce","OCCUPANCY && "+new_gate+" && debounce")
sub('tests/occupancy_report_by_mode_test.sh',"OCCUPANCY && signalLEDStatus() && debounceRemainingMs()","OCCUPANCY && "+new_gate+" && debounceRemainingMs()",2)
sub('tests/sleep_wake_equivalence_test.sh',"OCCUPANCY && signalLEDStatus() && debounceRemainingMs()","OCCUPANCY && "+new_gate+" && debounceRemainingMs()")
sub('tests/sleep_wake_structural_test.py','LED_CLOSE_NEW = "signalLEDStatus() && debounceRemainingMs() == 0) {','LED_CLOSE_NEW = "(signalLEDStatus() || (session.pieceUntrusted && CurrentReadings::get_occupied())) && debounceRemainingMs() == 0) {')
sub('tests/debounce_expiry_c1_test.sh','"(signalLEDStatus() || (session.pieceUntrusted && CurrentReadings::get_occupied())) && debounceRemainingMs() == 0" &&','"OCCUPANCY && (signalLEDStatus() || (session.pieceUntrusted && CurrentReadings::get_occupied())) && debounceRemainingMs() == 0" &&')
EOF
git diff --numstat -- src
python3 tests/sleep_wake_structural_test.py 2>&1 | tail -8**

$ cd /private/tmp/gcc-step0/c2 && python3 - <<'EOF'
def sub(p,o,n,c=1):
    s=open(p).read(); assert s.count(o)==c,(p,o,s.count(o)); open(p,'w').write(s.replace(o,n))
new_gate="(signalLEDStatus() || (session.pieceUntrusted && CurrentReadings::get_occupied()))"
sub('src/state/State_Modes.cpp',"OCCUPANCY &&\n        (signalLEDStatus() || (session.pieceUntrusted && CurrentReadings::get_occupied())) && debounce","OCCUPANCY && "+new_gate+" && debounce")
sub('tests/occupancy_report_by_mode_test.sh',"OCCUPANCY && signalLEDStatus() && debounceRemainingMs()","OCCUPANCY && "+new_gate+" && debounceRemainingMs()",2)
sub('tests/sleep_wake_equivalence_test.sh',"OCCUPANCY && signalLEDStatus() && debounceRemainingMs()","OCCUPANCY && "+new_gate+" && debounceRemainingMs()")
sub('tests/sleep_wake_structural_test.py','LED_CLOSE_NEW = "signalLEDStatus() && debounceRemainingMs() == 0) {','LED_CLOSE_NEW = "(signalLEDStatus() || (session.pieceUntrusted && CurrentReadings::get_occupied())) && debounceRemainingMs() == 0) {')
sub('tests/debounce_expiry_c1_test.sh','"(signalLEDStatus() || (session.pieceUntrusted && CurrentReadings::get_occupied())) && debounceRemainingMs() == 0" &&','"OCCUPANCY && (signalLEDStatus() || (session.pieceUntrusted && CurrentReadings::get_occupied())) && debounceRemainingMs() == 0" &&')
EOF
git diff --numstat -- src
python3 tests/sleep_wake_structural_test.py 2>&1 | tail -8

<details>
<summary>6 lines</summary>

```
3	1	src/Generalized-Core-Counter.cpp
2	0	src/state/StateMachine.h
14	7	src/state/State_Common.h
5	1	src/state/State_Modes.cpp
FAIL: State_Modes.cpp changed above the new owner beyond added includes
<shellId: 23 completed with exit code 0>
```

</details>


---

<sub>4m 57s</sub>

### `bash`

**cd /private/tmp/gcc-step0/c2 && sed -n 200,300p tests/sleep_wake_structural_test.py**

$ cd /private/tmp/gcc-step0/c2 && sed -n 200,300p tests/sleep_wake_structural_test.py

<details>
<summary>69 lines</summary>

```
LED_NEW = "(unsigned long)signalLEDTimeRemaining());"
LED_BASE = "(unsigned long)(signalLEDTimeRemaining() / 1000UL));"
if log_of(sleep).count(LED_NEW) != 1:
    fail("logWakeSummary() must print signalLEDTimeRemaining() (seconds) without a further division")
if tail_of(sleep) != tail_of(base_sleep):
    fail("the S-2 functions (struct WakeContext .. end of the anonymous namespace) differ from the base")
if prefix_of(sleep) != prefix_of(base_sleep):
    fail("the State_Sleep.cpp includes differ from the base")
if log_of(sleep).replace(LED_NEW, LED_BASE) != log_of(base_sleep):
    fail("logWakeSummary() differs from the base")
base_modes = git_show(base, "src/state/State_Modes.cpp")
# WO-2026-11-03-004: the base's inline debounce subtraction and its LED close criterion map to the predicate.
DEBOUNCE_OLD = """  // V3.23: Occupancy debounce timeout comes from sensor.setting1
  const uint32_t debounceMs = occupancyDebounceMs();

  uint32_t lastEvent = CurrentReadings::get_lastOccupancyEvent();
  if (lastEvent == 0) {
    // If this is 0 (for example, occupancy set from a PIR wake path that
    // didn't update lastOccupancyEvent), we must not immediately expire.
    lastEvent = millis();
    CurrentReadings::set_lastOccupancyEvent(lastEvent);
  }
  uint32_t timeSinceLastEvent = millis() - lastEvent;

  // Check if debounce timeout has expired
  if (timeSinceLastEvent > debounceMs) {"""
LED_CLOSE_OLD = "signalLEDTimeRemaining() == 0 && signalLEDStatus()) {\n      // LED timeout expired - debounce period elapsed without motion"
LED_CLOSE_NEW = "(signalLEDStatus() || (session.pieceUntrusted && CurrentReadings::get_occupied())) && debounceRemainingMs() == 0) {\n      // Debounce period elapsed without motion (the millis predicate, not the LED's wall deadline)"
if base_modes is not None:
    base_modes = base_modes.replace(DEBOUNCE_OLD, "  if (debounceRemainingMs() == 0) {").replace(LED_CLOSE_OLD, LED_CLOSE_NEW)
cur_modes_head = modes[:modes.index("\nbool processSleepWakeEvent(bool pirWake) {")]
added = [l for l in modes.splitlines(keepends=True) if l not in base_modes.splitlines(keepends=True)]
head_added = [l for l in cur_modes_head.splitlines(keepends=True) if l not in base_modes.splitlines(keepends=True)]
# WO-2026-11-03-004: updateOccupancyState() now asks the one debounce predicate.
if head_added and not all(l.startswith("#include") or l == "  if (debounceRemainingMs() == 0) {\n" for l in head_added):
    fail("State_Modes.cpp changed above the new owner beyond added includes")
if "".join(l for l in base_modes.splitlines(keepends=True) if not l.startswith("#include")).rstrip("\n") != \
        "".join(l for l in cur_modes_head.splitlines(keepends=True) if not l.startswith("#include")).rstrip("\n"):
    fail("State_Modes.cpp's existing functions differ from the base")


def moved_blocks(sleep_text, modes_text):
    """The route and the event owner mapped back to the base's inline region."""
    route = body_of(sleep_text, ROUTE_SIG)
    event = body_of(modes_text, EVENT_SIG)
    if route is None or event is None or route.count(EVENT_CALL) != 1 or not event.endswith(EVENT_TAIL):
        return None
    event = event[event.index(BASE_EVENT_START):-len(EVENT_TAIL)].replace("return true;", "return;")
    return route.lstrip("\n").replace(EVENT_CALL, event, 1)


# The moved blocks and the handler are byte-identical to the base apart from the listed edits.
cur_region = moved_blocks(sleep, modes)
if cur_region is None:
    fail("could not extract postWakeRouting() and processSleepWakeEvent()")
a = base_sleep.index(BASE_REGION_START)
b = base_sleep.index(BASE_REGION_END, a) + len(BASE_REGION_END)
base_region = base_sleep[a:b].replace(LED_CLOSE_OLD, LED_CLOSE_NEW)
if cur_region.rstrip("\n") != base_region.rstrip("\n"):
    fail("postWakeRouting() plus processSleepWakeEvent() differ from the base region beyond `return true;`/`return false;` "
         "and the call")
handler = body_of(sleep, HANDLER_SIG)
if handler.count(ROUTE_CALL) != 1:
    fail("the handler must call postWakeRouting() once")
if not handler.rstrip().endswith("if (sleepAndResume(wakeInSeconds, useNetworkStandby, sleepReason, wake)) return;\n" + ROUTE_CALL.rstrip("\n")):
    fail("the handler must end with the sleepAndResume call followed by the postWakeRouting call")

# The byte-diff check itself is shown to fire: one changed character inside each moved block is detected.
for old, new, which in (("sleep-timer-occupied-suppress-report", "sleep-timer-occupied-suppress-reportx", "sleep"),
                        ("sleep-pir-occupancy-report", "sleep-pir-occupancy-reportx", "modes")):
    s2, m2 = (sleep.replace(old, new, 1), modes) if which == "sleep" else (sleep, modes.replace(old, new, 1))
    t = moved_blocks(s2, m2)
    if (s2, m2) == (sleep, modes) or t is None or t.rstrip("\n") == base_region.rstrip("\n"):
        fail("a changed moved block was not detected")

print("sleep_wake_structural_test: ok")
<shellId: 24 completed with exit code 0>
```

</details>


---

<sub>5m 11s</sub>

### `bash`

**cd /private/tmp/gcc-step0/c2 && python3 - <<'EOF'
p='tests/sleep_wake_structural_test.py'
s=open(p).read()
o='''if base_modes is not None:
    base_modes = base_modes.replace(DEBOUNCE_OLD, "  if (debounceRemainingMs() == 0) {").replace(LED_CLOSE_OLD, LED_CLOSE_NEW)
'''
n='''# WO-2026-11-03-006: a piece's millis() start and trust flag are recorded where an occupancy session opens.
def c2_open(text, pad, anchor):
    lines = pad + "session.occupancyStartMs = millis();\\n" + pad + "session.pieceUntrusted = !Clock::isTrusted();\\n"
    return text.replace(anchor, anchor + lines, 1)
OPEN_AWAKE = "      CurrentReadings::set_occupancyStartTime(Time.now());\\n"
OPEN_WAKE = "          CurrentReadings::set_occupancyStartTime(Time.now());\\n"
if base_modes is not None:
    base_modes = base_modes.replace(DEBOUNCE_OLD, "  if (debounceRemainingMs() == 0) {").replace(LED_CLOSE_OLD, LED_CLOSE_NEW)
    base_modes = c2_open(c2_open(base_modes, "      ", OPEN_AWAKE), "          ", OPEN_WAKE)
'''
assert o in s; s=s.replace(o,n)
o2="base_region = base_sleep[a:b].replace(LED_CLOSE_OLD, LED_CLOSE_NEW)"
assert o2 in s
s=s.replace(o2,"base_region = c2_open(base_sleep[a:b].replace(LED_CLOSE_OLD, LED_CLOSE_NEW), \"          \", OPEN_WAKE)")
open(p,'w').write(s)
EOF
python3 tests/sleep_wake_structural_test.py 2>&1 | tail -5**

$ cd /private/tmp/gcc-step0/c2 && python3 - <<'EOF'
p='tests/sleep_wake_structural_test.py'
s=open(p).read()
o='''if base_modes is not None:
    base_modes = base_modes.replace(DEBOUNCE_OLD, "  if (debounceRemainingMs() == 0) {").replace(LED_CLOSE_OLD, LED_CLOSE_NEW)
'''
n='''# WO-2026-11-03-006: a piece's millis() start and trust flag are recorded where an occupancy session opens.
def c2_open(text, pad, anchor):
    lines = pad + "session.occupancyStartMs = millis();\\n" + pad + "session.pieceUntrusted = !Clock::isTrusted();\\n"
    return text.replace(anchor, anchor + lines, 1)
OPEN_AWAKE = "      CurrentReadings::set_occupancyStartTime(Time.now());\\n"
OPEN_WAKE = "          CurrentReadings::set_occupancyStartTime(Time.now());\\n"
if base_modes is not None:
    base_modes = base_modes.replace(DEBOUNCE_OLD, "  if (debounceRemainingMs() == 0) {").replace(LED_CLOSE_OLD, LED_CLOSE_NEW)
    base_modes = c2_open(c2_open(base_modes, "      ", OPEN_AWAKE), "          ", OPEN_WAKE)
'''
assert o in s; s=s.replace(o,n)
o2="base_region = base_sleep[a:b].replace(LED_CLOSE_OLD, LED_CLOSE_NEW)"
assert o2 in s
s=s.replace(o2,"base_region = c2_open(base_sleep[a:b].replace(LED_CLOSE_OLD, LED_CLOSE_NEW), \"          \", OPEN_WAKE)")
open(p,'w').write(s)
EOF
python3 tests/sleep_wake_structural_test.py 2>&1 | tail -5

```
sleep_wake_structural_test: ok
<shellId: 25 completed with exit code 0>
```


---

<sub>5m 23s</sub>

### `bash`

**cd /private/tmp/gcc-step0/c2 && python3 - <<'EOF'
p='src/Generalized-Core-Counter.cpp'
s=open(p).read()
o="(CurrentReadings::get_occupied() && !session.pieceUntrusted)\n"
assert o in s
s=s.replace(o,"CurrentReadings::get_occupied() && !session.pieceUntrusted\n")
open(p,'w').write(s)
EOF
git diff src/Generalized-Core-Counter.cpp; zsh tests/occupancy_session_restart_test.sh 2>&1 | tail -15**

$ cd /private/tmp/gcc-step0/c2 && python3 - <<'EOF'
p='src/Generalized-Core-Counter.cpp'
s=open(p).read()
o="(CurrentReadings::get_occupied() && !session.pieceUntrusted)\n"
assert o in s
s=s.replace(o,"CurrentReadings::get_occupied() && !session.pieceUntrusted\n")
open(p,'w').write(s)
EOF
git diff src/Generalized-Core-Counter.cpp; zsh tests/occupancy_session_restart_test.sh 2>&1 | tail -15

<details>
<summary>28 lines</summary>

```diff
diff --git a/src/Generalized-Core-Counter.cpp b/src/Generalized-Core-Counter.cpp
index 0420b63..0a829a4 100644
--- a/src/Generalized-Core-Counter.cpp
+++ b/src/Generalized-Core-Counter.cpp
@@ -500,7 +500,9 @@ void setup() {
   // was already open so the first trusted close can credit it (item B3). The
   // anchor is captured here, once: a report sent later in this boot advances
   // lastReport and must not move the cap.
-  session.occupancySessionBootAnchor = CurrentReadings::get_occupied()
+  session.pieceUntrusted = CurrentReadings::get_occupied() && !Clock::isTrusted();  // straddler: millis() start is boot (0)
+  if (session.pieceUntrusted) Log.info("Occ: untrusted straddler, pre-boot part dropped");
+  session.occupancySessionBootAnchor = CurrentReadings::get_occupied() && !session.pieceUntrusted
       ? std::max(CurrentReadings::get_occupancyStartTime(), SystemConfig::get_lastReport()) : 0;
   CurrentReadings::set_lastOccupancyEvent(0);
 
/var/folders/_r/m12xn8s15wb_15djjdgfp5gr0000gn/T//occupancy_session_restart_test.cpp:177:41: error: no member named 'pieceUntrusted' in 'SessionState'
        const bool piece = occupied && session.pieceUntrusted;
                                       ~~~~~~~ ^
/var/folders/_r/m12xn8s15wb_15djjdgfp5gr0000gn/T//occupancy_session_restart_test.cpp:211:76: error: no member named 'occupancyStartMs' in 'SessionState'
                rawSessionSeconds = (CurrentReadings::get_lastOccupancyEvent() - session.occupancyStartMs + occupancyDebounceMs()) / 1000UL;
                                                                                 ~~~~~~~ ^
/var/folders/_r/m12xn8s15wb_15djjdgfp5gr0000gn/T//occupancy_session_restart_test.cpp:213:100: error: no member named 'occupancyStartMs' in 'SessionState'
                Log.info("Occ: untrusted piece credit=%lld start=%lu", rawSessionSeconds, (unsigned long)session.occupancyStartMs);
                                                                                                         ~~~~~~~ ^
/var/folders/_r/m12xn8s15wb_15djjdgfp5gr0000gn/T//occupancy_session_restart_test.cpp:264:10: error: no member named 'pieceUntrusted' in 'SessionState'
        session.pieceUntrusted = false;
        ~~~~~~~ ^
4 errors generated.
<shellId: 26 completed with exit code 0>
```

</details>


---

<sub>5m 27s</sub>

### `bash`

**cd /private/tmp/gcc-step0/c2 && sed -n 120,300p tests/occupancy_session_restart_test.sh**

$ cd /private/tmp/gcc-step0/c2 && sed -n 120,300p tests/occupancy_session_restart_test.sh

<details>
<summary>162 lines</summary>

```
#include <ctime>
#include <iostream>
#include <string>
#include <vector>

// ----- Fake Device OS / application surface ----------------------------------

static unsigned long g_millis = 0;
unsigned long millis() { return g_millis; }

static time_t g_now = 0;
struct FakeTime {
  time_t now() const { return g_now; }
};
FakeTime Time;

struct FakeLog {
  std::vector<std::string> lines;
  void warn(const char *fmt, ...) {
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    lines.push_back(buf);
  }
  void info(const char *fmt, ...) { (void)fmt; }
  void trace(const char *fmt, ...) { (void)fmt; }
  int count(const char *needle) const {
    int n = 0;
    for (const std::string &l : lines) {
      if (l.find(needle) != std::string::npos) ++n;
    }
    return n;
  }
};
FakeLog Log;

namespace CurrentReadings {
bool occupied = false;
time_t occupancyStartTime = 0;
uint32_t totalOccupiedSeconds = 0;
uint32_t lastOccupancyEvent = 0;
bool get_occupied() { return occupied; }
void set_occupied(bool v) { occupied = v; }
time_t get_occupancyStartTime() { return occupancyStartTime; }
void set_occupancyStartTime(time_t v) { occupancyStartTime = v; }
uint32_t get_totalOccupiedSeconds() { return totalOccupiedSeconds; }
void set_totalOccupiedSeconds(uint32_t v) { totalOccupiedSeconds = v; }
uint32_t get_lastOccupancyEvent() { return lastOccupancyEvent; }
void set_lastOccupancyEvent(uint32_t v) { lastOccupancyEvent = v; }
}  // namespace CurrentReadings

namespace RecoveryState {
int8_t alertCode = 0;
int8_t get_alertCode() { return alertCode; }
}  // namespace RecoveryState

namespace Clock {
bool trusted = true;
bool isTrusted() { return trusted; }
// WO-2026-10-29-002: this test runs a normal-hours-Open site; the close-hour gate has its own test
// (occupancy_report_by_mode_test).
enum class Openness { Open, Closed, Unknown };
Openness openness() { return Openness::Open; }
}  // namespace Clock

namespace DailyBoundary {
struct Result {
  bool due;
  time_t boundary;
  uint8_t close;
};
Result check(time_t) { return {false, 0, 0}; }
}  // namespace DailyBoundary

namespace SystemConfig {
enum ConnectionMode { CONNECTED = 0, DISCONNECTED_KEEP_ALIVE = 2, INTERMITTENT_KEEP_ALIVE = 3 };
uint8_t connectionMode = INTERMITTENT_KEEP_ALIVE;
uint8_t get_configuredConnectionMode() { return connectionMode; }
time_t lastReport = 0;
time_t get_lastReport() { return lastReport; }
namespace SensorSettings {
uint32_t sensorSetting1 = 0;
uint32_t get_sensorSetting1() { return sensorSetting1; }
}  // namespace SensorSettings
}  // namespace SystemConfig

// This test never sets lowBatteryMode, so the mode in use equals the configured one.
struct PowerManager {
  static PowerManager &instance() {
    static PowerManager pm;
    return pm;
  }
  uint8_t effectiveConnectionMode() const { return SystemConfig::get_configuredConnectionMode(); }
};

namespace Config {
uint32_t occupancyDebounceMsForRuntime() { return 300000UL; }  // the shipped default, 5 minutes
}  // namespace Config

struct SessionState {
  time_t occupancySessionBootAnchor = 0;
  bool occupancyChangeTriggered = false;
};
SessionState session;

// updateOccupancyState()'s collaborators.
enum State { IDLE_STATE, REPORTING_STATE, SLEEPING_STATE };
State state = IDLE_STATE;
int transitionCount = 0;
void transitionTo(State next, const char *reason) {
  (void)reason;
  state = next;
  ++transitionCount;
}
bool ledOn = false;
void signalLED(bool on) { ledOn = on; }
int unoccupiedEvents = 0;
uint32_t lastLoggedSessionSeconds = 0;
void logUnoccupiedEvent(const char *why, uint32_t sessionSeconds, uint32_t totalSeconds, bool reportNow) {
  (void)why;
  (void)totalSeconds;
  (void)reportNow;
  ++unoccupiedEvents;
  lastLoggedSessionSeconds = sessionSeconds;
}
CPP

  echo ""
  echo "// ===== Extracted verbatim from src/state/State_Common.h ====="
  extract_braced_block "$common_src" "inline bool reportsOccupancyChangesNow() {"
  echo ""
  extract_braced_block "$common_src" "inline bool occupancyIgnoredWhenClosed() {"
  echo ""
  extract_braced_block "$common_src" "struct OccupancyCloseResult {"
  echo ""
  extract_braced_block "$common_src" "inline uint32_t occupancyDebounceMs() {"
  echo ""
  extract_braced_block "$common_src" "inline uint32_t debounceRemainingMs() {"
  echo ""
  extract_braced_block "$common_src" \
    "inline OccupancyCloseResult closeOccupancySessionSafely(const char *path, time_t closeAt = Time.now()) {"

  echo ""
  echo "// ===== Extracted verbatim from src/state/State_Modes.cpp ====="
  extract_braced_block "$modes_src" "void updateOccupancyState() {"

  cat <<'CPP'

// ----- Test driver ------------------------------------------------------------

const uint32_t kDebounceMs = 300000UL;   // Config::occupancyDebounceMsForRuntime()
const time_t kDebounceSec = 300;

// A device that has been occupied since `startEpoch` and has just rebooted:
// setup() cleared lastOccupancyEvent and captured the cross-boot anchor.
void bootMidSession(time_t startEpoch, time_t bootEpoch, unsigned long sinceBootMs, time_t lastReport) {
  Log.lines.clear();
  CurrentReadings::occupied = true;
  CurrentReadings::occupancyStartTime = startEpoch;
  CurrentReadings::totalOccupiedSeconds = 0;
  CurrentReadings::lastOccupancyEvent = 0;        // setup()'s clear (B1)
  SystemConfig::lastReport = lastReport;
  SystemConfig::SensorSettings::sensorSetting1 = 0;
  RecoveryState::alertCode = 0;
  Clock::trusted = true;
  session.occupancySessionBootAnchor = std::max(startEpoch, lastReport);  // setup()'s anchor (B3)
  session.occupancyChangeTriggered = false;
  state = IDLE_STATE;
  transitionCount = 0;
  unoccupiedEvents = 0;
  g_millis = sinceBootMs;
  g_now = bootEpoch + (time_t)(sinceBootMs / 1000UL);
}

// --- 1. The debounce period is the one the debounce checks use.
void testDebounceSourceIsShared() {
  SystemConfig::SensorSettings::sensorSetting1 = 0;
  assert(occupancyDebounceMs() == kDebounceMs);
  SystemConfig::SensorSettings::sensorSetting1 = 90000UL;
<shellId: 27 completed with exit code 0>
```

</details>


---

<sub>5m 34s</sub>

### `bash`

**cd /private/tmp/gcc-step0/c2 && python3 - <<'EOF'
p='tests/occupancy_session_restart_test.sh'
s=open(p).read()
o="  bool occupancyChangeTriggered = false;\n};\nSessionState session;"
assert s.count(o)==1
s=s.replace(o,"  bool occupancyChangeTriggered = false;\n  uint32_t occupancyStartMs = 0;\n  bool pieceUntrusted = false;\n};\nSessionState session;")
open(p,'w').write(s)
EOF
zsh tests/occupancy_session_restart_test.sh 2>&1 | tail -15; sed -n 300,470p tests/occupancy_session_restart_test.sh**

$ cd /private/tmp/gcc-step0/c2 && python3 - <<'EOF'
p='tests/occupancy_session_restart_test.sh'
s=open(p).read()
o="  bool occupancyChangeTriggered = false;\n};\nSessionState session;"
assert s.count(o)==1
s=s.replace(o,"  bool occupancyChangeTriggered = false;\n  uint32_t occupancyStartMs = 0;\n  bool pieceUntrusted = false;\n};\nSessionState session;")
open(p,'w').write(s)
EOF
zsh tests/occupancy_session_restart_test.sh 2>&1 | tail -15; sed -n 300,470p tests/occupancy_session_restart_test.sh

<details>
<summary>158 lines</summary>

```
Occupancy session restart test passed (mid-session restart credited and capped, untrusted clock keeps the session open)
  SystemConfig::SensorSettings::sensorSetting1 = 0;
  assert(occupancyDebounceMs() == kDebounceMs);
  SystemConfig::SensorSettings::sensorSetting1 = 90000UL;
  assert(occupancyDebounceMs() == 90000UL);
  SystemConfig::SensorSettings::sensorSetting1 = 0;
}

// --- 2. B3: a mid-session restart credits up to the moment this boot started,
//        when that is earlier than anchor + debounce. The old code credited
//        through to the close time, counting the power-off as occupancy.
void testRestartCreditsToBootTimeWhenEarlierThanTheCap() {
  // Occupied since 1000. The device restarted at 1100 (100 s of real session)
  // and the first trusted close happens 50 s into the new boot.
  bootMidSession(/*startEpoch=*/1000, /*bootEpoch=*/1100, /*sinceBootMs=*/50000, /*lastReport=*/0);
  // anchor = max(1000, 0) = 1000, anchor + debounce = 1300; bootEpoch = 1100.
  const OccupancyCloseResult r = closeOccupancySessionSafely("test");
  assert(r.valid);
  assert(!r.stillOpen);
  assert(r.sessionSeconds == 100);        // 1000 -> 1100, the boot moment
  assert(r.totalSeconds == 100);
  assert(!CurrentReadings::occupied);     // a trusted close really closes
  assert(CurrentReadings::occupancyStartTime == 0);
  assert(Log.count("OccAnom") == 0);
  // The flag is one-shot: a later session in the same boot is not capped.
  assert(session.occupancySessionBootAnchor == 0);
}

// --- 3. B3: the cap bites when the device was off for longer than a debounce.
void testRestartIsCappedAtAnchorPlusOneDebounce() {
  // Occupied since 1000, the device was off for a while and booted at 2000.
  bootMidSession(/*startEpoch=*/1000, /*bootEpoch=*/2000, /*sinceBootMs=*/50000, /*lastReport=*/0);
  const OccupancyCloseResult r = closeOccupancySessionSafely("test");
  assert(r.valid);
  assert(r.sessionSeconds == (uint32_t)kDebounceSec);   // 1000 -> 1300
  assert(Log.count("OccAnom") == 0);
}

// --- 4. B-anchor: lastReport later than the session start moves the anchor,
//        because a report proves the session was still open at that moment.
void testLastReportMovesTheAnchor() {
  bootMidSession(/*startEpoch=*/1000, /*bootEpoch=*/9000, /*sinceBootMs=*/50000, /*lastReport=*/5000);
  // anchor = max(1000, 5000) = 5000, so the credit runs 1000 -> 5300.
  const OccupancyCloseResult r = closeOccupancySessionSafely("test");
  assert(r.valid);
  assert(r.sessionSeconds == (uint32_t)(5000 + kDebounceSec - 1000));
  assert(r.sessionSeconds == 4300);

  // A lastReport EARLIER than the start must not move the anchor backwards.
  bootMidSession(/*startEpoch=*/1000, /*bootEpoch=*/9000, /*sinceBootMs=*/50000, /*lastReport=*/400);
  const OccupancyCloseResult r2 = closeOccupancySessionSafely("test");
  assert(r2.valid);
  assert(r2.sessionSeconds == (uint32_t)kDebounceSec);  // anchor stayed at 1000
}

// --- 5. B3 acceptance: a long power-off over-credits by at most one debounce,
//        however long the device was off.
void testLongPowerOffOverCreditsAtMostOneDebounce() {
  for (const time_t offSeconds : {(time_t)600, (time_t)3600, (time_t)36000, (time_t)864000}) {
    const time_t start = 1000;
    const time_t lastKnownOpen = start;                 // the anchor
    bootMidSession(start, /*bootEpoch=*/start + offSeconds, /*sinceBootMs=*/50000, /*lastReport=*/0);
    const OccupancyCloseResult r = closeOccupancySessionSafely("test");
    assert(r.valid);
    const time_t creditedThrough = start + (time_t)r.sessionSeconds;
    const time_t overCredit = creditedThrough - lastKnownOpen;
    assert(overCredit <= kDebounceSec);
  }
  // Ten hours off: exactly one debounce of over-credit, not ten hours of it.
  bootMidSession(1000, /*bootEpoch=*/1000 + 36000, /*sinceBootMs=*/50000, /*lastReport=*/0);
  const OccupancyCloseResult r = closeOccupancySessionSafely("test");
  assert(r.sessionSeconds == (uint32_t)kDebounceSec);
}

// --- 6. The cap applies to the FIRST trusted close only. A session that
//        started cleanly in this boot is credited in full.
void testNormalCloseIsNotCapped() {
  bootMidSession(/*startEpoch=*/1000, /*bootEpoch=*/1000, /*sinceBootMs=*/50000, /*lastReport=*/0);
  session.occupancySessionBootAnchor = 0;               // no session was open at boot
  CurrentReadings::occupancyStartTime = 1000;
  g_now = 1000 + 3600;                                  // a one-hour session
  const OccupancyCloseResult r = closeOccupancySessionSafely("test");
  assert(r.valid);
  assert(r.sessionSeconds == 3600);                     // the whole hour, uncapped
}

// --- 7. B2: an untrusted clock keeps the session open, credits nothing, and
//        re-arms the debounce. The old code cleared the session instead.
void testUntrustedClockKeepsTheSessionOpen() {
  bootMidSession(/*startEpoch=*/1000, /*bootEpoch=*/1100, /*sinceBootMs=*/50000, /*lastReport=*/0);
  CurrentReadings::totalOccupiedSeconds = 777;
  Clock::trusted = false;

  const OccupancyCloseResult r = closeOccupancySessionSafely("idle");
  assert(!r.valid);
  assert(r.stillOpen);
  assert(r.sessionSeconds == 0);
  assert(CurrentReadings::occupied);                              // still open
  assert(CurrentReadings::occupancyStartTime == 1000);            // start preserved
  assert(CurrentReadings::totalOccupiedSeconds == 777);           // nothing credited
  assert(CurrentReadings::lastOccupancyEvent == (uint32_t)g_millis);  // debounce re-armed
  assert(Log.count("OccAnom") == 0);                              // not an anomaly
  // The cross-boot flag survives, so the credit still happens at the first
  // close that IS trusted.
  assert(session.occupancySessionBootAnchor != 0);

  Clock::trusted = true;
  g_millis += kDebounceMs;
  g_now += (time_t)kDebounceSec;
  const OccupancyCloseResult r2 = closeOccupancySessionSafely("idle");
  assert(r2.valid);
  assert(!r2.stillOpen);
  assert(!CurrentReadings::occupied);
  assert(r2.sessionSeconds == 100);   // still capped at the boot moment, 1000 -> 1100
}

// --- 8. B2: no report storm and no repeated unoccupied transition while the
//        clock is untrusted. This drives the REAL updateOccupancyState().
void testUntrustedClockCausesNoReportStorm() {
  bootMidSession(/*startEpoch=*/1000, /*bootEpoch=*/1100, /*sinceBootMs=*/1000, /*lastReport=*/0);
  Clock::trusted = false;
  SystemConfig::connectionMode = SystemConfig::INTERMITTENT_KEEP_ALIVE;

  // B1: lastOccupancyEvent is 0 after the boot clear, so the first pass
  // re-arms it from THIS boot's millis() rather than expiring immediately on
  // the previous boot's value.
  updateOccupancyState();
  assert(CurrentReadings::lastOccupancyEvent == 1000);
  assert(transitionCount == 0);
  assert(CurrentReadings::occupied);

  // Run for an hour of simulated time, a loop pass every 10 s.
  for (int i = 0; i < 360; ++i) {
    g_millis += 10000;
    g_now += 10;
    updateOccupancyState();
  }
  assert(CurrentReadings::occupied);          // never thrown away
  assert(transitionCount == 0);               // no REPORTING_STATE storm
  assert(unoccupiedEvents == 0);              // no unoccupied transition signalled
  assert(Log.count("OccAnom") == 0);          // no OccAnom storm
  assert(CurrentReadings::totalOccupiedSeconds == 0);

  // Each attempt re-armed the debounce, so attempts are one debounce apart,
  // not once per loop pass.
  assert(CurrentReadings::lastOccupancyEvent > kDebounceMs);

  // Once the clock is trusted, the very next expiry closes and reports.
  Clock::trusted = true;
  g_millis += kDebounceMs + 1;
  g_now += (time_t)kDebounceSec + 1;
  updateOccupancyState();
  assert(!CurrentReadings::occupied);
  assert(unoccupiedEvents == 1);
  assert(transitionCount == 1);
  assert(state == REPORTING_STATE);
  assert(session.occupancyChangeTriggered);
  // 1000 -> the boot moment (1100), the capped credit. bootEpoch is recomputed
  // at close time as Time.now() - millis()/1000, so the integer-second floor
  // can land one second either side.
  assert(lastLoggedSessionSeconds >= 100 && lastLoggedSessionSeconds <= 101);
}

// --- 9. The pre-existing safety checks are untouched: a zero or future start
//        is still an anomaly, and is still cleared.
void testExistingAnomalyHandlingIsUnchanged() {
  bootMidSession(/*startEpoch=*/0, /*bootEpoch=*/1100, /*sinceBootMs=*/50000, /*lastReport=*/0);
  session.occupancySessionBootAnchor = 0;
  const OccupancyCloseResult zeroStart = closeOccupancySessionSafely("zero");
  assert(!zeroStart.valid);
  assert(!zeroStart.stillOpen);
  assert(!CurrentReadings::occupied);
<shellId: 28 completed with exit code 0>
```

</details>


---

<sub>5m 53s</sub>

### `bash`

**cd /private/tmp/gcc-step0/c2 && grep -rn "570\|exerciser" tests/exerciser_baseline_test.py | head -12; grep -n "occupancyDebounceMsForRuntime" -B2 -A6 src/Config.h | head -30; grep -rn "pulse" docs/work-orders/WO-2026-11-03-004-item19-c1-millis-expiry.md | head -5**

$ cd /private/tmp/gcc-step0/c2 && grep -rn "570\|exerciser" tests/exerciser_baseline_test.py | head -12; grep -n "occupancyDebounceMsForRuntime" -B2 -A6 src/Config.h | head -30; grep -rn "pulse" docs/work-orders/WO-2026-11-03-004-item19-c1-millis-expiry.md | head -5

<details>
<summary>13 lines</summary>

```
tests/exerciser_baseline_test.py:2:"""Known-answer tests for tools/exerciser_baseline.py (WO-2026-10-23-001); hook JSON is generated here.
tests/exerciser_baseline_test.py:26:SCRIPT = os.path.join(HERE, "..", "tools", "exerciser_baseline.py")
tests/exerciser_baseline_test.py:113:    print("exerciser_baseline_test:", "FAIL" if bad else "PASS")
68-bool isValid(bool logFailures, const char **failureReason);
69-uint16_t reportingIntervalSecForRuntime();
70:uint32_t occupancyDebounceMsForRuntime();
71-
72-void markLedgerConfigurationValid();
73-void markStorageConfigurationLoaded();
74-void markFactoryDefaultsActive();
75-
76-void logDiagnostics(const char *tag = "ConfigDiag");
<shellId: 29 completed with exit code 0>
```

</details>


---

<sub>6m 35s</sub>

### `bash`

**cd /private/tmp/gcc-step0/c2 && sed -n 470,570p tests/occupancy_session_restart_test.sh; grep -n "reportsOccupancyChangesNow\|occupancyIgnoredWhenClosed" -A12 src/state/State_Common.h | sed -n 1,50p**

$ cd /private/tmp/gcc-step0/c2 && sed -n 470,570p tests/occupancy_session_restart_test.sh; grep -n "reportsOccupancyChangesNow\|occupancyIgnoredWhenClosed" -A12 src/state/State_Common.h | sed -n 1,50p

<details>
<summary>124 lines</summary>

```
assert(!CurrentReadings::occupied);
  assert(Log.count("OccAnom") == 1);

  bootMidSession(/*startEpoch=*/0, /*bootEpoch=*/1100, /*sinceBootMs=*/50000, /*lastReport=*/0);
  session.occupancySessionBootAnchor = 0;
  CurrentReadings::occupancyStartTime = g_now + 600;   // well in the future
  const OccupancyCloseResult future = closeOccupancySessionSafely("future");
  assert(!future.valid);
  assert(!future.stillOpen);
  assert(!CurrentReadings::occupied);
  assert(Log.count("OccAnom") == 1);

  // A session longer than a day is still rejected.
  bootMidSession(/*startEpoch=*/1000, /*bootEpoch=*/1000, /*sinceBootMs=*/50000, /*lastReport=*/0);
  session.occupancySessionBootAnchor = 0;
  g_now = 1000 + 90000;
  const OccupancyCloseResult tooLong = closeOccupancySessionSafely("long");
  assert(!tooLong.valid);
  assert(!CurrentReadings::occupied);
  assert(Log.count("OccAnom") == 1);
}

// --- 10. A boundary-aware close (the daily cleanup) still honours its
//         explicit closeAt, and is capped the same way when it is the first
//         trusted close after a restart.
void testExplicitCloseAtStillWins() {
  bootMidSession(/*startEpoch=*/1000, /*bootEpoch=*/1100, /*sinceBootMs=*/50000, /*lastReport=*/0);
  session.occupancySessionBootAnchor = 0;
  const OccupancyCloseResult r = closeOccupancySessionSafely("daily-cleanup", 1050);
  assert(r.valid);
  assert(r.sessionSeconds == 50);

  // With the cross-boot flag set, a boundary later than the cap is capped.
  bootMidSession(/*startEpoch=*/1000, /*bootEpoch=*/2000, /*sinceBootMs=*/50000, /*lastReport=*/0);
  const OccupancyCloseResult capped = closeOccupancySessionSafely("daily-cleanup", 2040);
  assert(capped.valid);
  assert(capped.sessionSeconds == (uint32_t)kDebounceSec);

  // ... and a boundary EARLIER than the cap still wins, because the cap is a
  // ceiling, not a target.
  bootMidSession(/*startEpoch=*/1000, /*bootEpoch=*/2000, /*sinceBootMs=*/50000, /*lastReport=*/0);
  const OccupancyCloseResult early = closeOccupancySessionSafely("daily-cleanup", 1100);
  assert(early.valid);
  assert(early.sessionSeconds == 100);
}

// --- 11. Stage 7 regression: a report is sent after the restart but before
//         the clock is trusted. That report advances lastReport; the credit
//         must still be capped at the BOOT-TIME anchor plus one debounce.
//         Codex's reproduction: 300 s debounce, session from 1000, last report
//         and power-off at 5000, ten hours off, boot at 41000.
void testReportAfterRestartBeforeTrustDoesNotMoveTheCap() {
  const time_t start = 1000;
  const time_t lastKnownOpen = 5000;   // the last report before the power-off
  bootMidSession(start, /*bootEpoch=*/lastKnownOpen + 36000, /*sinceBootMs=*/10000, /*lastReport=*/lastKnownOpen);

  // Untrusted: a close attempt keeps the session open and credits nothing.
  Clock::trusted = false;
  const OccupancyCloseResult untrusted = closeOccupancySessionSafely("idle");
  assert(untrusted.stillOpen);
  assert(CurrentReadings::totalOccupiedSeconds == 0);

  // A scheduled report goes out after the boot, still untrusted.
  g_millis += 40000;
  g_now += 40;
  SystemConfig::lastReport = g_now;    // 41050, the State_Report.cpp lastReport write

  // The clock becomes trusted and the session closes.
  Clock::trusted = true;
  g_millis += kDebounceMs;
  g_now += (time_t)kDebounceSec;
  const OccupancyCloseResult r = closeOccupancySessionSafely("idle");
  assert(r.valid);
  assert(r.sessionSeconds == 4300);    // 1000 -> 5000 + 300, not 1000 -> 41000
  const time_t overCredit = start + (time_t)r.sessionSeconds - lastKnownOpen;
  assert(overCredit <= kDebounceSec);
}

int main() {
  testDebounceSourceIsShared();
  testRestartCreditsToBootTimeWhenEarlierThanTheCap();
  testRestartIsCappedAtAnchorPlusOneDebounce();
  testLastReportMovesTheAnchor();
  testLongPowerOffOverCreditsAtMostOneDebounce();
  testNormalCloseIsNotCapped();
  testUntrustedClockKeepsTheSessionOpen();
  testUntrustedClockCausesNoReportStorm();
  testExistingAnomalyHandlingIsUnchanged();
  testExplicitCloseAtStillWins();
  testReportAfterRestartBeforeTrustDoesNotMoveTheCap();

  std::cout << "Occupancy session restart test passed "
               "(mid-session restart credited and capped, untrusted clock keeps the session open)\n";
  return 0;
}
CPP
} > "$generated"

clang++ -std=c++17 -Wall -Wextra -pedantic \
  "$generated" \
  -o "$binary"
56:inline bool reportsOccupancyChangesNow() {
57-	const uint8_t mode = PowerManager::instance().effectiveConnectionMode();
58-	return mode == SystemConfig::INTERMITTENT_KEEP_ALIVE || mode == SystemConfig::CONNECTED;
59-}
60-
61-/// WO-2026-10-29-002: occupancy ends at the daily close. `== Closed` only:
62-/// Unknown (untrusted clock) and 0-24 sites (never Closed) behave as before.
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
--
379:	if (occupancyIgnoredWhenClosed()) {
380-		const time_t boundary = DailyBoundary::check(now).boundary;
381-		if (start < boundary) closeAt = std::min(closeAt, boundary);
382-	}
383-	// WO-2026-10-04-001 item B3: the first trusted close of a session that was
384-	// already open at boot credits no later than this boot started, and no
385-	// later than one debounce past the last evidence the session was open.
386-	if (session.occupancySessionBootAnchor != 0) {
387-		const time_t bootEpoch = now - (time_t)(millis() / 1000UL);
388-		const time_t anchor = session.occupancySessionBootAnchor;
389-		session.occupancySessionBootAnchor = 0;
390-		closeAt = std::min(closeAt, std::min(bootEpoch, anchor + (time_t)(occupancyDebounceMs() / 1000UL)));
391-	}
<shellId: 30 completed with exit code 0>
```

</details>


---

<sub>6m 43s</sub>

### `bash`

**cd /private/tmp/gcc-step0/c2 && python3 - <<'EOF'
p='tests/occupancy_session_restart_test.sh'
s=open(p).read()
def sub(o,n):
    global s
    assert s.count(o)==1,o; s=s.replace(o,n)
sub("  session.occupancyChangeTriggered = false;\n  state = IDLE_STATE;","  session.pieceUntrusted = false;  // WO-2026-11-03-006: B2/B3 below model a piece that opened trusted; the untrusted piece is untrusted_piece_c2_test.sh\n  session.occupancyStartMs = 0;\n  session.occupancyChangeTriggered = false;\n  state = IDLE_STATE;")
sub("// --- 7. B2: an untrusted clock keeps the session open,","// --- 7. B2 (a piece that opened trusted and then loses trust): an untrusted clock keeps the session open,")
open(p,'w').write(s)
EOF
zsh tests/occupancy_session_restart_test.sh 2>&1 | tail -3**

$ cd /private/tmp/gcc-step0/c2 && python3 - <<'EOF'
p='tests/occupancy_session_restart_test.sh'
s=open(p).read()
def sub(o,n):
    global s
    assert s.count(o)==1,o; s=s.replace(o,n)
sub("  session.occupancyChangeTriggered = false;\n  state = IDLE_STATE;","  session.pieceUntrusted = false;  // WO-2026-11-03-006: B2/B3 below model a piece that opened trusted; the untrusted piece is untrusted_piece_c2_test.sh\n  session.occupancyStartMs = 0;\n  session.occupancyChangeTriggered = false;\n  state = IDLE_STATE;")
sub("// --- 7. B2: an untrusted clock keeps the session open,","// --- 7. B2 (a piece that opened trusted and then loses trust): an untrusted clock keeps the session open,")
open(p,'w').write(s)
EOF
zsh tests/occupancy_session_restart_test.sh 2>&1 | tail -3

```
Occupancy session restart test passed (mid-session restart credited and capped, untrusted clock keeps the session open)
<shellId: 31 completed with exit code 0>
```


---

<sub>7m 8s</sub>

### `create`

**create a new file at /private/tmp/gcc-step0/c2/tests/untrusted_piece_c2_test.sh.**

/private/tmp/gcc-step0/c2/tests/untrusted_piece_c2_test.sh

```
Created file /private/tmp/gcc-step0/c2/tests/untrusted_piece_c2_test.sh with 9118 characters
```


---

<sub>7m 40s</sub>

### `bash`

**cd /private/tmp/gcc-step0/c2 && python3 - <<'EOF'
p='tests/untrusted_piece_c2_test.sh'
s=open(p).read()
a=s.index("// Dev-09: a 570 s session every 20 min")
b=s.index("  clang++ -std=c++17 -w")
new='''// Dev-09 timeline: a session every 20 min; ten PIR pulses 30 s apart (270 s) then a 300 s debounce = 570 s.
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
  session_of(10);                                        // pulses to 270 s, then 30 s more
  g_millis = 0; g_now += 0;
  // reboot at t=300 s of the piece (persisted open, untrusted clock): millis() restarts
  reset();
  run(1000);
  assert(!CurrentReadings::occupied);
  // no PIR after the reboot: the debounce re-arms at the first loop pass (1 s) and runs 300 s => 301 s, not 570 s
  assert(CurrentReadings::total == 301);
  assert(Log.count("Occ: untrusted piece credit=301") == 1);

  // The WO's 236 s case: 60 s debounce, PIR pulses continue 176 s after the reboot.
  world(60000);
  for (int i = 0; i < 11; ++i) { pir(); run(30); }       // 0..330 s, session open
  reset();
  for (int t = 26; t <= 176; t += 30) { run(t - 0 - (int)(g_millis / 1000)); pir(); }
  run(1000);
  assert(CurrentReadings::total == 236);                 // 176 + 60; 536 would be a one-debounce fallback
}

// Trust returns while the piece is open: the wall clock jumps, the credit stays on millis().
void testPieceOpenedUntrustedKeepsMillisWhenTrustReturns() {
  world(300000);
  pir(); run(60);
  Clock::trusted = true; g_now += 36000;                 // the sync step
  run(1000);
  assert(!CurrentReadings::occupied);
  assert(CurrentReadings::total == 360);                 // 60 + 300, not 36360
  assert(session.pieceUntrusted == false);
  // ... and the next piece, opened trusted, is a wall piece: the same code credits its wall span.
  pir(); run(60); g_now += 100; run(1000);
  assert(CurrentReadings::total == 360 + 760 - 0 - 0 + 0 - 0 + 0 - 0 + 0 - 0);
}

// A piece opened trusted is unchanged: it credits the wall span (a wall step shows through).
void testTrustedPieceUnchanged() {
  world(300000);
  Clock::trusted = true;
  pir(); run(60); g_now += 100; run(1000);
  assert(!CurrentReadings::occupied);
  assert(CurrentReadings::total == 460);                 // wall: 60 + 100 + 300; millis would say 360
  assert(Log.count("untrusted piece") == 0);
}

// A fresh piece after each close: its start is its own (millis and flag are not carried over).
void testNextPieceStartsClean() {
  world(300000);
  pir(); run(400);
  assert(!CurrentReadings::occupied && CurrentReadings::total == 300);
  run(5000);
  const uint32_t before = session.occupancyStartMs;
  pir(); run(400);
  assert(session.occupancyStartMs > before + 5000000UL);
  assert(CurrentReadings::total == 600);                 // not 300 + 5300
}

// A mode that does not report on occupancy changes still credits, without a REPORTING transition.
void testNoReportInQuietModes() {
  world(300000);
  SystemConfig::connectionMode = SystemConfig::DISCONNECTED_KEEP_ALIVE;
  pir(); run(400);
  assert(CurrentReadings::total == 300 && reports == 0 && unoccupied == 1);
  SystemConfig::connectionMode = SystemConfig::INTERMITTENT_KEEP_ALIVE;
}

// Trust returned mid-piece and the day's boundary is due: the explicit-closeAt close still credits on-time.
void testBoundaryCloseOfAnUntrustedPiece() {
  world(300000);
  pir(); run(100);
  Clock::trusted = true; Clock::open = Clock::Openness::Closed; DailyBoundary::boundary = g_now - 50;
  const OccupancyCloseResult r = closeOccupancySessionSafely("daily-cleanup", DailyBoundary::boundary);
  assert(r.valid && !r.stillOpen && !CurrentReadings::occupied);
  assert(r.sessionSeconds == 300);                       // measured (last PIR to expiry), not the wall span
  Clock::open = Clock::Openness::Unknown;
}

int main() {
  testControlCreditsEachPieceItsOwnOnTime();
  testRebootStraddlerCreditsPostBootOnly();
  testPieceOpenedUntrustedKeepsMillisWhenTrustReturns();
  testTrustedPieceUnchanged();
  testNextPieceStartsClean();
  testNoReportInQuietModes();
  testBoundaryCloseOfAnUntrustedPiece();
  std::cout << "driver ok\\n";
  return 0;
}
CPP
'''
s=s[:a]+new+s[b:]
open(p,'w').write(s)
EOF
zsh tests/untrusted_piece_c2_test.sh**

$ cd /private/tmp/gcc-step0/c2 && python3 - <<'EOF'
p='tests/untrusted_piece_c2_test.sh'
s=open(p).read()
a=s.index("// Dev-09: a 570 s session every 20 min")
b=s.index("  clang++ -std=c++17 -w")
new='''// Dev-09 timeline: a session every 20 min; ten PIR pulses 30 s apart (270 s) then a 300 s debounce = 570 s.
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
  session_of(10);                                        // pulses to 270 s, then 30 s more
  g_millis = 0; g_now += 0;
  // reboot at t=300 s of the piece (persisted open, untrusted clock): millis() restarts
  reset();
  run(1000);
  assert(!CurrentReadings::occupied);
  // no PIR after the reboot: the debounce re-arms at the first loop pass (1 s) and runs 300 s => 301 s, not 570 s
  assert(CurrentReadings::total == 301);
  assert(Log.count("Occ: untrusted piece credit=301") == 1);

  // The WO's 236 s case: 60 s debounce, PIR pulses continue 176 s after the reboot.
  world(60000);
  for (int i = 0; i < 11; ++i) { pir(); run(30); }       // 0..330 s, session open
  reset();
  for (int t = 26; t <= 176; t += 30) { run(t - 0 - (int)(g_millis / 1000)); pir(); }
  run(1000);
  assert(CurrentReadings::total == 236);                 // 176 + 60; 536 would be a one-debounce fallback
}

// Trust returns while the piece is open: the wall clock jumps, the credit stays on millis().
void testPieceOpenedUntrustedKeepsMillisWhenTrustReturns() {
  world(300000);
  pir(); run(60);
  Clock::trusted = true; g_now += 36000;                 // the sync step
  run(1000);
  assert(!CurrentReadings::occupied);
  assert(CurrentReadings::total == 360);                 // 60 + 300, not 36360
  assert(session.pieceUntrusted == false);
  // ... and the next piece, opened trusted, is a wall piece: the same code credits its wall span.
  pir(); run(60); g_now += 100; run(1000);
  assert(CurrentReadings::total == 360 + 760 - 0 - 0 + 0 - 0 + 0 - 0 + 0 - 0);
}

// A piece opened trusted is unchanged: it credits the wall span (a wall step shows through).
void testTrustedPieceUnchanged() {
  world(300000);
  Clock::trusted = true;
  pir(); run(60); g_now += 100; run(1000);
  assert(!CurrentReadings::occupied);
  assert(CurrentReadings::total == 460);                 // wall: 60 + 100 + 300; millis would say 360
  assert(Log.count("untrusted piece") == 0);
}

// A fresh piece after each close: its start is its own (millis and flag are not carried over).
void testNextPieceStartsClean() {
  world(300000);
  pir(); run(400);
  assert(!CurrentReadings::occupied && CurrentReadings::total == 300);
  run(5000);
  const uint32_t before = session.occupancyStartMs;
  pir(); run(400);
  assert(session.occupancyStartMs > before + 5000000UL);
  assert(CurrentReadings::total == 600);                 // not 300 + 5300
}

// A mode that does not report on occupancy changes still credits, without a REPORTING transition.
void testNoReportInQuietModes() {
  world(300000);
  SystemConfig::connectionMode = SystemConfig::DISCONNECTED_KEEP_ALIVE;
  pir(); run(400);
  assert(CurrentReadings::total == 300 && reports == 0 && unoccupied == 1);
  SystemConfig::connectionMode = SystemConfig::INTERMITTENT_KEEP_ALIVE;
}

// Trust returned mid-piece and the day's boundary is due: the explicit-closeAt close still credits on-time.
void testBoundaryCloseOfAnUntrustedPiece() {
  world(300000);
  pir(); run(100);
  Clock::trusted = true; Clock::open = Clock::Openness::Closed; DailyBoundary::boundary = g_now - 50;
  const OccupancyCloseResult r = closeOccupancySessionSafely("daily-cleanup", DailyBoundary::boundary);
  assert(r.valid && !r.stillOpen && !CurrentReadings::occupied);
  assert(r.sessionSeconds == 300);                       // measured (last PIR to expiry), not the wall span
  Clock::open = Clock::Openness::Unknown;
}

int main() {
  testControlCreditsEachPieceItsOwnOnTime();
  testRebootStraddlerCreditsPostBootOnly();
  testPieceOpenedUntrustedKeepsMillisWhenTrustReturns();
  testTrustedPieceUnchanged();
  testNextPieceStartsClean();
  testNoReportInQuietModes();
  testBoundaryCloseOfAnUntrustedPiece();
  std::cout << "driver ok\\n";
  return 0;
}
CPP
'''
s=s[:a]+new+s[b:]
open(p,'w').write(s)
EOF
zsh tests/untrusted_piece_c2_test.sh

```
FAILED: both piece-open sites must set occupancyStartMs = millis() and pieceUntrusted = !Clock::isTrusted()
<shellId: 32 completed with exit code 1>
```


---

<sub>7m 45s</sub>

### `bash`

**cd /private/tmp/gcc-step0/c2 && python3 - <<'EOF'
p='tests/untrusted_piece_c2_test.sh'
s=open(p).read()
o='set_occupied\\(true\\);\\n\\s*CurrentReadings'
assert o in s
s=s.replace(o,'set_occupied\\(true\\);\\n(?:\\s*//.*\\n)*\\s*CurrentReadings')
open(p,'w').write(s)
EOF
zsh tests/untrusted_piece_c2_test.sh**

$ cd /private/tmp/gcc-step0/c2 && python3 - <<'EOF'
p='tests/untrusted_piece_c2_test.sh'
s=open(p).read()
o='set_occupied\\(true\\);\\n\\s*CurrentReadings'
assert o in s
s=s.replace(o,'set_occupied\\(true\\);\\n(?:\\s*//.*\\n)*\\s*CurrentReadings')
open(p,'w').write(s)
EOF
zsh tests/untrusted_piece_c2_test.sh

```
tests/untrusted_piece_c2_test.sh:279: parse error near `\n'
<shellId: 33 completed with exit code 1>
```


---

<sub>7m 48s</sub>

### `bash`

**cd /private/tmp/gcc-step0/c2 && sed -n 100,125p tests/untrusted_piece_c2_test.sh; tail -12 tests/untrusted_piece_c2_test.sh**

$ cd /private/tmp/gcc-step0/c2 && sed -n 100,125p tests/untrusted_piece_c2_test.sh; tail -12 tests/untrusted_piece_c2_test.sh

<details>
<summary>39 lines</summary>

```
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
  testPieceOpenedUntrustedKeepsMillisWhenTrustReturns();
  testTrustedPieceUnchanged();
  testNextPieceStartsClean();
  testNoReportInQuietModes();
  testBoundaryCloseOfAnUntrustedPiece();
  std::cout << "driver ok\n";
  return 0;
}
CPP
  clang++ -std=c++17 -w -o "$work/driver" "$out" && "$work/driver"
}
build_and_run "$src/state/State_Common.h" "$src/state/State_Modes.cpp"
<shellId: 34 completed with exit code 0>
```

</details>


---

<sub>7m 54s</sub>

### `bash`

**cd /private/tmp/gcc-step0/c2 && python3 - <<'EOF'
p='tests/untrusted_piece_c2_test.sh'
s=open(p).read()
s=s.replace("}\nCPP\n  clang++","}\nCPP\n  } > \"$out\"\n  clang++")
open(p,'w').write(s)
EOF
zsh tests/untrusted_piece_c2_test.sh**

$ cd /private/tmp/gcc-step0/c2 && python3 - <<'EOF'
p='tests/untrusted_piece_c2_test.sh'
s=open(p).read()
s=s.replace("}\nCPP\n  clang++","}\nCPP\n  } > \"$out\"\n  clang++")
open(p,'w').write(s)
EOF
zsh tests/untrusted_piece_c2_test.sh

```
Assertion failed: (unoccupied == 10 && reports == 10), function testControlCreditsEachPieceItsOwnOnTime, file driver.cpp, line 263.
<shellId: 35 completed with exit code 134>
```


---

<sub>8m 4s</sub>

### `bash`

**cd /private/tmp/gcc-step0/c2 && python3 - <<'EOF'
p='tests/untrusted_piece_c2_test.sh'
s=open(p).read()
def sub(o,n):
    global s
    assert s.count(o)==1,o; s=s.replace(o,n)
sub("updateOccupancyState(); } }\nvoid reset()","updateOccupancyState(); state = IDLE_STATE; } }\nvoid reset()")
sub("  g_millis = 0; g_now += 0;\n  // reboot at t=300 s of the piece (persisted open, untrusted clock): millis() restarts\n","  // reboot 300 s into the piece (persisted open, untrusted clock): millis() restarts\n")
sub("  for (int t = 26; t <= 176; t += 30) { run(t - 0 - (int)(g_millis / 1000)); pir(); }\n","  for (int i = 0; i < 6; ++i) { run(i == 0 ? 26 : 30); pir(); }   // post-boot PIRs at 26..176 s\n")
sub("  assert(CurrentReadings::total == 360 + 760 - 0 - 0 + 0 - 0 + 0 - 0 + 0 - 0);","  assert(CurrentReadings::total == 360 + 460);")
open(p,'w').write(s)
EOF
zsh tests/untrusted_piece_c2_test.sh**

$ cd /private/tmp/gcc-step0/c2 && python3 - <<'EOF'
p='tests/untrusted_piece_c2_test.sh'
s=open(p).read()
def sub(o,n):
    global s
    assert s.count(o)==1,o; s=s.replace(o,n)
sub("updateOccupancyState(); } }\nvoid reset()","updateOccupancyState(); state = IDLE_STATE; } }\nvoid reset()")
sub("  g_millis = 0; g_now += 0;\n  // reboot at t=300 s of the piece (persisted open, untrusted clock): millis() restarts\n","  // reboot 300 s into the piece (persisted open, untrusted clock): millis() restarts\n")
sub("  for (int t = 26; t <= 176; t += 30) { run(t - 0 - (int)(g_millis / 1000)); pir(); }\n","  for (int i = 0; i < 6; ++i) { run(i == 0 ? 26 : 30); pir(); }   // post-boot PIRs at 26..176 s\n")
sub("  assert(CurrentReadings::total == 360 + 760 - 0 - 0 + 0 - 0 + 0 - 0 + 0 - 0);","  assert(CurrentReadings::total == 360 + 460);")
open(p,'w').write(s)
EOF
zsh tests/untrusted_piece_c2_test.sh

```
Assertion failed: (CurrentReadings::total == 360), function testPieceOpenedUntrustedKeepsMillisWhenTrustReturns, file driver.cpp, line 295.
<shellId: 36 completed with exit code 134>
```


---

<sub>8m 9s</sub>

### `bash`

**cd /private/tmp/gcc-step0/c2 && sed -i 's|^  assert(CurrentReadings::total == 360);                 // 60 + 300, not 36360|  std::cerr << "TOTAL " << CurrentReadings::total << " " << Log.lines.size() << "\\n"; for (auto\&l: Log.lines) std::cerr << l << "\\n";\n&|' tests/untrusted_piece_c2_test.sh && zsh tests/untrusted_piece_c2_test.sh 2>&1 | head**

$ cd /private/tmp/gcc-step0/c2 && sed -i 's|^  assert(CurrentReadings::total == 360);                 // 60 + 300, not 36360|  std::cerr << "TOTAL " << CurrentReadings::total << " " << Log.lines.size() << "\\n"; for (auto\&l: Log.lines) std::cerr << l << "\\n";\n&|' tests/untrusted_piece_c2_test.sh && zsh tests/untrusted_piece_c2_test.sh 2>&1 | head

```
sed: 2: "tests/untrusted_piece_c ...": undefined label 'ests/untrusted_piece_c2_test.sh'
<shellId: 37 completed with exit code 1>
```


---

<sub>8m 18s</sub>

### `bash`

**cd /private/tmp/gcc-step0/c2 && sed -n 293,296p build-tmp/wo-2026-11-03-006-stage6/c2_test/driver.cpp**

$ cd /private/tmp/gcc-step0/c2 && sed -n 293,296p build-tmp/wo-2026-11-03-006-stage6/c2_test/driver.cpp

```
run(1000);
  assert(!CurrentReadings::occupied);
  assert(CurrentReadings::total == 360);                 // 60 + 300, not 36360
  assert(session.pieceUntrusted == false);
<shellId: 38 completed with exit code 0>
```


---

<sub>8m 22s</sub>

### `bash`

**cd /private/tmp/gcc-step0/c2/build-tmp/wo-2026-11-03-006-stage6/c2_test && sed -i 's|^  assert(CurrentReadings::total == 360);.*|  std::cerr << "T=" << CurrentReadings::total << "\\n"; for (auto\&l: Log.lines) std::cerr << l << "\\n";|' driver.cpp && clang++ -std=c++17 -w -o d2 driver.cpp && ./d2 2>&1 | head**

$ cd /private/tmp/gcc-step0/c2/build-tmp/wo-2026-11-03-006-stage6/c2_test && sed -i 's|^  assert(CurrentReadings::total == 360);.*|  std::cerr << "T=" << CurrentReadings::total << "\\n"; for (auto\&l: Log.lines) std::cerr << l << "\\n";|' driver.cpp && clang++ -std=c++17 -w -o d2 driver.cpp && ./d2 2>&1 | head

```
sed: 1: "driver.cpp
": extra characters at the end of d command
<shellId: 39 completed with exit code 1>
```


---

<sub>8m 25s</sub>

### Copilot

macOS sed quirk; use python.

---

<sub>8m 31s</sub>

### `bash`

**cd /private/tmp/gcc-step0/c2/build-tmp/wo-2026-11-03-006-stage6/c2_test && python3 - <<'EOF'
s=open('driver.cpp').read()
s=s.replace("  assert(CurrentReadings::total == 360);                 // 60 + 300, not 36360",'  std::cerr << "T=" << CurrentReadings::total << "\\n"; for (auto&l: Log.lines) std::cerr << l << "\\n";')
open('driver.cpp','w').write(s)
EOF
clang++ -std=c++17 -w -o d2 driver.cpp && ./d2 2>&1 | head**

$ cd /private/tmp/gcc-step0/c2/build-tmp/wo-2026-11-03-006-stage6/c2_test && python3 - <<'EOF'
s=open('driver.cpp').read()
s=s.replace("  assert(CurrentReadings::total == 360);                 // 60 + 300, not 36360",'  std::cerr << "T=" << CurrentReadings::total << "\\n"; for (auto&l: Log.lines) std::cerr << l << "\\n";')
open('driver.cpp','w').write(s)
EOF
clang++ -std=c++17 -w -o d2 driver.cpp && ./d2 2>&1 | head

```
T=300
Occ: untrusted piece credit=300 start=7000000
Assertion failed: (CurrentReadings::total == 360 + 460), function testPieceOpenedUntrustedKeepsMillisWhenTrustReturns, file driver.cpp, line 299.
<shellId: 40 completed with exit code 0>
```


---

<sub>8m 44s</sub>

### `bash`

**cd /private/tmp/gcc-step0/c2 && python3 - <<'EOF'
p='tests/untrusted_piece_c2_test.sh'
s=open(p).read()
def sub(o,n):
    global s
    assert s.count(o)==1,o; s=s.replace(o,n)
sub("  pir(); run(60);\n  Clock::trusted = true; g_now += 36000;","  pir(); run(60); pir();\n  Clock::trusted = true; g_now += 36000;")
sub("  pir(); run(60); g_now += 100; run(1000);\n  assert(CurrentReadings::total == 360 + 460);","  pir(); run(60); pir(); g_now += 100; run(1000);\n  assert(CurrentReadings::total == 360 + 460);")
sub("  Clock::trusted = true;\n  pir(); run(60); g_now += 100; run(1000);","  Clock::trusted = true;\n  pir(); run(60); pir(); g_now += 100; run(1000);")
open(p,'w').write(s)
EOF
zsh tests/untrusted_piece_c2_test.sh 2>&1 | tail**

$ cd /private/tmp/gcc-step0/c2 && python3 - <<'EOF'
p='tests/untrusted_piece_c2_test.sh'
s=open(p).read()
def sub(o,n):
    global s
    assert s.count(o)==1,o; s=s.replace(o,n)
sub("  pir(); run(60);\n  Clock::trusted = true; g_now += 36000;","  pir(); run(60); pir();\n  Clock::trusted = true; g_now += 36000;")
sub("  pir(); run(60); g_now += 100; run(1000);\n  assert(CurrentReadings::total == 360 + 460);","  pir(); run(60); pir(); g_now += 100; run(1000);\n  assert(CurrentReadings::total == 360 + 460);")
sub("  Clock::trusted = true;\n  pir(); run(60); g_now += 100; run(1000);","  Clock::trusted = true;\n  pir(); run(60); pir(); g_now += 100; run(1000);")
open(p,'w').write(s)
EOF
zsh tests/untrusted_piece_c2_test.sh 2>&1 | tail

```
driver ok
<shellId: 41 completed with exit code 0>
```


---

<sub>9m 1s</sub>

### `bash`

**cd /private/tmp/gcc-step0/c2 && python3 - <<'EOF'
p='tests/untrusted_piece_c2_test.sh'
s=open(p).read()
def sub(o,n):
    global s
    assert s.count(o)==1,o; s=s.replace(o,n)
sub("// Trust returns while the piece is open:",'''// Dev-09, 23 Oct: sessions from 10:35:21 every 1200 s, untrusted all day, the window ends 14:00:00 (12279 s).
// Credited at 14:00 is compared with the 205 min of v45 (the whole wall span).
void replay(bool withReset) {
  world(300000);
  const long window = 12279;
  long t = 0;                                             // seconds since 10:35:21
  for (int k = 0; k <= 10; ++k) {
    const long startAt = 1200L * k;
    if (t < startAt) { run(startAt - t); t = startAt; }
    for (int i = 0; i < 10; ++i) {
      if (withReset && k == 0 && i == 4) { reset(); }      // 10:40:55 is 334 s in: after the last pulse (270 s)
      pir(); run(30); t += 30;
    }
    if (withReset && k == 0) { assert(true); }
    if (t > window) break;
  }
  run(window - t > 0 ? window - t : 0);
}
void testDev09Replay() {
  // Control: ten pieces close by 14:00 (the eleventh opens at 13:55:21), each its own 570 s.
  world(300000);
  for (int k = 0; k < 10; ++k) { session_of(10); gap(1200, 300); }
  assert(CurrentReadings::total == 5700);                 // 95.0 min; v45 credited 205 min for this window
  assert(CurrentReadings::total < 100 * 60);
  // Fault pass: reset 334 s into the first piece (after its last pulse at 270 s); the other nine are whole.
  world(300000);
  session_of(10);                                         // 300 s: pulses to 270 s, piece open
  run(34);                                                // 334 s: the reset
  reset();
  gap(1200 - 334, 0);                                     // no PIR after the reboot: the straddler closes post-boot
  for (int k = 1; k < 10; ++k) { session_of(10); gap(1200, 300); }
  assert(CurrentReadings::total == 301 + 9 * 570);        // 90.5 min, never the 205 of the merged span
  assert(Log.count("Occ: untrusted piece credit=301") == 1);
  assert(unoccupied == 10);
}

// Trust returns while the piece is open:''')
sub("int main() {\n  testControl","int main() {\n  testDev09Replay();\n  testControl")
# drop the unused replay() helper
a=s.index("// Dev-09, 23 Oct: sessions from 10:35:21"); b=s.index("void testDev09Replay()")
s=s[:a]+s[b:]
open(p,'w').write(s)
EOF
zsh tests/untrusted_piece_c2_test.sh 2>&1 | tail**

$ cd /private/tmp/gcc-step0/c2 && python3 - <<'EOF'
p='tests/untrusted_piece_c2_test.sh'
s=open(p).read()
def sub(o,n):
    global s
    assert s.count(o)==1,o; s=s.replace(o,n)
sub("// Trust returns while the piece is open:",'''// Dev-09, 23 Oct: sessions from 10:35:21 every 1200 s, untrusted all day, the window ends 14:00:00 (12279 s).
// Credited at 14:00 is compared with the 205 min of v45 (the whole wall span).
void replay(bool withReset) {
  world(300000);
  const long window = 12279;
  long t = 0;                                             // seconds since 10:35:21
  for (int k = 0; k <= 10; ++k) {
    const long startAt = 1200L * k;
    if (t < startAt) { run(startAt - t); t = startAt; }
    for (int i = 0; i < 10; ++i) {
      if (withReset && k == 0 && i == 4) { reset(); }      // 10:40:55 is 334 s in: after the last pulse (270 s)
      pir(); run(30); t += 30;
    }
    if (withReset && k == 0) { assert(true); }
    if (t > window) break;
  }
  run(window - t > 0 ? window - t : 0);
}
void testDev09Replay() {
  // Control: ten pieces close by 14:00 (the eleventh opens at 13:55:21), each its own 570 s.
  world(300000);
  for (int k = 0; k < 10; ++k) { session_of(10); gap(1200, 300); }
  assert(CurrentReadings::total == 5700);                 // 95.0 min; v45 credited 205 min for this window
  assert(CurrentReadings::total < 100 * 60);
  // Fault pass: reset 334 s into the first piece (after its last pulse at 270 s); the other nine are whole.
  world(300000);
  session_of(10);                                         // 300 s: pulses to 270 s, piece open
  run(34);                                                // 334 s: the reset
  reset();
  gap(1200 - 334, 0);                                     // no PIR after the reboot: the straddler closes post-boot
  for (int k = 1; k < 10; ++k) { session_of(10); gap(1200, 300); }
  assert(CurrentReadings::total == 301 + 9 * 570);        // 90.5 min, never the 205 of the merged span
  assert(Log.count("Occ: untrusted piece credit=301") == 1);
  assert(unoccupied == 10);
}

// Trust returns while the piece is open:''')
sub("int main() {\n  testControl","int main() {\n  testDev09Replay();\n  testControl")
# drop the unused replay() helper
a=s.index("// Dev-09, 23 Oct: sessions from 10:35:21"); b=s.index("void testDev09Replay()")
s=s[:a]+s[b:]
open(p,'w').write(s)
EOF
zsh tests/untrusted_piece_c2_test.sh 2>&1 | tail

```
driver ok
<shellId: 42 completed with exit code 0>
```


---

<sub>9m 10s</sub>

### Copilot

Now the mutation harness.

---

<sub>9m 19s</sub>

### `bash`

**cd /private/tmp/gcc-step0/c2 && cat >> tests/untrusted_piece_c2_test.sh <<'EOF'
print "driver and structure ok"

# --- mutations: each must fail the driver or the structural checks ------------
mutate() {  # name file old new
  local name="$1" file="$2" old="$3" new="$4" m="$work/mut"
  rm -rf "$m"; mkdir -p "$m/state"
  cp "$src/state/State_Common.h" "$src/state/State_Modes.cpp" "$src/state/StateMachine.h" "$m/state/"
  cp "$src/Generalized-Core-Counter.cpp" "$m/"
  python3 - "$m/$file" "$old" "$new" <<'PY'
import sys
p, old, new = sys.argv[1:]
s = open(p).read()
if s.count(old) != 1:
    print("MUTATION_NOT_APPLIED:", old, s.count(old)); sys.exit(3)
open(p, "w").write(s.replace(old, new))
PY
  (( $? == 0 )) || { print -u2 "mutation did not apply: $name"; exit 1; }
  local caught=0
  check_structure "$m" >/dev/null 2>&1 || caught=1
  build_and_run "$m/state/State_Common.h" "$m/state/State_Modes.cpp" >/dev/null 2>&1 || caught=1
  if (( caught )); then print "caught: $name"; else print -u2 "NOT CAUGHT: $name"; exit 1; fi
}

mutate "keep the B2 re-arm for an untrusted piece" state/State_Common.h "const bool piece = occupied && session.pieceUntrusted;" "const bool piece = false;"
mutate "credit the wall span" state/State_Common.h "rawSessionSeconds = (CurrentReadings::get_lastOccupancyEvent() - session.occupancyStartMs + occupancyDebounceMs()) / 1000UL;" "rawSessionSeconds = (long long)Time.now() - (long long)start;"
mutate "credit from the persisted wall start" state/State_Common.h "- session.occupancyStartMs +" "- (uint32_t)CurrentReadings::get_occupancyStartTime() +"
mutate "one-debounce fallback at boot" state/State_Common.h "		durationComputable = true;
		Log.info(\"Occ: untrusted" "		if (session.occupancyStartMs == 0) rawSessionSeconds += occupancyDebounceMs() / 1000UL;
		durationComputable = true;
		Log.info(\"Occ: untrusted"
mutate "start a trusted piece on millis()" state/State_Common.h "const bool piece = occupied && session.pieceUntrusted;" "const bool piece = occupied;"
mutate "mark a trusted piece untrusted at open" state/State_Modes.cpp "      session.pieceUntrusted = !Clock::isTrusted();" "      session.pieceUntrusted = true;"
mutate "drop the report call" state/State_Modes.cpp "if (reportNow && !closeResult.stillOpen) {
      session.occupancyChangeTriggered = true;
      if (state == IDLE_STATE) {
        transitionTo(REPORTING_STATE, \"occupancy cleared\");" "if (false) {
      session.occupancyChangeTriggered = true;
      if (state == IDLE_STATE) {
        transitionTo(REPORTING_STATE, \"occupancy cleared\");"
mutate "reuse the old piece's start (awake open)" state/State_Modes.cpp "      session.occupancyStartMs = millis();
" ""
mutate "straddler not marked at boot" Generalized-Core-Counter.cpp "session.pieceUntrusted = CurrentReadings::get_occupied() && !Clock::isTrusted();" "session.pieceUntrusted = false;"
mutate "wake close gated on the LED again" state/State_Modes.cpp "(signalLEDStatus() || (session.pieceUntrusted && CurrentReadings::get_occupied()))" "signalLEDStatus()"

print "untrusted_piece_c2_test: Dev-09 replay, straddler, trust-return, trusted-unchanged and all mutations detected"
EOF
zsh tests/untrusted_piece_c2_test.sh 2>&1 | tail -20**

$ cd /private/tmp/gcc-step0/c2 && cat >> tests/untrusted_piece_c2_test.sh <<'EOF'
print "driver and structure ok"

# --- mutations: each must fail the driver or the structural checks ------------
mutate() {  # name file old new
  local name="$1" file="$2" old="$3" new="$4" m="$work/mut"
  rm -rf "$m"; mkdir -p "$m/state"
  cp "$src/state/State_Common.h" "$src/state/State_Modes.cpp" "$src/state/StateMachine.h" "$m/state/"
  cp "$src/Generalized-Core-Counter.cpp" "$m/"
  python3 - "$m/$file" "$old" "$new" <<'PY'
import sys
p, old, new = sys.argv[1:]
s = open(p).read()
if s.count(old) != 1:
    print("MUTATION_NOT_APPLIED:", old, s.count(old)); sys.exit(3)
open(p, "w").write(s.replace(old, new))
PY
  (( $? == 0 )) || { print -u2 "mutation did not apply: $name"; exit 1; }
  local caught=0
  check_structure "$m" >/dev/null 2>&1 || caught=1
  build_and_run "$m/state/State_Common.h" "$m/state/State_Modes.cpp" >/dev/null 2>&1 || caught=1
  if (( caught )); then print "caught: $name"; else print -u2 "NOT CAUGHT: $name"; exit 1; fi
}

mutate "keep the B2 re-arm for an untrusted piece" state/State_Common.h "const bool piece = occupied && session.pieceUntrusted;" "const bool piece = false;"
mutate "credit the wall span" state/State_Common.h "rawSessionSeconds = (CurrentReadings::get_lastOccupancyEvent() - session.occupancyStartMs + occupancyDebounceMs()) / 1000UL;" "rawSessionSeconds = (long long)Time.now() - (long long)start;"
mutate "credit from the persisted wall start" state/State_Common.h "- session.occupancyStartMs +" "- (uint32_t)CurrentReadings::get_occupancyStartTime() +"
mutate "one-debounce fallback at boot" state/State_Common.h "		durationComputable = true;
		Log.info(\"Occ: untrusted" "		if (session.occupancyStartMs == 0) rawSessionSeconds += occupancyDebounceMs() / 1000UL;
		durationComputable = true;
		Log.info(\"Occ: untrusted"
mutate "start a trusted piece on millis()" state/State_Common.h "const bool piece = occupied && session.pieceUntrusted;" "const bool piece = occupied;"
mutate "mark a trusted piece untrusted at open" state/State_Modes.cpp "      session.pieceUntrusted = !Clock::isTrusted();" "      session.pieceUntrusted = true;"
mutate "drop the report call" state/State_Modes.cpp "if (reportNow && !closeResult.stillOpen) {
      session.occupancyChangeTriggered = true;
      if (state == IDLE_STATE) {
        transitionTo(REPORTING_STATE, \"occupancy cleared\");" "if (false) {
      session.occupancyChangeTriggered = true;
      if (state == IDLE_STATE) {
        transitionTo(REPORTING_STATE, \"occupancy cleared\");"
mutate "reuse the old piece's start (awake open)" state/State_Modes.cpp "      session.occupancyStartMs = millis();
" ""
mutate "straddler not marked at boot" Generalized-Core-Counter.cpp "session.pieceUntrusted = CurrentReadings::get_occupied() && !Clock::isTrusted();" "session.pieceUntrusted = false;"
mutate "wake close gated on the LED again" state/State_Modes.cpp "(signalLEDStatus() || (session.pieceUntrusted && CurrentReadings::get_occupied()))" "signalLEDStatus()"

print "untrusted_piece_c2_test: Dev-09 replay, straddler, trust-return, trusted-unchanged and all mutations detected"
EOF
zsh tests/untrusted_piece_c2_test.sh 2>&1 | tail -20

<details>
<summary>9 lines</summary>

```
driver ok
driver and structure ok
caught: keep the B2 re-arm for an untrusted piece
caught: credit the wall span
caught: credit from the persisted wall start
caught: one-debounce fallback at boot
caught: start a trusted piece on millis()
MUTATION_NOT_APPLIED:       session.pieceUntrusted = !Clock::isTrusted(); 2
<shellId: 43 completed with exit code 0>
```

</details>


---

<sub>9m 34s</sub>

### `bash`

**cd /private/tmp/gcc-step0/c2 && python3 - <<'EOF'
p='tests/untrusted_piece_c2_test.sh'
s=open(p).read()
def sub(o,n):
    global s
    assert s.count(o)==1,o; s=s.replace(o,n)
sub('"      session.pieceUntrusted = !Clock::isTrusted();" "      session.pieceUntrusted = true;"','"\\n      session.pieceUntrusted = !Clock::isTrusted();" "\\n      session.pieceUntrusted = true;"')
sub('"      session.occupancyStartMs = millis();\n" ""','"\\n      session.occupancyStartMs = millis();\\n" "\\n"')
sub('# The reboot straddler:','''# The wake-close must not hang on the LED when an untrusted piece is open (the C1 verdict's LED-gate gap).
need("OCCUPANCY && (signalLEDStatus() || (session.pieceUntrusted && CurrentReadings::get_occupied())) && debounceRemainingMs() == 0" in modes, "the wake close must also fire for an open untrusted piece")
# The reboot straddler:''')
open(p,'w').write(s)
EOF
zsh tests/untrusted_piece_c2_test.sh 2>&1 | tail -14**

$ cd /private/tmp/gcc-step0/c2 && python3 - <<'EOF'
p='tests/untrusted_piece_c2_test.sh'
s=open(p).read()
def sub(o,n):
    global s
    assert s.count(o)==1,o; s=s.replace(o,n)
sub('"      session.pieceUntrusted = !Clock::isTrusted();" "      session.pieceUntrusted = true;"','"\\n      session.pieceUntrusted = !Clock::isTrusted();" "\\n      session.pieceUntrusted = true;"')
sub('"      session.occupancyStartMs = millis();\n" ""','"\\n      session.occupancyStartMs = millis();\\n" "\\n"')
sub('# The reboot straddler:','''# The wake-close must not hang on the LED when an untrusted piece is open (the C1 verdict's LED-gate gap).
need("OCCUPANCY && (signalLEDStatus() || (session.pieceUntrusted && CurrentReadings::get_occupied())) && debounceRemainingMs() == 0" in modes, "the wake close must also fire for an open untrusted piece")
# The reboot straddler:''')
open(p,'w').write(s)
EOF
zsh tests/untrusted_piece_c2_test.sh 2>&1 | tail -14

<details>
<summary>9 lines</summary>

```
driver ok
driver and structure ok
caught: keep the B2 re-arm for an untrusted piece
caught: credit the wall span
caught: credit from the persisted wall start
caught: one-debounce fallback at boot
caught: start a trusted piece on millis()
MUTATION_NOT_APPLIED: \n      session.pieceUntrusted = !Clock::isTrusted(); 0
<shellId: 44 completed with exit code 0>
```

</details>


---

<sub>10m 3s</sub>

### `bash`

**cd /private/tmp/gcc-step0/c2 && python3 - <<'EOF'
p='tests/untrusted_piece_c2_test.sh'
s=open(p).read()
s=s.replace('"\\n      session.pieceUntrusted = !Clock::isTrusted();" "\\n      session.pieceUntrusted = true;"',"$'\\n      session.pieceUntrusted = !Clock::isTrusted();' $'\\n      session.pieceUntrusted = true;'")
s=s.replace('"\\n      session.occupancyStartMs = millis();\\n" "\\n"',"$'\\n      session.occupancyStartMs = millis();\\n' $'\\n'")
open(p,'w').write(s)
EOF
zsh tests/untrusted_piece_c2_test.sh 2>&1 | tail -12**

$ cd /private/tmp/gcc-step0/c2 && python3 - <<'EOF'
p='tests/untrusted_piece_c2_test.sh'
s=open(p).read()
s=s.replace('"\\n      session.pieceUntrusted = !Clock::isTrusted();" "\\n      session.pieceUntrusted = true;"',"$'\\n      session.pieceUntrusted = !Clock::isTrusted();' $'\\n      session.pieceUntrusted = true;'")
s=s.replace('"\\n      session.occupancyStartMs = millis();\\n" "\\n"',"$'\\n      session.occupancyStartMs = millis();\\n' $'\\n'")
open(p,'w').write(s)
EOF
zsh tests/untrusted_piece_c2_test.sh 2>&1 | tail -12

<details>
<summary>13 lines</summary>

```
driver and structure ok
caught: keep the B2 re-arm for an untrusted piece
caught: credit the wall span
caught: credit from the persisted wall start
caught: one-debounce fallback at boot
caught: start a trusted piece on millis()
caught: mark a trusted piece untrusted at open
caught: drop the report call
caught: reuse the old piece's start (awake open)
caught: straddler not marked at boot
caught: wake close gated on the LED again
untrusted_piece_c2_test: Dev-09 replay, straddler, trust-return, trusted-unchanged and all mutations detected
<shellId: 45 completed with exit code 0>
```

</details>


---

<sub>10m 8s</sub>

### `bash`

**cd /private/tmp/gcc-step0/c2 && grep -n "ledOn\|signalLEDStatus\|wake pir\|processSleepWakeEvent" tests/occupancy_report_by_mode_test.cpp | head -30**

$ cd /private/tmp/gcc-step0/c2 && grep -n "ledOn\|signalLEDStatus\|wake pir\|processSleepWakeEvent" tests/occupancy_report_by_mode_test.cpp | head -30

<details>
<summary>21 lines</summary>

```
16://   - from State_Modes.cpp: processSleepWakeEvent() (the wake end and wake start blocks, whole)
18://     call to processSleepWakeEvent() and the timer-wake block
119:bool ledOn = false;
122:  ledOn = on;
126:bool signalLEDStatus() { return ledOn; }
138:// What processSleepWakeEvent() collaborates with beyond the above.
288:  ledOn = false;
313:  ledOn = true;
438:// WO-2026-10-22-001: a PIR wake whose LED timer ran out. processSleepWakeEvent() closes the session first
450:                 "wake pir+end: the close reports before the PIR event is processed");
452:      CHECK_MODE(m, state == SLEEPING_STATE && wakeFellThrough && !anyOccupancyReport(), "wake pir+end: no immediate report");
454:                 "wake pir+end: the PIR event starts a new session after the close");
467:    ledOn = false;
530:    ledOn = false;
648:  ledOn = true;
666:    CHECK_MODE(m, !CurrentReadings::get_occupied() && !ledOn && !session.occupancyChangeTriggered &&
686:    CHECK_MODE(m, !CurrentReadings::get_occupied() && !ledOn && !session.occupancyChangeTriggered && !anyOccupancyReport(),
695:    ledOn = true;
735:    ledOn = true;
743:    ledOn = true;
<shellId: 46 completed with exit code 0>
```

</details>


---

<sub>10m 11s</sub>

### `bash`

**cd /private/tmp/gcc-step0/c2 && sed -n 296,335p tests/occupancy_report_by_mode_test.cpp; sed -n 436,475p tests/occupancy_report_by_mode_test.cpp; grep -n "closeOccupancySessionSafely" tests/occupancy_report_by_mode_test.cpp | head**

$ cd /private/tmp/gcc-step0/c2 && sed -n 296,335p tests/occupancy_report_by_mode_test.cpp; sed -n 436,475p tests/occupancy_report_by_mode_test.cpp; grep -n "closeOccupancySessionSafely" tests/occupancy_report_by_mode_test.cpp | head

<details>
<summary>77 lines</summary>

```
teardownWanted = false;
  cloudGate.cloudSyncStartMs = 0;
  gateWanted = false;
  gateTimedOut = false;
  gateElapsedMs = 0;
  publishCount = 0;
  flagAtPublish = false;
  Particle.connectedValue = false;
}

// An open session whose debounce has just run out.
void occupiedPastDebounce(State where) {
  CurrentReadings::set_occupied(true);
  CurrentReadings::set_occupancyStartTime(Time.nowValue - 600);
  CurrentReadings::set_lastOccupancyEvent(millisValue);
  millisValue += kDebounceMs + 1000;
  Time.nowValue += (kDebounceMs + 1000) / 1000;
  ledOn = true;
  ledRemainingSec = 0;
  state = where;
  oldState = where;
}

bool wentTo(const char *reason) {
  return std::find(reasons.begin(), reasons.end(), reason) != reasons.end();
}

bool anyOccupancyReport() {
  return wentTo("occupancy change") || wentTo("occupancy cleared") || wentTo("occupancy transition") ||
         wentTo("occupancy change pending") || wentTo("sleep-occupancy-debounce-report") ||
         wentTo("sleep-pir-occupancy-report");
}

std::string label(const ModeCase &m, const std::string &what) { return std::string(m.name) + ": " + what; }

#define CHECK_MODE(m, cond, what) CHECK(cond, label(m, what))

// --- The rule table: awake path -------------------------------------------------
void testAwakeStart() {
  for (const ModeCase &m : kModes) {
}

// WO-2026-10-22-001: a PIR wake whose LED timer ran out. processSleepWakeEvent() closes the session first
// and processes the PIR event after it, so a reporting mode reports the close, and any other mode opens a
// new session at the wake's time.
void testWakePirAfterEnd() {
  for (const ModeCase &m : kModes) {
    arm(m);
    occupiedPastDebounce(SLEEPING_STATE);
    const time_t before = CurrentReadings::get_occupancyStartTime();
    wakeKind = WAKE_PIR;
    sleepPass();
    if (m.reportsNow) {
      CHECK_MODE(m, state == REPORTING_STATE && wentTo("sleep-occupancy-debounce-report") && !wentTo("sleep-pir-occupancy-report"),
                 "wake pir+end: the close reports before the PIR event is processed");
    } else {
      CHECK_MODE(m, state == SLEEPING_STATE && wakeFellThrough && !anyOccupancyReport(), "wake pir+end: no immediate report");
      CHECK_MODE(m, CurrentReadings::get_occupied() && CurrentReadings::get_occupancyStartTime() > before,
                 "wake pir+end: the PIR event starts a new session after the close");
    }
  }
}

// --- The soak: the end is latched by the main-loop handler while SLEEPING -------
// Pass N is a timer wake whose end gate is false (the LED was switched off).
// The tail then latches the end. Pass N+1 must report before it sleeps again;
// the order of the pending check against the suppress decision is the real one.
void testSoakLatchedWhileSleeping() {
  for (const ModeCase &m : kModes) {
    arm(m);
    occupiedPastDebounce(SLEEPING_STATE);
    ledOn = false;
    wakeKind = WAKE_TIMER;
    sleepPass();
    CHECK_MODE(m, CurrentReadings::get_occupied(), "soak: the wake end gate does not fire");
    const bool suppresses = m.reportsNow && m.configured == kKeepAlive;
    CHECK_MODE(m, wentTo(suppresses ? "sleep-timer-occupied-suppress-report" : "sleep-timer-report"),
               "soak: the first wake takes its scheduled decision");
    state = SLEEPING_STATE;
    handleOccupancyMode();  // the main-loop tail, state already SLEEPING
10://   - reportsOccupancyChangesNow(), closeOccupancySessionSafely() and
658:    closeOccupancySessionSafely("daily-cleanup", DailyBoundary::boundaryValue);
753:    closeOccupancySessionSafely("clamp-test", Time.nowValue);
759:    closeOccupancySessionSafely("clamp-test", Time.nowValue + 1000);
<shellId: 47 completed with exit code 0>
```

</details>


---

<sub>10m 22s</sub>

### `bash`

**cd /private/tmp/gcc-step0/c2 && python3 - <<'EOF'
p='tests/occupancy_report_by_mode_test.cpp'
s=open(p).read()
o="// --- The soak: the end is latched by the main-loop handler while SLEEPING"
n='''// WO-2026-11-03-006: the LED can be low while a session is open (a reset, a night-sleep prep). An open untrusted
// piece is still closed on the debounce predicate alone; a trusted piece keeps the old LED gate.
void testWakeClosesUntrustedPieceWithLedOff() {
  for (const ModeCase &m : kModes) {
    arm(m);
    occupiedPastDebounce(SLEEPING_STATE);
    ledOn = false;
    Clock::trusted = false;
    session.pieceUntrusted = true;
    session.occupancyStartMs = CurrentReadings::get_lastOccupancyEvent() - 60000UL;
    const uint32_t before = CurrentReadings::get_totalOccupiedSeconds();
    wakeKind = WAKE_PIR;
    sleepPass();
    CHECK_MODE(m, CurrentReadings::get_totalOccupiedSeconds() == before + 60 + kDebounceMs / 1000,
               "untrusted piece, LED low: the wake closes it and credits its millis() on-time");
    if (m.reportsNow) {
      CHECK_MODE(m, state == REPORTING_STATE && wentTo("sleep-occupancy-debounce-report"), "untrusted piece, LED low: the close reports");
    }
    arm(m);
    occupiedPastDebounce(SLEEPING_STATE);
    ledOn = false;
    Clock::trusted = true;
    session.pieceUntrusted = false;
    const uint32_t trustedBefore = CurrentReadings::get_totalOccupiedSeconds();
    wakeKind = WAKE_PIR;
    sleepPass();
    CHECK_MODE(m, CurrentReadings::get_totalOccupiedSeconds() == trustedBefore && CurrentReadings::get_occupied(),
               "trusted piece, LED low: the wake path is unchanged (no close)");
    Clock::trusted = true;
  }
}

'''+o
assert s.count(o)==1
s=s.replace(o,n)
open(p,'w').write(s)
EOF
grep -n "testWakePirAfterEnd();" tests/occupancy_report_by_mode_test.cpp**

$ cd /private/tmp/gcc-step0/c2 && python3 - <<'EOF'
p='tests/occupancy_report_by_mode_test.cpp'
s=open(p).read()
o="// --- The soak: the end is latched by the main-loop handler while SLEEPING"
n='''// WO-2026-11-03-006: the LED can be low while a session is open (a reset, a night-sleep prep). An open untrusted
// piece is still closed on the debounce predicate alone; a trusted piece keeps the old LED gate.
void testWakeClosesUntrustedPieceWithLedOff() {
  for (const ModeCase &m : kModes) {
    arm(m);
    occupiedPastDebounce(SLEEPING_STATE);
    ledOn = false;
    Clock::trusted = false;
    session.pieceUntrusted = true;
    session.occupancyStartMs = CurrentReadings::get_lastOccupancyEvent() - 60000UL;
    const uint32_t before = CurrentReadings::get_totalOccupiedSeconds();
    wakeKind = WAKE_PIR;
    sleepPass();
    CHECK_MODE(m, CurrentReadings::get_totalOccupiedSeconds() == before + 60 + kDebounceMs / 1000,
               "untrusted piece, LED low: the wake closes it and credits its millis() on-time");
    if (m.reportsNow) {
      CHECK_MODE(m, state == REPORTING_STATE && wentTo("sleep-occupancy-debounce-report"), "untrusted piece, LED low: the close reports");
    }
    arm(m);
    occupiedPastDebounce(SLEEPING_STATE);
    ledOn = false;
    Clock::trusted = true;
    session.pieceUntrusted = false;
    const uint32_t trustedBefore = CurrentReadings::get_totalOccupiedSeconds();
    wakeKind = WAKE_PIR;
    sleepPass();
    CHECK_MODE(m, CurrentReadings::get_totalOccupiedSeconds() == trustedBefore && CurrentReadings::get_occupied(),
               "trusted piece, LED low: the wake path is unchanged (no close)");
    Clock::trusted = true;
  }
}

'''+o
assert s.count(o)==1
s=s.replace(o,n)
open(p,'w').write(s)
EOF
grep -n "testWakePirAfterEnd();" tests/occupancy_report_by_mode_test.cpp

```
843:  testWakePirAfterEnd();  testSoakLatchedWhileSleeping();
<shellId: 48 completed with exit code 0>
```


---

<sub>10m 55s</sub>

### `bash`

**cd /private/tmp/gcc-step0/c2 && python3 - <<'EOF'
p='tests/occupancy_report_by_mode_test.cpp'
s=open(p).read()
s=s.replace("  testWakePirAfterEnd();  testSoakLatchedWhileSleeping();","  testWakePirAfterEnd();  testWakeClosesUntrustedPieceWithLedOff();  testSoakLatchedWhileSleeping();")
open(p,'w').write(s)
EOF
zsh tests/occupancy_report_by_mode_test.sh 2>&1 | grep -v "^OK (mutation)" | tail -15**

$ cd /private/tmp/gcc-step0/c2 && python3 - <<'EOF'
p='tests/occupancy_report_by_mode_test.cpp'
s=open(p).read()
s=s.replace("  testWakePirAfterEnd();  testSoakLatchedWhileSleeping();","  testWakePirAfterEnd();  testWakeClosesUntrustedPieceWithLedOff();  testSoakLatchedWhileSleeping();")
open(p,'w').write(s)
EOF
zsh tests/occupancy_report_by_mode_test.sh 2>&1 | grep -v "^OK (mutation)" | tail -15

```
<command with shellId: 49 is still running after 30 seconds. The command is still running but hasn't produced output yet. You will be automatically notified when it completes; if you need the command to complete end your response with no tool calls to wait for the notification, or use stop_bash to stop it.>
```


---

<sub>11m 28s</sub>

### `read_bash`

**Waiting up to 150 seconds for command output**

<details>
<summary>Arguments</summary>

```json
{
  "delay": 150,
  "shellId": "49"
}
```

</details>

```
occupancy_report_by_mode_test: all checks passed
occupancy_report_by_mode_test: clean run passed and all mutations detected
<shellId: 49 completed with exit code 0>
```


---

<sub>11m 36s</sub>

### `bash`

**cd /private/tmp/gcc-step0/c2 && python3 - <<'EOF'
p='tests/occupancy_report_by_mode_test.sh'
s=open(p).read()
o='mutate_and_expect_failure "connected clear restored, entry take removed"'
n='''# WO-2026-11-03-006: the wake close of an open untrusted piece must not depend on the LED.
mutate_and_expect_failure "wake close gated on the LED for an untrusted piece" "untrusted piece, LED low:" "state/State_Modes.cpp" \\
  "(signalLEDStatus() || (session.pieceUntrusted && CurrentReadings::get_occupied()))" \\
  "signalLEDStatus()"
'''+o
assert s.count(o)==1
open(p,'w').write(s.replace(o,n))
EOF
sed -n 440,470p tests/occupancy_report_by_mode_test.sh | head -5; grep -n "mutate_and_expect_failure()" -A25 tests/occupancy_report_by_mode_test.sh | head -40**

$ cd /private/tmp/gcc-step0/c2 && python3 - <<'EOF'
p='tests/occupancy_report_by_mode_test.sh'
s=open(p).read()
o='mutate_and_expect_failure "connected clear restored, entry take removed"'
n='''# WO-2026-11-03-006: the wake close of an open untrusted piece must not depend on the LED.
mutate_and_expect_failure "wake close gated on the LED for an untrusted piece" "untrusted piece, LED low:" "state/State_Modes.cpp" \\
  "(signalLEDStatus() || (session.pieceUntrusted && CurrentReadings::get_occupied()))" \\
  "signalLEDStatus()"
'''+o
assert s.count(o)==1
open(p,'w').write(s.replace(o,n))
EOF
sed -n 440,470p tests/occupancy_report_by_mode_test.sh | head -5; grep -n "mutate_and_expect_failure()" -A25 tests/occupancy_report_by_mode_test.sh | head -40

<details>
<summary>32 lines</summary>

```
mutate_and_expect_failure "predicate adds INTERMITTENT" "." "state/State_Common.h" \
  "$predicate_old" "$predicate_old || mode == SystemConfig::INTERMITTENT"
mutate_and_expect_failure "predicate adds DISCONNECTED" "." "state/State_Common.h" \
  "$predicate_old" "mode != SystemConfig::INTERMITTENT"
mutate_and_expect_failure "sleep-prep check removed" "soak:" "state/State_Sleep.cpp" \
383:mutate_and_expect_failure() {
384-  # $1 = name, $2 = regex the failing checks must match, $3 = rel file,
385-  # then pairs of: old literal, new literal (each applied once, in order)
386-  local name="$1" expect="$2" rel="$3"
387-  shift 3
388-  local dir="$work/mut_${name// /_}"
389-  mkdir -p "$dir"
390-  cp -R "$src_root" "$dir/src"
391-  python3 - "$src_root/$rel" "$dir/src/$rel" "$@" <<'PY'
392-import sys
393-
394-src, dst = sys.argv[1:3]
395-pairs = sys.argv[3:]
396-text = open(src).read()
397-for old, new in zip(pairs[0::2], pairs[1::2]):
398-    if text.count(old) < 1:
399-        sys.exit(f"MUTATION_FAILED: {old!r} not found in {src}")
400-    text = text.replace(old, new, 1)
401-open(dst, "w").write(text)
402-PY
403-  if ! extract_and_build "$dir/src" "$dir/w" "$dir/mutant" 2>"$dir/build.log"; then
404-    echo "FAIL (mutation harness): mutant '$name' did not build - the mutation is malformed" >&2
405-    cat "$dir/build.log" >&2
406-    exit 1
407-  fi
408-  if "$dir/mutant" >"$dir/run.log" 2>&1; then
<shellId: 50 completed with exit code 0>
```

</details>


---

<sub>11m 41s</sub>

### `bash`

**cd /private/tmp/gcc-step0/c2 && grep -n -i "BUILD_PATH_BASE\|particle compile\|make \|nm " docs/work-orders/WO-2026-11-03-004-stage6-copilot-report.md | head**

$ cd /private/tmp/gcc-step0/c2 && grep -n -i "BUILD_PATH_BASE\|particle compile\|make \|nm " docs/work-orders/WO-2026-11-03-004-stage6-copilot-report.md | head

```
9:- Local build: Device OS 6.6.0, boron, fresh `BUILD_PATH_BASE=build-tmp/wo-2026-11-03-004-stage6/obj-release`, rc=0, text 151692 / data 1114 / bss 2200. Log: `build.log` in that directory.
40:## nm / disassembly evidence (`tree.elf`; the objdump excerpt is saved as `callsites.txt` in the scratch dir)
45:- Object-file `nm` was not done (the objects live under the `BUILD_PATH_BASE`; I did not repeat it per object).
<shellId: 51 completed with exit code 0>
```


---

<sub>11m 43s</sub>

### `bash`

**cd /private/tmp/gcc-step0/c2 && grep -n -i "make\|toolchain\|compile" docs/work-orders/WO-2026-11-03-004-stage6-copilot-transcript.md | grep -i "device-os\|DEVICE_OS\|PLATFORM\|particle" | head -8; ls ~/.particle/toolchains | head; ls ~/.particle/toolchains/* | head -20**

$ cd /private/tmp/gcc-step0/c2 && grep -n -i "make\|toolchain\|compile" docs/work-orders/WO-2026-11-03-004-stage6-copilot-transcript.md | grep -i "device-os\|DEVICE_OS\|PLATFORM\|particle" | head -8; ls ~/.particle/toolchains | head; ls ~/.particle/toolchains/* | head -20

<details>
<summary>29 lines</summary>

```
1511:Particle firmware compile remains the final integration check.
5910:**cd /private/tmp/gcc-step0/c1 && ls /Users/chipmc/.particle/toolchains; ls /Users/chipmc/.particle/toolchains/* | head -20; ls; which particle**
5912:$ cd /private/tmp/gcc-step0/c1 && ls /Users/chipmc/.particle/toolchains; ls /Users/chipmc/.particle/toolchains/* | head -20; ls; which particle
5923:/Users/chipmc/.particle/toolchains/buildscripts:
5927:/Users/chipmc/.particle/toolchains/buildtools:
5930:/Users/chipmc/.particle/toolchains/deviceOS:
5936:/Users/chipmc/.particle/toolchains/gcc-arm:
5940:/Users/chipmc/.particle/toolchains/openocd:
buildscripts
buildtools
deviceOS
gcc-arm
openocd
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
<shellId: 52 completed with exit code 0>
```

</details>


---

<sub>11m 45s</sub>

### `bash`

**cd /private/tmp/gcc-step0/c2 && sed -n 5940,6080p docs/work-orders/WO-2026-11-03-004-stage6-copilot-transcript.md | grep -n "make\|PARTICLE\|BUILD_PATH" | head**

$ cd /private/tmp/gcc-step0/c2 && sed -n 5940,6080p docs/work-orders/WO-2026-11-03-004-stage6-copilot-transcript.md | grep -n "make\|PARTICLE\|BUILD_PATH" | head

<details>
<summary>11 lines</summary>

```
35:**cd /private/tmp/gcc-step0/c1 && grep -rn "BUILD_PATH_BASE\|compile-local\|make" tools/*.sh AI_DEVELOPMENT_WORKFLOW.md 2>/dev/null | head; ls tools; cat .particle/*.json 2>/dev/null | head; ls /Users/chipmc/.particle/toolchains/buildscripts/1.17.2 /Users/chipmc/.particle/toolchains/deviceOS/6.6.0 | head -20**
37:$ cd /private/tmp/gcc-step0/c1 && grep -rn "BUILD_PATH_BASE\|compile-local\|make" tools/*.sh AI_DEVELOPMENT_WORKFLOW.md 2>/dev/null | head; ls tools; cat .particle/*.json 2>/dev/null | head; ls /Users/chipmc/.particle/toolchains/buildscripts/1.17.2 /Users/chipmc/.particle/toolchains/deviceOS/6.6.0 | head -20
43:AI_DEVELOPMENT_WORKFLOW.md:23:| Architect | Claude, in the Claude app chat | Authors and revises Work Orders; makes design decisions together with Chip; recommends model tiers | Approves its own architecture; commits, pushes, merges, or operates devices |
44:AI_DEVELOPMENT_WORKFLOW.md:24:| Workflow controller | Claude Code | Dispatches Copilot and Codex through their CLIs; runs read-only investigation (repository, Fleet Ops, telemetry); runs builds and tests; performs workflow completeness checks; makes narrow edits only when pre-authorized case by case; commits and pushes each WO's records on the WO branch at the end of each stage; after the architect's Stage 8, commits the verified implementation and opens the WO's PR | Commits unverified source; merges, releases, or operates devices |
45:AI_DEVELOPMENT_WORKFLOW.md:102:  so `make` sees nothing to rebuild and silently links the previous build's
46:AI_DEVELOPMENT_WORKFLOW.md:106:  - **Agents:** use a fresh `BUILD_PATH_BASE` for every build type, in a
47:AI_DEVELOPMENT_WORKFLOW.md:109:  - **Reusing a path** (for example, Chip in a terminal): run `make clean` in
48:AI_DEVELOPMENT_WORKFLOW.md:111:    `BUILD_PATH_BASE` arguments as the build.
49:AI_DEVELOPMENT_WORKFLOW.md:113:  Workbench's `make clean-user` (from `buildscripts/<ver>/Makefile`) only runs
50:AI_DEVELOPMENT_WORKFLOW.md:114:  `make clean` in `deviceOS/<ver>/main` with Workbench's own arguments, so it
<shellId: 53 completed with exit code 0>
```

</details>


---

<sub>11m 48s</sub>

### `bash`

**cd /private/tmp/gcc-step0/c2 && sed -n 95,135p AI_DEVELOPMENT_WORKFLOW.md; grep -n "make -f\|PLATFORM=\|DEVICE_OS_PATH\|APPDIR" docs/work-orders/WO-2026-11-03-004-stage6-copilot-transcript.md | head -5**

$ cd /private/tmp/gcc-step0/c2 && sed -n 95,135p AI_DEVELOPMENT_WORKFLOW.md; grep -n "make -f\|PLATFORM=\|DEVICE_OS_PATH\|APPDIR" docs/work-orders/WO-2026-11-03-004-stage6-copilot-transcript.md | head -5

<details>
<summary>43 lines</summary>

```
- **Delete the object file for the affected translation unit and rebuild.**
  Clearing `target/` is *not* a clean build — it holds link output only. The
  compiled objects live at
  `~/.particle/toolchains/deviceOS/<ver>/build/target/user/platform-<id>-m/<app>/`.
- **Start every build type from clean objects, or stale object files are
  reused.** A flag passed only on the command line changes no file timestamp,
  so `make` sees nothing to rebuild and silently links the previous build's
  objects. Found 2026-09-28 in WO-2026-09-28-001, when the first bench builds
  with `-DENABLE_DIAGNOSTICS_PUBLISH_MODE=1` reused the release objects. In
  this Device OS tree (6.6.0):
  - **Agents:** use a fresh `BUILD_PATH_BASE` for every build type, in a
    directory that doesn't exist yet, for example
    `build-tmp/<wo>-stage7/obj-release` and `…/obj-bench`.
  - **Reusing a path** (for example, Chip in a terminal): run `make clean` in
    `deviceOS/6.6.0/main` with the same `PLATFORM`, `APPDIR` and
    `BUILD_PATH_BASE` arguments as the build.

  Workbench's `make clean-user` (from `buildscripts/<ver>/Makefile`) only runs
  `make clean` in `deviceOS/<ver>/main` with Workbench's own arguments, so it
  cleans only Workbench's default build path under
  `deviceOS/<ver>/build/target`. It does **not** clean a build made with the
  §3 command and its own `BUILD_PATH_BASE`. (Corrected 2026-10-07: this rule
  used to say `make clean-user`, which has no effect on agents' scratch
  builds.)
- **Keep build copies out of the repo while the host suite runs.** Structural
  tests scan the repository, and a copy of `src/` under `build-tmp/` fails
  `thermal_coupling_structural_test.py`. Run the suite before making the copy,
  or after removing it. (Temporary: remove this bullet once the test excludes
  `build-tmp/`; see the recovery plan, "Rollout and follow-ups".)
- Prove presence/absence with `nm` on the linked ELF **and** on the object
  itself. Never with `strings`.
- Record the `text`/`data` sizes at both flag values. A size that matches the
  *other* flag setting is the signature of a stale link.

**Prohibited:**

- **Restoring a build-config file in any way that rewrites its timestamp
  backwards.** `mv config.h.bak config.h` is the known example, but the rule is
  general: `cp -p`, `git stash pop`, `rsync -t`, archive extraction, and editor
  "revert file" can all restore content while preserving or backdating mtime.
6057:    `deviceOS/6.6.0/main` with the same `PLATFORM`, `APPDIR` and
6086:110:    `deviceOS/6.6.0/main` with the same `PLATFORM`, `APPDIR` and
6095:433:    make -s PLATFORM=boron APPDIR=<copy-of-tree> TARGET_DIR=<copy>/localbuild \
6151:    make -s PLATFORM=boron APPDIR=<copy-of-tree> TARGET_DIR=<copy>/localbuild \
6152:         DEVICE_OS_PATH=~/.particle/toolchains/deviceOS/6.6.0 \
<shellId: 54 completed with exit code 0>
```

</details>


---

<sub>11m 52s</sub>

### `bash`

**cd /private/tmp/gcc-step0/c2 && cat > build-tmp/wo-2026-11-03-006-stage6/runall.zsh <<'EOF'
#!/bin/zsh
cd /private/tmp/gcc-step0/c2
p=0; f=0; failed=()
for t in tests/*.sh; do
  if zsh $t > build-tmp/wo-2026-11-03-006-stage6/log_$(basename $t).txt 2>&1; then p=$((p+1)); else f=$((f+1)); failed+=($t); fi
done
for t in tests/*.py; do
  if python3 $t > build-tmp/wo-2026-11-03-006-stage6/log_$(basename $t).txt 2>&1; then p=$((p+1)); else f=$((f+1)); failed+=($t); fi
done
echo "pass=$p fail=$f total=$((p+f)) failed: $failed"
EOF
ls build-tmp/ | head**

$ cd /private/tmp/gcc-step0/c2 && cat > build-tmp/wo-2026-11-03-006-stage6/runall.zsh <<'EOF'
#!/bin/zsh
cd /private/tmp/gcc-step0/c2
p=0; f=0; failed=()
for t in tests/*.sh; do
  if zsh $t > build-tmp/wo-2026-11-03-006-stage6/log_$(basename $t).txt 2>&1; then p=$((p+1)); else f=$((f+1)); failed+=($t); fi
done
for t in tests/*.py; do
  if python3 $t > build-tmp/wo-2026-11-03-006-stage6/log_$(basename $t).txt 2>&1; then p=$((p+1)); else f=$((f+1)); failed+=($t); fi
done
echo "pass=$p fail=$f total=$((p+f)) failed: $failed"
EOF
ls build-tmp/ | head

```
wo-2026-11-03-004-stage6
wo-2026-11-03-006-stage6
<shellId: 55 completed with exit code 0>
```


---

<sub>20m 37s</sub>

### `bash`

**cd /private/tmp/gcc-step0/c2 && zsh build-tmp/wo-2026-11-03-006-stage6/runall.zsh 2>&1 | tail -5**

$ cd /private/tmp/gcc-step0/c2 && zsh build-tmp/wo-2026-11-03-006-stage6/runall.zsh 2>&1 | tail -5

```
pass=109 fail=0 total=109 failed:
<shellId: 56 completed with exit code 0>
```


---

<sub>20m 42s</sub>

### `bash`

**cd /private/tmp/gcc-step0/c2 && sed -n 6140,6165p docs/work-orders/WO-2026-11-03-004-stage6-copilot-transcript.md; grep -n "obj-release" docs/work-orders/WO-2026-11-03-004-stage6-copilot-transcript.md | head -5**

$ cd /private/tmp/gcc-step0/c2 && sed -n 6140,6165p docs/work-orders/WO-2026-11-03-004-stage6-copilot-transcript.md; grep -n "obj-release" docs/work-orders/WO-2026-11-03-004-stage6-copilot-transcript.md | head -5

<details>
<summary>25 lines</summary>

````
The two use different include-search orders. A source file can compile cleanly in
the cloud and fail locally — or the reverse — so a cloud build passing is **not**
evidence that the firmware builds. The most common form is a header name that
collides with one Device OS ships (`Config.h` is the known case; Device OS has
`services/inc/Config.h`). Within `src/`, subdirectory files include app headers
by relative path (`"../Config.h"`) precisely to avoid this; a bare include is
resolution-order dependent and may work only by accident.

    export PATH=~/.particle/toolchains/buildtools/1.1.1/bin:~/.particle/toolchains/gcc-arm/10.2.1/bin:$PATH
    cd ~/.particle/toolchains/deviceOS/6.6.0/main
    make -s PLATFORM=boron APPDIR=<copy-of-tree> TARGET_DIR=<copy>/localbuild \
         DEVICE_OS_PATH=~/.particle/toolchains/deviceOS/6.6.0 \
         BUILD_PATH_BASE=<fresh scratch dir that doesn't exist yet>

This is also the build that produces the symbolised ELF the linkage check above
requires, so the two checks share the work.

Use a new `BUILD_PATH_BASE` for each build type (see §2, "Verifying compile-time
flags"); `make clean-user` does not clean this path.

Added 2026-08-26 after WO-2026-08-25-001. An include rewritten from
`"../Config.h"` to `"Config.h"` broke the local build and survived **seven
implementation rounds and three independent Stage 7 reviews** — because every
<shellId: 65 completed with exit code 0>
```
6055:    `build-tmp/<wo>-stage7/obj-release` and `…/obj-bench`.
6176:**cd /private/tmp/gcc-step0/c1 && S=$PWD/build-tmp/wo-2026-11-03-004-stage6 && mkdir -p $S/tree && cp -R src lib project.properties $S/tree/ && export PATH=~/.particle/toolchains/buildtools/1.1.1/bin:~/.particle/toolchains/gcc-arm/10.2.1/bin:$PATH && cd ~/.particle/toolchains/deviceOS/6.6.0/main && make -s PLATFORM=boron APPDIR=$S/tree TARGET_DIR=$S/tree/localbuild DEVICE_OS_PATH=$HOME/.particle/toolchains/deviceOS/6.6.0 BUILD_PATH_BASE=$S/obj-release > $S/build.log 2>&1; echo rc=$?; tail -15 $S/build.log; ls $S/tree/localbuild**
6178:$ cd /private/tmp/gcc-step0/c1 && S=$PWD/build-tmp/wo-2026-11-03-004-stage6 && mkdir -p $S/tree && cp -R src lib project.properties $S/tree/ && export PATH=~/.particle/toolchains/buildtools/1.1.1/bin:~/.particle/toolchains/gcc-arm/10.2.1/bin:$PATH && cd ~/.particle/toolchains/deviceOS/6.6.0/main && make -s PLATFORM=boron APPDIR=$S/tree TARGET_DIR=$S/tree/localbuild DEVICE_OS_PATH=$HOME/.particle/toolchains/deviceOS/6.6.0 BUILD_PATH_BASE=$S/obj-release > $S/build.log 2>&1; echo rc=$?; tail -15 $S/build.log; ls $S/tree/localbuild
<shellId: 57 completed with exit code 0>
````

</details>


---

<sub>28m 27s</sub>

### `bash`

**cd /private/tmp/gcc-step0/c2 && S=$PWD/build-tmp/wo-2026-11-03-006-stage6 && mkdir -p $S/tree && cp -R src lib project.properties $S/tree/ && export PATH=~/.particle/toolchains/buildtools/1.1.1/bin:~/.particle/toolchains/gcc-arm/10.2.1/bin:$PATH && cd ~/.particle/toolchains/deviceOS/6.6.0/main && make -s PLATFORM=boron APPDIR=$S/tree TARGET_DIR=$S/tree/localbuild DEVICE_OS_PATH=$HOME/.particle/toolchains/deviceOS/6.6.0 BUILD_PATH_BASE=$S/obj-release > $S/build.log 2>&1; echo rc=$?; tail -8 $S/build.log; ls $S/tree/localbuild**

$ cd /private/tmp/gcc-step0/c2 && S=$PWD/build-tmp/wo-2026-11-03-006-stage6 && mkdir -p $S/tree && cp -R src lib project.properties $S/tree/ && export PATH=~/.particle/toolchains/buildtools/1.1.1/bin:~/.particle/toolchains/gcc-arm/10.2.1/bin:$PATH && cd ~/.particle/toolchains/deviceOS/6.6.0/main && make -s PLATFORM=boron APPDIR=$S/tree TARGET_DIR=$S/tree/localbuild DEVICE_OS_PATH=$HOME/.particle/toolchains/deviceOS/6.6.0 BUILD_PATH_BASE=$S/obj-release > $S/build.log 2>&1; echo rc=$?; tail -8 $S/build.log; ls $S/tree/localbuild

<details>
<summary>20 lines</summary>

```
rc=0
      |            ^~~~~~~~
/private/tmp/gcc-step0/c2/build-tmp/wo-2026-11-03-006-stage6/tree/lib/LocalTimeRK/src/LocalTimeRK.cpp: In member function 'void LocalTimeConvert::nextDay(LocalTimeHMS)':
/private/tmp/gcc-step0/c2/build-tmp/wo-2026-11-03-006-stage6/tree/lib/LocalTimeRK/src/LocalTimeRK.cpp:1255:12: warning: unused variable 'origTime' [-Wunused-variable]
 1255 |     time_t origTime = time;
      |            ^~~~~~~~
Creating /private/tmp/gcc-step0/c2/build-tmp/wo-2026-11-03-006-stage6/tree/localbuild/platform_user_ram.ld ...
   text	   data	    bss	    dec	    hex	filename
 152036	   1114	   2208	 155358	  25ede	/private/tmp/gcc-step0/c2/build-tmp/wo-2026-11-03-006-stage6/tree/localbuild/tree.elf
obj
tree.bin
tree.bin.crc_updated
tree.bin.no_crc
tree.bin.product
tree.bin.suffix_updated
tree.elf
tree.hex
tree.lst
tree.map
<shellId: 58 completed with exit code 0>
```

</details>


---

<sub>28m 33s</sub>

### Copilot

Now `nm` linkage and callers.

---

<sub>28m 35s</sub>

### `bash`

**cd /private/tmp/gcc-step0/c2 && S=$PWD/build-tmp/wo-2026-11-03-006-stage6; export PATH=~/.particle/toolchains/gcc-arm/10.2.1/bin:$PATH; E=$S/tree/localbuild/tree.elf; arm-none-eabi-nm -C $E | grep -i "closeOccupancySessionSafely\|updateOccupancyState\|processSleepWakeEvent\|debounceRemainingMs"; arm-none-eabi-nm -C $E | grep -c "session$"; arm-none-eabi-objdump -d -C $E > $S/dis.txt; grep -n "closeOccupancySessionSafely" $S/dis.txt | grep -i "bl\b\|bl " | head; strings -a $E | grep -c "untrusted piece credit"; strings -a $E | grep "pre-boot part dropped"**

$ cd /private/tmp/gcc-step0/c2 && S=$PWD/build-tmp/wo-2026-11-03-006-stage6; export PATH=~/.particle/toolchains/gcc-arm/10.2.1/bin:$PATH; E=$S/tree/localbuild/tree.elf; arm-none-eabi-nm -C $E | grep -i "closeOccupancySessionSafely\|updateOccupancyState\|processSleepWakeEvent\|debounceRemainingMs"; arm-none-eabi-nm -C $E | grep -c "session$"; arm-none-eabi-objdump -d -C $E > $S/dis.txt; grep -n "closeOccupancySessionSafely" $S/dis.txt | grep -i "bl\b\|bl " | head; strings -a $E | grep -c "untrusted piece credit"; strings -a $E | grep "pre-boot part dropped"

<details>
<summary>12 lines</summary>

```
000c2520 W debounceRemainingMs()
000c25bc T updateOccupancyState()
000c2780 T processSleepWakeEvent(bool)
000c1d38 W closeOccupancySessionSafely(char const*, long long)
1
22375:   c2086:	f7ff fe57 	bl	c1d38 <closeOccupancySessionSafely(char const*, long long)>
22876:   c25e2:	f7ff fba9 	bl	c1d38 <closeOccupancySessionSafely(char const*, long long)>
23044:   c27b4:	f7ff fac0 	bl	c1d38 <closeOccupancySessionSafely(char const*, long long)>
23321:   c2ad6:	f7ff f92f 	bl	c1d38 <closeOccupancySessionSafely(char const*, long long)>
1
Occ: untrusted straddler, pre-boot part dropped
<shellId: 59 completed with exit code 0>
```

</details>


---

<sub>28m 41s</sub>

### `bash`

**cd /private/tmp/gcc-step0/c2 && S=$PWD/build-tmp/wo-2026-11-03-006-stage6; for n in 22375 23321; do awk -v n=$n 'NR<=n && /^[0-9a-f]+ <.*>:$/ {l=$0} NR==n {print l}' $S/dis.txt; done; export PATH=~/.particle/toolchains/gcc-arm/10.2.1/bin:$PATH; find $S/obj-release -name "State_Modes.o" -o -name "Generalized-Core-Counter.o" | head -3 | while read o; do echo $o; arm-none-eabi-nm -C $o | grep -i "closeOccupancy\|session$"; done; git status --short; git diff --numstat -- src**

$ cd /private/tmp/gcc-step0/c2 && S=$PWD/build-tmp/wo-2026-11-03-006-stage6; for n in 22375 23321; do awk -v n=$n 'NR<=n && /^[0-9a-f]+ <.*>:$/ {l=$0} NR==n {print l}' $S/dis.txt; done; export PATH=~/.particle/toolchains/gcc-arm/10.2.1/bin:$PATH; find $S/obj-release -name "State_Modes.o" -o -name "Generalized-Core-Counter.o" | head -3 | while read o; do echo $o; arm-none-eabi-nm -C $o | grep -i "closeOccupancy\|session$"; done; git status --short; git diff --numstat -- src

<details>
<summary>25 lines</summary>

```
000c2004 <handleIdleState()>:
000c2a60 <handleReportingState()>:
/private/tmp/gcc-step0/c2/build-tmp/wo-2026-11-03-006-stage6/obj-release/user/platform-13-m/tree/src/state/State_Modes.o
00000000 W closeOccupancySessionSafely(char const*, long long)
         U session
/private/tmp/gcc-step0/c2/build-tmp/wo-2026-11-03-006-stage6/obj-release/user/platform-13-m/tree/src/Generalized-Core-Counter.o
00000000 B session
 M src/Generalized-Core-Counter.cpp
 M src/state/StateMachine.h
 M src/state/State_Common.h
 M src/state/State_Modes.cpp
 M tests/debounce_expiry_c1_test.sh
 M tests/occupancy_report_by_mode_test.cpp
 M tests/occupancy_report_by_mode_test.sh
 M tests/occupancy_session_restart_test.sh
 M tests/sleep_wake_equivalence_test.cpp
 M tests/sleep_wake_equivalence_test.sh
 M tests/sleep_wake_structural_test.py
 M tests/stubs/sleep_teardown/state/State_Common.h
?? tests/untrusted_piece_c2_test.sh
3	1	src/Generalized-Core-Counter.cpp
2	0	src/state/StateMachine.h
14	7	src/state/State_Common.h
5	1	src/state/State_Modes.cpp
<shellId: 60 completed with exit code 0>
```

</details>


---

<sub>29m 9s</sub>

### `create`

**create a new file at /private/tmp/gcc-step0/c2/docs/work-orders/WO-2026-11-03-006-stage6-copilot-report.md.**

/private/tmp/gcc-step0/c2/docs/work-orders/WO-2026-11-03-006-stage6-copilot-report.md

```
Created file /private/tmp/gcc-step0/c2/docs/work-orders/WO-2026-11-03-006-stage6-copilot-report.md with 7655 characters
```


---

<sub>29m 14s</sub>

### `bash`

**cd /private/tmp/gcc-step0/c2 && python3 - <<'EOF'
p='docs/work-orders/WO-2026-11-03-006-stage6-copilot-report.md'
s=open(p).read().replace("clears `lastOccupancyEvent` and set the B3 anchor","clears `lastOccupancyEvent` and sets the B3 anchor")
open(p,'w').write(s)
EOF
git status --short | wc -l**

$ cd /private/tmp/gcc-step0/c2 && python3 - <<'EOF'
p='docs/work-orders/WO-2026-11-03-006-stage6-copilot-report.md'
s=open(p).read().replace("clears `lastOccupancyEvent` and set the B3 anchor","clears `lastOccupancyEvent` and sets the B3 anchor")
open(p,'w').write(s)
EOF
git status --short | wc -l

```
14
<shellId: 61 completed with exit code 0>
```


---

<sub>29m 21s</sub>

### Copilot

C2 is implemented in the working tree, uncommitted, at **+15 net** in `src/`. The host suite passes 109/109 (sh via zsh, py via python3), and the Device OS 6.6.0 boron build succeeds.

- **Design:** an untrusted piece is credited its `millis()` on-time at the debounce expiry, closed, and reported through the same call a trusted close makes. It stays on `millis()` even if trust returns.
- **Reboot straddler:** it credits post-boot time only, and `setup()` logs one line when the pre-boot part is dropped.
- **LED gate:** the wake-close no longer waits on the LED for an open untrusted piece. The trusted wake path and `tests/sleep_wake_transcript.txt` are unchanged. The same LED-low gap for a trusted piece is not fixed.
- **Tests:** new `tests/untrusted_piece_c2_test.sh` covers the Dev-09 replay, the straddler, trust returning mid-piece, and every WO mutation (all caught). I also added a wake-close scenario to `occupancy_report_by_mode_test`.
- **Test edits:** the existing B2 tests now model a trusted piece that loses trust, with assertions untouched. The `SessionState` stubs and a few wake-gate text strings were also updated.
- **Linkage:** `nm` on the ELF shows `closeOccupancySessionSafely` called from `handleIdleState`, `updateOccupancyState`, `processSleepWakeEvent` and `handleReportingState`.

Two WO figures do not reproduce, and Chip should check the first before the Dev-14 bench:
- **Straddler credit:** the WO says about 236 s, but with a 300 s debounce and no PIR after the reboot the formula credits about 301 s. 236 s only comes out when the debounce is 60 s and PIRs continue 176 s after boot. The expected 236 s vs 536 s bench result therefore depends on Dev-14's debounce setting.
- **Dev-09 replay:** the control credits 95.0 min and the reset run 90.5 min, against 205 min before. Neither lands in 96–98, which comes from a 97.21 min duty-cycle estimate, and each piece is exactly 570 s.

The full report is `docs/work-orders/WO-2026-11-03-006-stage6-copilot-report.md`. The Dev-14 bench is not done and not claimed.

---

<sub>Generated by [GitHub Copilot CLI](https://github.com/features/copilot/cli)</sub>

# WO-2026-11-03-004: item 19 C1 — one millis-based debounce expiry for the awake path, the sleep plan and the wake close

**Author:** Claude Code, 3 Nov 2026, DRAFT for the architect's review · **Status:** DRAFT; not dispatched
**Branch:** `wo/2026-11-03-004-item19-c1-millis-expiry`, from `main` (v48 `21804e3` or later)
**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §3 Stages 5-8; §4 (SHA rule); §5 (time and boundaries: Codex xhigh); §12; §13 (c), (j)
**Target:** **v50**, with the AB1805 native-driver switch and item 21 (architect, 3 Nov). C1 is the first of two WOs; C2 (the untrusted split, credit and reporting) follows.
**Budget:** `src/` at most **+16 net** (removals count). Stop and report rather than trim style.
**Agents:** Stage 6 Copilot standard, medium. Stage 7 Codex `gpt-5.6-sol`, **xhigh**, an `UNDER REVIEW` header, `-stage7-r<N>` files, at most two rounds.

## Plain goal
Today one open session is judged by **three clocks**: a persisted wall `occupancyStartTime`, a persisted-but-boot-cleared `millis()` last-PIR (`lastOccupancyEvent`), and a **retained wall** LED deadline (`ledOffTime`). The awake path expires the debounce on `millis()`; the sleep plan and the wake-close expire it on the wall clock. While the clock is untrusted that disagreement is what lets sessions merge (recovery item 19, Dev-09 23 Oct: 205 min credited, about 97 real).
C1 makes the debounce expiry **one `millis()` predicate** used everywhere inside a boot: remaining = `debounceMs - (millis() - lastOccupancyEvent)`. It changes **no credit**: C2 does that. A trusted clean day must behave exactly as v48 does.

## Architect ruling (3 Nov) and where it comes from
Design **C** of the item-19 Step 0 (`WO-2026-11-03-001-step0-report.md` §4, §6, §10-§13): accept bounded loss; split and credit at each untrusted debounce expiry; the reboot straddler credits post-boot `millis()` only (no one-debounce fallback); at most one partial session lost per reboot while untrusted. Item 15 stays separate. C1 is the "shared monotonic start/expiry plus the sleep plan (≤16)" half.

## The change (re-check every line at Step 0; numbers are from main `21804e3`)
1. **One predicate** (`src/state/State_Common.h`, beside `closeOccupancySessionSafely`): `debounceRemainingMs()` from `millis()` and `lastOccupancyEvent`, with the zero-sentinel re-arm (`State_Idle.cpp:51-56`, `State_Modes.cpp:136-142`) handled once.
2. **Awake path** (`State_Idle.cpp:48-61`, `State_Modes.cpp:128-148`) uses it instead of its own inline subtraction.
3. **Sleep plan** (`State_Sleep.cpp:607-620`): the occupied branch schedules `min(remaining + 1 s margin, report wake)` from the predicate. **This replaces item 20's RTC branch** (`signalLEDTimeRemaining()`, WO-2026-10-25-001), which is removed, not kept as a second deadline. If the predicate says expired, keep today's full-debounce fallback so behavior never gets worse.
4. **Wake close** (`State_Modes.cpp:166-185`): the PIR-wake path closes on the same predicate before it processes the PIR. Wake sources do not change (`State_Sleep.cpp:760-780`).
5. **`ledOffTime` stays for the LED** (the COUNTING/OCCUPANCY LED on-time is still a wall deadline); only the **close criterion** moves off it. Do not touch `signalLED*` otherwise.
6. **Within-boot only:** a reboot clears `lastOccupancyEvent` as today (`Generalized-Core-Counter.cpp:497-505`); C1 adds no persistence and no retained field.

## Protected / not in scope
`closeOccupancySessionSafely` and its B2/B3 rules (C2 changes them); credit arithmetic (`State_Common.h:388-405`); the report path; the boundary close; the persisted and retained layouts (zero change); `ledOffTime`'s LED role; item 21; item 15.

## Tests (host; the real helpers, not copies)
- Plan harness: occupied with remaining 300, 120, 5, 1, 0 s crossed with a near and a far report boundary: wake = `min(remaining + 1, boundary)`, full debounce when expired. The S-4 transcript (`tests/fixtures/sleep_plan_transcript.txt`) changes only in the occupied cases listed at Step 0 (check the list, do not just accept the fixture).
- Wake close: a PIR wake after expiry closes first, then handles the PIR; before expiry it extends.
- **Untrusted vs trusted give the same expiry times** (the point of C1): the same transcript with the clock trusted and untrusted.
- B2/B3 harness (`tests/occupancy_session_restart_test.sh`) and `occupancy_report_by_mode_test.cpp` unchanged and green.
- Mutations (each must fail): restore the RTC deadline in the plan; use the full debounce always; wake close using the wall clock; predicate off by the margin; sentinel not handled.
- `nm` on the local-toolchain ELF: the predicate's production call sites in the awake path, the plan and the wake close.

## Acceptance and bench (Codex xhigh; the Step 0 report's §8 exerciser plan)
Control and fault passes on Dev-14 (exerciser: 570 s session every 20 min), no cloud, valid RTC-seeded `Time` with no confirmed sync this boot. For C1 alone: sessions still **merge exactly as v48 does while untrusted** (C1 changes no credit) but each sleep and wake lands within the debounce window (1 s margin) in both trust states; capture `Occ`, `Sleep`, `WakeReturn`, `ClockResync`. The credit acceptance (96-98 min for 97.21 real) belongs to C2.

## Rollback
Revert the PR; v48 behavior returns; no data to migrate.

## Open questions for the architect
1. Is the 1 s margin kept as in WO-2026-10-25-001, or should the predicate return the exact remaining and the caller add the margin?
2. Does `lastOccupancyEvent` need a name change (it is persisted, boot-cleared `millis()`), or stay as is (zero layout change)? Recommendation: stay.
3. Confirm C1 is to ship in v50 only if C2 does (C1 alone fixes nothing in the field).

## Amendment 1 (architect rulings, 3 Nov) — status: APPROVED to dispatch
1. **The predicate returns the time to expiry** (`debounceRemainingMs()`, 0 when expired); **the caller (the sleep plan) adds the 1 s margin.** The awake path and the wake close use the raw value.
2. **Keep the name `lastOccupancyEvent`** (zero layout change).
3. **C1 may merge when verified, but releases only with C2, in v50** (the PR is marked "do not release before C2"; the product default is not changed by C1 alone).
4. **Acceptance adds:** re-pass the **item-20 :52 straddle bench test** (the exerciser phase that puts one 570 s session across the hourly :00 report; v45 credited 12-13 min instead of 9.5; the sleep must wake at the time left, not the full 300 s). That bench test is run by Chip on Dev-09/Dev-14 after Stage 7; the PR states it as pending until then.
5. **PRs:** the drafts have no PR of their own; this WO goes with its implementation PR.
6. Agents and budget unchanged: Copilot standard/medium; Codex `gpt-5.6-sol` xhigh, §4 SHA rule, at most two rounds; `src/` ≤ +16.

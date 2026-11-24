# WO-2026-11-06-001: v50 piece S4 — the oscillator-fault latch ("fault seen since bind")

**Author:** Claude Code, 6 Nov 2026, DRAFT for the architect's review; not dispatched · **Status:** DRAFT (pre-ruled: S1 Amendment 2 and Amendment 3 ruling 4 send the latch here, at most +10 `src/`)
**Parent:** `WO-2026-11-05-002-v50-ab1805-switch.md` (including the 6 Nov rulings: the S-set stays on `integration/v50-ab1805` until all five bench items and both 8 h nights pass on its CI binaries) · **Depends on:** S1 (`WO-2026-11-05-003`, the bind and `Am18x5.status()`) · **Consumer:** I21 (`WO-2026-11-05-007`), which reads only the boolean
**Branch (when dispatched):** `wo/…-v50-s4-oscillator-fault-latch`, from `integration/v50-ab1805` after S1 has passed Stage 7 and its PR has been merged to the integration branch; S4's PR goes to the integration branch, **never to `main` alone**.
**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §3 Stages 5-8; §4 (SHA rule); §5 (time and trust inputs: Codex xhigh); §12; §13 (c), (j) · **Agents:** Stage 6 Copilot standard/medium (re-run at the dispatched level if a run comes back lower on anything larger than a few lines); Stage 7 Codex `gpt-5.6-sol` **xhigh**, at most two rounds · **Budget:** `src/` at most **+10 net** (removals count); stop and report rather than trim style.
**Dispatch gate:** S1 reviewed (Stage 7 passed) and merged to the integration branch. S4 does not merge to `main` before the set.

## Plain goal
I21 will let clock trust carry across a restart, and must withdraw it on evidence. One piece of evidence is "the oscillator faulted since the chip was bound". Device OS reports it as an event; the application needs a single boolean it can read. S4 adds that boolean and nothing else: **a process-lifetime flag in RAM, never persisted** (Chip's ruling: no persisted latch and no persisted clock-source byte). It is S1's budget overflow: S1 implemented the callback and compiled it, but it was about +9 lines over S1's +20, so it was removed and moved here.

## Rulings that bind this piece (not reopened)
1. **The latch is a boolean, RAM only,** named for what it means (for example `oscillatorFaultSeenSinceBind`), declared where I21 can read it without a new dependency (Step 0 chooses the home; `time/ClockTrust.h` is the pure, host-testable neighbour; the setter lives with the registration).
2. **It is set** (a) by an `ExternalTime.onEvent` callback for `RtcEvent::CLOCK_SOURCE_EXTERNAL_FAILURE`, and (b) **at registration** if `Am18x5.status().clockSource()` reports the RC fallback (a persistent fault is caught at boot by the clock source, not by an event). It is **never cleared** in the process; a reboot starts clean and re-derives (b).
3. **I21 reads only the boolean.** S4 contains no trust logic, no revoke, no flush: those are I21's.
4. **Order (Chip, 6 Nov): `clockSource()` is read BEFORE the handler is registered.** Registering a handler clears the chip's oscillator-fail flag (`am18x5.cpp:646`), so a fault already flagged at boot is seen only through the clock source; read it first, set the flag from it, then register the callback. Step 0 confirms the order from source and records it; it does not reopen it.
5. **Persisted continuity evidence is not added** (Chip, 6 Nov; see I21's "Why persisted continuity evidence is not needed" and the residual window).

## Facts from S1's Stage 6 Step 0 (Device OS 6.6.0; **Step 0 here re-verifies each with file:line**)
- `RtcEvent::CLOCK_SOURCE_EXTERNAL_FAILURE = 0x02` (`exrtc_hal.h:86-88`); the callback is `void(RtcEvents, void*)`; the event is mapped from the chip's XT oscillator-failure condition at `am18x5.cpp:695-715`.
- The oscillator-failure interrupt (OFIE) is enabled **only with the external-crystal oscillator** (`am18x5.cpp:595`, `:635`), and **registering a handler clears the OF flag** (`:646`). So the clock source must be read **before** registration (ruling 4), or a fault already flagged is lost.
- S1 already publishes `usingRC` from `Am18x5.status().clockSource()` at boot; S4 reuses that read.

## The change (re-cite on the then-current integration head)
1. The boolean and its accessor (`inline`, no new `.cpp` if it fits).
2. After S1's bind in `setup()`: read `clockSource()`; if RC fallback, set the flag; register the `ExternalTime.onEvent` callback whose body only sets the flag (and may log one line, counted in the budget).
3. Nothing else. No change to `Clock`, the write-back, the payload, `retained` or persisted layouts.

## Step 0 (read-only, inside Stage 6, before editing; stop and report if any answer breaks the plan)
1. Re-verify the three Device OS facts above with file:line, including the flag clear at `:646`, which is why the ruled order (read `clockSource()`, then register) is the one that cannot lose a fault; and whether the callback can run before the flag is declared (interrupt versus loop context: is the setter safe to call from the event path, and does the event path run on the application thread?).
2. **Where the flag lives** so I21 can read it at the first `isClockTrusted()` call after boot (that call must see the registration-time value): name the header, the lifecycle, and whether `setup()` order guarantees it is set before the first trust decision.
3. What a **mid-boot fault** does today to anything that already read the flag (nothing, by design: I21 owns the revoke); confirm no S1/S2/S3 code path consumes the flag.
4. **Count** the net `src/` lines; if over +10, stop and report the cut (S1 measured about +9 for the callback and registration; the boolean and the registration-time read are the remainder).

## Tests (host; the real functions)
Fakes for `Am18x5`/`ExternalTime`: the flag is false after a normal bind (crystal source); true after a bind that reports the RC fallback (registration-time rule); true after a delivered `CLOCK_SOURCE_EXTERNAL_FAILURE` event; stays true after a second event and after the source later reads healthy (never cleared); the callback is registered once and after the bind (structural, in order); no persistence write is made (structural: no `set_`/`flushNow` in S4's code). Mutations (each must fail): registration-time check removed; event ignored; flag cleared by a later read; callback registered before the bind; the handler registered before the `clockSource()` read (ruling 4, a structural order test); flag persisted; the wrong event enum.

## Bench (Chip; part of the integrated head's bench, not alone)
Host injection is the required proof. Physical induction of a crystal fault is **optional** (the spike marked it unsafe). On the integrated head, confirm on a bound device that the flag reads false on a healthy crystal and (if a device on the RC fallback is available) true at boot, via the existing boot log line S4 adds or the `usingRC` status field.

## Protected / not in scope
I21 (every trust decision), S1-S3, L; the clock-trust rule, the failsafe, item 19; the persisted and `retained` layouts; M-SoM.

## Rollback
Revert the S4 commit on the integration branch before the set merges. After the set ships, rollback is to v49.1 (the guard deletes the stored binding).

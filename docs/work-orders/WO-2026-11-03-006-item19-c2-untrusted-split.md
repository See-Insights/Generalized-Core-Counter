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

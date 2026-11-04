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

## Amendment 2 (controller, 4 Nov 2026; from Stage 7 round 1, `WO-2026-11-03-006-stage7-r1-verdict.md`, NOT VERIFIED on `acbace55`): fixes and corrected acceptance numbers — architect to confirm
Three blockers; round 2 (the last) follows the fixes.
1. **Stale piece start at the boundary reopen.** `closeOccupancySessionSafely` clears `pieceUntrusted` but not `occupancyStartMs` (`State_Common.h:453`), and the `close==24` always-open reopen (`State_Report.cpp:77`) sets only `occupied` and the wall start. **Fix:** clear both piece fields at close, and every open path (including that reopen, a trusted piece) sets `occupancyStartMs = millis()` and `pieceUntrusted = !Clock::isTrusted()`.
2. **A piece must stay wholly on `millis()` after trust returns, but two wall-start checks still apply to it** (`start == 0` at `State_Common.h:398`; `start > now + 5` at `:416`): a ten-hour backward correction or a zero wall start discards a correctly measured 360 s. **Fix:** those two checks apply to the trusted wall-subtraction path only; the one-day session cap and the daily-total validation stay for pieces. Add the backward-correction and zero-wall-start tests.
3. **The bench acceptance numbers in this WO ("96-98 min", "about 236 s") are wrong for the specified timeline; replace them** (Dev-14 inherits `setting1` = 300000 ms; sessions of 570 s every 1,200 s from 10:35:21; pulses 30 s apart):
   - **Before the day boundary is processed (checkpoint at 14:00):** control = 10 closed pieces = 5,700 s = **95.0 min**; fault (reset 334 s into the first piece) = 301 + 9 × 570 = 5,431 s = **90.5 min**; the payload truncates /60, so it shows 95 and 90. (v45 credited 205.)
   - **At the close report after trust returns past the boundary** the eleventh piece (opened 13:55:21) is credited its full 570 s, because the piece formula ignores `closeAt`: control 6,270 s = **104.5 min** (payload 104), fault 6,001 s = **100.0 min** (payload 100), plus wake overhead. Bench readers must say which checkpoint they use.
   - "97.21 min" is 47.5 % of the whole 12,279 s window; the physical session time through 14:00 is 5,979 s (99.65 min) because the eleventh piece is 279 s old then. It is not an acceptance figure.
   - **Straddler:** the first predicate evaluation re-arms `lastOccupancyEvent`, so with no PIR after the reboot the straddler credits first-evaluation millis + 300 s, about **301 s** (not 236 s); a PIR before expiry gives last PIR since boot + 300 s. 236 s reproduces only with a 60 s debounce and PIRs to 176 s after boot. 536 s would mean a one-debounce fallback crept in (pre-boot part counted).
   - Item 15: if the persisted `occupied` is lost, there is no straddler and the credit is zero.

## Amendment 3 (proposed by the controller, 4 Nov 2026, after Stage 7 round 2; **APPROVED by the architect, 4 Nov**): the bench pass rule is a formula on measured pieces
Stage 7 round 2 (`WO-2026-11-03-006-stage7-r2-verdict.md`, NOT VERIFIED on `bb6e75d8`) verified the implementation (stale-field fix on every open/close path, wall checks confined to trusted pieces, 109/109, six independent mutants caught, linkage) and kept **one blocking acceptance-spec finding**: the fixed figures of Amendment 2 are nominal, not a deterministic pass rule. The straddler credits `floor((firstEvaluationMs − setupStartMs + 300000) / 1000)`, which is 301 s only when the setup-to-first-evaluation gap is 1,000-1,999 ms (setup can add up to 31.9 s of explicit delay, plus unbounded synchronous work that feeds the watchdog); bench sessions run 569-581 s, not exactly 570; `dailyoccupancy` is `floor(totalSeconds / 60)`.
**Proposed pass rule (Dev-14, Chip):**
- **Pre-boundary control:** `dailyoccupancy` equals `floor(S / 60)` where `S` is the sum of the first ten measured clean pieces (each from its `Occ` open and close log lines; expect 569-581 s each).
- **Pre-boundary fault:** `S = measured straddler + nine clean pieces`; the straddler is `(first-evaluation millis − setup millis) + 300 s` from the log (about 301 s; **a value near 536 s, or any value including pre-boot time, means a one-debounce fallback crept in**).
- **Close report (after trust returns past the boundary):** add the measured eleventh piece (it is credited in full); payload = `floor(S / 60)`.
- Nominal examples for ten or eleven exact 570 s pieces and a 1 s gap: 95 / 90 min before the boundary, 104 / 100 at the close report (v45 credited 205).
- Fault pass also requires: a close in every 630 s gap (never one 20-minute session), and the straddler credits post-boot time only.
**Disposition requested:** approve Amendment 3. Stage 7 then closes by architect ruling with no round 3: the code at `bb6e75d8` is verified; the only blocker was the bench criteria text.

**Architect ruling (4 Nov): Amendment 3 approved.** The bench pass rule is a formula on measured pieces: `dailyoccupancy` = floor of the measured sum / 60 (control: ten measured clean pieces; fault: measured straddler + nine clean pieces; close report: add the measured eleventh piece); the straddler is about 301 s, and **about 536 s means a fallback crept in**; a close in every 630 s gap. **Stage 7 closes by architect ruling on `bb6e75d8` (code verified in round 2); no round 3.**
**Hold (architect correction, 4 Nov): releases tag `main`, so C1 (WO-2026-11-03-004) and C2 do not merge until v49 is tagged.** Both PRs are held (draft, label "v50: merge after the v49 tag"); they ship in v50.

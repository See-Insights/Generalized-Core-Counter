AGENT: Codex · MODEL: gpt-5.6-sol (probed 2026-10-29) · REASONING: high · stdin closed except this dispatch

# Stage 7 dispatch: WO-2026-10-29-002 (occupancy ends at the close; item 10; v48)

You are the independent verifier. Do not edit `src/` or `tests/`. You may write only the verdict file given by `-o`. No network.

**Review the uncommitted diff** (`git diff` and the untracked `tests/occupancy_ends_at_close_structural_test.py`) against: `docs/work-orders/WO-2026-10-29-002-occupancy-ends-at-close.md` (binding), `...-step0-report.md` (with the architect's rulings: gate `closeHour == 24` sites with the same helper and keep the carry-over; clamp the credit to the boundary), `...-stage6-copilot-dispatch.md`, and `...-stage6-report.md` (Copilot's report plus the controller's reproduction).

**Check, with file:line evidence for each:**
1. The gate is `== Closed` only; 0-24 sites and Unknown openness behave exactly as before at both sites (`State_Modes.cpp` awake and wake paths) and in the clamp.
2. COUNTING is unchanged. The cleanup in `State_Report.cpp` and the `close == 24` carry-over are unchanged.
3. The closing report's `occupancy` field and the next day's first `dailyoccupancy` (0), traced through `PIRSensor::resetTotals` and `reportFields`.
4. The clamp: `DailyBoundary::check(now).boundary` is the most recent boundary in every case (before open, after close, `close == 24`). Any caller of `closeOccupancySessionSafely()` whose behavior changes when Closed (list them: "modes", "idle", "sleep", "daily-cleanup", others).
5. **Concerns to rule on (Low/Medium/High each):**
   a. The wake-path gate returns before the low-battery policy refresh that sits just below it in the OCCUPANCY branch; closed PIR wakes therefore skip that refresh. Is that a regression?
   b. A `close == 24` carried session now ends with 0 credit (its start is the boundary, the clamp holds closeAt at the boundary). The architect said "keep the carry-over unchanged": is the credit change within the ruling, and does the carried `occupied=true` still report correctly?
   c. The awake-path `Log.info` fires on every ignored detection; any log-volume or flash-wear concern.
   d. After an ignored wake the routing continues as before (no new wake loop or thrash); confirm from `processSleepWakeEvent`'s callers.
   e. Tests +305 against the +100 guideline: anything redundant, or masking a real gap.
6. Linkage: a production call site for the helper (it is inlined: the controller's `nm` shows size growth of the callers, not a symbol).
7. Test adequacy: do the added cases and the four named mutations prove the behavior; name any gap.

**Verdict file** (`docs/work-orders/WO-2026-10-29-002-stage7-verdict.md`, at most 80 lines): `VERIFIED`, `VERIFIED WITH CONCERNS` (each rated Low/Medium/High) or `NOT VERIFIED`, with the evidence per check above and the model and reasoning level used.

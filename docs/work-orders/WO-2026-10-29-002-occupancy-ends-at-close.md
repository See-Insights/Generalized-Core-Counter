# WO-2026-10-29-002: occupancy ends at the close; nothing between the close and night sleep is counted (recovery item 10)

**Author:** architect, 2026-10-29 · **Status:** ready (Step 0 first)
**Branch:** `wo/2026-10-29-002-occupancy-ends-at-close`, from main. It ships in **v48**. It is the gate for the main waves of the legacy-occupancy migration (WO-2026-10-29-001).
**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §3 Stages 6–8; §12.1–12.4; §13 (b), (c), (g), (j)
**Evidence and decision:** recovery item 10 and its Codex Step 0 (#164). Exerciser cases: Dev-09 +7 and Dev-14 +5 min credited into the next day on 28 Oct; Dev-09 on 23 Oct. **The architect's decision (29 Oct): discard.**

## Plain goal

**Today:** the daily-close cleanup zeroes the day's total. A device still awake after the close keeps its sensor powered until night sleep begins, because the close report's connect can take minutes. Any session it opens or closes in that window is added to the **zeroed** counter, and the morning wake doesn't zero it again, so it lands in the **next** day's total.

**After this WO,** for normal park hours (`openHour` < `closeHour`, not 0–24):
1. A session open at the close is credited **up to the close** (today's rule) and is **ended** by the close.
2. **No detection between the close and the next opening starts a session or adds credit.**
3. The next day starts at **0**.

0–24 sites keep today's boundary-splitting behavior unchanged.

## Step 0 (Claude Code, read-only, §13 (c))

1. Re-check the mechanism Codex found, citing every site at main:
   - the close cleanup;
   - where a session open at close is credited, and whether the cleanup ends it;
   - the awake detection path (`handleOccupancyMode()`, `updateOccupancyState()` in `State_Modes.cpp`);
   - the wake detection path (`processSleepWakeEvent()`, after S-6);
   - the 0–24 boundary splitting.
2. **Find the single cleanest gate.** The expected answer is one check, "openness is `Closed` (trusted) on a normal-hours site → ignore the detection", at the point where both paths start or extend a session. A check at the close cleanup ends any open session. Prefer one shared helper over a check in each path.
3. **Edge cases to reason through:**
   - Unknown openness (untrusted clock): keep today's behavior, so item 19's fix stays separate;
   - a reset between the close and night sleep;
   - COUNTING mode, which is unaffected (OCCUPANCY only; the counting rules come with convergence);
   - the closing report's `occupancy` field.
4. **History (§12.1):** git history for the close cleanup and the occupancy gates.
5. **Size:** expected +5 to +12 `src/` lines. **PROCEED** at ≤ +20.

## Tests

- **Host, on the S-6 wake harness and the occupancy tests:**
  - a session open at the close is credited to the close and ended;
  - a detection after the close, awake and on a wake, starts nothing;
  - the next day's first report shows 0;
  - a 0–24 site is unchanged;
  - Unknown openness is unchanged.
- **Mutations,** each of which must fail: remove the gate; gate on `!= Open` instead of `== Closed`; leave the session open at the close.

## Bench (the exerciser does it)

On the v48 soak, the first report each morning reads `dailyoccupancy` **0**, with no carry-in (against +5 to +7 on v47). The day's last session before 22:00 is still credited to the close.

## Budget

- **`src/`:** +5 to +12 net, hard limit +20.
- **Tests:** guideline +100. Record ruling 9's numbers.

## Agents and models

- **Step 0:** Claude Code, read-only.
- **Stage 6:** Copilot, Sonnet tier, **medium**: a small gate at a site Step 0 names.
- **Stage 7:** Codex `gpt-5.6-sol`, **high**, stdin closed. This changes what is counted at the day boundary on every occupancy device. It checks:
  - the 0–24 and Unknown cases unchanged;
  - the closing report;
  - no COUNTING change.
- **Stage 8:** pre-approved on VERIFIED, or on VERIFIED WITH CONCERNS when every concern is Low. Controller test-only fixes follow §13 (d).
- **PR:** marked "ships in v48". It can merge when its Stage 8 closes, since v47 is already cut.

## Closing record

- Budget versus actual.
- The gate site.
- The edge-case answers.
- Ruling 9's numbers.

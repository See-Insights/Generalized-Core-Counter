AGENT: Codex · MODEL: gpt-6-astra · REASONING: high
AUTHORIZATION SCOPE: review the uncommitted diff; run the host suite and a local ARM build in a scratch copy (or with outputs restored); write temporary harnesses under your scratch directory **`build-tmp/wo20260924-004-stage7/`** and remove **only that directory** when done; temporarily mutate files for checks, restoring each byte-identically by rewriting in place / Not authorized: lasting edits, commits, pushes, stash, reset, checkout, flashing, device settings, network or AWS access, and deleting anything you didn't create, including `build-tmp/` itself and `build-tmp/connectivity-archive/`.

# Stage 7 dispatch (narrow) — WO-2026-09-24-004 (v36-HourRules)

**Goal, in plain language:** open and close hours always follow the three rules, enforced where settings are applied.

**The rules:** (1) always-open is exactly 0/24; (2) `closeHour > openHour`; (3) `0 ≤ openHour ≤ 12`. Together: `openHour ∈ [0, 12]`, `closeHour ∈ (openHour, 24]`.

**Repository:** `Generalized-Core-Counter`, branch `wo/2026-09-24-004-hour-rules`, base `6d8aaf9` (v35), with the uncommitted diff.
**Binding spec:** `docs/work-orders/WO-2026-09-24-004-retire-open-equals-close-convention.md`, including Step 0, the fact check, the Stage 5 decisions (including assumptions 4 and 5) and Acceptance.
**Stage 6 report:** `docs/work-orders/WO-2026-09-24-004-stage6-copilot-report.md`.

Narrow review: check against the WO's acceptance criteria only. Don't widen the fault model or propose new mechanisms. For any defect, give the smallest fix within the approved design.

## Checks (PASS/FAIL with evidence for each)

1. **Each rule rejects its violation and keeps the previous hours:**
   - for each rule, at least one violating pair is rejected at all three validators: `applyTimingConfig()`, `sysStatusData::validate()` and `Config::validateConfigFields()`;
   - in `applyTimingConfig()`, the previous hours are kept (both or neither applied), the rejection is logged with values and rule, and `success` is false;
   - the rules are written in exactly one place.

   Also say whether the "rule 1" label for `closeHour > 24` in `hoursRuleFailure()` is accurate. It only affects the log text, so report it as a note.
2. **0/24 behaves exactly as always-open did** under the old `open == close` sentinel:
   - open at every hour;
   - the daily close at midnight (`todayAt(24)`);
   - the v32 failsafe's open-hours age.

   Report the `secondsUntilNextOpenForSeconds()` value difference noted in the WO, and confirm whether any caller's behavior changes because of it.
3. **6/22 (the default) and Trail02's 6/23 are unchanged:** open/closed at every hour, the next-open seconds, and the daily close boundary are identical to `6d8aaf9`.
4. **For every valid pair,** v32's connectivity failsafe (`Generalized-Core-Counter.cpp`, the open-hours age) and v28's close-before-sleep give identical results before and after. A differential harness against the `6d8aaf9` sources is fine.
5. **Mutations:** removing or weakening each rule in turn fails a test. Also check the pair rule in `applyTimingConfig()`: applying the hours separately must fail a test.
6. **Removals are complete and safe:**
   - no `open == close` sentinel and no overnight branch remain in `src/`;
   - no other reader of the hours depended on them (the WO's fact check lists the readers);
   - the `daily_cleanup_boundary_test.py` update keeps its intent.
7. **Persisted pairs at boot** (assumption 4): a stored pair that breaks the rules makes `sysStatusData::validate()` fail, so StorageHelperRK falls back to `initialize()` (6/22). Confirm this is the only new effect, and that every Step 0 device's current hours pass.
8. **Suite and build:**
   - every `tests/*.sh` with **zsh**, plus every bare `tests/*.py` with python3, as `N/N` (Copilot: 62/62);
   - `tests/publish_with_ack_structural_test.py`, `tests/sleep_config_ownership_structural_test.py` and `tests/ledger_no_retry_test.sh` unchanged and green;
   - a clean boron release build: text/data/bss against v35's 150684 / 1090 / 2180 (Copilot: 150740 / 1090 / 2180); `strings` shows `v36-HourRules`, product 36.
9. **The budget:** net `src/` lines (nonblank, non-comment) must be negative (Copilot: −16), by your counting rule; nothing compressed.

## Verdict (required)

VERIFIED, NOT VERIFIED, or VERIFIED WITH NOTES, per check and overall, with evidence; binary sizes; the model and reasoning level actually used. Confirm that the working tree is byte-identical to how you found it, and that nothing outside `build-tmp/wo20260924-004-stage7/` was created or deleted.

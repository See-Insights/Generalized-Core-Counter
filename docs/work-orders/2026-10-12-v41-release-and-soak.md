# v41 release and bench soak

**Author:** architect, 2026-10-12 · **Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §2, §13 (h), (i), (j)
**Agent:** Claude Code (release PR, then soak grading). No model dispatch needed. **Budget:** 0 `src/` lines beyond the version bump.

## Plain goal

Ship v41 = the TimeDiag fix (#85, WO-2026-10-09-001), the once-a-day config window (#86, WO-2026-10-09-002) and the standby-latch daily clear (WO-2026-10-09-003), after a 24 h soak on the bench.

## 1. Release PR (Claude Code)

- From current main, after confirming all three WOs are merged.
- `bump_version.sh` to `v41-ConfigWindowLatch` (or the closest name under the convention); product version 41.
- CHANGELOG: move "Unreleased" into v41, one line per WO, plain language. Include: config changes now reach every sleeping device within a day (one short wait per day and at boot); a latched modem standby is re-allowed at each daily close; TimeDiag logs only on change in CONNECTED.
- No other source changes. Fresh `BUILD_PATH_BASE` build; `strings` shows the version; tests `N/N (sh via zsh, py via python3)`.
- Open the PR for Chip. Don't merge.

## 2. Bench soak (forwarder, OTA) — Chip does the device steps

Start: Chip OTAs v41 to Dev-09 and Dev-14 only (both locked to v41). Each device graded on its own 24 h window from its first v41 boot.

**Dev-14:** KEEP_ALIVE throughout (reference).

**Dev-09, in this order:**
1. **Config window:** once on v41, Chip changes one harmless setting in Dev-09's device-settings (e.g. `messaging.verboseTimeoutMin`). Pass: `LedgerCallback: kind=input` then `Config:` at the next boot or first-after-open connection, **with no pin reset**. Record the connection that delivered it.
2. **CONNECTED:** then Chip sets `"connectionMode": 0`. Pass over ≥ 4 open hours: one `TimeDiag` per state change, no flood; no reset 140 (data 2); scheduled reports present. This also closes 1b's deferred CONNECTED step.
3. **Back to KEEP_ALIVE:** Chip sets `connectionMode` back to 3. Pass: it lands while connected and the device resumes sleeping.

**Both devices — standard checks:** occupancy start and end report immediately (KEEP_ALIVE stretches); no unexpected resets; heap stable; no `GateFail` except the known post-boot alert 44; `watchdogResetCount` unchanged. Count the daily config wait: about one extra wait per device per day (log the gate wait at the first-after-open connection). Any breadcrumb-28 watchdog holds the fleet rollout.

**Standby latch:** not directly testable on the bench (both devices hibernate). If any `MODEM_POLICY: standby re-allowed at daily close` line appears, record it.

## 3. Grading

PASS or FINDING per device with log lines, ≤ 60 lines in chat; then add the result to this file and commit on the release branch or a docs branch. If both pass, list the fleet steps for Chip: set v41 as the product default; Dev-09/Dev-14 stay locked for the next soak.

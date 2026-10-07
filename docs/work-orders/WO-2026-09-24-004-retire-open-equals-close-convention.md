# WO-2026-09-24-004: Open and close hours follow three rules

**Goal, in plain language:** open and close hours always follow three rules, enforced where settings are applied. A setting that breaks them is rejected, the device keeps its last valid hours, and the rejection is logged. Then remove the code the rules make unnecessary. Expected net change: fewer lines.

**Status:** **APPROVED at USER GATE 2** (Chip, 2026-10-03), after Stage 7 (Codex `gpt-6-astra` high) found no production defect. 62/62; **−16** net `src/` lines (budget: net negative). Both Stage 7 notes are applied. Committed for Chip's merge. Version `v36-HourRules`, product 36.

**Branch:** `wo/2026-09-24-004-hour-rules`, from `main` at `6d8aaf9` (v35 merged, PR #58; PR #59 merged). All file:line below were checked against `6d8aaf9` on 2026-10-03.

**History:** filed 2026-09-24 (per WO-2026-09-24-001 Revised, Stage 5 decision 4) to retire the `openHour == closeHour` always-open sentinel. It was blocked on a per-site configuration audit. That audit was done on 2026-10-02 (below), and no device depends on the sentinel.

## The rules (set by Chip, 2026-10-02)

1. **Always-open is exactly `openHour = 0`, `closeHour = 24`.**
2. **`closeHour > openHour`.** This also retires overnight windows.
3. **`0 ≤ openHour ≤ 12`.**

Together: `openHour ∈ [0, 12]` and `closeHour ∈ (openHour, 24]`. These are also the bounds checks for the future web front-end, so the front-end and the firmware must accept and reject exactly the same pairs.

## Why (the v32 reason)

WO-2026-10-02-001 (v32) makes the connectivity failsafe count **open hours only**. Its age is `now − max(lastConnection, todayAt(openHour))`, and it acts only while open. With the legacy `open == close` (always-open) convention, that rule misbehaves:
- at 6 = 6 it first acts at **09:00**;
- at 21, 22 or 23, three hours never accumulate before the daily base advances, so **it never acts**.

Stage 7 (Codex, 2026-10-02) confirmed that the current validators make this reachable. Retiring the convention, with always-open expressed as 0/24, removes it.

## Effective hours across the fleet (read-only, 2026-10-02)

**Sources:**
- Particle Ledger API (read-only): the product-scoped `default-settings` instance (42131) and every `device-settings` instance. Only 3 exist.
- **The only code path that sets the hours** is `Cloud::applyTimingConfig()`, merging device over product (`ConfigApply.cpp:279–300`).
- The firmware default is 6/22 (`Config.h:31–32`).
- Corroborated by behavior: closing reports stamped one second before the configured close, and the bench devices' `TimeDiag open=6 close=22`.

| Device | Override (`device-settings`) | Effective open / close | Behavioral evidence | Rules |
|---|---|---|---|---|
| ToM-MCP-Court1, Court2, Court3 | none | 6 / 22 (product default) | closing reports stamped 21:59:59 EDT | OK |
| ToM-MCP-PCKL1, PCKL2, PCKL3 | none | 6 / 22 | 21:59:59 EDT | OK |
| Morrisville MAFC-1, MAFC-2 | none | 6 / 22 | 21:59:59 EDT | OK |
| SAMIT-TRAIL02 | `closeHour: 23` (and `enableHibernateSleep: true`, set 2026-10-01 23:17Z) | **6** (product default) / **23** | closing stamp 22:59:59 EDT | OK |
| Boron-Dev-09 | `openHour: 6`, `closeHour: 22` | 6 / 22 | `TimeDiag open=6 close=22` | OK |
| Boron-Dev-14 | `openHour: 6`, `closeHour: 22` | 6 / 22 | `TimeDiag open=6 close=22` | OK |

**No device breaks the rules, and none uses the always-open sentinel.** That resolves the earlier "Trail02 might be 23/23" concern: it has no `openHour` override, so it gets the product default 6.

**Caveat:** the ledgers hold the cloud-pushed configuration, not the on-device value. The device-status ledger publishes only a hash of the timing values (`DeviceStatusPublisher.cpp:109–110`); WO-2026-09-24-003 would publish the values themselves. The behavioral evidence above is what ties the ledger values to what each device actually uses.

## Step 0: effective hours re-checked (read-only, 2026-10-03, Claude Code)

**Sources:** the product `default-settings` instance (open 6, close 22) and every `device-settings` instance, from the Particle Ledger API.

| Device | Override | Effective open / close | Rules |
|---|---|---|---|
| Court1, Court2, Court3, PCKL1, PCKL2, PCKL3, MAFC-1, MAFC-2 | none (no instance) | 6 / 22 | OK |
| SAMIT-TRAIL02 | `closeHour: 23` | 6 / 23 | OK |
| Boron-Dev-09, Boron-Dev-14 | `openHour: 6`, `closeHour: 22` | 6 / 22 | OK |

**Result:** no device breaks a rule, so the work proceeds. This matches the 2026-10-02 audit above.

## Fact check (Claude Code, 2026-10-03, against `6d8aaf9`)

The cited lines are confirmed:
- `ConfigApply.cpp:279–300`;
- `DailyBoundary.cpp:37`;
- `Clock.cpp:14–24` (`isWithinOpenHoursForHour()`: the overnight branch at `:17–19`, the always-open branch at `:20–22`);
- `Clock.cpp:26–61` (`secondsUntilNextOpenForSeconds()`: the overnight branch at `:47–56`, the always-open branch at `:57–60`).

**Two more validators the change list missed** would block 0/24, because each rejects a close hour above 23:
- **`MyPersistentData.cpp:94–104`** (`sysStatusData::validate()`). When it fails at load, StorageHelperRK **resets the whole `sysStatus` record to firmware defaults** (`lib/StorageHelperRK/src/StorageHelperRK.h:890–891`). So without this change, a device given 0/24 would lose all its persisted settings at the next boot.
- **`Config.cpp:69–90`** (`Config::validateConfigFields()`, called from `Cloud.cpp:601` after an apply). It would mark the configuration invalid.

**The other readers of the hours** handle 24 safely:
- `isWithinOpenHoursForHour(h, 0, 24)` is always true;
- `DailyBoundary::todayAt(24)` is the next midnight (`DailyBoundary.cpp:12–13`);
- the v32 failsafe's `todayAt(openHour)` with open 0 is midnight (`Generalized-Core-Counter.cpp:2597`);
- the alert-40 suppression at the opening hour (`:1470`) never fires at 0/24, because the device never hibernates;
- the rest only log the hours.

**One difference from the old sentinel:** `secondsUntilNextOpenForSeconds()` with `openNow` true returns the time until tomorrow's opening. At 0/24 that's midnight. Under the sentinel it was tomorrow at the shared hour. The window is always open in both cases, so this is a value difference only. Stage 7 confirms it has no behavioral effect.

## Stage 5 decisions (2026-10-03)

1. **The three rules** (Chip, 2026-10-02), as above.
2. **On a rejected setting:** keep the last valid hours and log why (the opening dispatch).
3. **Budget:** net negative `src/` lines. If it can't be net negative, STOP and report.
4. **Open question 1** (a persisted pair that breaks the rules, at boot). Claude Code's assumption, **to confirm at USER GATE 2**: keep today's mechanism. With the rules in `sysStatusData::validate()`, StorageHelperRK resets the record to firmware defaults (6/22), and the cloud configuration re-applies at the next connection. This is what already happens to hours above 23. No device has such a pair (Step 0).
5. **Open question 2** (more than a log line?). Assumption, to confirm at USER GATE 2: **a log line only**, per the opening dispatch ("log why").

## Change (after v32)

**Enforce, in `Cloud::applyTimingConfig()`** (`ConfigApply.cpp:279–300`):
- **Today** `openHour` and `closeHour` are each range-checked as 0–23 and applied **independently**. So `closeHour = 24` is rejected, which means always-open = 0/24 can't be set at all, and a half-valid pair can be applied.
- **After:** take the merged pair, check it against the three rules, and apply **both or neither**.
- A pair that breaks a rule is rejected: the device **keeps its last valid hours**, the rejection is logged (with the offending values and the rule), and the apply is reported as failed, as `validateRange()` failures are today.

**Apply the same rules in the two other validators** (from the fact check). Use one shared check, the single place the rules are written, instead of separate range checks:
- `MyPersistentData.cpp:94–104`;
- `Config.cpp:69–90`.

**Remove what the rules make unnecessary:**
- `DailyBoundary.cpp:37`: `close = (openHour == closeHour) ? 24 : closeHour;` becomes `close = closeHour` (24 is now a real value).
- `Clock.cpp:20–22`: the `openHour == closeHour` always-open branch in `isWithinOpenHoursForHour()`. 0/24 is a plain window.
- `Clock.cpp:57–60`: the same branch in `secondsUntilNextOpenForSeconds()`.
- `Clock.cpp:17–19`: the overnight-window branch (`openHour > closeHour`), now unreachable under rule 2. Confirm before removing.

**Check against 0/24:**
- `todayAt(24)` and `isWithinOpenHoursForHour()` both handle `closeHour = 24` (always open).
- The v32 failsafe's open-hours age with `openHour = 0`.
- The daily close at 24:00.

**Open questions for Stage 5:**
1. **A persisted pair that breaks the rules** (from older firmware or an earlier push) at boot: keep it, or fall back to the firmware default 6/22? No current device has one.
2. **Does the error log need to be more than a log line,** for example an alert code or a field in the device-status ledger?

## Tests (outline)

- A table test of the rules: valid pairs (6/22, 6/23, 0/24, 12/13) are accepted and applied as a pair. Invalid pairs are rejected with the last valid hours kept, including 6/6, 22/22, 20/6, 13/22, 0/25, 6/24-but-open-13 (rule 3), and close ≤ open.
- `DailyBoundary` and `Clock` behave unchanged for every valid pair, and correctly for 0/24.
- Structural checks that no `openHour == closeHour` sentinel and no overnight branch remain.

## Interaction with WO-2026-09-24-001

WO-2026-09-24-001 (Revised) normalized the sentinel inline (`DailyBoundary.cpp:37`, originally in `State_Report.cpp`) so the daily-reset fix would work for always-open devices before this WO landed. This WO removes that normalization once the rules make it unnecessary.

## Interaction with WO-2026-10-02-001 (v32)

v32's failsafe counts open hours only; with open == close (legacy always-open), it acts from 09:00 at 6 = 6, and never at 21–23. Retiring the convention removes this.

## Acceptance (Stage 7, narrow)

1. **Each rule rejects its violation and keeps the previous hours,** and the apply is reported as failed. One shared check is used by all three validators.
2. **0/24 behaves exactly as always-open did:** open at every hour, the daily close at midnight, and v32's failsafe as before. Report the `secondsUntilNextOpenForSeconds()` value difference (see the fact check) and confirm it has no behavioral effect.
3. **6/22 (the default) and Trail02's 6/23 are unchanged.**
4. **For every valid pair, v32's failsafe and v28's close-before-sleep give identical results** before and after.
5. **A mutation for each rule** makes a test fail.
6. **The suite passes:** every `tests/*.sh` with zsh plus every bare `tests/*.py` with python3, as N/N. The WITH_ACK, sleep-configuration and ledger no-retry tests pass unchanged.
7. **The build:** a clean boron release build; `strings` shows `v36-HourRules`, product 36.
8. **The budget:** net negative `src/` lines, with nothing compressed. Record budget versus actual.

## Bench (Dev-09 and Dev-14, v36-HourRules; Chip)

**Setup:** Chip set rule-breaking hours in each device's `device-settings` ledger.

| Device | Setting | Rule broken | Changed (ledger `updated_at`) |
|---|---|---|---|
| Dev-09 | `openHour` 13 (close 22) | rule 3 | 10:34:18Z, 3 Oct |
| Dev-14 | `openHour` 11, `closeHour` 10 | rule 2 | 10:36:21Z, 3 Oct |

**Pass:** each device logs `Invalid hours openHour=… closeHour=… (rule …); keeping 6/22`, and no `Open/close hours ->` line appears. After that:
- `TimeDiag` still shows `open=6 close=22`;
- reports and the open/closed state are unchanged;
- the status shows `v36-HourRules`;
- there is no run of `LedgerDuplicateStillInflight`.

**Expected side effect:** a rejected setting makes the apply report failure, so ConnectState raises alert 41 ("Configuration apply failed") at each connection while the test values stay.

### Check 1, 2026-10-03 10:38Z (Claude Code: serial archive and Particle API). Result: PENDING, neither device has connected since the change

| Device | Last heard (Particle) | Last connection in serial | Firmware |
|---|---|---|---|
| Dev-09 | 10:30:33Z | `10:30:07 Connect: ok elapsed=25794ms` (after a reboot at about 10:29Z) | fw 36 |
| Dev-14 | 10:28:45Z | `10:28:20 ConnSummary: ok` | fw 36 |

- **Neither device has seen the new settings yet,** so there is no `Invalid hours` or `Open/close hours ->` line in either log.
- **Next connection:** both should pick the settings up at the 11:00Z report. A second check is scheduled for 11:12Z.
- **Observation (not this WO):** Dev-14 logged `Config: Connection mode -> INTERMITTENT_KEEP_ALIVE` at both connections (10:22:12, 10:28:26), and its sleep line reads `mode=INT tier=C` (10:30:28). That is live evidence of the connection-mode tug-of-war already recorded under Step 6 of the recovery plan.

### Check 2, 2026-10-03 11:12Z (Claude Code: serial archive, status and report events, Particle API)

**Dev-09 (open 13, rule 3): PASS.** It updated to v36 by OTA: status `v36-HourRules`, reset reason 70, at 10:30:03Z. The first connection after the change that synced the settings was the 19:00 SGT report. Serial (UTC):

```
11:00:21 ConnSummary: ok elapsed=11219 last=CONNECTED … sig=82/29 heap=76384 q=1
11:00:25 WARN: Invalid hours openHour=13 closeHour=22 (rule 3: openHour must be 0-12); keeping 6/22
11:00:26 WARN: Configuration apply failed: sensor=OK timing=FAIL messaging=OK modes=OK reporting=OK power=OK
11:00:26 WARN: Configuration apply failed (will raise alert 41)
11:00:27 WARN: Alert 41 context: ledgersSynced=true connectDuration=394 ms
11:00:29 Connect: ok elapsed=11720ms sig=82/33 q=1 heap=76344
```

- **The rejection:** the right rule and values, and the device kept 6/22. **No `Open/close hours ->` line** at any point since the change.
- **Only the timing section failed;** every other section applied. **Alert 41 was raised, as expected.**
- **The status ledger** was rewritten at 11:00:23Z: `firmware.version` `v36-HourRules`, `config.generation` `BA69F072`, clock trusted.
- **Reports are unchanged:** 18:43:51 SGT (dd 96, cyc 28), then the hourly 19:00:03 (dd 96, cyc 29, slp 28).
- **Limits of the evidence:**
  - **TimeDiag:** no `TimeDiag` line has been logged since the change, so the "keeping 6/22" line is the direct evidence of the hours in use;
  - **open/closed state:** at 19:00 SGT both 6/22 and 13/22 count as open, so this check can't tell them apart. The next evidence is tomorrow's 06:00 SGT wake: under 13/22 the device would stay asleep until 13:00.
- **`LedgerDuplicateStillInflight`:** one line, not a run, at 10:46:43 (`kind=DATA orig=ReportState new=ConnectState age=159842 … pendingData=1`). That's v35's single deferral during a slow connection (cloud recovery stage 2, 159 s).

**Dev-14 (open 11, close 10, rule 2): NOT TESTED YET.** It has not connected since the change.
- **Particle `last_heard`:** 10:28:45Z.
- **Status ledger:** last written 10:28:28Z (`v36-HourRules`, generation `4A9E5686`), before the 10:36:21Z change.
- **Serial:** nothing since 10:30:33Z. The forwarder logged `SerialException … device disconnected` at 10:30:30.
- **What was expected:** in CONSERVING / intermittent (1800 s × 2), the next aligned connection was 11:00Z, and it didn't happen.
- **Earlier events:** an OTA to v36 at 10:23:24Z (reset reason 70), then a restart between 10:23 and 10:28 (cyc 5 → 2) with no status event in the archive.
- **Needs a physical check** (USB, power), then a recheck.

### Check 3, 2026-10-03 15:02Z (Claude Code: serial archive, report events, Particle API). Result: **Dev-14 PASS**, so **the bench PASSES on both devices**

**Dev-14 (open 11, close 10, rule 2): PASS.** It reconnected at 12:00Z, after Check 2. Its tier had dropped, so connections were spaced 2 hours apart. Serial (UTC):

```
12:00:25 ConnSummary: ok elapsed=13988 last=CONNECTED … sig=78/25 heap=76152 q=5
12:00:44 WARN: Invalid hours openHour=11 closeHour=10 (rule 2: closeHour must be greater than openHour); keeping 6/22
12:00:45 WARN: Configuration apply failed: sensor=OK timing=FAIL messaging=OK modes=OK reporting=OK power=OK
12:00:53 INFO: TimeDiag: tz=SGT-8 … local=2026-10-03 20:00:44 open=6 close=22 isOpen=1 trusted=1 …
14:01:32 WARN: Invalid hours openHour=11 closeHour=10 (rule 2: closeHour must be greater than openHour); keeping 6/22
14:01:36 WARN: Configuration apply failed (will raise alert 41)
14:01:51 WARN: Invalid hours openHour=11 closeHour=10 (rule 2: …); keeping 6/22
```

- **The rejection:** the right rule and values, and the device kept 6/22. **`TimeDiag` confirms `open=6 close=22`.** No `Open/close hours ->` line at any point.
- **Alert 41** was raised at the 14:01 connection. A connection can apply the configuration twice (on connect, and again when the settings ledger syncs). Each apply rejected the hours.
- **Reports are unchanged:** every 30 minutes (Dev-14's interval is 1800 s), from 18:30:02 to 21:30:02 SGT, delivered in batches at 12:00Z and 14:01Z. **The closing report is stamped 21:59:59 SGT**, so the device closed at 22:00 under 6/22, which shows the rejected hours were never used.
- **Status ledger:** rewritten at 14:01:27Z, `v36-HourRules`.
- **`LedgerDuplicateStillInflight`:** one line per connection, not a run (12:00:35 and 14:01:39). Each is v35's single deferral. Because connections are 2 hours apart, the in-flight DATA write had waited about 90 minutes (`age=5415393`).

**Observations (not this WO):**
- **The connection-mode tug-of-war (Step 6):** `Config: Connection mode -> INTERMITTENT_KEEP_ALIVE` at both connections (12:00:32, 14:01:33).
- **An odd battery tier:** the 12:00:52 sleep line reads `tier=CR` (CRITICAL), while reports show 65% charge, which would normally be CONSERVING. Possibly the voltage-floor path; worth a look separately.

**Bench result: PASS.** Both rule violations were rejected, each with its rule and values, and both devices kept 6/22. Dev-09's open/closed check at the 06:00 SGT wake is a further confirmation, not a gate.

## Approval record

- [x] Stage 5: Chip, 2026-10-03, in the opening dispatch. Covers the rules, enforcement in ConfigApply, the removals, a net-negative budget, the Stage 7 checks, and v36 / product 36. Routing: one Copilot round (`claude-opus-5`, medium) and one narrow Stage 7 (Codex, `gpt-6-astra`, high). Not authorized: commits, merging, flashing, device settings.
- [x] Step 0 (Claude Code, 2026-10-03): no device breaks a rule.
- [x] Fact check (Claude Code, 2026-10-03): two validators added to the change list. Open questions 1 and 2 are assumed as above, to confirm at USER GATE 2.
- [x] Stage 6: Copilot (`claude-opus-5`, medium), 2026-10-03. Report: `WO-2026-09-24-004-stage6-copilot-report.md`.
  - **Result:** **net −16 `src/` lines** (budget: net negative). One shared `Config::hoursRuleFailure()` / `hoursFollowRules()`, used by all three validators.
  - **Tests:** 61/61 before, 62/62 after. The new `hour_rules_test` checks the rules exhaustively and that every valid pair behaves as before, and its three rule mutations are detected. `daily_cleanup_boundary_test.py` was updated because it pinned the removed normalization.
  - **Build:** 150740 / 1090 / 2180 (+56 text over v35).
  - **Deviations (2):** `#include "../Config.h"` in `ConfigApply.cpp` (another `Config.h` shadows it), and the check is `inline` in `Config.h`.
  - **Claude Code's review note:** `hoursRuleFailure()` labels `closeHour > 24` as "rule 1". That bound comes from the combined range `(openHour, 24]`, so only the log text is affected.
- [x] Stage 7: Codex (`gpt-6-astra`, high), 2026-10-03. Verdict: `WO-2026-09-24-004-stage7-verdict.md`. **VERIFIED WITH NOTES.**
  - **Checks:** all 9 pass.
  - **Coverage:** 1,024 pairs at all three validators. A differential harness ran 234 valid pairs × 86,400 s (20.2 M cases) for openness, the failsafe age and the daily boundary / close-before-sleep, all identical. 6/22 and 6/23 match `6d8aaf9` at every second. 961 stored-pair checks; every Step 0 device passes.
  - **Mutations:** all 5 fail, including applying an hour independently.
  - **Suite and build:** 62/62; build 150740 / 1090 / 2180.
  - **Working tree:** byte-identical afterwards.
  - **Note 1 (log label):** `closeHour > 24` should be labeled "combined hour bounds", not "rule 1" (`Config.h:50`).
  - **Note 2 (stale test model):** `daily_cleanup_boundary_test.py:48` still normalizes equal hours and uses a 7/7 midnight fixture. Fix: `close = close_hour` and a 0/24 fixture.
  - **Qualification:** 0/24 preserves the old 0/0 failsafe age. Next-open seconds differ from a nonzero sentinel (at noon, 6/6 gave 64,800 and 0/24 gives 43,200), but neither caller changes behavior: the sleep caller requires Closed, and the diagnostic caller excludes open hours.
- [x] **Notes 1 and 2 applied (Claude Code, authorized by Chip, 2026-10-03):**
  - `Config.h`: the `closeHour > 24` message now reads "combined hour bounds: closeHour must be at most 24". Behavior is unchanged.
  - `daily_cleanup_boundary_test.py`: the model uses `close = close_hour`, and the midnight fixture is 0/24 (was 7/7).
  - **Suite:** 62/62 (sh via zsh, py via python3). `hour_rules_test` still detects all three rule mutations.
- [x] **USER GATE 2:** Chip, 2026-10-03. Approved; assumptions 4 and 5 confirmed (reset to defaults at boot through the existing mechanism; a log line only for a rejected setting). Commit, push and open the PR; don't merge.

## Budget versus actual (closing record)

| Item | Budget | Actual net `src/` lines | Tests |
|---|---|---|---|
| Rules enforced (three validators) plus the removals | net negative | **−16** (Stage 7's count) | `hour_rules_test` (+1); `daily_cleanup_boundary_test.py` updated |

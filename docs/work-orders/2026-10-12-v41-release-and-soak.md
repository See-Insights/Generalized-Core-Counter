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

## 4. Result (graded 2026-10-14 by Claude Code; architect accepted the config-window test as PASS)

**Both devices: PASS.** No breadcrumb-28 watchdog, so the stop condition does not apply. **v42 may go to the fleet.**

**Build under test:** product v41 holds the release PR's first build, which reports `v41-ConfigWindowAndStandby` (`caf6760`). The only `src/` difference from the merged v41 head is the version string. That is why the fleet release is v42 (PR #92).

**Sources:**
- S3 raw events for 12–13 Oct (`status`, `watchdog`, `hibernate_wake`, `Ubidots-Sensor-Hook-v1`);
- forwarder serial: Dev-14 06:01Z 12 Oct–06:02Z 13 Oct (240 lines), and Dev-09 06:02Z 12 Oct–06:03Z 13 Oct (420 lines);
- Particle API ledger reads.

**Windows** (first v41 boot, reset 70):
- Dev-14: 12 Oct 06:01:20Z to 13 Oct 06:01:20Z.
- Dev-09: 12 Oct 06:02:23Z to 13 Oct 06:02:23Z.

| Check | Dev-14 | Dev-09 |
|---|---|---|
| Resets after the v41 boot (S3 `status`) | reset 30 only (hibernate wake, 22:00:38Z 12 Oct, `ALARM`, ok) | reset 30 only (22:00:23Z 12 Oct, `DEEP_POWER_DOWN`, ok); no reset 140, no pin reset |
| `watchdogResetCount` / last-watchdog uptime (S3 `status`) | 126 → 126, unchanged | 46 → 46, unchanged |
| `watchdog` events (S3) | none | none |
| Reports in the window (S3 webhook) | 42; none missing in open hours (30-min slots) | 26; none lost. In CONNECTED the hourly report runs from the last report (10:52:36 SGT), then aligned to 12:00 after the revert |
| cyc − slp = 1 (S3 webhook) | all 42 | all 26 (CONNECTED: cyc 1 / slp 0) |
| Occupancy changes reported immediately (S3 webhook; serial) | 9 change reports, delivered ≤ 14 s after stamp | 12 change reports, ≤ 16 s; each serial `Occ:` line is followed within 3 s by `StateReq: …->Report` (7 of 7 forwarded) |
| Close stamped close − 1 s; morning daily total 0 (S3 webhook) | 13:59:59Z, then `dailyoccupancy` 0 at 22:00Z | 13:59:59Z (delivered 14:07:31Z after a 437 s cloud recovery), then 0 at 22:00Z |
| Heap (serial `heap=`; webhook `fh`) | 75424–89768 (89768 = just after a boot) | 75616–77416 |
| Alerts in reports; `GateFail` (serial) | none; none | none; none |
| Daily config wait (serial `ConfigHold: ended`) | 2: 14:01:43 SGT on 12 Oct (v41 boot), 06:00:53 SGT on 13 Oct (wake boot); both `timeout`, 10 s | 2: 14:03:06 SGT on 12 Oct (v41 boot, `timeout`), 11:20:30 SGT on 13 Oct (`timeout`; see the CONNECTED note) |
| `MODEM_POLICY` / `MODEM_HEALTH` lines (serial) | none | none |

**Dev-09 steps:**
1. **Config change without a pin reset: PASS.** `verboseTimeoutMin: 59` and `connectionMode: 0` were written at 14:17:56 SGT on 12 Oct. Both landed together at **22:07:47 SGT on 12 Oct**, at the close-hour connection (`LedgerCallback: kind=input ledger=device-settings`, then `Config: Verbose timeout -> 59 min` and `Config: Connection mode -> CONNECTED`). That was during the sleep gate's ordinary outbound wait, not a config hold. The hold's own onSync path was proved separately, outside the window: `verboseTimeoutMin: 60`, written 22:17:20 SGT on 13 Oct, landed at the 14 Oct 06:00 boot connection (`06:00:53 ConfigHold: ended reason=onSync elapsed=0`, `06:00:54 LedgerCallback: kind=input ledger=device-settings`, `06:00:55 Config: Verbose timeout -> 60 min`). The architect accepted it as PASS.
2. **CONNECTED for 4 or more open hours: PASS.** 06:00 to 11:20 SGT on 13 Oct, 5 h 20 min.
   - TimeDiag: **11** lines, each on an entry into Idle. No flood.
   - No reset 140; scheduled and change reports present.
   - Cloud loss: 06:23:56–07:00, with no reconnect until the scheduled report. This is queued WO 8.
3. **Back to KEEP_ALIVE: PASS.** Written 11:20:16 SGT, then `11:20:21 Config: Connection mode -> INTERMITTENT_KEEP_ALIVE` while connected. It slept, and woke at `12:00:06 Sleep->Report` with continuous uptime.

**Counts asked for:**
- **THRASH (WO 8 false alarm):** Dev-09 **7**, all tier 1, all on leaving Idle in CONNECTED (06:24:01, 07:00:10, 07:42:22, 08:42:22, 09:46:57, 10:52:40, 11:20:25 SGT). There were at most 2 in any 60-minute escalation window (07:42:22 and 08:42:22 were 59:59 apart), so tier 2 and tier 3 were not reached. Dev-14: **0**.
- **Daily config waits:** one per device per day, plus the v41 boot.
- **`MODEM_POLICY: standby re-allowed`:** **0** on both, as expected; the bench devices hibernate.

**Notes, not findings:**
- **Dev-09's first-after-open hold ran at 11:20:30, not at 06:00.** In CONNECTED during open hours, the sleep gate (where the hold is evaluated) isn't reached until the device next sleeps.
- **Dev-09's cloud errors** (DNS 202, PPP data=5, `Failed to dial`, `No response from NCP`) are the Singtel bench pattern. Every connection recovered, and no report was lost.

**Fleet steps (Chip):**
1. Upload `v42-ConfigWindowLatch-boron-ff1715c.bin` as product firmware v42. Check that Particle shows **152558 bytes** and sha256 **`d34e7288bba5faa6336d89690244487c9f336400a3f8f1534ba08884d5ddf31a`** (§13 (j)).
2. OTA v42 to **Dev-14 only**. Confirm one boot (reset 70) and one normal report on `v42-ConfigWindowLatch`.
3. Set v42 as the product default. Dev-09 and Dev-14 stay locked for the next soak.
4. After this, #94 (WO 2a) may merge; it ships in v43.


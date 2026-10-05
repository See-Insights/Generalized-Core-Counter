AGENT: Codex · MODEL: gpt-6-astra · REASONING: high
AUTHORIZATION SCOPE: review the uncommitted diff; run the host suite and a local ARM build in a scratch copy (or with outputs restored); write temporary harnesses under your scratch directory **`build-tmp/wo20261004-001-stage7/`** and remove **only that directory** when done; temporarily mutate files for checks, restoring each byte-identically by rewriting in place / Not authorized: lasting edits, commits, pushes, stash, reset, checkout, flashing, device settings, network or AWS access, and deleting anything you didn't create, including `build-tmp/` itself and `build-tmp/connectivity-archive/`.

# Stage 7 dispatch (narrow) — WO-2026-10-04-001 (v37-PreStep6Fixes)

**Goal, in plain language:** the device never waits on the modem in its main loop or goes to sleep with the modem half off; a restart in the middle of an occupancy session doesn't lose that session's minutes; an on-time hibernate wake is reported as a success; and every report carries the battery's cell voltage.

**Repository:** `Generalized-Core-Counter`, branch `wo/2026-10-04-001-pre-step6-fixes`, base `79abe84` (v36), with the uncommitted diff. Ignore the unrelated uncommitted edit to `docs/work-orders/WO-2026-09-24-004-retire-open-equals-close-convention.md` and the untracked `docs/` files.
**Binding spec:** `docs/work-orders/WO-2026-10-04-001-pre-step6-fixes.md`, including its fact-check corrections, Chip's decisions (B-timing, B-anchor, C-rule), and the Stage 6 record.
**Stage 6 report:** `docs/work-orders/WO-2026-10-04-001-stage6-copilot-report.md`.

Narrow review: check each item against its goal and the WO's acceptance criteria only. Don't widen the fault model or propose new mechanisms. For any defect, give the smallest fix within the approved design, as a proposed diff; don't apply it.

## Checks (PASS/FAIL with evidence for each)

1. **A1, no modem wait for signal:**
   - no `Cellular.RSSI()`/`WiFi.RSSI()` (direct, or through `sampleConnectionSignal()`) is reachable in `State_Connect.cpp` before `Particle.connected()` is true or on the `elapsedMs > budgetMs` timeout path;
   - exactly one read remains, after connection;
   - `Connect: ok` reuses it;
   - say whether any other `src/` call to `Cellular.RSSI()` is reachable from the main loop;
   - confirm `tests/connect_no_modem_wait_structural_test.py` fails under a mutation that restores each removed read.
2. **A2, sleep only with the modem off:**
   - on the non-standby cellular path, `System.sleep()` (hibernate `:~1156`, ULP `:~1409`, STOP fallbacks) is unreachable while `Cellular.isOff()` is false, including `isOn=false, isOff=false`;
   - the wait uses the existing budget and alert-15 timeout;
   - standby sleep is unchanged;
   - no new timer, state or breadcrumb;
   - confirm `tests/sleep_gate_modem_off_test.sh` exercises the real gate logic (not a re-typed copy that could drift) or say how it's tied to the source, and that reverting any of the three sites fails it.
3. **B, a restart doesn't lose the session's minutes:**
   - a restart mid-session credits up to the boot time, capped at `max(occupancyStartTime, lastReport) + debounce`, at the first trusted close;
   - an untrusted clock leaves the session open, credits nothing, re-arms the debounce, and causes no repeated `REPORTING_STATE` transition or `OccAnom` line at any of the three callers;
   - the debounce no longer uses the previous boot's `millis()`;
   - a host check of a long power-off (≥ 10 h) shows over-credit ≤ one debounce;
   - no new persisted or `retained` field (check `SessionState` isn't retained or persisted);
   - the daily-boundary close (`State_Report.cpp:59`) still behaves correctly for a session that was open across a restart;
   - **the Stage 6 review note:** quantify the over-credit when trust lapses mid-boot (24 h after the last sync, `ClockTrust.h:42`) with the debounce expiring during the lapse. Say whether v36 discarded those sessions (under-credit) and whether v37's behavior is acceptable within the WO's goal, or needs the smallest fix within budget (B has 1 line left).
4. **C, on-time hibernate wakes succeed:**
   - `DEEP_POWER_DOWN` on time (0 to +60 s) → `result:"ok"` with real `actual`/`err`;
   - late (+61 s), early (a button), `WATCHDOG` and `UNKNOWN` → `fail`;
   - `ALARM` unchanged;
   - mutations of each window bound fail a test.
5. **D, every report shows the cell voltage:**
   - `vc` is an unquoted JSON number in both formats, `0.00` when no plausible sample, never NaN;
   - no new quoted key;
   - recompute the worst-case payload length for both formats (state your width assumptions) before and after; ≤ 255 + NUL.
6. **Existing tests changed by Stage 6 keep their intent:** `hibernate_wake_diagnostics_test.sh`/`.cpp` and `report_payload_fields_test.py`.
7. **Suite and build:**
   - every `tests/*.sh` with **zsh**, plus every bare `tests/*.py` with python3, as `N/N` (Stage 6 and Claude Code: 65/65);
   - `publish_with_ack_structural_test.py`, `sleep_config_ownership_structural_test.py` and `ledger_no_retry_test` unchanged and green;
   - a clean local boron 6.4.1 release build: text/data/bss against v36's 150740 / 1090 / 2180 (Stage 6: 150788 / 1090 / 2188); `strings` shows `v37-PreStep6Fixes`, product 37;
   - linkage: `nm` shows the changed functions in the ELF;
   - Claude Code already ran the Particle cloud compile (succeeded; flash 152014 / RAM 3282).
8. **Budget:** net `src/` lines per item (nonblank, non-comment) against A ≤ 20, B ≤ 15, C ≤ 3, D ≤ 4, by your counting rule; nothing compressed.

## Verdict (required)

VERIFIED, NOT VERIFIED, or VERIFIED WITH NOTES, per check and overall, with evidence. Include:
- binary sizes;
- budget versus actual per item;
- the model and reasoning level actually used.

Confirm that the working tree is byte-identical to how you found it, and that nothing outside `build-tmp/wo20261004-001-stage7/` was created or deleted.

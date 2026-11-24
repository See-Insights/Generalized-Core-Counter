# WO-2026-11-16-001: Feed the chip watchdog through its register, and make a failed feed visible

**Author:** architect, 16 Nov 2026 (dispatched by Chip's relay the same day, after the Dev-14 control test) · **Status:** APPROVED, Stage 6 next
**Branch:** `wo/2026-11-16-001-watchdog-feed-register` from `integration/v50-ab1805 @ 1d1d5af7a22dc47094a4c23b515b348bd8ae153d`; the PR targets `integration/v50-ab1805` after the architect's Stage 8
**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §3 Stages 5-8; §4 (SHA rule, at most two Codex rounds, three-level verdicts)
**Agents:** Stage 6 **Copilot** (`claude-sonnet-5.5`, **low** effort); Stage 7 **Codex `gpt-5.6-sol` at high**.
**Budget:** `src/` **at most +4 net** (Step 0 estimate +3). Tests: new Python test counts as a new harness, guideline +175 added lines; record the actual, do not stop for it.

## Plain goal
A device that is awake and running must not be reset by the chip watchdog every 124 s. The loop must keep the chip fed, and if a feed ever fails the log must say so.

## Evidence
- Dev-09 on 5001, 15 Nov: resets #52-#54 (reason 20 / data 0, alert 19 inferred) 119-128 s after an ULP wake, loop still running (breadcrumb 18). No awake window over 117 s survived. Report: `docs/work-orders/2026-11-16-v50-chip-watchdog-124s-codex-report.md` on branch `docs/2026-11-16-v50-watchdog-124s-investigation` (commit `d66ae98`; Codex Stage 4, gpt-5.6-sol high, on `1d1d5af`).
- Control test, Dev-14, 5001 flashed 16 Nov 12:55Z, `connectionMode: 0`, never sleeping: USB drops at 12:57:58, 13:00:02, 13:02:05, about 124 s apart; the longest awake window was about 124 s. Serial-forwarder events in the S3 archive (`serialLog/e00fce688e592afaf23ac4fb`).
- Mechanism (VERIFIED in Device OS 6.6.0 `hal/shared/am18x5.cpp:934-946`): with a watchdog pin configured, `feedWatchdog()` only toggles that GPIO; with none, it writes the WDT register. The fleet configuration passes `.watchdogPin(WATCHDOG_DONE_PIN)` (D5). Whether D5 is wired to the chip's WDI pin is **unverified**. The legacy firmware fed the chip by register write (`ab1805.setWDT` / `loop`).
- The reset reason of the 124 s resets was **confirmed by timing, not by a captured reason**.

## The change
1. **`src/Generalized-Core-Counter.cpp:295`:** remove `.watchdogPin(WATCHDOG_DONE_PIN)` from the fleet `Am18x5Configuration`; leave a trailing comment saying the feed goes through the WDT register. `WATCHDOG_DONE_PIN` stays declared in `device_pinout` (hardware record).
2. **`src/Generalized-Core-Counter.cpp:1133`:** keep the refresh call (still the only one, still after the state handlers and before `checkClockResync()`), keep its return code, and log it when it changes: `ExternalWatchdog: refresh rc=%d` (the first failure, and the return to 0). One latch, `static int lastRefreshRc`. This is "when the return code changes", not "once per wake" (see Step 0, item 4).
3. **Tests:**
   - `tests/external_rtc_structural_test.py:36-37`: the fleet config is Wire/WKP with **no** watchdog pin. Add the mutation "pin restored" (`.watchdogPin(WATCHDOG_DONE_PIN)` put back) to its list; it must be caught.
   - `tests/external_rtc_test.cpp:26-28, 87-99`: `fleet()` has no watchdog pin (`wdtPin == -1`); a stored config with `wdtPin == 5` (a chip bound by 5001/5099) differs and is re-bound once.
   - **New `tests/external_watchdog_feed_test.py`** (Codex's mirror): the fleet config is read from `src/`; a model of Device OS's `feedWatchdog()` route and of the chip's 124 s counter runs a wake of more than 124 s with a feed every 30 s. With the real config: a register write per feed, the counter never expires. Mutations: the pin restored (the counter expires at 124 s: the test must fail), the refresh removed, and the rc logging removed. The real function cannot run on the host (the Device OS watchdog HAL is a stub), so this is a model; its limits go in the test header.
   - `tests/s2_alarm_watchdog_wake_structural_test.py` must stay green (`ExternalWatchdog.refresh()` appears once, before `checkClockResync()`).

## Protected areas
`ExternalWatchdog.init/start/stop` sites and the 124000 ms timeout, `State_Sleep.cpp`, `ConnectivityFailsafe.cpp`, the awake (Device OS) watchdog (60 s), the RTC bind path other than the one pin argument.

## Non-goals
No change to timeouts, to the sleep/wake bracket, to alert 19, or to the wiring question. Anything else goes to the recovery plan.

## Compatibility
A chip already bound by 5001/5099/5097 has the D5 config stored: the first boot of the fix logs `AB1805: stored config differs, re-bound r=0` once (`ExternalRtc::ensureBound`), then boots clean. Units coming from v49 are unbound and bind once.

## Acceptance (bench, Chip, in order)
1. Fixed build, `connectionMode: 0`: no reset for 30 min.
2. 5097 (no refresh) rebuilt on the fixed base: it must still reset at about 124 s, so the watchdog still bites when it should.
3. Then the remaining items (3b, 1, 2, 4, the S2 proof, 5096) and one more night, on the fixed build.
The CI-built integration binary's SHA-256 is recorded in the checklist.

## Readiness notes
- D5's wiring to the chip's WDI pin is unverified; the fleet now feeds through the register, as the legacy firmware did.
- The reason of the 124 s resets was confirmed by timing, not by a captured reset reason.

## Rollback
Revert the PR on `integration/v50-ab1805`. Nothing is persisted by this change.

## Approval record
Architect and Chip, 16 Nov 2026 (relay: "Dispatch WO-2026-11-16-001 now, as prepared").

## Closing record (17 Nov 2026)
**Stage 7:** round 1 on `99ece4e`: VERIFIED WITH FIXES (1 Medium, tests: refresh placement not mutation-protected). Fix applied by Copilot (Stage 6 r2), no further Codex round; head `365358e`. Stage 8: the architect read the `src/` diff and signed it off (17 Nov, no `watchdogPin`; refresh rc logged when it changes).
**Controller checks on `365358e`:** suite 122/122 (sh via zsh, py via python3); the three mutation-bearing tests pass (`external_watchdog_feed_test` 5 mutations caught, `external_rtc_structural_test` all caught, `s2_alarm_watchdog_wake_structural_test` 21 probes detected); CI `build` on `365358e` green (firmware and host-tests). Local build text/data/bss `151124/1114/2264` against base `151084/1114/2256` (+40/0/+8).

| Item | Budget | Raised to (reason) | Actual net `src/` lines | Tests |
|---|---|---|---|---|
| Pin removal and refresh rc log | <= +4 | - | +3 | +137 added / -8 removed (guideline +175): new `external_watchdog_feed_test.py` 115, `s2` order check and IDLE_STATE probe 15, RTC tests 7 |

**Stated deviation:** the rc log fires when the return code changes, not once per wake (per wake would be +6 net, over budget).
**Controller edits:** none.
**Bench binaries (CI builds, SHA-256 of the `.bin`):** fixed `365358e` (product 5001, `v50.1-NativeRTC`) `ffa84c20...89ec`; 5097 no-refresh on the fixed base `42e6956` (NEVER RELEASE, `v50.1-NativeRTC-NOREFRESH-BENCH-ONLY`) `73a9d7d5...fb75`. The 5097 CI run's host-tests job fails by design (the structural tests require the refresh); its firmware job is green. Full sums are in the bench kit and the checklist.

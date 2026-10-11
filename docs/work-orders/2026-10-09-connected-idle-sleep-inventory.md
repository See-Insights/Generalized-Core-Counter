# Read-only inventory: CONNECTED devices cycling Idle/Sleep and flooding TimeDiag

- **Base commit:** `c745ac3` (main plus docs; v40 `v40-FailsafeAndSensorType` firmware)
- **Date:** 2026-10-09
- **Model used:** claude-sonnet-5-5, reasoning: high (set by `--effort high`)
- **Scope:** 0 `src/` lines, no fix or design proposal. OBS = read from code, diff or record. INF = my inference.

## Summary

- **No WO or commit in the record targets this behaviour.** I found no WO, verdict, CHANGELOG entry or commit message that names the CONNECTED Idle→Sleep→Idle cycle or the per-pass TimeDiag flood as a bug (searches in Method below). That differs from "several earlier WOs tried to fix it."
- **What the record does contain:**
  - The original design, a logging commit and an observability commit.
  - A trust-standard rewrite and a getter swap.
  - One open CONCERN on the abort.
- **None was reverted.** All of it is still in the code (OBS, `git log --all -S`).
- **The field sample shows one abort, then steady state (OBS/INF).** The sample has one `Sleep->Idle`, then TimeDiag only. Idle's CONNECTED branch calls `logTimeDiag()` on every pass and never leaves Idle, so the flood is Idle's normal CONNECTED-and-Open state. It is not a cycle.

## Attempts and related changes

Each row's line numbers are at HEAD. Net lines come from the diffs.

| # | Identity (date, release) | Change (net lines) | Status |
|---|---|---|---|
| A | `41bc674` "4.13 long term test candidate" (2026-01-17); re-added in `2c19b08` "3.18 Limited Test candidate" (2026-01-21). First tag containing `2c19b08`: v3.24. | Original design. Idle CONNECTED park-hours block, about +12 (now `State_Idle.cpp:119-133`). Sleep abort `SLEEP abort: CONNECTED+OPEN`, about +7 (now `State_Sleep.cpp:406-410`). | Merged, in code today. Not an attempt to fix the bug. |
| B | `a95e284` "Tune cloud recovery and reduce release logging noise" (2026-06-06). First tag: v14. | Added `logTimeDiag()` (40-line function, `Generalized-Core-Counter.cpp:1848`, declared `State_Common.h:199`). Idle call +2 net (`const bool openNow` plus the call). Sleep call +2 net. | Merged, in code. The call has been in Idle's CONNECTED branch with no guard since this commit. |
| C | `c6fc382` "Add state transition observability for v15 soak" (2026-06-10). First tag: v16.0.0. | Replaced `state = IDLE_STATE` with `transitionTo(IDLE_STATE, "sleep-abort-open-hours")`, ±0 net (`State_Sleep.cpp:408`). | Merged, in code. This is why `StateReq: Sleep->Idle reason=sleep-abort-open-hours` exists. |
| D | `293f4f7` (2026-08-31, WO-2026-08-29-002) | Added `trusted=`, `syncAgeMs=` and `lastSyncEpoch=` to the TimeDiag format (`:1899`). | In code. Logging fields only; per-pass frequency unchanged. |
| E | `7d56c25` Step 3b, WO-2026-09-19-001 (2026-09-19; product 24 in tree, no tag) | Idle CONNECTED hunk −4/+11 (net +7). Sleep abort hunk −1/+7 (net +6). Both gate on `Clock::openness()`. Added `openness=` to TimeDiag (`:1897`, `:1919`), about +9 (the diff has no isolated count). | Merged, in code. |
| F | `e3173f3` (2026-09-29), WO-2026-09-29-001 item D (v27-SmallFixes) | `logTimeDiag()` `local=` computed live with `LocalTimeConvert`, +5/−5. | Merged, in code. |
| G | `0e94f81` WO-2026-10-07-002 (2026-10-07; released as v39-OccupancyReports, `ed12d7b`) | Swapped `get_connectionMode()` for `effectiveConnectionMode()` at `State_Idle.cpp:119, :217, :225, :296` and `State_Sleep.cpp:406`, ±0 net at these sites. | Merged, in code. |
| H | `50234a0` "fix(idle): add modem-on ceiling safety net" (2026-03-16; first tag v11.0.0; CHANGELOG 4.02) | Added the Idle ceiling (`State_Idle.cpp:262-338`). `healthyConnectedAwakePath` (`:295-298`) exempts CONNECTED, Open and cloud-up. | Merged, in code. Not an attempt on this bug. It is the one other Idle→Sleep path a CONNECTED device can take (INF). |

### Why none of them fixed the bug, in the records' words

| Row | Record |
|---|---|
| A | The comment at `State_Sleep.cpp:397-399` says the abort exists to "stay awake/connected and resume counting." It was designed to keep a CONNECTED device awake, not to prevent a cycle. |
| B | The commit subject says "reduce release logging noise", yet it added the call. I found no WO text on why it is unconditional (OBS: no matches for `TimeDiag` in WO-2026-09-15-001, the log-spam WO). |
| C | Observability only. No behaviour change. |
| E | `WO-2026-09-19-001-clock-trust-standard.md:47-49`: "Open-equivalent (stay awake/connected, permit a report - itself a resync opportunity) at every CONNECTED-mode and reporting site." The WO reasoned about a wrong CLOSED verdict committing the device to an overnight hibernate. It says nothing on log volume or on a repeat cycle. |
| E (exception) | `:52-56`: the Idle ceiling "also denies `Unknown` (ceiling applies) rather than granting it." |
| E (dead code) | `State_Idle.cpp:219-224` flags `:225` as unreachable ("this condition's own `connectionMode() == CONNECTED` term can never be true here ... Not touching the surrounding logic"). |
| G | `WO-2026-10-07-004-stage7-round2-verdict.md:6, :123-127` says WO-002 "only swapped its getter" (the CONCERN is in the CONCERN row below). |
| Step 0 | `WO-2026-10-07-004-step0-report.md:85, :87`: in mode 0 while Open "the state is Idle (`Idle:112-125`) and sleep is aborted (`Sleep:406`)" and "CONNECTED does not sleep while Open." The abort and the Idle block were treated as correct, settled behaviour. |

### CONCERN (open, not an attempt)

`WO-2026-10-07-004-stage7-round2-verdict.md:123-127` and `docs/RECOVERY_PLAN_2026-09-26.md:116`:

- The abort at `State_Sleep.cpp:406-409` leaves for Idle without resetting `cloudSyncStartMs` (`:426`).
- A later sleep gate could time out at once.
- This was deliberately not folded into WO-004.
- It is not a fix for the flood.

### TimeDiag logging specifically

| Question | Answer |
|---|---|
| When did it become per-pass in Idle's CONNECTED branch? | `a95e284`, 2026-06-06 (v14), by an unconditional call inside `if (Time.isValid() && connectionMode == CONNECTED)`. The call has stayed in the loop-called Idle handler since. Row E rewrote the lines but kept the call unconditional (`State_Idle.cpp:125`). |
| Did any WO or commit rate-limit it? | No. `git log -G'logTimeDiag\|TimeDiag' -- src` lists only `a95e284`, `293f4f7`, `b517cbf`, `7d56c25`, `a6a283c` and `d14c7e3`, and none adds a guard, timer or change-detection (`d14c7e3`'s match is the v28 release-string change). The nearest log-spam fix, `f6bcdec` (WO-2026-09-15-001 Amendment C), covers PowerDiag and LedgerPayloadStatus only. |
| What does the documentation say? | `docs/FIELD_MEANINGS_REFERENCE.md:27-29` describes TimeDiag as "at state transition (not sleep-mode related)". The code logs it every Idle pass in CONNECTED. The reference is wrong (OBS). |
| Other records on `logTimeDiag()` | `2026-10-02-wake-heap-loss-codex-report.md:51, :78` treats it as one call per sleep prep (`:1002`) that adds temporary String churn from row F. It does not mention the Idle call. A per-pass call running at about 100 passes/s is a much larger allocation count (INF, not measured). |
| Evidence it is rare in other modes | `WO-2026-09-24-004-retire-open-equals-close-convention.md:186`: "no `TimeDiag` line has been logged since the change". In non-CONNECTED modes it appears only at sleep entry (`State_Sleep.cpp:1002`). |

## Leftovers that exist only because of these attempts

| Leftover | file:line at HEAD | From |
|---|---|---|
| Reason string `sleep-abort-open-hours` | `State_Sleep.cpp:408` | C |
| `ensureSensorEnabled("SLEEP abort: CONNECTED+OPEN")` | `State_Sleep.cpp:407` | A |
| Per-pass `logTimeDiag(isWithinOpenHours())` and the "meant to be compared" comment | `State_Idle.cpp:121-125` | B, E |
| `isOpen=` (fail-open) and `openness=` in the same line | `Generalized-Core-Counter.cpp:1897, :1899` | E |
| Same call at sleep entry | `State_Sleep.cpp:1002` | B, E |
| Unreachable `... == CONNECTED` term inside the `!= CONNECTED` block | `State_Idle.cpp:217, :225` | E, G |
| Ceiling exemption `healthyConnectedAwakePath` | `State_Idle.cpp:295-298` | H, E |
| Ceiling trip to Sleep (`idle ceiling exceeded`) | `State_Idle.cpp:324, :334` | H |
| No gate-timer reset on the abort (CONCERN) | `State_Sleep.cpp:406-409`, `:426` | A, WO-004 |

## Today's path (OBS unless marked)

| # | Step | file:line | What it does |
|---|---|---|---|
| 1 | `loop()` | `Generalized-Core-Counter.cpp:1613`, `:1618-1630` | `connectivityFailsafeSupervisor()`, then `switch(state)`: Idle calls `handleIdleState()` (`:1625`), Sleep calls `handleSleepingState()` (`:1629`). Then `thrashGuard.loop` (`:1651`), AB1805, Cloud, publish queue. Every pass runs the handler once. |
| 2 | Supervision | `ThrashGuard.cpp:72-74`, `State_Idle.cpp:263` | ThrashGuard returns 0 for `IDLE_STATE`, so nothing supervises Idle's pass rate. |
| 3 | Idle hand-off to Sleep | `State_Idle.cpp:217`, `:234`, `:257` | The `!= CONNECTED` block runs. With no updates pending, it logs and calls `transitionTo(SLEEPING_STATE, "low power idle")`. The 14:26:54 line shows this ran, so the effective mode was not CONNECTED then (INF). |
| 4 | Config applies mode 0 | `cloud/ConfigApply.cpp:448-454` | Sets the configured mode and logs `Config: Connection mode -> CONNECTED` (14:27:00). `PowerManager.cpp:245-247` returns the configured mode unless the OCCUPANCY and KEEP_ALIVE low-battery downgrade is active. |
| 5 | Sleep abort | `State_Sleep.cpp:406-410` | On the next Sleep pass, before the gate (`:426` onward). Mode CONNECTED and `Clock::openness() != Closed` calls `ensureSensorEnabled`, then `transitionTo(IDLE_STATE, "sleep-abort-open-hours")`. `StateReq` logs at `Generalized-Core-Counter.cpp:2505`. That is the 14:27:03 line. |
| 6 | Idle CONNECTED branch | `State_Idle.cpp:119-133` | Mode CONNECTED: read `Clock::openness()` (`:120`; `Clock.cpp:567`: Unknown if untrusted or config invalid, else Open or Closed), then call `logTimeDiag()` (`:125`) on every pass. If Closed, go to Sleep as "park closed" (`:128`). If Open or Unknown, fall through. |
| 7 | Rest of Idle | `:217`, `:262-338` | The `!= CONNECTED` block is skipped. The ceiling is exempt when cloud is up and openness is Open (`:295-298`), so there is no transition. Idle returns and the next loop pass repeats step 6. This steady state matches the sample: no `StateReq`, TimeDiag about 10 ms apart. |

**Cycle-capable paths (INF, not seen in the sample):**
- **Ceiling trip:** a CONNECTED device with cloud down or openness Unknown is not exempt (`:295-298`). After `connectAttemptBudgetSec` (30-900 s, 300 s fallback), the ceiling trips (`:324-335`) and sends it to Sleep. The abort at `:406` returns it to Idle, because Unknown is not Closed. The trip forces the radio off (`:331`).
- **Boundary flip:** a Closed↔Open flip between an Idle pass and the next Sleep pass would bounce between `:128` and `:406`. Both use `Clock::openness()`, so this needs a trust or hour change between passes.

## Where the attempts agree and disagree

- **Agree:**
  - In CONNECTED, Open and Unknown mean "stay awake". Only Closed commits to sleep (rows A and E; `WO-2026-09-19-001:47-49`; Step 0 `:85, :87`).
  - The abort and the Idle branch are intended behaviour.
  - No record names the cycle or the flood as a defect.
- **Disagree or inconsistent:**
  1. The ceiling denies Unknown, while Idle and the abort grant it (`WO-2026-09-19-001:52-56`). That is the only recorded asymmetry able to produce Idle→Sleep→Idle in CONNECTED (INF).
  2. `isOpen=` (fail-open `isWithinOpenHours()`) and `openness=` come from different sources by design (`State_Idle.cpp:121-124`).
  3. `FIELD_MEANINGS_REFERENCE.md:27-29` says TimeDiag logs at state transitions; the code logs it every Idle pass.
  4. The brief and the sample differ: the brief says "cycling", but the sample shows one abort and then steady state.

## What not to try again

No record shows a failed attempt at this bug, so I cannot list approaches that already failed. The record does show these constraints:
- **Do not gate CONNECTED on `Time.isValid()` or `isWithinOpenHours()`.** Row E replaced that pattern because a wrong RTC-seeded clock gave a wrong CLOSED verdict.
- **Do not merge `isOpen=` and `openness=`.** The comment at `State_Idle.cpp:121-124` says they are meant to be compared.
- **Do not treat "release logging" as a gate.** Row B is titled to reduce noise, but `State_Idle.cpp:125` has no guard.
- **Do not grant the ceiling exemption to Unknown.** `WO-2026-09-19-001:52-56` rejects this as a battery-drain regression (Dev-11).
- **The existing ping-pong guard does not cover CONNECTED.** It defers Sleep on `sensorDetect` or the COUNTING LED, inside the `!= CONNECTED` block (`State_Idle.cpp:217-240`).

## Open points

1. **Device log:** only a forwarded 1-line/s sample exists. I cannot say from the repo whether any `Sleep->Idle` repeats beyond the one shown. A full-rate log or `StateReq` count would settle cycle vs steady state.
2. **Radio after the ceiling:** after a ceiling trip and abort, the radio is off and Idle's CONNECTED branch has no reconnect call. I did not trace who reconnects (INF).
3. **Snapshot lag:** `Clock::openness()` reads a per-UTC-minute `LocalTimeCache` snapshot (`Clock.cpp:581-582`), while TimeDiag's `local=` is live (row F). The two can disagree for up to about 59 s at an open or close hour (INF).
4. **First tag:** I did not find a first-shipped tag for rows D, E and F (no tag contains them; product versions 24 and 26 are read from the tree).
5. **Method:** I searched the strings listed in the dispatch plus `ping-pong`, `abort`, `flood`, per-pass phrases and `revert`. I used `git log -S`, `-G`, `--follow`, `-L` and `blame`. I did not read every WO in full.

## Budget versus actual

| Item | Budget (`src/` lines) | Estimate | Actual | Tests run |
|---|---|---|---|---|
| Inventory | 0 | — | 0 | none run |

Historical net-line counts and release identities come from the diffs and tree contents, not from re-opened files. Every `src/` file:line cited at HEAD was re-opened.

---

*Appended by Claude Code, 2026-10-09:*
- **Spot checks:** `a95e284` (2026-06-06) adds `logTimeDiag(openNow)` inside Idle's CONNECTED branch. `git log --grep` finds no commit message naming `sleep-abort`, a ping-pong, a TimeDiag flood or a CONNECTED Idle/Sleep cycle, and no file in `docs/work-orders/` mentions `sleep-abort-open-hours`.
- **Field outcome:** Chip powered Dev-09 off at about 14:40 SGT, because the flood was saturating the serial log forwarder (about 100 lines/s emitted, about 1 line/s forwarded). Dev-09's v40 soak window is broken, and the WO 1b CONNECTED step did not complete.

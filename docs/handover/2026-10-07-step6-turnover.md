# Step 6 turnover (2026-10-07)

This is a factual handover for the fresh Step 6 session, as of main `41ad238` on 2026-10-07. Every fact names its source. "Chip, 2026-10-07" marks a decision Chip made in the turnover dispatch that has no other record yet.

## 1. How we work

**Roles** (`AI_DEVELOPMENT_WORKFLOW.md` §2):
- **Architect:** Claude in the app. Writes WOs and decides with Chip.
- **Claude Code:** the workflow controller. Dispatches agents, does read-only investigation, builds and tests, and makes narrow edits only when pre-authorized.
- **Copilot:** implements, in Stage 6.
- **Codex:** investigates (Stage 4) and verifies (Stage 7).
- **Chip:** approves, commits, merges, releases, and does every device operation.

**Stages** (§3):

| # | Stage |
|---|---|
| 1 | Intake |
| 2 | Evidence |
| 3 | Preliminary architecture |
| 4 | Independent investigation |
| 5 | Approval gate |
| 6 | Implementation |
| 7 | Independent verification (mandatory linkage check, local toolchain build, retirement check) |
| 8 | Final gate |
| 9 | Release and feedback |

**Standing rules:**
- **Plain goal:** every WO starts with a plain-language goal (§12.2).
- **Size budget:** every dispatch has a size budget in net `src/` lines; going over means stop and report (§12.3).
- **No compressed code** to meet a budget (§12.3).
- **Budget versus actual,** one row per item, in each WO's closing record (§12.3, §6).
- **Every dispatch names its agent, model and reasoning level** (§4 header line, §5). The architect picks the model by complexity, and Claude Code resolves the ID from each CLI at dispatch time (§5).
- **Facts are checked against their source:** log lines for runtime values, code for constants. See lesson 3 below.
- **The two-round rule:** two rounds that each add a mechanism, or two rounds without VERIFIED, mean stop and restate the goal (§12.4).
- **Agents never merge.** They may open PRs; only Chip merges (§2, each role's restrictions).
- **Shell tests run under zsh,** never bash; reports say `N/N (sh via zsh, py via python3)` (§10).
- **`make clean-user` between build types,** or stale objects are reused. Prove a flag's presence with `nm` on both the object and the ELF (§2, "Verifying compile-time flags").
- **Verify the binary, not the source** (§12.5), and the linkage of anything new (§3, Stage 7).
- **Alerts:** any WO that changes how an alert is raised, ranked or cleared updates `docs/reference/alert-codes.md` in the same PR, and Stage 7 checks it (§3, Stage 7, added by PR #65).
- **History first:** search the history before designing anything new; restoration is the default fix (§12.1).

## 2. Where things stand

**Releases v25–v38** (`CHANGELOG.md`; commits on main):

| Release | WO | What it fixed |
|---|---|---|
| v25-WithAck | WO-2026-09-25-001 (with WO-2026-09-24-001) | Restored `WITH_ACK` on queue publishes, with Idle handing the queue to the sleep gate (`c413e1e`). The closing report is stamped boundary−1 (`599038e`). |
| v26-NoPdiag | WO-2026-09-28-001 | Release builds don't publish `pdiag` (`41fb02b`). |
| v27-SmallFixes | WO-2026-09-29-001 | The closing report connects at close; the broad `hook-response/` subscription was removed; status overflow guard; live TimeDiag; cloud builds use the vendored libraries (`a6a283c`). |
| v28-CloseBeforeSleep | WO-2026-09-30-001 | Never sleeps for the night while the close is still due (`d14c7e3`). |
| v29-ConfigCleanup | WO-2026-09-29-002 | Build switches in `BuildProfile.h`, version in `FirmwareVersion.h`, notes in `CHANGELOG.md`, one webhook-timeout range (`df9c4cb`). |
| v30-LedOffAtNight | none | Occupancy LED off at night sleep (`f0566c5`). |
| v31-ConnectivityFixes | WO-2026-10-01-001 | OTA-aware dwell, failed-attempt counting, reset-cause codes, signal validity (`37437a3`). |
| v32-RecoveryVisibility | WO-2026-10-02-001 | Failsafe recovers in about 3 open hours (stage 1 retired); reports carry `fh`, `lfb`, `cyc`, `slp` (`52ab58d`, PR #55). |
| v33-HourlyWhileOccupied | WO-2026-10-02-002 | A report every hour, occupied or not (`691b6c7`, PR #55). |
| v34-SleepConfigLeak | WO-2026-10-02-003 | A fresh `SystemSleepConfiguration` per sleep, ending the per-wake heap loss; the out-of-memory reset restored (`1c74ac8`, PR #57). |
| v35-LedgerNoRetry | WO-2026-10-03-001 | A waiting ledger write is never retried; refused writes are deferred, not dropped (`589421d`, PR #58). |
| v36-HourRules | WO-2026-09-24-004 | Open and close hours follow three rules (`ba97977`, PR #60). |
| v37-PreStep6Fixes | WO-2026-10-04-001 | No modem waits in the loop or at sleep; restarts keep the session's minutes; on-time DEEP_POWER_DOWN wakes report ok; reports carry `vc` (PR #62). |
| v38-EventAlertsClear | WO-2026-10-06-001 | Event alerts 19 and 42 are reported once, then cleared; 18 stays sticky (PR #64). |

**The fleet** (Particle API, about 03:30Z on 7 Oct):
- **v38:** Dev-09, Dev-14, Trail02, PCKL1 and PCKL2.
- **v36, all targeted at v38:** Court1, Court2, Court3, PCKL3, MAFC-1 and MAFC-2. They take v38 at their next connection.
- **Product default:** **v38** ("Release Candidate - v38", uploaded 2026-10-07).
- **v38 field check:** Dev-14 reported `alerts` 19 → 0 → 0 (WO-2026-10-06-001, bench result).
- **v37 acceptance:** PASS (WO-2026-10-04-001, bench section).

**In progress or waiting on Chip:**
- **The v38 rollout:** six devices are pending their OTA. The product default is already v38.
- **Ubidots `vc`:** add `"vc": "{{vc}}"` to the template once every device runs v38 or later (`docs/RECOVERY_PLAN_2026-09-26.md`, "Rollout and follow-ups").
- **Particle bug report:** file the `SystemSleepConfiguration` move-assignment leak. The draft is `docs/work-orders/WO-2026-10-02-003-particle-bug-report-draft.md`; no issue link yet (recovery plan, "Upstream reports").

**Open PRs:** none (GitHub, 7 Oct).

**The bench and the hardware** (Chip, 2026-10-07):
- **Dev-09 and Dev-14 are BRN404X Borons** (North American LTE-M) on Singtel, so their connectivity results don't represent the US fleet.
- **Dev-11 is retired:** its Boron's clock is faulty.
- **For Step 6's M-SoM work,** a Muon with an M524 is on hand; it connects in seconds on Singtel.
- **Future options:** an M635e (beta) and the M404.
- **The US production fleet stays on Borons.**

## 3. Step 6 inputs

**The Codex ownership report:** `docs/work-orders/2026-10-06-step6-ownership-codex-report.md` (PR #63; re-cited after v37 in PR #66). Its **proposed owners**:
- **SensorManager/PIR:** the motion lifecycle, and one occupancy owner for session start, expiry and close.
- **BatteryAuthority with BatteryHealth:** the accepted battery sample, trust and tier.
- **PowerManager:** the effective power source and profile, and the sampling and charging cycle. The effective connection mode is derived from the configured mode plus the downgrade.
- **ChargeInhibit, PmicFaultMonitor and PowerDiagnostics:** thermal hysteresis, fault classification, and formatting only. A single PowerPlatform path does every charging write.
- **The platform split:** compile-time implementations per platform (Boron FuelGauge and TMP36; M-SoM via Device OS APIs and TMP112). No virtual layer.

Its **WO order:**

| WO | Goal | Estimated net `src/` lines |
|---|---|---|
| 1a | Stop the config/downgrade overwrite | +15–35 |
| 1b | Failsafe counts only overdue expected connections | +25–50 |
| 2 | SensorManager split: dead code first, then moves | −180 to −280, then about 0 each |
| 3 | Battery evidence after a week of `vc`, then trust/tier tuning | +40–60, then 0–10 |
| 4 | M-SoM implementations, then Muon bring-up | +20–50 each |

**Adjustments already agreed** (Chip, 2026-10-07):
- moved lines are counted separately, with `git diff --color-moved`;
- the 135-line charge-cycle test is removed in WO 2's dead-code step;
- every duplicate decision the report found is mapped to a WO;
- WO 4 starts with a real M-SoM compile. "Compiles cleanly" is unverified: the report and `WO-2026-09-29-002-config-cleanup.md:86`.

**Extensibility requirements** (Chip, 2026-10-07):
- **One small shape for every sensor:** start, sample, wake sources, ask for a report, add fields, reset daily totals.
- **Reading modes:** polling, threshold and interrupt.
- **Wakes** are routed to the sensor that caused them.
- **Payloads and the data ledger** are built from the sensors, in the form Ubidots expects.
  - **Today's webhook template names each field (12 of them),** so any new report field needs its own template line. Add it only after every device sends the field: an edit naming a field the fleet doesn't send yet has already broken the JSON once.
  - **If payloads become fully Ubidots-ready,** the webhook could forward the event as it is. That belongs with the webhook-rename WO after Step 6.
- **Per-sensor settings,** and **compile-time selection.**
- **Tested against** PIR occupancy, the counting mode, temperature, and humidity.

**Hardware direction** (Chip, 2026-10-07):
- **A 3-wire battery pack with a hardware charge window.** The firmware reports the charge-temperature status instead of deciding it.
- **A temperature-and-humidity sensor on the Muon.** The temperature owner is defined as "environment".

**Correction:** CONSERVING starts **below 70%** and returns to HEALTHY at **75%** (`src/cloud/BatteryBackoffPolicy.h:21–26`; recovery plan, Step 6, corrected in PR #63).

**The fresh session's first task** (Chip, 2026-10-07): review the Step 6 plan against the extensibility requirements.

## 4. References

- **The recovery plan:** `docs/RECOVERY_PLAN_2026-09-26.md`. Its backlog groups:
  - Phases 1–4;
  - Step 6 (the power-owner split; citations checked at `e78d59d`);
  - Clock owner;
  - Upstream reports;
  - Connectivity;
  - After Step 6;
  - Rollout and follow-ups;
  - Observations to watch;
  - Guardrails.
- **The alert-code reference:** `docs/reference/alert-codes.md`, cited at `2c477e8` with "since v38" behaviour (PR #67).
- **Work orders, dispatches, reports and verdicts:** `docs/work-orders/`.

## 5. Lessons worth keeping

1. **Check the history before inventing.** Reports went missing because `WITH_ACK` was dropped in `eda6b7e` (v3.24). Restoring it fixed the problem (recovery plan, "What we know"; v25).
2. **Restate the goal when rounds grow.** Fix B for WO-2026-09-25-001 grew to about 1,000 lines over three review rounds before it was abandoned for restoration. This is where the guardrails and the two-round rule came from (`docs/work-orders/WO-2026-09-25-001-report-webhook-loss.md:7`; `AI_DEVELOPMENT_WORKFLOW.md` §12).
3. **Check log lines, not configuration limits.** The connect budget is configured as up to 900 s, but the code overrides it: 300 s by default, 660 s for a deep attempt. The logs showed what actually ran (`docs/work-orders/2026-10-01-connectivity-investigation-codex-report.md:11, 66–70`).
4. **Measure, then narrow down when and where.** The heap loss was measured per wake cycle, then traced to the reused `SystemSleepConfiguration` and Device OS's move assignment (WO-2026-10-02-003; `docs/work-orders/2026-10-02-wake-heap-loss-codex-report.md`).
5. **Spell out "don't merge".** The v33 PR was merged when only opening it was intended (`AI_DEVELOPMENT_WORKFLOW.md` §2, the 2026-10-02 notes).
6. **Check the power before blaming the firmware.** Dev-14 stopped charging on 3 Oct because of a faulty USB cable. After the swap it charged to DONE overnight (WO-2026-10-04-001, bench section).
7. **One change at a time in production.** Editing the shared webhook template affects every device at once. Copy the old version first, change one thing, and check for 201 Created before doing anything else (Chip, 2026-10-07).

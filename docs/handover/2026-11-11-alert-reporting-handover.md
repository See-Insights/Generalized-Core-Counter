# Handover: keep and improve fleet alert reporting in the switch to GCC

**Date:** 2026-11-11 · **From:** Architect (Claude, app chat) with Chip · **To:** GCC agents (Claude Code first)
**Governing roles:** `AI_DEVELOPMENT_WORKFLOW.md` §2 (Architect writes WOs; Claude Code controls the workflow) and §3, Stage 1 Intake. This is input for intake, **not** a dispatch. No code changes are authorized by this document. A WO with a line budget comes next (§6).

## 1. Goal, in plain words

Today the legacy fleet tells Chip **which devices need a visit**, and it does this without anyone watching the device. When ~400 devices move from v50/v54 to GCC, that must not get worse. It should get better: fewer posts, and each post more useful.

Two outcomes:
1. **Keep:** every alert Chip relies on today still fires, with the same meaning, after a device moves to GCC.
2. **Improve:** less noise, and some new signal: when a problem started, how often it happened, and fleet-wide trends.

Chip's constraints: keep it small and simple. Firmware (`src/`) lines matter much more than test lines. Ask Chip to cut scope rather than let it grow.

## 2. What the legacy system delivers today (verified 2026-11-10/11)

The value is a **chain**. Most of it lives in Ubidots, not in firmware:

```
firmware alert code ──► webhook ("alerts" field) ──► Ubidots event (device group "Active Solar Devices")
                                                          ├─► Slack #iot-alerts post
                                                          ├─► Freshdesk ticket (UbiFunction, E02/E03/E11/E12)
                                                          └─► Ubidots incident
firmware ERROR_STATE ──► self-heal (reset / 30 s deep power-down by code)
```

**Firmware side (Visitation-Counters `src/Visitation-Counters.ino`, v54 = v50 + Device OS 6.6.0):**
- **One alert code at a time.** It's reported in the hourly `Ubidots-Counter-Hook-v1` payload as `alerts`, alongside `resets` and `connecttime` (`:780`), then cleared after publish (`:784`).
- **The alert is also published right away** on `Ubidots_Alert_Hook` with its own timestamp (`:600-601`, plus `:345-346`, `:1170-1171`). This gives an exact event time, but a reconnecting device replays its backlog as a burst.
- **ERROR_STATE picks a self-heal action by code** (`:607` on): 12/13/15 → deep power-down; 14/30 → reset; 31 → power-down if there's been no connection for 2 h.
- **Daily reset of `resetCount`.** More than 8 resets raises 13 (`:327-328`).

**Ubidots side (cloud rules, read via the Ubidots MCP; all on device group "Active Solar Devices"):**

| Event | Trigger | Action |
|---|---|---|
| E01 offline 3 h | `battery` inactive 183 min → UbiFunction `3h-event-validation` sets `3h-flag` | Slack; "reconnected" via `3h-btn-flag` |
| E02 offline 1 day | `battery` inactive 1 day | Slack (+ back-to-normal) and **Freshdesk ticket** |
| E03 offline 1 week | `battery` inactive 7 days | Slack and **Freshdesk ticket** |
| E11 battery ≤ 65 % for 3 days | `battery` ≤ 65, delay 4320 min | Slack (+ recovered) and **Freshdesk ticket** |
| E12 battery ≤ 40 % | `battery` ≤ 40 | Slack and **Freshdesk ticket** |
| E30-xx | `alerts` == code | Slack. Events exist for 11, 12, 13, 14, 15, 20, 21, 22, 23, 30, 31, 40 |

The Freshdesk payload uses device context `email1`, `email2` and `phone`. Ubidots device ID = Particle device ID, so moving a device keeps its Ubidots device, history, group membership and context (Chip, 29 Oct, `WO-2026-10-29-001-step0-report.md:143`).

**This means most of the value survives automatically**, provided GCC keeps sending the `battery` (with `key1` context) and `alerts` fields on its regular reporting schedule. Today it does (`src/cloud/ReportPublisher.cpp:103-114` at `origin/main` `f00aa04`).

## 3. What breaks or is lost when a device moves to GCC

Code table: legacy codes (`Visitation-Counters.ino:14-35`) against GCC codes (`docs/reference/alert-codes.md`).

| Legacy code (meaning) | Ubidots event | GCC today | Result |
|---|---|---|---|
| 10 too hot/cold to charge | none | 20 PMIC thermal | see 20 below |
| 11 PMIC reset required | E30-11 | 21/22/23 (PMIC) | **lost**: no event for 21/22 |
| 12 init error (FRAM) | E30-12 | none | **lost** (no FRAM in GCC; check whether an equivalent is needed) |
| 13 excessive resets | E30-13 | 17 boot storm / 19 watchdog | **lost**: no event for 17/19 |
| 14 out of memory | E30-14 | reset cause 7 (v34); 14 no longer raised | **lost** as an alert |
| 15 disconnect/modem failure | E30-15 | 15 | kept |
| 20–22 firmware update (legacy: deleted) | E30-20/21/22 | **20–22 = PMIC faults** | **WRONG LABEL** |
| 23 update limit reached (legacy: still raised) | E30-23 | **23 = PMIC battery fault** | **collision** while the fleet is mixed |
| 30 cellular up, cloud not | E30-30 | not raised | **lost** (useful coverage signal) |
| 31 failed to connect | E30-31 | 31 | kept |
| 32 slow connect, retrying | none | not raised | n/a |
| 40 webhook response missing | E30-40 | 40 (now a condition) | kept; meaning broadened |
| — | none | 16, 18, 41, 42, 43, 44 | **silent**: raised, but never reach Slack |

**Evidence of the wrong label:** on Nov 7, GCC device Morrisville-Tennis-ChurchSt-1 (reports `fh`/`lfb`/`cyc`/`slp`, so it's on GCC) posted *"firmware Update Completed Successfully. Msg E30-20"* to #iot-alerts. Under GCC, 20 means a PMIC thermal fault. A charging problem showed up in Slack as good news.

**Other GCC behaviors that weaken the signal:**
- **Severity masking.** `raiseAlert()` keeps only the more severe code (`MyPersistentData.cpp:811-816`). A code that never clears (18 has no clear site) hides every lower code for good.
- **No event time in the report.** The hourly report carries the code but not when it was raised (`lastAlert` is only in the `status` event). Legacy had the time on the alert hook.
- **Alerts reach Ubidots only with the next report.** There's no immediate alert publish (`git grep Ubidots_Alert_Hook src` finds nothing). This is fine for noise, but the time of onset is lost.

**Noise baseline to beat (#iot-alerts, Nov 3–10, legacy fleet):** 449 posts from 89 devices. Three devices produced 46% of them. E30-31, E30-30 and E30-13 make up 59%, mostly backlog replays when a device reconnects. The silent devices are the ones that most need a visit: 26 devices were silent for more than 24 h, and each posted once and then stopped. Real fleet-wide event: Nov 5 18:00 ET, 34 devices posted E30-40 within 4 minutes (a backend/webhook problem). Weekly analysis canvas: https://seeinsightsllc.slack.com/docs/TG2JVRDUK/F0CGBKJCGV9

## 4. Requirements

### Must preserve (before any fleet conversion)
- **P1. Field contract.** Keep `battery` (number, with `key1` context), `alerts` (number), `resets` and `connecttime` in every scheduled report, on the regular schedule. E01/E02/E03/E11/E12 and the Freshdesk tickets depend on them. Add a test that fails if one of these names changes.
- **P2. No code means two things.** Fix the 20–23 collision. *Proposed:* make no firmware change and re-point the Ubidots E30-20/21/22 events to the GCC PMIC meanings (legacy no longer raises 20–22). For 23, either renumber GCC's PMIC battery fault or accept a mixed-fleet ambiguity until v54 is retired. **Chip rules.**
- **P3. Map every GCC code to an outcome.** For each code in `alert-codes.md`, record: Slack (and with what text), weekly digest only, or log only. Add the missing Ubidots events this needs (Chip creates them, since Ubidots is a cloud-side change). Minimum: a "charging problem" alert (21/22/23, replacing legacy 11) and a "resetting too often" alert (17/19, replacing legacy 13).
- **P4. Keep self-healing.** Keep ERROR_STATE recovery actions for the equivalents of legacy 13/15/30/31. GCC's existing table in `alert-codes.md` already covers 15/16/31/40; confirm nothing is lost.
- **P5. No permanent masking.** Every code must have a clear path, so a stuck code can't hide later ones. 18 is the known case.

### Should extend (small, after P1–P5)
- **X1. Report on onset and clearing.** An alert reaches the cloud once when the condition starts and once when it clears, with the time it began. This removes replay bursts and gives "when did this start." (Can the existing `lastAlertTime` simply go into the report?)
- **X2. Counters instead of repeats.** Add a few numeric per-period fields to the report, e.g. failed connects and webhook misses, next to `resets`. This makes trends (Chip's "alert code trend that's not normal") a data query rather than a count of Slack posts. Follow the Ubidots rule: top-level fields are numbers, and text goes in context.
- **X3. Bring back "cellular up, cloud not"** (legacy 30) as a counter or code. It was the best coverage signal in the legacy data.
- **X4. Keep firmware out of fleet-level logic.** Fleet outage paging (for example, 20 or more devices with the same code in an hour) and the weekly top-10 run in the cloud (fleet-ops / scheduled task), not on the device.

### Budget and simplicity
- P1–P5 should be mostly configuration and tests: **≤ 40 `src/` lines** expected (P2/P5 only).
- X1–X3: **≤ 80 `src/` lines** combined.
- If either budget looks tight, stop and ask Chip which item to drop. Don't expand quietly.

## 5. Open rulings for Chip
1. **P2:** re-point the Ubidots events for 20–22, and for 23, renumber the GCC code or accept the ambiguity until v54 is retired?
2. **P3:** which GCC codes deserve a Slack post, and which belong only in the weekly digest?
3. **Is legacy 12 (FRAM init error) still relevant** to GCC hardware, or retired?
4. **Timing:** P1–P5 must land before the occupancy-to-counter fleet conversion. Should X1–X3 ship in the release candidate or after the soak?

## 6. Suggested next steps (Claude Code, under §3)
1. **Stage 2 evidence.** Re-cite `docs/reference/alert-codes.md` against current `main` (its citations predate 31+ `src/` commits). Confirm the report field list and every raise/clear site.
2. **Stage 4.** Codex investigates: for each GCC code, the clear path (P5), plus the smallest way to add `lastAlertTime` and the counters to the report (X1/X2).
3. **Architect writes the WO** with per-item line budgets (§6). Copilot implements; Codex does Stage 7 verification.
4. **Ubidots changes (P2/P3)** are Chip's to make in the Ubidots UI; the WO lists them as a checklist.

## 7. Sources checked for this handover
- Legacy firmware: `Visitation-Counters/src/Visitation-Counters.ino` (v54), lines cited above.
- GCC: `origin/main` `f00aa04` (2026-11-10): `src/cloud/ReportPublisher.cpp`, `src/MyPersistentData.cpp/.h`, `docs/reference/alert-codes.md`.
- Ubidots: active event definitions and trigger rules (read 2026-11-10/11); device variables for Morrisville-Tennis-ChurchSt-1.
- Slack #iot-alerts: all posts from Nov 3 22:30 to Nov 10 21:42 IST (449), tallied by script.

## 8. Rulings (Chip, 11 Nov 2026, on the architect's recommendations)
1. **P2 (code collision): fix in Ubidots, not firmware.** Migrated devices go in their **own Ubidots device group** (e.g. "GCC Devices") with **their own E30 events carrying GCC meanings**. The code-independent events (E01–E03, E11, E12, incl. Freshdesk) apply to **both** groups unchanged. No GCC code renumbering. Moving a device between groups is a step in the migration checklist. Chip makes the Ubidots changes; the WO lists them as a checklist.
2. **P3 (what reaches Slack): only what needs a visit or an action.** Slack: offline (E01–E03), battery (E11, E12), **charging problem** (one event for 21/22/23, replacing legacy 11), **boot storm** (17), **thrash** (18). Weekly digest only: 15, 31, 40, 19, 20.
3. **Legacy 12 (FRAM init error): retired.** GCC has no FRAM; a "storage failed" alert would be a new item.
4. **Timing:** P1–P5 land **before the counter-fleet conversion, not in RC1** (RC1 scope frozen). X1–X3 after RC1's soak; X1 (`lastAlertTime` in the report) first.
5. **Added to P5:** give code 18 a clear path (report once, then clear, like 19 and 42).
   - **Confirmed by Chip, 11 Nov, after the Stage 2 questions:** this includes tier 2. If the thrash recurs, raising and reporting 18 again is real information, not noise, and tier 2 is rare. The WO gives 18 a clear site of its own so that it stops masking lower codes.
6. **Note:** ChurchSt-1a raised **20 on 7 Nov** (its migration day; the "firmware update completed" Slack post) and **21 on 10 Nov**. Investigate both with the `vc` charging check.
   - **Close-out, code 20 on 7 Nov (Chip, 11 Nov): it was legacy.** The Ubidots `alerts` variable on device `6195a5aa1d847201d504f196` has value 20 at device timestamp 2026-11-07T12:49:42Z, received (`created_at`) at 12:57:05.9Z: a queued legacy publish. Slack posted at 12:57:06Z, the time Ubidots received it. **Rule: to tell legacy from GCC, use the Ubidots value timestamp, not the Slack post time.** The GCC data agrees (all hooks that day carry `alerts` 0; first GCC report 12:55Z; Stage 2 evidence §3).
   - **Code 21 on 10 Nov stays as recorded:** a near-full-pack false positive (`vc` 4.08 to 4.16 V while the SoC sat at 89 percent). The WO includes a `vc` ≥ 4.05 V exclusion for alert 21.
7. **X4 (fleet-wide paging, weekly top-10)** belongs in the **fleet-ops** thread and repo.

## 9. Migration notes (added 11 Nov)
- **Legacy gauge reads lower than GCC.** On ChurchSt-1a the legacy firmware reported `battery` 64 to 66 until 7 Nov 12:18Z (74 at 12:48Z); the first GCC report read 76.18 with `vc` 4.07. Migration therefore cannot cause a false E11 (battery ≤ 65 % for 3 days) or E12 alert. **Chip: no migration-checklist step.** Source: one device; Stage 2 evidence §3.

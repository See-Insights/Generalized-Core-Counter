# WO-2026-09-24-004: Open and close hours follow three rules

**Goal, in plain language:** open and close hours always follow three rules, enforced where settings are applied. A setting that breaks them is rejected, the device keeps its last valid hours, and the rejection is logged. Then remove the code the rules make unnecessary. Expected net change: fewer lines.

**Status:** Rewritten 2026-10-02 (Chip). **Ready; runs after v32 (WO-2026-10-02-001).** No implementation yet.

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

## Change (after v32)

**Enforce, in `Cloud::applyTimingConfig()`** (`ConfigApply.cpp:279–300`):
- **Today** `openHour` and `closeHour` are each range-checked as 0–23 and applied **independently**. So `closeHour = 24` is rejected, which means always-open = 0/24 can't be set at all, and a half-valid pair can be applied.
- **After:** take the merged pair, check it against the three rules, and apply **both or neither**.
- A pair that breaks a rule is rejected: the device **keeps its last valid hours**, the rejection is logged (with the offending values and the rule), and the apply is reported as failed, as `validateRange()` failures are today.

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

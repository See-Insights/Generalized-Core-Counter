AGENT: Codex · MODEL: gpt-6-astra · REASONING: high (a cross-module ownership map that has to be exact about who reads and writes what)
AUTHORIZATION SCOPE: read-only access to the code at v37's branch head `85bd316` (the working tree; ignore the uncommitted edit to `docs/work-orders/WO-2026-10-04-001-pre-step6-fixes.md`), git history, and existing reports under `docs/` / Not authorized: any edit, commit, build, network or AWS access, or device action, and creating or deleting any file. Your final message is saved as the report by the dispatcher.

# Pre-Step-6 investigation: sensing and power ownership map

Per `AI_DEVELOPMENT_WORKFLOW.md`:
- the Roles table (Codex: investigation);
- §12 guardrail 1 (history first: `git log -S` / `-G` before proposing anything new);
- checking facts against their source;
- the size-budget guardrail.

The recovery plan is `docs/RECOVERY_PLAN_2026-09-26.md`, "Step 6" and its backlog. The backlog items were added in PR #61; if that file at `85bd316` lacks them, read them from `git show origin/docs/recovery-plan-backlog-2026-10-05:docs/RECOVERY_PLAN_2026-09-26.md`.

**Plain goal of Step 6:** each piece of sensing and power logic has one owner with one clear job, and everything Boron-specific sits behind those owners, so the M-SoM version is a matter of filling in, not rewriting.

**Output budget:** at most three pages: the inventory tables, the data-flow list, and the proposed plan. No prose beyond that. Every claim about the code carries a file:line.

## Inputs (check each against the source; the dispatcher's notes are marked)

- **The M-SoM portability check (2026-09-29).** *Dispatcher's note: no document of it was saved.* Its only record is one line in `docs/work-orders/WO-2026-09-29-002-config-cleanup.md:86`: "it compiles cleanly, but PMIC, battery and pins need M-SoM implementations." Treat that as a claim to verify, not a finding.
- **The configuration inventory (2026-09-29):** `docs/work-orders/2026-09-29-config-inventory-codex-report.md`, and what WO-2026-09-29-002 left for Step 6.
- **The 4 Oct pre-Step-6 report, item D:** `docs/work-orders/2026-10-04-pre-step6-known-issues-codex-report.md` §D.
- **The Step 6 backlog items.** *Dispatcher's line corrections, checked at `85bd316`:*
  - **the connection-mode tug-of-war:** `src/cloud/ConfigApply.cpp:445–455` against `src/power/BatteryAuthorityCommand.cpp:80–84`;
  - **the failsafe's low-battery block:** `src/Generalized-Core-Counter.cpp:2641–2656` (not 2638–2641);
  - **battery trust:** `src/power/BatteryHealth.cpp`, `kOcvKnots` at `:19–32` (not 19–31), `evaluate()` at about `:81–108`;
  - **the CONSERVING threshold:** `BatteryBackoffPolicy.h:21`;
  - **the power-source misreads** (USB_ADAPTER and USB_HOST swapping on one supply).
- **Temperature.** *Dispatcher's note:* `TMP36_SENSE_PIN` is A4 on Boron (`src/device_pinout.cpp:34–39`, with a `MUON_TMP36_SENSE_PIN` override). `SensorManager` also documents a TMP112A I²C read (`SensorManager.h`, around `:271`). Say which one actually drives temperature, the thermal charge inhibit and the report's `temp` today.

## 1. Inventory (tables)

Cover SensorManager, PowerManager, BatteryAuthority/BatteryHealth, PowerPlatform, ChargeInhibit, PowerDiagnostics, and anything else that reads a sensor, the PMIC or the fuel gauge. For each:
- its files, and its job in one sentence as it actually behaves today;
- the state it owns (RAM or persisted, and where), and the public functions others call;
- who calls it, and what it writes that another module also writes (every case of two writers);
- the Boron-specific code in it (PMIC and fuel-gauge calls, pin names, platform guards), with file:line.

## 2. Data flow (a numbered list)

Trace the battery path end to end, naming the owner at each step and marking every place where two owners decide the same thing:
1. gauge and voltage reading;
2. trust (BatteryHealth);
3. tier;
4. the reporting-interval multiplier (ReportingPolicy);
5. connection mode;
6. the failsafe's low-battery block;
7. what's published (reports, the status ledger).

Do the same, briefly, for temperature and for occupancy (the PIR).

## 3. Duplicates and dead code

- The duplicates the configuration inventory left for Step 6: the power-source codes copied into three files, the PMIC fault masks, the battery labels.
- Anything in these modules that nothing calls. Note `SensorManager::getSignalStrength()`: v37's Stage 7 found no caller in `src/` and no ELF symbol.

These are removal candidates, which pay for some of Step 6.

## 4. Proposed Step 6 plan

- **The owners:** for each, its job in one sentence and what it alone decides. Prefer moving existing code to rewriting it. Prefer compile-time platform selection (one implementation file per platform) over virtual interfaces or new abstraction layers, unless there's a concrete reason.
- **The platform boundary:** the few functions each owner needs from the platform (for example: read the cell voltage, read the charge, read the temperature, set the charge limits). For each, say where the Boron version comes from and what the M-SoM version would call (Device OS APIs; TMP112 over I²C).
- **The WOs in order.** Each gets a plain goal, a size budget in net `src/` lines (aim for net negative wherever code moves or duplicates go), and what it depends on. Expected order:
  1. the battery-authority fixes (the tug-of-war and the failsafe allowance);
  2. the SensorManager split;
  3. battery trust and tier tuning, after a week of v37's `vc` data;
  4. the M-SoM side and Muon bring-up.

  Change the order if the inventory shows a better one, and say why.
- **Size:** flag anything that would make a WO bigger than about 60 lines, and propose how to split it.

## Report (your final message)

The four sections within the output budget, plus the model and reasoning level actually used. Distinguish what you observed in the code from what you infer.

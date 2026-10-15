# WO-2026-10-15-003: Step 6 — the power-owner split, and shrinking the sleep and main cores (Step 0 only)

**Author:** architect, 2026-10-15 · **Status:** Step 0 (read-only); no implementation until the architect accepts a sequence
**Branch:** `wo/2026-10-15-003-step6-power-owner-split`, from main (`71e346f` or later)
**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §3 Stages 2–4; §12.1 (history first); §12.2–12.4; §13 (b), (c), (j)
**Sources:** `docs/RECOVERY_PLAN_2026-09-26.md` "Step 6: the power-owner split" and "Connectivity"; `docs/architecture-review-2026-09-03.md` "Where complexity is concentrated", "Bottom line" and "Next session — agreed direction" item 4.
**Start from:** `2026-10-06-step6-ownership-codex-report.md` and `2026-10-07-step6-extensibility-evidence.md` (the earlier Step 6 ownership work). Update what changed since 6 Oct (2a, 2b, v41–v43); don't redo it.
**Naming note:** "Step 7" stays reserved for the `SysData`/`CurrentData` layout change (`WO-2026-09-23-001-step5-persistent-data-facade-split.md:216`), which waits until this split settles field ownership. WO 2a and 2b were the sensor half of Step 6; this is the power half.

## Plain goal

Give power, connectivity and sleep one clear owner each, by **moving** code out of `State_Sleep.cpp` and `Generalized-Core-Counter.cpp` into `power/`, `time/` and `cloud/`. Moves are behavior-neutral and come first; the behavior fixes the recovery plan queued for Step 6 are then made in the new owner, each as its own small WO.

## Where we are (architect, measured on `71e346f`)

| File | Grok, 3 Sep | Now |
|---|---:|---:|
| `src/Generalized-Core-Counter.cpp` | ~3,202 | 2,719 |
| `src/state/State_Sleep.cpp` | ~1,715 | 1,814 |
| `src/sensors/SensorManager.cpp` | ~1,416 | 1,227 |
| `src/MyPersistentData.cpp` + `.h` | ~1,842 | 2,065 |

Grok's items 1–3 are done (`src/time/` owns post-wake clock state; `MyPersistentData.h` has 7 includers, down from 22; the stub modules are gone). Persistence layout is Step 7, out of scope.

## Step 0 questions (report ≤ 150 lines on the WO branch, committed and pushed)

1. **Inventory.** For `State_Sleep.cpp` and `Generalized-Core-Counter.cpp`, list every function (file:line, length) and its one job, grouped as (a) belongs here, (b) belongs in an existing module (name it), (c) needs a new module (justify; prefer existing ones).
2. **History first (§12.1).** For each (b)/(c) group: was it ever elsewhere and moved back? Restoring the earlier home is the default unless the reason still holds.
3. **Coupling.** Per group: the file-statics, retained fields and facade accessors it touches, and who else touches them. A move needing a new shared global or persisted field is a STOP for that group.
4. **The recovery plan's Step 6 items**, mapped to owners. For each, where it lives today (file:line), which owner it moves to, and whether the fix is a move or a behavior change:
   - the duplicated power-source codes and PMIC fault masks;
   - the Boron versus M-SoM build selection;
   - the connection-mode tug-of-war (configured mode versus battery downgrade);
   - the failsafe's allowance for deliberate spacing;
   - the 2b leftovers 6c (`PowerDiagnostics` labels) and 6d (`PowerCapabilities::hasFuelGauge`);
   - slow modem teardown and the standby latch (field evidence 15 Oct, recovery plan): where the modem-off budget, alert 15 and the latch live.

   Battery trust, WO 3 and the CONSERVING threshold wait for data; map them only.
5. **Grok item 4.** Can `SessionState` (RAM) and the retained forensic fields disagree after an unexpected reset? Name the fields and paths. Answer only.
6. **Sequence.** Up to **six** WOs: behavior-neutral moves first (largest core shrink first), then the behavior fixes on the new owners. Each WO:
   - **≤ +20 net `src/` lines, hard**; a pure move (code relocated unchanged plus its include and declaration) counts 0;
   - golden payload test unchanged; existing tests passing (re-pointed anchors count as test lines);
   - states the core lines removed and the expected ELF change (a pure move is about 0 text).
7. **Stop and report** if a move can't be behavior-neutral, needs a persisted field, or the useful part needs more than six WOs (say what the six-WO cut leaves).

## Standing rulings that carry over

- The PIR-wake block stays in `State_Sleep.cpp` (2b ruling, 14 Oct) unless there's a clear reason to move it.
- No framework or new abstraction layer: move code, don't wrap it.
- The daily close order (publish → `dailyCleanup()` → restart) stays where it is.
- 2b-7 and 2b-8 stay deferred to the first new-sensor WO (Option A).

## Agents and models

- **Step 0:** Codex `gpt-5.6-sol`, **high**, read-only. An independent map of the two largest files and a design sequence; Codex is the investigator (§2).
- **Controller:** Claude Code checks every file:line in the report against the tree before it reaches the architect (§13 (j)). No ARM build.

## Budget

Step 0: 0 `src/` lines. The eventual WOs: stated per WO when the architect accepts the sequence.

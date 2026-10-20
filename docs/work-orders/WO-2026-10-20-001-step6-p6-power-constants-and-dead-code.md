# WO-2026-10-20-001: Step 6 P-6 — one home for power-source codes and PMIC masks; delete 6c/6d

**Author:** architect, 2026-10-20 · **Status:** ready (citation re-check done 2026-10-20, below)
**Branch:** `wo/2026-10-20-001-step6-p6-power-constants`, from main **after #123 (P-5) merges**
**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §3 Stages 6–8; §12.2–12.4; §13 (b), (c), (g), (j)
**Spec source:** `WO-2026-10-15-003-step0-report.md` Q4 rows "Power-source codes", "PMIC masks", "6c/6d", closing table WO 6; recovery plan, Step 6 list. Architect's citations at main `d24c401`.

## Plain goal

Remove duplication and dead code in the power modules: one definition of the power-source codes, one of the PMIC fault masks, and delete two unused labels and an unread field. **No behavior change; net `src/` should go down** (Codex estimated about −52). This is the last Step 6 power WO before v45.

## What changes (architect, at `d24c401`; re-check every site)

1. **Power-source codes, defined three times today:** `PowerManager.cpp:12–17`, `PowerPlatform.cpp:10–15`, `PowerDiagnostics.cpp:18–23` (`kPowerSourceUnknown` … `kPowerSourceBattery`, values 0–5, each in an anonymous namespace). Define them **once**, public, in `power/PowerPlatform.h`; the three files use that and their local copies go. Values unchanged. (`PowerManager.cpp`'s `[[maybe_unused]]` on `kPowerSourceUnknown` goes with its copy.)
2. **PMIC REG09 masks, defined twice:** `SensorManager.cpp:77–78` (`PMIC_CHRG_FAULT_MASK` 0x30, `PMIC_BAT_FAULT_MASK` 0x08) and `PmicFaultMonitor.cpp:19–21` (`kPmicChargeFaultMask` 0x30, `kPmicBatFaultMask` 0x08, `kPmicNtcFaultMask` 0x07). Define them **once**, public, in `power/PmicFaultMonitor.h`; `SensorManager.cpp` uses those. Values unchanged.
3. **6c, dead labels:** `availabilityLabel` (`PowerDiagnostics.cpp:133`) and `batteryContextLabel` (`:148`), declared at `PowerDiagnostics.h:7–8`. The architect's bare-name search at `d24c401` finds no caller in `src/` (only the definitions, the declarations and the four test stubs under `tests/stubs/**/power/PowerDiagnostics.h:8–9`). **Repeat that search yourself, bare and qualified, before deleting (§13 (c); the 2a lesson with `profileSelectionReasonLabel`).** Delete the definitions, the declarations and the stub copies.
4. **6d, unread field:** `PowerCapabilities::hasFuelGauge` (`PowerManager.h:62`), written at `PowerPlatform.cpp:123` and `:131`, read nowhere (bare-name search, `src/` and `tests/`). Delete the field and both writes.

## Rules

- No value, name of a log string, or call order changes. Constants keep their values; only where they're defined moves.
- Retained: the P-0 struct stays at `0x2003f420`; `nm` retained check (narrowed rule). No `retained` keyword added.
- Whitespace on moved or deleted lines follows the P-3 waiver; new lines must be clean.
- If a "dead" item turns out to have a caller, keep it, report it, and carry on with the rest.

## Budget

- **`src/`: net must be ≤ 0 (expected about −50), and ≤ +20 hard in any case.**
- Tests: guideline +100 (stub edits, a structural test that each constant has exactly one definition in `src/`).

## Tests and checks

- Golden payload test unchanged; suite `N/N`, serial.
- **Structural:** exactly one definition of each power-source code and each PMIC mask in `src/`; no `availabilityLabel`, `batteryContextLabel` or `hasFuelGauge` left in `src/`. A mutation that re-adds a local `kPowerSourceVin = 1` copy in `PowerManager.cpp` must fail.
- **Values:** a test (or `static_assert`) pins the six source codes (0–5) and the three masks (0x30, 0x08, 0x07); a mutation changing `kPowerSourceUsbAdapter` to 4 must fail.
- **ELF:** text should shrink or stay level; explain any growth by symbol. Also build with `ENABLE_DIAGNOSTICS_PUBLISH_MODE=1` and `ENABLE_PMIC_FORENSICS=1` (the guarded code paths that use these constants) to show they still compile.

## Agents and models

- **Stage 6:** Copilot, Sonnet tier, **medium**. Small, mechanical, mostly deletions.
- **Controller (before Stage 7, §13 (g)):** reproduce the builds (release plus the two flag builds), the `nm` check, the mutations and the suite.
- **Stage 7:** Codex `gpt-5.6-sol`, **medium**. Checks: values identical, the dead items really are dead (its own bare and qualified searches), the flag builds compile, net `src/` negative.

## Bench

None in this WO. With P-6 merged, Step 6's power half is code-complete; v45 (P-0 to P-6) follows, built on Device OS 6.6.0 once v44 is the default.

## Closing record

Budget versus actual (net `src/`, tests), the dead-code searches, ELF by symbol, retained outcome.

## Claude Code's citation re-check (2026-10-20, main `db98ba6`, P-0 to P-5 merged)

**Every site holds** (the `d24c401` numbers are unchanged): the power-source codes at `PowerManager.cpp:12–17` (with `[[maybe_unused]]` on `kPowerSourceUnknown` at `:12`), `PowerPlatform.cpp:10–15` and `PowerDiagnostics.cpp:18–23` (values 0–5: Unknown, Vin, UsbHost, UsbAdapter, UsbOtg, Battery); the PMIC masks at `SensorManager.cpp:77–78` (`PMIC_CHRG_FAULT_MASK` 0x30, `PMIC_BAT_FAULT_MASK` 0x08, used at `:155` and `:172`) and `PmicFaultMonitor.cpp:19–21` (`kPmicChargeFaultMask` 0x30, `kPmicBatFaultMask` 0x08, `kPmicNtcFaultMask` 0x07; used at `:24, :129, :153, :159, :189, :197, :388, :397`); `SensorManager.cpp` already includes both `power/PowerPlatform.h` (`:15`) and `power/PmicFaultMonitor.h` (`:21`).

**My own bare and qualified caller searches (all tracked files except `docs/html` and the WO documents):**
- **`availabilityLabel`:** only the definition `PowerDiagnostics.cpp:133`, the declaration `PowerDiagnostics.h:7`, and one inline copy in each of the four test stubs `tests/stubs/{power,pmic_fault_monitor_overrides,power_composition_overrides,power_source_override_overrides}/power/PowerDiagnostics.h:8`. No qualified form, no address-of, no macro.
- **`batteryContextLabel`:** only `PowerDiagnostics.cpp:148`, `PowerDiagnostics.h:8` and the same four stubs at `:9`.
- **`hasFuelGauge`:** only the field `PowerManager.h:62` and the writes `PowerPlatform.cpp:123` and `:131`; no read in `src/`, `tests/` or `lib/`. (The WO documents mention it historically.) All three are dead as the WO says.

**Findings the implementer must plan for (not WO changes):**
- **Four test stubs replace `PowerPlatform.h`** (`tests/stubs/power/`, `diag_overrides/`, `pmic_fault_monitor_overrides/`, `power_source_override_overrides/`; `connection_mode_overrides/` just forwards to `stubs/power/`). Host tests compile the real `PowerManager.cpp` and `PowerDiagnostics.cpp` against those stubs, so once the codes live in the real `power/PowerPlatform.h` the stubs need the same six constants (or one shared include) or those tests stop compiling. `tests/power_source_override_test.cpp:71–76` defines its own `kPowerSource*` copies, which would then redefine them: re-point it to use the shared constants.
- **Scope of the new definitions:** the three source files use the names unqualified inside anonymous namespaces. Defining the codes at global scope in `PowerPlatform.h` (not inside `namespace PowerPlatform`) keeps every use line unchanged; say which you chose.
- **`PmicFaultMonitor.h`** includes only `<stdint.h>` and forward-declares `PMIC`; the three masks go there as `constexpr uint8_t`. No test stub replaces that header (`pmic_fault_monitor_test.cpp` includes the real one).
- **The flag builds.** `ENABLE_DIAGNOSTICS_PUBLISH_MODE` guards large blocks of `PowerDiagnostics.cpp` (`:9–12`, `:25–129`, `:358–369`, `:372–445`) and `PmicFaultMonitor.cpp:416–420`. `ENABLE_PMIC_FORENSICS` **already defaults to 1** in `BuildProfile.h:130`, so a build "with `ENABLE_PMIC_FORENSICS=1`" is the release build; the arm the default does not exercise is `-DENABLE_PMIC_FORENSICS=0` (`SensorManager.cpp:44–65`). Build the release, `-DENABLE_DIAGNOSTICS_PUBLISH_MODE=1`, and `-DENABLE_PMIC_FORENSICS=0`.
- **`hasFuelGauge` (Chip, 20 Oct):** it will be needed again for P2 support. Delete it anyway, since it is unread today, but **record in the closing record and in the recovery plan's Boron/M-SoM portability item where it was** (`PowerManager.h:62`, set at `PowerPlatform.cpp:123` and `:131`) **and that it returns with its first reader in the P2/M-SoM portability work.**
- **Budget:** expected about −50 `src/` lines; the stub edits and the new constant-pinning test are test lines (guideline +100).


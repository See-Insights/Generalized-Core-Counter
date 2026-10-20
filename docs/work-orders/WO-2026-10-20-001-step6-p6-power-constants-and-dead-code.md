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



## Stage 6/7 result, rulings and closing record (2026-10-20)

**Stage 6** (Copilot, Sonnet 5.5, medium): one implementation round plus one test-only follow-up. **Controller reproduction** (Claude Code, 6.4.1 Boron, fresh `BUILD_PATH_BASE` per build, `-frandom-seed=p6`; "before" is `git archive HEAD`, whose `src/` equals main `db98ba6`): suite **91/91** (sh via zsh, py via python3, serial; baseline 89); text/data/bss release `151748/1094/2228 → 151724/1094/2228` (−24), `-DENABLE_DIAGNOSTICS_PUBLISH_MODE=1` `152864/1094/2476 → 152848/1094/2476` (−16), `-DENABLE_PMIC_FORENSICS=0` `150844/1062/2228 → 150812/1062/2228` (−32); five mutations each caught by a named test. **Stage 7** (Codex `gpt-5.6-sol`, medium): **VERIFIED WITH CONCERNS**, retained outcome A; checks 1, 2, 4, 6, 7 PASS, checks 3 and 5 CONCERN (`WO-2026-10-20-001-stage7-verdict.md`, restored from the Codex session log because `-o` overwrote the file).

**Findings and rulings (Chip, 20 Oct):**
- **F1 (the test mirror `PowerSourceCodes.h` could drift: `kPowerSourceUsbOtg = 44` survived 91/91) and F2 (its comment named a nonexistent test).** Ruling: delete the mirror and have the stubs include the real header, else keep it with a `static_assert` cross-check. Copilot's follow-up tried the real header by explicit path and it failed to compile (`redefinition of 'PowerSourceSnapshot'` and `'PowerConfigurationApplyResult'`: the real header declares what the stubs declare), so the **fallback** was taken: `power_constants_values_test.cpp` now `static_assert`s each mirror code against the real header (+13 lines), and the comment names the right file. Reproduced: the 44 mutation in the mirror and in the real header each fail `power_constants_values_test.sh`. Tests-only; `src/` unchanged.
- **F3 (the tracked Doxygen pages under `docs/html/` still list `availabilityLabel`, `batteryContextLabel` and `hasFuelGauge`): left for the next Doxygen regeneration.** The pages are generated and not compiled; they are stale until then.

## Closing record

| Item | Budget | Actual |
|---|---|---|
| `src/` net | ≤ 0 (hard ≤ +20) | **−61**: `+14/−75` over 9 files (`PowerPlatform.h` +8, `PmicFaultMonitor.h` +4; `PowerDiagnostics.cpp` −45, `PowerPlatform.cpp` −9, `PowerManager.cpp` −7, `SensorManager.cpp` −6, `PmicFaultMonitor.cpp` −3, `PowerDiagnostics.h` −2, `PowerManager.h` −1) |
| Tests | guideline +100 | about **+103** after the follow-up (+90 at Stage 7, +13 for the cross-check): new `power_constants_structural_test.py` (54), `power_constants_values_test.{cpp,sh}` (35 with the cross-check, 13), `PowerSourceCodes.h` (about 12); one include line in each of the four `PowerPlatform.h` stubs; the dead label copies out of five `PowerDiagnostics.h` stubs; `power_source_override_test.cpp` −6 |
| Suite | baseline 89 | **91/91** (sh via zsh, py via python3, serial); golden fixture sha256 `07de1e1980e4a44e03420540e6c721a93a61b5d6392539219928420c048f0637` unchanged |

- **Candidate pinned:** the `src/` tree hash (`find src -type f | sort | shasum -a 256 | shasum -a 256`) is `da6ae1317803daf9db53c0dd95feb66cc8bb834474f2cc61d25bc8d0d017ac7d` at Codex's review and after the follow-up.
- **Decisions:** the codes are at global scope in `PowerPlatform.h` and the masks at global scope in `PmicFaultMonitor.h` (outside the `PmicFaultMonitor` namespace, because the use sites in `PmicFaultMonitor.cpp` are in a global anonymous namespace), so no use line changed except the two mask names in `SensorManager.cpp`. The stubs share one include, `tests/stubs/power/PowerSourceCodes.h`, instead of four copies of the constants.
- **The dead-item searches** (Copilot's, Claude Code's at the citation re-check, and Codex's own): `availabilityLabel`, `batteryContextLabel`, `hasFuelGauge`: no caller, no qualified form, no address-of, no macro, in `src/`, `tests/` or `lib/`; all three absent from the release ELF. Nothing kept.
- **`hasFuelGauge` (Chip, 20 Oct):** deleted although it will be needed again. **It was declared at `PowerManager.h:62` and set at `PowerPlatform.cpp:123` and `:131` (main `db98ba6`); it returns with its first reader in the P2/M-SoM portability work.** Recorded in the recovery plan's Boron versus M-SoM item.
- **ELF (6.4.1 Boron):** text −24 / −16 / −32 for the three builds, data and bss unchanged. By symbol (release, Codex): `PowerManager::PowerManager()` 92 → 84, `refreshInputProfile()` 572 → 564, `setup()` 46 → 40, `detectCapabilities()` 22 → 20, `hasPmic()` 4 → 18 (inlining), `readPowerSource()` 152 → 168; unnamed literal and alignment fill −30 B accounts for the rest. Static-init stubs 22 / 23 / 22 before and after.
- **Retained outcome A:** the whole `.backup` section (`0x98` bytes at `0x2003f400`, byte-identical, SHA-256 `adfe530f…b7`) and every symbol unchanged (`retainedBoot` `0x2003f420`, hibernate fields `0x2003f480/488/48c/490`, signature `0x2003f494`); only its flash load address moved (`0xd9448 → 0xd9430`). No `retained` keyword added.
- **Deviations:** the stubs include a shared mirror header and not the real header (F1 fallback, above); the masks are at global scope.
- **Rounds:** Stage 6 one round plus one test-only follow-up; Stage 7 once (VERIFIED WITH CONCERNS, rulings applied). Net `src/` is negative and the power half of Step 6 is code-complete; v45 (P-0 to P-6) follows on Device OS 6.6.0 once v44 is the default.

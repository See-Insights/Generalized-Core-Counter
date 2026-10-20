AGENT: Copilot · MODEL: claude-sonnet-5.5 (confirmed with a one-line probe on 2026-10-20, §5) · REASONING: medium
AUTHORIZATION SCOPE:

**Authorized:**
- **Edit:**
  - `src/power/PowerPlatform.h`: add the six power-source codes (`kPowerSourceUnknown` … `kPowerSourceBattery`, values 0–5), once, public, unchanged in value; `src/power/PowerManager.cpp`, `src/power/PowerPlatform.cpp`, `src/power/PowerDiagnostics.cpp`: remove their local copies (and `PowerManager.cpp`'s `[[maybe_unused]]` with its copy);
  - `src/power/PmicFaultMonitor.h`: add `kPmicChargeFaultMask` (0x30), `kPmicBatFaultMask` (0x08) and `kPmicNtcFaultMask` (0x07) as `constexpr uint8_t`, once, public; `src/power/PmicFaultMonitor.cpp`: remove its local copies; `src/sensors/SensorManager.cpp`: remove `PMIC_CHRG_FAULT_MASK` and `PMIC_BAT_FAULT_MASK` and use the shared names at `:155` and `:172`;
  - `src/power/PowerDiagnostics.{h,cpp}`: delete `availabilityLabel` and `batteryContextLabel` (definitions and declarations); `src/power/PowerManager.h` and `src/power/PowerPlatform.cpp`: delete the `hasFuelGauge` field and both writes;
  - the four test stubs `tests/stubs/*/power/PowerDiagnostics.h` (delete the two inline label copies) and the four `tests/stubs/*/power/PowerPlatform.h` that replace the real header (add the six constants, or one shared include; say which).
- **Tests:** re-point `tests/power_source_override_test.cpp` (its local `kPowerSource*` at `:71–76`) and any other test the change breaks; add the structural test and the values test described below; report each change.
- **Run** the host suite and local ARM builds.
- **Scratch directory:** **`build-tmp/wo20261020-001-stage6/`**. Remove **only it** when you finish.

**Not authorized:**
- Any change to a value, to a log string or to the order of any call; renaming any constant.
- **Before deleting each of the three dead items, run your own bare-name and qualified searches** (`git grep` over `src/`, `tests/` and `lib/`, plus address-of and macros); if one has a caller, keep it, report it, and continue with the rest.
- Any other `src/` change, `docs/`, `lib/`, `project.properties` or Device OS; the `retained` definitions (including `retainedBoot`).
- Commits, pushes, merges, branch changes, stash, reset, checkout or restore. Device or network access beyond the Particle compile. Deleting anything you didn't create, including `build-tmp/` itself.

**SIZE BUDGET:**
- **`src/`: net must be ≤ 0 (expected about −50), and ≤ +20 hard in any case.** Over +20 means STOP and report (§13 (b)); a net above 0 but within +20 means report why.
- **Tests:** guideline +100 (stub edits, the structural test, the values test); record the actual.

# Stage 6 dispatch: WO-2026-10-20-001 (Step 6 P-6: one home for power-source codes and PMIC masks; delete 6c/6d)

**Goal, in plain language:** remove duplication and dead code in the power modules: one definition of the six power-source codes, one of the three PMIC REG09 fault masks, and delete two unused labels (`availabilityLabel`, `batteryContextLabel`) and one unread field (`hasFuelGauge`). **No behaviour change** and the net line count of `src/` goes down.

**Repository:** this worktree, branch `wo/2026-10-20-001-step6-p6-power-constants` (from main `db98ba6`, P-0 to P-5 merged). HEAD is at the commit that adds the WO and this dispatch. Don't switch branches. Main is Device OS 6.4.1; build with the 6.4.1 toolchain.

**Binding spec:** `docs/work-orders/WO-2026-10-20-001-step6-p6-power-constants-and-dead-code.md`, including **Claude Code's citation re-check at `db98ba6`** (every site, the dead-item searches, and the findings about the four `PowerPlatform.h` stubs and the flag builds). Where this dispatch and the WO differ, the WO wins; report the difference.

## Do

1. The two shared definitions and the three deletions as listed above. The codes go at global scope in `PowerPlatform.h` unless you find a reason not to (say which), so the use sites do not change.
2. Update the stubs and `power_source_override_test.cpp` so every host test that compiles the real power sources still builds and passes.
3. **Structural test (new):** exactly one definition of each power-source code and each PMIC mask in `src/`; none of `availabilityLabel`, `batteryContextLabel`, `hasFuelGauge` anywhere in `src/` (after stripping comments). **Values test (new, or a `static_assert` the suite compiles):** the six codes are 0–5 and the masks are 0x30, 0x08 and 0x07.

## Verify (all required)

- **Golden payload test** unchanged; **full suite** before and after, `N/N (sh via zsh, py via python3)`, **run serially, one suite at a time**; the baseline at `HEAD` is 89.
- **Mutations, each in a scratch copy, each must fail a named test (or be recorded as already unpinned at `HEAD`):** (1) re-add a local `kPowerSourceVin = 1` copy in `PowerManager.cpp`; (2) change `kPowerSourceUsbAdapter` to 4 in the shared definition; (3) re-add a local `PMIC_CHRG_FAULT_MASK` copy in `SensorManager.cpp`; (4) change `kPmicChargeFaultMask` to 0x20; (5) re-add `hasFuelGauge`.
- **Builds (6.4.1 Boron; a fresh `BUILD_PATH_BASE` for each; before from `git archive HEAD`, after from the working tree):** the **release** build, a build with `EXTRA_CFLAGS=-DENABLE_DIAGNOSTICS_PUBLISH_MODE=1`, and a build with `EXTRA_CFLAGS=-DENABLE_PMIC_FORENSICS=0` (the release already has `ENABLE_PMIC_FORENSICS=1`). Text/data/bss for all six; text should shrink or stay level; explain any growth by symbol with `nm -C -S`; list any new static-initialization stub.
- **Retained check under the narrowed rule:** the whole `.backup` section before and after (release build); `retainedBoot` at `0x2003f420`, the hibernate fields at `0x2003f480/488/48c/490`, the signature word at `0x2003f494`; list any move.
- **`git diff --check`:** clean on new lines (hits on moved or deleted lines are waived only where the original has the same whitespace).

## Implementation Report (required, as your final message)

- **Files:** changed and new, with net `src/` lines and the arithmetic; test lines added and removed; each stub edit and re-pointed test and why it is not weaker.
- **The dead-item searches** you ran (commands and results) for each of the three deletions.
- **Results:** the suite before and after, the mutations and the tests that caught them, the six build sizes and symbol deltas, the full `.backup` table.
- **Confirmations:** you removed only your scratch directory; no value changed.
- **Deviations** (or "none"), and **the model used**.

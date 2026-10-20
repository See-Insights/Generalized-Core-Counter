AGENT: Codex · MODEL: gpt-5.6-sol (standard tier; confirmed with a one-line probe on 2026-10-20, §5) · REASONING: medium
AUTHORIZATION SCOPE:

**Authorized:**
- Review the uncommitted diff in this worktree (branch `wo/2026-10-20-001-step6-p6-power-constants`; main `db98ba6`).
- Run the host suite and the tests (**one suite at a time**: parallel suites collide on shared temp files); run local ARM builds in scratch copies.
- Write temporary files under **`build-tmp/wo20261020-001-stage7/`** or `$TMPDIR`, and remove **only** what you create.
- Mutate only in scratch copies (use `git worktree add --detach` for copies: three structural tests need a real git history and fail in a plain file copy).
- Write your verdict to `docs/work-orders/WO-2026-10-20-001-stage7-verdict.md` (the only file you may add in the repo).

**Not authorized:**
- Changes outside your scratch locations and the verdict file.
- Commits, pushes, stash, reset, checkout or restore in the review worktree.
- Device, network or AWS access.

# Stage 7 dispatch: WO-2026-10-20-001 (Step 6 P-6: one home for power-source codes and PMIC masks; delete 6c/6d)

**Goal, in plain language:** the six power-source codes and the three PMIC REG09 masks were each defined in several places; they are now defined once. Two unused labels (`availabilityLabel`, `batteryContextLabel`) and one unread field (`PowerCapabilities::hasFuelGauge`) are deleted. **No behaviour change, and net `src/` goes down.**

**Binding spec:** `docs/work-orders/WO-2026-10-20-001-step6-p6-power-constants-and-dead-code.md`, including Claude Code's citation re-check at `db98ba6`. Standing: no `retained` keyword added; `git diff --check` hits on moved or deleted lines are waived only where the original has the same whitespace.

**Inputs:** `docs/work-orders/WO-2026-10-20-001-stage6-copilot-dispatch.md`, and Copilot's report (not in the repo; the figures below are Claude Code's reproduction). Treat every claim as unverified.

**What changed** (uncommitted):
- **`src/`: 9 files, +14 / −75, net −61.** `PowerPlatform.h` gains the six codes at global scope (+8); `PmicFaultMonitor.h` gains the three masks at global scope as `constexpr uint8_t` (+4; they sit outside the `PmicFaultMonitor` namespace because the use sites in `PmicFaultMonitor.cpp` are in a global anonymous namespace); the local copies are removed from `PowerManager.cpp` (including the `[[maybe_unused]]`), `PowerPlatform.cpp`, `PowerDiagnostics.cpp`, `PmicFaultMonitor.cpp` and `SensorManager.cpp` (whose two uses now name `kPmicChargeFaultMask` and `kPmicBatFaultMask`); `availabilityLabel`, `batteryContextLabel` and `hasFuelGauge` (declared `PowerManager.h:62`, written `PowerPlatform.cpp:123,131` at `db98ba6`) are deleted.
- **Tests: +104 / −14, net +90 (guideline +100).** New `power_constants_structural_test.py`, `power_constants_values_test.{sh,cpp}` (compiled against the real headers, `static_assert`s) and `tests/stubs/power/PowerSourceCodes.h`; the four `PowerPlatform.h` stubs include that file; the dead label copies are removed from five `PowerDiagnostics.h` stubs; `power_source_override_test.cpp` drops its local `kPowerSource*` copies.
- **Claude Code's reproduction (6.4.1 Boron, fresh `BUILD_PATH_BASE` per build, `-frandom-seed=p6`; "before" is `git archive HEAD`):** suite **91/91** (baseline 89; sh via zsh, py via python3; serial). text/data/bss: release `151748/1094/2228 → 151724/1094/2228` (−24); `-DENABLE_DIAGNOSTICS_PUBLISH_MODE=1` `152864/1094/2476 → 152848/1094/2476` (−16); `-DENABLE_PMIC_FORENSICS=0` `150844/1062/2228 → 150812/1062/2228` (−32). `.backup` is `0x98` at `0x2003f400` before and after, with every retained symbol at the same address (`retainedBoot` `0x2003f420`; hibernate fields `0x2003f480/488/48c/490`; signature `0x2003f494`); only the flash load address moved (`0xd9448 → 0xd9430`). Static-init stubs 22/23/22 before and after. The three dead names are absent from the release ELF. `PowerPlatform::hasPmic()` 4 → 18 B (inlining), `detectCapabilities()` 22 → 20 B.
- **Mutations I ran serially (scratch copies):** a local `kPowerSourceVin = 1` in `PowerManager.cpp` (`power_constants_structural_test.py`, `power_source_override_test.sh`, `pmic_fault_monitor_test.sh`, and others); `kPowerSourceUsbAdapter = 4` (`power_constants_values_test.sh`, `power_composition_test.sh`); a local `PMIC_CHRG_FAULT_MASK` in `SensorManager.cpp` (structural); `kPmicChargeFaultMask = 0x20` (`power_constants_values_test.sh`, `pmic_fault_monitor_test.sh`); `hasFuelGauge` re-added (structural).

## Checks (each one: PASS / FAIL / CONCERN, with evidence)

1. **Values identical.** The six codes are 0–5 (Unknown, Vin, UsbHost, UsbAdapter, UsbOtg, Battery) and the masks are 0x30, 0x08, 0x07, against `db98ba6`. Check every use site resolves to the shared definition with the same type (`int`, `uint8_t`) and no change in promotion or comparison (`PmicFaultMonitor.cpp`, `SensorManager.cpp:155,172`, the three power sources). Any change to a log string or call order is a FAIL.
2. **One definition each,** by your own search over `src/` (including `lib/`, macros and the `PowerPlatform::` / `PmicFaultMonitor::` qualified forms): exactly one definition of each of the nine names; no `PMIC_CHRG_FAULT_MASK` or `PMIC_BAT_FAULT_MASK` left. State whether global scope in the two headers can collide with any name in Device OS or the libraries (ODR, ADL, a second include path).
3. **The dead items are really dead,** by your own bare and qualified searches (`git grep` over `src/`, `tests/`, `lib/`; address-of; macros; string-built names): `availabilityLabel`, `batteryContextLabel`, `hasFuelGauge`. Note also whether anything outside this repository's tree (generated files, docs that are built) could reference them.
4. **Both flag builds compile** (release is `ENABLE_PMIC_FORENSICS=1` already): `-DENABLE_DIAGNOSTICS_PUBLISH_MODE=1` and `-DENABLE_PMIC_FORENSICS=0`, with your own local build or, if the sandbox forbids it, a reading of every guarded block that uses the moved names (`PowerDiagnostics.cpp:9–12, :25–129, :358–369, :372–445`; `PmicFaultMonitor.cpp:416–420`; `SensorManager.cpp:44–65`) and the include path each one relies on.
5. **The tests pin what they claim.** `power_constants_values_test.cpp` includes the real headers (not the stub); `PowerSourceCodes.h` is a test-side mirror that the four `PowerPlatform.h` stubs include: is there any way the mirror can drift from `src/power/PowerPlatform.h` without a test failing? (Its header comment names `power_source_codes_values_test.cpp`, which does not exist; the file is `power_constants_values_test.cpp`: state whether that is a stale reference.) Run two mutations of your own. Diff `power_source_override_test.cpp`: is it any weaker?
6. **ELF and retained.** Accounting of the text deltas by symbol (`nm -C -S`); no new static-init stub; the whole `.backup` section before and after, outcome A/B/C; no `retained` keyword in any added line.
7. **Budget.** `src/` net ≤ 0 (limit +20 hard); tests against the +100 guideline (flag, don't fail); `git diff --check`; full suite `N/N (sh via zsh, py via python3)`, run serially; the golden payload test passes with the fixture unchanged.

## Verdict (write it to the file above, then end with a short summary as your final message)

- **Header:** one of VERIFIED / VERIFIED WITH CONCERNS / NOT VERIFIED; the model and reasoning used; the test interpreter line.
- **A table of checks 1–7,** and, on its own line, **the retained outcome from check 6 (A, B or C)**.
- **Findings,** with file:line and severity, separating observation from inference.
- **Budget versus actual,** as one row.
- **Confirmations:** the tree matches the start, and you removed only your own scratch files.

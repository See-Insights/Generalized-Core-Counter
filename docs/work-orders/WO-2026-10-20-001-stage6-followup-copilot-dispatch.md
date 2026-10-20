AGENT: Copilot · MODEL: claude-sonnet-5.5 (confirmed with a one-line probe on 2026-10-20, §5) · REASONING: medium
AUTHORIZATION SCOPE:

**Authorized:**
- **Edit, tests only:** `tests/stubs/power/PowerSourceCodes.h` (delete, or keep per the fallback below); the four `tests/stubs/*/power/PowerPlatform.h` that include it (`power/`, `diag_overrides/`, `pmic_fault_monitor_overrides/`, `power_source_override_overrides/`); `tests/power_constants_values_test.{cpp,sh}` and `tests/power_constants_structural_test.py` if needed; any other test the change breaks (report each).
- **Run** the host suite (serially).
- **Scratch directory:** **`build-tmp/wo20261020-001-followup/`**. Remove **only it** when you finish.

**Not authorized:**
- **Any change under `src/`** (the `src/` tree is hashed before and after; one byte of difference fails the follow-up). In particular, do not move the constants into a new header.
- Any change to a value, a test's assertion strength, `docs/`, `lib/`, `project.properties`.
- Commits, pushes, merges, branch changes, stash, reset, checkout or restore. Device or network access. Deleting anything you didn't create.

**SIZE BUDGET:** `src/`: **0**. Tests: net should be negative (you are deleting a file).

# Stage 6 follow-up dispatch: WO-2026-10-20-001 (Step 6 P-6), Stage 7 findings F1 and F2

**Context:** Stage 7 (Codex) found that `tests/stubs/power/PowerSourceCodes.h`, a test-side copy of the six power-source codes, can drift silently: changing its `kPowerSourceUsbOtg` from 4 to 44 left all 91 tests passing (F1), and its header comment names a file that does not exist (F2). Verdict: `docs/work-orders/WO-2026-10-20-001-stage7-verdict.md`.

**Ruling (Chip):** delete the mirror and make the stubs get the codes from the **real production header** `src/power/PowerPlatform.h` (use an explicit path such as `#include "../../../../src/power/PowerPlatform.h"` where the include path would resolve `power/PowerPlatform.h` to the stub itself, and say what you did). **Only if that is impossible** (for example the real header declares things the stub also declares, so including both is a redefinition), **keep the mirror, add a `static_assert` cross-check against the real header's values, and fix its comment** to name `power_constants_values_test.cpp`. Report which path you took and why, with the compiler error if you fell back. Do not edit `src/` to make the first path work.

## Verify (all required)

- **The mutation that survived must now fail a named test:** change `kPowerSourceUsbOtg` from 4 to 44 in whichever file now carries the values the stubs use (the real header in path one, the mirror in the fallback); also change the real header's value and show a test fails in both paths. Do each in a scratch copy of the tree. A scratch copy has no `.git`; three structural tests need history and fail there for that reason only, so judge the mutation on the tests that run the power code.
- **Full suite**, `N/N (sh via zsh, py via python3)`, **serial**; the baseline is **91/91** (it should become 90 or 91 depending on the path).
- `git diff --check` clean on new lines.
- **Confirm `src/` is untouched:** `git status --short src` lists the same 9 modified files and `git diff -- src | shasum -a 256` is the same before and after your work (record both).

## Report (≤ 150 lines, as your final message)

Path taken and why; files changed with test lines added and removed; the mutation results (test names); the suite; the `src/` diff hashes; scratch removal; deviations (or "none"); the model used.

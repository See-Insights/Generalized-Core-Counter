<!-- Copilot claude-opus-5 (medium) Stage 6 item B Implementation Report, extracted verbatim from its --share transcript by Claude Code, 2026-10-02. Claude Code checked: item A's 7 files hash-identical; archive unchanged (120584). -->

# Implementation Report — WO-2026-10-02-003, item B

**Model/reasoning actually used:** claude-opus-5, medium.

## The change

| File | Net code lines |
|---|---|
| `src/Generalized-Core-Counter.cpp` (`loop()` OOM block) | **−2** (−5 `Log.error`×3 / `raiseAlert(14)` / `transitionTo`; +3 `Log.info` / `delay(100)` / `System.reset`) |
| `src/ResetCause.h` | **+1** (`RESET_CAUSE_OUT_OF_MEMORY = 7`) |
| `src/state/State_Error.cpp` (`resolveErrorAction()` case 14) | **−6** |
| **Item B total** | **−7** (budget ≈ +5; under, not over) |

## Alert-14 decisions

1. **`resolveErrorAction()` case 14 — REMOVED.** Nothing in `src/` raises 14 any more, so the case was unreachable from code. A 14 persisted by pre-v34 firmware now falls to `default: return 0` → ERROR_STATE returns to Idle without resetting, i.e. exactly what the old `resets >= 3` branch did, and strictly safer than soft-resetting on a stale alert. A comment records this.
2. **Boot-time clear (`clearOomAlertOnBoot`, `:1081`) — KEPT.** Reachable by *data*, not code: FRAM survives the upgrade, so a 14 written by pre-v34 firmware can still be present at boot. Without the clear it would persist as a severity-3 critical alert.
3. **`getAlertSeverity()` case 14 (`MyPersistentData.cpp:864`) — KEPT.** `raiseAlert()` calls it on the *existing* code. If 14 fell to `default: return 1`, a persisted critical 14 would be silently overwritten by any minor alert. Keeping it handles a persisted 14 safely.

## Tests

- **`tests/reset_cause_structural_test.py`** (extended): site count 6→7, plus explicit checks that `RESET_CAUSE_OUT_OF_MEMORY == 7` is declared and that some site passes it. Mutations, each on a throwaway `src/` copy — all 5 **caught**: bare `System.reset()`; OOM reusing code 6; enum entry deleted; enum value set to 0; OOM site reverted to the ERROR_STATE route.
- **`tests/oom_immediate_reset_test.sh` (new) — behavioral**, following the `firmware_update_dwell_test.sh` pattern: the real `if (outOfMemory >= 0)` block is extracted verbatim from `loop()` and compiled against fakes, with the real `src/ResetCause.h`. Asserts exactly one reset carrying code 7, no transition, no alert, at resetCount 0/3/4/200, and no action when `outOfMemory < 0`. **Mutation** (ERROR_STATE route restored, applied to a copy in `TMPDIR`; the checked-in source is never written): **caught** — the mutant compiles cleanly and fails on `issued 0 reset(s), expected exactly 1`.
- No existing test pinned text this item changed, so none other was updated. `tests/publish_with_ack_structural_test.py` passes **unchanged** (zero diff).

## Commands and results

1. **Host suite** (`zsh` for every `tests/*.sh`, `python3` for every bare `tests/*.py`): **59/59 before**, **60/60 after** (+1 new test).
2. **Local ARM release build** — boron, Device OS 6.4.1, fresh `BUILD_PATH_BASE` in scratch (this tree has no `clean-user` rule; a fresh base is the equivalent clean user build): **text 150556 / data 1090 / bss 2180** vs item A's 150692 / 1090 / 2180 → **−136 text, 0 data, 0 bss**. `strings`: `v34-SleepConfigLeak` ×1, `out of memory occurred size=%d` present; `Out-of-memory event detected` and `OOM alert but reset count` gone.
3. **Linkage**: in `loop()`, `movs r0, #7` / `bl cc17c <SystemClass::reset(...)>`.
4. `git diff --stat`: item B touches only `src/ResetCause.h` (+1), `src/state/State_Error.cpp`, the OOM hunk of `src/Generalized-Core-Counter.cpp`, and the two test files.

## Cleanup / scope

Deleted **only** `build-tmp/wo20261002-003-stage6-b/` and my own `$TMPDIR` build artifacts; `build-tmp/`, `build-tmp/connectivity-archive/` and everything else are untouched. No edits to item A's work (`State_Sleep.cpp`, `StateMachine.h`, version files, its tests — their diff stats are byte-for-byte what they were before I started), none to `lib/`, `project.properties` or `docs/`. No commit, push, merge, branch change, flash, or network access.

**Deviations:** one — `make clean-user` has no rule in this Device OS tree, so a fresh scratch `BUILD_PATH_BASE` was used instead (same precedent as the WO-2026-09-29 report).

---


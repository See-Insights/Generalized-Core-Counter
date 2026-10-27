# WO-2026-10-25-001 Stage 6 report (Copilot, claude-sonnet-5.5, medium) and controller acceptance

## Copilot's Implementation Report (verbatim)


Nothing is committed, pushed or merged. The full host suite passes 105/105 after the change (104/104 before).

**`src/` (only `State_Sleep.cpp`)**
- The occupied branch of `sleepPlan()` is the Step 0 §3 sketch with two comments and no cap. It is 7 lines added and 3 removed.
- `logWakeSummary()` no longer divides by 1000 again: 0 net lines.
- Net is **+4** against the +5 to +7 expected and the hard limit of +20. `State_Sleep.cpp` goes from 1275 to 1279 lines.
- Total `src/` goes from 22,378 to 22,382 lines.
- The `src/` tree hash changed from `87715dba33ea1d75ef6c689da75ec79d1b6d97307eb8301750d97b29d9c2f56b` to `1c5659d45f8544bee15dcc5665dc64f0811b6270aca3a0730a4d88d02e186dc8`.

**Harness (`tests/sleep_plan_equivalence_test.{cpp,sh}`)**
- There are 10 new scenarios, S81–S90, named `debounce-left`. They use the LED remaining at 300, 120, 5, 1 and 0 s, each with the reporting boundary nearer and further. The expected wake is computed in the harness from the rule, and a mismatch prints `EXPECT_MISMATCH` and fails the run.
- Values: 300/nearer → 300, 300/further → 301, 120 → 120 and 121, 5 → 5 and 6, 1 → 1 and 2, 0 → 100 (the boundary) and 300 (the debounce).
- The new scenarios set the boundary through the untrusted-clock interval, because with a trusted clock the boundary wake cannot go below 2 s.
- Three mutations are added to the harness's own mutation list.

**Fixture diff**
- Exactly S55–S67 gain one `CALL signalLEDTimeRemaining` line each. No plan value changes, and the other 75 scenarios are byte-identical.
- The 10 new scenarios are appended, and the header counts become 98 scenarios, release 91.
- `git diff --stat` for the fixture is +131/−7. Besides the 13 call lines and the new scenarios, the changes are the header and the summary counts.
- This matches the Step 0 list.

**New test:** `wake_summary_led_test.{cpp,sh}` extracts the real `logWakeSummary()` and checks the copy byte for byte. It asserts `led=150s`, and it fails on the old `/ 1000UL` code with `led=0s`.

**Mutations, each run in a `git worktree add --detach` copy and each caught by its aimed check (none was a compile failure)**

| # | Mutation | Caught by |
|---|---|---|
| 1 | old full-debounce block restored | harness `EXPECT_MISMATCH` and fixture diff; plan structural test |
| 2 | `+ 1` dropped | harness `EXPECT_MISMATCH`; plan structural test |
| 3 | `remaining > 0` guard dropped | harness `EXPECT_MISMATCH` |
| 4 | `/ 1000UL` restored | `wake_summary_led_test` (led=0s); `sleep_wake_structural_test` |

**ARM (Boron, Device OS 6.6.0, fresh `BUILD_PATH_BASE`, `-frandom-seed=$$@`):**

| | text | data | bss |
|---|---|---|---|
| before | 151572 | 1114 | 2200 |
| after | 151580 | 1114 | 2200 |

- Text grows by 8 B, inside ±64 B. By symbol, `handleSleepingState` is +16 B and `logWakeSummary` is −8 B.
- `.backup` is 152 B before and after, so nothing was added to retained memory.
- The release build used the `EXTRA_CFLAGS=-frandom-seed=$$@` form from the workflow, not the literal `-frandom-seed=wo1025` from the dispatch.

**Ruling 9 (base `bf4e628` plus the WO commits → now)**

| Measure | Before | After | Change |
|---|---|---|---|
| `src/` lines | 22,378 | 22,382 | +4 |
| Test lines excluding fixtures | 34,143 | 34,332 | +189 |
| Fixture lines | 20,435 | 20,559 | +124 |
| Suite | 104 | 105 | +1 |

- The test growth is about 1.9× the +100 guideline, so it sits right at the flag threshold.
- `git diff --check` is clean.

**Test files touched**
- `sleep_plan_equivalence_test.cpp` and `.sh`
- `fixtures/sleep_plan_transcript.txt`
- `sleep_plan_structural_test.py`
- `sleep_wake_structural_test.py`
- `sleep_preflight_structural_test.py` (it also copy-checks the plan block)
- `wake_summary_led_test.cpp` and `.sh` (new)

**Deviations**
- **Fixture regeneration:** `--regenerate` could not stay on the base blocks, because those predate the change. It now regenerates from the current blocks (mode b). The base check still runs: the base blocks must reproduce the fixture once this WO's delta is removed.
- **Structural tests:** they map the new text back to the old before the byte compare, so every other byte of the block is still compared.
- **Scratch:** `build-tmp/wo20261025-001-stage6/` is removed, and no worktrees remain.


## Controller acceptance (Claude Code, Sonnet 5.5, 25 Oct 2026; WO §13 (g): the mechanical proofs reproduced before Stage 7)

- **`src/` diff read:** one file, `State_Sleep.cpp`, matches the Step 0 §3 sketch (the occupied branch, no cap) and the one-line `logWakeSummary()` fix; net +4. **Hash** `1c5659d45f8544bee15dcc5665dc64f0811b6270aca3a0730a4d88d02e186dc8` (before: `87715dba…c2f56b`); recomputed by the controller, equal to Copilot's, and unchanged after the controller's mutation runs.
- **Suite:** re-run by the controller, serially (`.sh` via zsh, `.py` via python3): **105/105** (60 sh + 45 py).
- **Mutations (in place on the working file, restored and re-hashed after each; each caught by the check aimed at it):** (1) full debounce restored, (2) `+ 1` dropped, (3) the `remaining > 0` guard dropped: the plan harness exits 1 (`EXPECT_MISMATCH` and the fixture) and `sleep_plan_structural_test.py` exits 1; (4) `/ 1000UL` restored: `wake_summary_led_test.sh` and `sleep_wake_structural_test.py` exit 1. None is a compile failure.
- **Fixture diff against the Step 0 list:** the removed lines are the header and summary lines only; the added `CALL signalLEDTimeRemaining` lines are 23 = the 13 expected (S55-S67) + one in each of the 10 new scenarios S81-S90; no plan value in S1-S80 changed.
- **Structural-test edits** (`sleep_plan_`, `sleep_wake_`, `sleep_preflight_structural_test.py`): each requires the exact new text exactly once, maps it back to the base text, and still compares every other byte of the block; accepted.
- **Not yet done (Stage 7):** the Particle cloud compile (controller), the linkage check on a symbolised ELF, and Codex's independent review.

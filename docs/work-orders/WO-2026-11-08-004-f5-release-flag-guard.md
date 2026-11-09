# WO-2026-11-08-004: F5, a `v*` tag build fails unless `DEV_BUILD` and `CONNECTIVITY_FAILSAFE_TEST_MODE` are 0 (recovery item 28)

**Author:** Claude Code, 8 Nov 2026 · **Status:** DISPATCHED (architect, 8 Nov 2026) · **Branch:** `wo/2026-11-08-004-f5-release-flag-guard`, from `main`; PR to `main`, for Chip to merge (workflow files are under CODEOWNERS). Never merge.
**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §3 Stages 5-8; §4 (SHA rule); §10 (CI discovery); §12 · **Agents:** Stage 6 Copilot medium (the report states the level actually run); Stage 7 Codex `gpt-5.6-sol` **high**, 90-minute box, at most two rounds.
**Inputs:** `docs/RECOVERY_PLAN_2026-09-26.md` item 28 (the spec); `docs/reviews/2026-11-grok-progress/triage.md` (F5 accepted now); `.github/workflows/build.yml`; `src/BuildProfile.h`.
**Budget:** `src/` **none** (no firmware change; the binary must be byte-identical); `.github/workflows/build.yml` at most +12 lines; one new test.

## Plain goal
Grok's F5: nothing stops a tagged release from being built with the developer profile or the short-timer failsafe test mode. The existing `FIRMWARE_PRODUCT_VERSION` check on tags already reads the macro "as the compiler sees it"; add the same kind of check for the two flags, so a release built with either set to anything but 0 fails before it is compiled, uploaded or published.

## The change
1. **One step in the `firmware` job of `.github/workflows/build.yml`,** beside "Tag matches source FIRMWARE_PRODUCT_VERSION (before building)", `if: github.ref_type == 'tag'`, before "Compile": preprocess `src/BuildProfile.h` the same way (`cc -E -P -x c++ -include src/BuildProfile.h -` on the two macro names) and **fail with a `::error::` message naming the macro and its value** unless both are exactly `0`. It must also fail (not pass silently) if the preprocess fails or prints anything other than two tokens.
2. **The step's script is extracted and tested:** a new host test `tests/release_flags_guard_test.sh` (zsh, exits 0 bare, no arguments) pulls the step's shell body out of `build.yml` and runs it against (a) the real tree: passes; (b) a temporary copy whose `BuildProfile.h` default is `DEV_BUILD 1`: fails and names `DEV_BUILD`; (c) a copy with `CONNECTIVITY_FAILSAFE_TEST_MODE 1`: fails and names it; (d) both set; (e) a copy whose `BuildProfile.h` does not preprocess: fails; (f) a `-D` override is not the case here (the compile action passes no flags), so the test also asserts that `build.yml` contains no `EXTRA_CFLAGS`, `-D`, or `cflags` input on the compile step (a flag added to the workflow later must trip the test and be ruled on, not slip past a header-only check).
3. **Mutation tests (each must fail the test):** the step's `if:` guard removed; the comparison loosened to `-ge 0`; only one of the two macros checked; the `::error::` exit removed; `DEV_BUILD` compared as a string with a different value; the step moved after Compile.
4. Optional, only if it fits: a one-line note in `docs/bench-validation.md` ("Production verification") naming the guard.

## Not in scope
Any `src/` change; the tag-version check; publish and upload jobs; making bench-only builds impossible to build on branches (they build on branches by design: the guard is for tags only).

## Verify (workflow §10)
CI's discovery in a git worktree (`tests/*.sh` via zsh, `tests/*.py` via python3; state the measured baseline before editing); the new test exits 0 bare; `thermal_coupling_structural_test.py` also on a clean export of tracked files; `git diff --stat` shows no file under `src/`. Confirm `gh pr checks` green before saying ready. A real failing-tag build cannot be tried (no tags are created), so the extracted-script test is the proof; say so.

## Rollback
Revert the commit; tag builds return to checking only the product version.

## Amendment 1 (architect, 9 Nov 2026, after Stage 7 round 1: PASS on dbb6d5a with two Low findings)
1. **Low 1 fixed (test-only):** `tests/release_flags_guard_test.sh` now bounds the guard and Compile checks to the `firmware:` job and requires the guard to appear exactly once in `build.yml` (a guard moved into `host-tests` survived round 1; it is now caught, with four further placement mutations: after Compile, removed, duplicated, no Compile step). Applied by Claude Code as a narrow test-only fix on the architect's instruction; no `build.yml` or `src/` change.
2. **Low 2 accepted:** a compiler `#warning` is non-fatal; the real header emits none and no warning can let a nonzero flag through.

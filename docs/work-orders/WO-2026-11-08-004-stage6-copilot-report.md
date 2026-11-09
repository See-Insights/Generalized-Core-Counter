# WO-2026-11-08-004 Stage 6 report (Copilot)

- **Model:** Claude Sonnet 5.5 (`claude-sonnet-5.5`). **Reasoning level:** dispatched as medium; the level actually run could not be observed from inside the session.
- **Interpreters:** zsh 5.9 (arm64-apple-darwin26.0), Python 3.14.6, Apple clang 14.0.3 as `cc` (macOS host; CI uses ubuntu `cc`, not exercised here).
- **Measured baseline (before editing):** 117/117 (sh via zsh, py via python3).
- **After:** 118/118 (sh via zsh, py via python3). `thermal_coupling_structural_test.py` also passes (rc 0) on a clean export of tracked files; the scratch export was removed afterwards.
- `git diff --stat`: `.github/workflows/build.yml` +9, `docs/bench-validation.md` +1; nothing under `src/` or `lib/`. New untracked: `tests/release_flags_guard_test.sh`, this report. Nothing committed.

## The step (as written, before "Read Device OS version", after the tag-version step)

```yaml
      - name: Release flags DEV_BUILD and CONNECTIVITY_FAILSAFE_TEST_MODE are 0 (before building)
        if: github.ref_type == 'tag'
        run: |
          set -euo pipefail
          out=$(echo DEV_BUILD CONNECTIVITY_FAILSAFE_TEST_MODE | cc -E -P -x c++ -include src/BuildProfile.h - | tr '\n' ' ') || { echo "::error::preprocessing src/BuildProfile.h failed; not building"; exit 1; }  # the active macros, as the compiler sees them
          read -r dev fs extra <<<"$out"
          [ -n "$dev" ] && [ -n "$fs" ] && [ -z "$extra" ] || { echo "::error::could not read DEV_BUILD and CONNECTIVITY_FAILSAFE_TEST_MODE from the preprocessor (got '$out'); not building"; exit 1; }
          [ "$dev" = 0 ] || { echo "::error::DEV_BUILD is '$dev', must be 0 for a tag build; not building"; exit 1; }
          [ "$fs" = 0 ] || { echo "::error::CONNECTIVITY_FAILSAFE_TEST_MODE is '$fs', must be 0 for a tag build; not building"; exit 1; }
```

## Test: `tests/release_flags_guard_test.sh` (exits 0 bare; optional `WF=` env points at a mutated workflow copy)
Extracts the step body from `build.yml` and runs it with bash. Cases: (a) real tree passes; (b) `DEV_BUILD 1` fails naming `DEV_BUILD`; (c) `CONNECTIVITY_FAILSAFE_TEST_MODE 1` fails naming it; (d) both; (e) header with `#error` fails with `::error::`; (f) the Compile step has no `EXTRA_CFLAGS`/`cflags`/`-D…`. Extra checks: step has the tag-only `if:`; step precedes Compile; undefined `DEV_BUILD` fails; extra preprocessor tokens fail.

## Mutations (each applied to `build.yml`, backed up to `build-tmp/wo-2026-11-08-004-stage6/build.yml.orig`, restored with `cp`, byte-compared)
| Mutation | Result |
|---|---|
| `if:` guard removed | killed |
| `= 0` → `-ge 0` | killed (b) |
| only DEV_BUILD checked | killed (c) |
| only FAILSAFE checked | killed (b) |
| `::error::` exit removed (`exit 1` dropped) | killed (b) |
| DEV_BUILD compared to string `"00"` | killed (a) |
| step moved after Compile | killed |
| `extra-cflags: -DDEV_BUILD=1` added to Compile | killed (f) |
| `[ -z "$extra" ]` removed | **survived first**; added the extra-token case; now killed |

## Deviations
- Item 4 done (one line in `docs/bench-validation.md`).
- Two additional tests beyond the six (undefined macro, extra tokens) and the step has a `||` handler on the `cc` call so a preprocess failure emits `::error::`.
- Step is +9 lines (limit +12). The compare is string `= 0`, so `00` or `0x0` is rejected (fail closed).
- First full-suite attempt in one chained command was denied by the permission layer; rerun in pieces, no effect on results.

## Open questions
- A real tag build was not run; only the extracted script is tested, on macOS clang, not the runner's `cc`. `-include` with `-x c++ -` on GNU cc was assumed to behave as in the existing version check.
- The `-D` check is a grep heuristic on the Compile step; other action inputs that inject flags would need a name added.
- `gh pr checks` not run (no PR; out of scope).

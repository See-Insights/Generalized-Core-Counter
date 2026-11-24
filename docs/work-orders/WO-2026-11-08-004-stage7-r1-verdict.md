UNDER REVIEW: wo/2026-11-08-004-f5-release-flag-guard @ dbb6d5a8c0ac569cd31531d78559abeb8303fe7e

PASS

| # | Verdict | Evidence |
|---|---|---|
| 1 | PASS | `.github/workflows/build.yml:78-86` reads both macros through preprocessing and requires exact token `0`. `set -euo pipefail` plus the `||` handler catches a failing `cc` inside the substitution/pipeline; verified with a fake compiler that emitted valid output then exited 17. Blank whitespace is safely ignored; header stdout or a third token fails. `BuildProfile.h:64-259` has no includes and no compiler-dependent definitions affecting these flags. No project `build.mk`, `particle.json`, alternate definition, or compile flags exist. The action receives only platform and Device OS; its other relevant defaults are source folder `.`, auto-version false, and runner-local buildpack without a token. Particle documents only source/output volumes and `PLATFORM_ID` for that buildpack. [Compile action](https://github.com/particle-iot/compile-action), [Particle buildpack documentation](https://docs.particle.io/firmware/best-practices/firmware-build-options/). |
| 2 | PASS | `.github/workflows/build.yml:60-96,126-168`: guard is tag-only, inside `firmware`, before Compile and artifact upload. `publish` needs `firmware`; `upload` needs `firmware` and `publish`, so guard failure blocks both. Branch/PR compilation remains unaffected. |
| 3 | PASS | `tests/release_flags_guard_test.sh:14-87` extracts the real workflow body, runs bare under zsh, invokes available bash, and uses GNU/BSD-compatible commands. All six required cases plus undefined/extra-token cases are meaningful. It passed with Apple clang and GNU arm-none-eabi-g++ 10.2.1. One low-severity placement-test blind spot is below. |
| 4 | PASS | No `src/` or `lib/` diff from `e467364`; binary source is unchanged. `docs/bench-validation.md:86` accurately describes the `v*` tag guard. Scoped diff is +9 workflow, +1 documentation, +87 test. |
| 5 | PASS | Final controller result: **118/118** using `zsh tests/*.sh` and `python3 tests/*.py`. `thermal_coupling_structural_test.py` passed on a clean tracked-file export. An initial 117/118 was caused solely by placing that export under `build-tmp`, which the legacy thermal test recursively scanned; after removing only that audit export, the worktree thermal test and complete rerun passed. |
| 6 | PASS | Exact-token behavior deliberately rejects semantically-zero spellings such as `(0)`, `0u`, and `false`; this matches the “exactly 0” fail-closed requirement and is documented in the Stage 6 report. No current route was found for the real compile to override either macro. |

Findings:

- **LOW — `tests/release_flags_guard_test.sh:24-27`: same-job placement is not pinned.** Moving the intact guard into `host-tests` before its test step survived (`rc=0`). Since `host-tests` and `firmware` run concurrently, compilation could begin before that misplaced guard. The reviewed workflow is correctly placed. Smallest fix: bound extraction/order checks to the `firmware:` job and assert both guard and Compile occur inside it.
- **LOW — `.github/workflows/build.yml:82`: stderr warnings are non-fatal.** A `#warning` mutation survived under both clang and GNU GCC because only stdout supplies the two parsed tokens. This does not permit a nonzero flag and the real header emits no warning. If “any compiler diagnostic” must fail closed, add `-Werror` and a warning fixture to the test.

Independent mutations:

| Mutation | Result |
|---|---|
| Error text no longer names `DEV_BUILD` | Caught |
| `if:` changed to `always()` | Caught |
| Guard reads `FirmwareVersion.h` | Caught |
| Header emits extra source text | Caught |
| `DEV_BUILD` becomes `(0)`, `0u`, or `false` | All caught |
| Header emits `#warning` with flags still `0` | Survived; finding above |
| Guard moved into an earlier `host-tests` step | Survived; finding above |

Not run: a real tag build, the GitHub-hosted Ubuntu runner’s exact `cc`, or the Particle buildpack compile. GNU preprocessing was exercised with arm-none-eabi GCC 9.2.1 and 10.2.1; the full extracted test passed under GCC 10.2.1.

Model/reasoning: **gpt-5.6-sol, high**.  
Interpreters: **zsh 5.9; GNU bash 3.2.57; Python 3.14.6; host cc Apple clang 14.0.3; supplemental GNU GCC 9.2.1/10.2.1**.

Final SHA remained exact and `git status --short` was clean.
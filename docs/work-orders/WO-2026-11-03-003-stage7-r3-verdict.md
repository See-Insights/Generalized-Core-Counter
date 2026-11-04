UNDER REVIEW: wo/2026-11-03-003-release-pipeline @ 971f91d9584d459b014351cea5690143800f99d8

# Verdict: NOT VERIFIED

## Blocking finding

1. **[BLOCKING] The source-version guard can validate a commented-out value while the compiler uses a different active definition.**

   `.github/workflows/build.yml:76` uses:

   ```bash
   sed -n 's/^#define FIRMWARE_PRODUCT_VERSION *//p'
   ```

   This does not understand C comments or preprocessing and ignores valid directives with leading whitespace. Against the exact committed block, this fixture passed for `v48.90`:

   ```cpp
   /*
   #define FIRMWARE_PRODUCT_VERSION 4890
   */
    #define FIRMWARE_PRODUCT_VERSION 1
   ```

   The check exited 0, although the active macro—and therefore `PRODUCT_VERSION(FIRMWARE_PRODUCT_VERSION)` at `src/Generalized-Core-Counter.cpp:43`—is 1. Two canonical definitions fail closed, but a commented/disabled canonical decoy plus a whitespace-prefixed active definition bypasses the guard.

   The ordinary committed form at `src/FirmwareVersion.h:20` works, tag-only execution and ordering are correct (`build.yml:70-81`), and regex validation precedes arithmetic. The historical v48 parity evidence plus the direct macro use establish that the normal source value becomes the embedded value.

   The manual `particle binary inspect` check at `AI_DEVELOPMENT_WORKFLOW.md:518` is an adequate independent pre-pinning check, but it happens after publication and cannot replace Amendment 4’s automatic “no mismatched Release” guarantee.

## Round-2 blockers

| Blocker | Disposition | Evidence |
|---|---|---|
| B-1 — publish token exposure | **Removed by new design** | Publish checkout is gone. The GitHub-owned `download-artifact` at `build.yml:124-127` receives the write-scoped default token; Amendment 4 explicitly accepts this risk. Explicit `GH_TOKEN` occurs only on the guard/Release step at `:128-135`. |
| B-2 — masked Docker digest failure | **Resolved** | Digest is assigned at `:102`, then required nonempty at `:103`. Stubbed Docker failure exited 42; empty output exited 1; neither wrote outputs. |
| B-3 — `release.sh` broken by syntax | **Resolved** | `bump_version.sh:27-35` accepts the new forms; `release.sh:150-172` passes the version through, no longer tags, and prints a canonical annotated-tag instruction. |
| B-4 — `BUMP_VERSION_SOURCE_ONLY` hook | **Resolved** | Hook is absent from production code. With the variable inherited, the scratch mutation run was byte-identical to the normal run; no-argument execution still returned 1 (`tests/bump_version_scheme_test.sh:62-65`). |
| B-5 — documentation inaccuracies | **Resolved** | Correct Development-device procedure at `AI_DEVELOPMENT_WORKFLOW.md:521`; annotated tags at `:515-516`; manual inspect at `:518`; corrected locations at `PROJECT_STATUS.md:234,248`. |
| B-6 — untrusted npm inspector | **Removed by new design** | Inspector job and npm installation are gone. The replacement source guard has the separate blocking defect above. |

## Guard and API results

**[NOTE]** Exact committed blocks were exercised with local stubs:

- Baseline `v48.90`: one Release create call.
- Overflow and `v48.100`: rejected before Release/API access.
- Lightweight tag, tag targeting another commit, changed artifact hash, second `.bin`, and existing Release: all failed with zero create calls.
- A replacement annotated-tag object targeting the same `GITHUB_SHA` passed.
- Docker inspect failure and empty output both failed the firmware job.

At `build.yml:147-150`, the API checks establish that the current remote ref is annotated and that its tag object targets `GITHUB_SHA`. A lightweight tag or a tag already moved to another commit cannot pass. A moved annotated tag targeting the same commit can pass because the event’s original tag-object SHA is not bound. A move after the checks is also a residual race; the separately verified immutable-tag ruleset is the controlling protection.

`--verify-tag` adds only a final remote-existence check—it prevents automatic tag creation but does not verify annotation, signature, target SHA, or immutability. The API `.message` value supplies the complete textual multiline annotation via `--notes-file`; tagger/signature metadata is separate.

The live `v48.90`/`v48.91` runs remain necessary to establish actual GitHub API permissions, runner `gh` behavior, rendered notes/assets, successful publication, and that the mismatch run creates no Release. They were out of scope and not performed.

## Privilege

**[NOTE]**

- Host-test checkouts at `build.yml:18-27`: default read-scoped token.
- Firmware checkout at `:67-69`: default read-scoped token.
- Pinned third-party compile action at `:81-86`: no explicit token input; action code can access only the job’s read-scoped token context.
- Publish download at `:124-127`: write-scoped default token, accepted by the architect.
- Guard/Release step at `:128-154`: the sole explicit `GH_TOKEN`, write-scoped.
- No publish checkout, npm, or third-party publish action exists. No third-party code receives a write-scoped repository token.

## Helpers, documentation, budget, and scope

**[NOTE]**

- `49.1`, `v49.1`, and `v49.1-Name` derive 4901; legacy `v28-Name` remains 28. `49`, `49.100`, `100.1`, empty, and overflow inputs reject.
- Scratch runs confirmed `bump_version.sh:52-53` writes `v49.1` consistently to `FirmwareVersion.h`, `Doxyfile`, and `README.md`.
- Direct and interactive `release` dry runs succeeded. Neither executed tag creation or tag push; `release.sh:169-172` only prints the manual `git tag -a` instruction. No live caller breaks (`release:74` is the only executable caller).
- Stage 9 and `PROJECT_STATUS.md` satisfy the requested documentation corrections.
- Workflow: 154 lines; base: 83; net **+71**, within +85.
- Round-3 commit diff: `src/` empty and `.github/CODEOWNERS` unchanged. Against `origin/main`, `src/FirmwareVersion.h` has only the previously authorized comment change; the constant remains 48. CODEOWNERS is the earlier required one-line addition.

## Local verification

**[NOTE]**

- YAML: PASS — jobs `host-tests`, `firmware`, `publish`.
- Workflow `run:` blocks: `bash -n` **7/7 PASS**.
- Script syntax: PASS.
- Host suite: **108/108 PASS** (`.sh` via zsh, `.py` via python3).
- No network or GitHub write operation was used.
- No unstaged tracked files were changed; the pre-existing staged round-3 dispatch remained untouched.
- Scratch: `build-tmp/wo-2026-11-03-003-stage7-r3/`.

**Model / reasoning:** `gpt-5.6-sol`, high.  
**Interpreter line:** YAML via Ruby 2.6.10 / Psych 3.1.0; shell syntax and stub logic via GNU Bash 3.2.57; host tests via zsh 5.9 and Python 3.14.6; local CLI semantics from `gh` 2.86.0.
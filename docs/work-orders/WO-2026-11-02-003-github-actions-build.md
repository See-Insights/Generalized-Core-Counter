# WO-2026-11-02-003 — Build and test in GitHub Actions

**Status:** APPROVED by the architect; Chip authorizes the dispatch.
**Governing sections:** `AI_DEVELOPMENT_WORKFLOW.md` §3 Stage 6 (implementation) and Stage 7 (independent verification, "Mandatory: local toolchain build"); §5 model routing; §7 access and security controls; §10 (host shell tests run under zsh).

## Goal (plain)
Every push and PR builds the Boron firmware and runs the host tests on GitHub's runners, the same way we build locally, and keeps the `.bin`. This replaces the Particle cloud-compile release gate, which keeps failing. Firmware is untouched.

## Design
- **One workflow file**, `.github/workflows/build.yml`, two jobs:
  1. **Host tests:** run every `tests/*.sh` under **zsh** (§10; install zsh on the runner) and every `tests/*.py` under python3. Report the interpreter, as §10 requires.
  2. **Firmware:** `particle-iot/compile-action`, `particle-platform-name: boron`, `device-os-version` pinned to the version in `project.properties` (6.6.0 on main). **No `particle-access-token`:** the action then compiles on the runner with Particle's buildpack image, not the Particle cloud. `lib/` (including the modified AB1805_RK) is built from the repo, as locally. Upload the `.bin` and ELF as workflow artifacts.
- **Supply chain (public repo):** pin every third-party action by full commit SHA, with the tag in a comment. No secrets; none are needed.
- **No `auto-version`.** GCC declares `PRODUCT_VERSION(FIRMWARE_PRODUCT_VERSION)` through a macro; a known community issue is the action failing to find the version macro when auto-versioning. If the build still trips on it, report and stop; don't change `src/`.

## Acceptance (parity, not just green)
On the same commit, the CI build and the controller's local build (Stage 7 local toolchain build) match on: `.bin` size; `nm` presence of 3 named symbols (the controller picks them, e.g. the `chargeLatch` static from #172 once merged); and the reported `device-os-version` and firmware version. Record any difference and its cause.

## Release-gate change (after acceptance)
Once CI is green and at parity on **3 consecutive PRs**, the cloud-compile gate (#156) is replaced by "CI green on the release branch head, plus the local build". Record that in the release WO template. The local toolchain build stays mandatory (§3 Stage 7).

## Budget
- `src/`: **0 lines.** Any `src/` change is out of scope.
- Workflow: **≤ 80 lines.**

## Agents
- **Stage 6 — Copilot:** standard model, medium reasoning.
- **Stage 7 — Codex:** standard model (`gpt-5.6-sol`), high reasoning (not safety-relevant firmware logic), stdin closed, new output file. At most two rounds, then an architect ruling.

## Out of scope
Release automation (flashing, tagging, product uploads); the Particle cloud compiler; any `src/` or test change.

## Amendment 1 (2026-11-02, after review of Particle's GitHub Actions guide)
Source: https://docs.particle.io/firmware/best-practices/github-actions/
1. **Action versions:** the guide uses `compile-action@v1`; we keep the SHA pin (public repo) with `# v1.x.y` in a comment. Its examples use `actions/checkout@v3` and `actions/upload-artifact@v3`, which are out of date; use the current major versions (`@v4` or later), also SHA-pinned.
2. **Local vs cloud:** the guide's tradeoff (the buildpack adds ~30–60 s of setup; a token switches to the cloud compiler) doesn't change our choice. We build on the runner, with no token.
3. **Before dispatch:** confirm GitHub Actions is enabled for the See-Insights organization (the guide notes organizations can disable it).
4. **Phase 2, not in this WO:** `flash-device-action` to Dev-14 / Dev-09 for bench runs (needs a `devices:update` API-user token and development-marked devices), and `firmware-upload-action` to product 42131 on a release tag (needs `firmware:create`). Both need Chip's decision; tokens only in GitHub secrets, minimal scope, never in the repo.

## Amendment 2 (2026-11-02) — least privilege for any Particle token given to GitHub (Chip's direction)
Sources: https://docs.particle.io/getting-started/cloud/cloud-api/#api-users ; https://docs.particle.io/firmware/best-practices/github-actions/ ; https://github.com/particle-iot/firmware-upload-action
1. **Phase 1 (this WO): no Particle token at all.** Compiling on the runner needs none.
2. **Phase 2, if Chip approves it:**
   - **Only product API users, never a human user token.** One API user per function, scoped to product 42131 only, each with a single scope:
     - upload: `firmware:create` only. `firmware-upload-action` uploads only; it does not release or trigger OTA. **CI never gets the right to release or set a default**: Chip does that in the console.
     - bench flashing: `devices:update` only, and only if an API user works for it. The Particle guide says `flash-device-action` needs a user-level token with the device claimed to the account; the upload action's README says an API-user token with `devices:update` can flash test devices. Resolve that conflict before phase 2. **If only a user token works, CI doesn't flash.**
   - **Storage:** GitHub Actions secrets only, attached to a GitHub Environment that requires Chip's approval, and used only by tag-triggered jobs on this repo. Never available to pull-request jobs (forks). Never in the repo, logs or command-line arguments.
   - **Lifetime:** API-user tokens don't expire, so rotate them on a schedule (proposed: quarterly) and immediately on any suspected exposure. Delete an API user when its job is retired.
3. Record each API user (name, product, scope, created, last rotated) in `docs/notes/`, without the token.

## Amendment 3 (architect ruling, 3 Nov, after the first CI run: host-tests 100/107)
1. `fetch-depth: 0` on the host-tests checkout only (the device-os sparse checkout stays shallow); workflow limit raised to **≤ 85 lines** (now 81).
2. Test portability fixes are in scope (`src/` stays 0): `#include <cmath>` in `tests/report_payload_golden_test.cpp` (`NAN`/`INFINITY` are not declared transitively by libstdc++), and the portable epoch in `tests/rtc_skew_test.sh`.
3. `rtc_skew_test.sh` diagnosis: line 295 used BSD `date -j -f`; GNU `date` has no `-j` (`date: invalid option -- 'j'`). Environmental, not a time-zone-data issue (`TZ=UTC` is set per call). Fix: try the BSD form, fall back to GNU `date -d`; the BSD anchors for 2024/2028/2034 equal the independently computed epochs. Never skipped.
4. The five sleep equivalence tests failed (not skipped) on the depth-1 clone: reproduced locally with `git clone --depth 1` (the baseline-region extraction yields an empty include and clang errors); with a full clone all six pass. 107 vs 108: this branch has 61 sh + 46 py = 107 tests; the 108 in WO -004 is the same suite plus its new `default_timezone_test.sh` (62 sh).
Verification unchanged: first green CI run at parity with the local build.

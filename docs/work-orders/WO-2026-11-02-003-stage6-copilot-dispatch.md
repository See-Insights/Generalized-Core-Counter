AGENT: Copilot · MODEL: claude-sonnet-5.5 · REASONING: medium
AUTHORIZATION SCOPE: create `.github/workflows/build.yml`; run local verification (actionlint if installed, `zsh tests/*.sh`, `python3 tests/*.py`); scratch directory `build-tmp/wo-2026-11-02-003-stage6/` / NOT authorized: any change under `src/` or `tests/`, committing, pushing, merging, `gh` write operations, adding secrets or tokens, deleting anything you didn't create (including `build-tmp/` itself).

# Stage 6 dispatch — WO-2026-11-02-003, build and test in GitHub Actions

**Binding spec:** `docs/work-orders/WO-2026-11-02-003-github-actions-build.md` (Design, Amendments 1 and 2). Branch `wo/2026-11-02-003-github-actions-build` at `70fc0a0` (main `21804e3` has the v48 firmware). Leave all changes uncommitted.

## Do
Create `.github/workflows/build.yml`, **at most 80 lines**, two jobs, triggers `push` and `pull_request`, `permissions: contents: read`, no secrets, no Particle token.

1. **host-tests** (ubuntu-latest): install zsh (`sudo apt-get install -y zsh`); run every `tests/*.sh` with `zsh` and every `tests/*.py` with `python3`; fail the job if any fails; print a summary line in the form `N/N (sh via zsh, py via python3)` (§10). Look at how the existing tests are invoked locally (e.g. any runner script or comment in `tests/`) and match it; do not edit tests.
2. **firmware** (ubuntu-latest): `actions/checkout`, then `particle-iot/compile-action` with `particle-platform-name: boron`, `device-os-version` equal to `deviceOS=` in `project.properties` (6.6.0 today; read it from the file in a prior step if that stays under budget, else hard-code 6.6.0 with a comment pointing at `project.properties`), **no `particle-access-token`, no `auto-version`**. Echo the action's `device-os-version` and `firmware-version` outputs to the job summary. Upload the `.bin` and the ELF (from the `target-path` output) with `actions/upload-artifact`.
3. **Pin every third-party action by full commit SHA with the tag in a comment.** SHAs resolved by the controller on 3 Nov (re-verify with `gh api repos/<owner>/<repo>/git/ref/tags/<tag>` if you have network; if an annotated tag, dereference to the commit):
   - `actions/checkout` v4.2.2 → `11bd71901bbe5b1630ceea73d27597364c9af683`
   - `actions/upload-artifact` v4.6.2 → `ea165f8d65b6e75b540449e92b4886f43607fa02`
   - `particle-iot/compile-action` v1.0.7 → `3b6e15e99da59d074120f854a71659d4c81f092c`
   (Amendment 1 says v4 or later for checkout/upload-artifact; v4.x is acceptable. Do not use any tag-only reference.)

## Verify
- `actionlint .github/workflows/build.yml` if available; otherwise a YAML parse (`python3 -c 'import yaml,sys; yaml.safe_load(open(...))'`) and a manual check of the keys.
- `wc -l` of the workflow ≤ 80; `git diff --stat` shows only the new workflow (and no `src/`).
- Run the host tests locally exactly as the job does (zsh for `.sh`, python3 for `.py`) and report counts.
- You cannot run the workflow; say so. The controller records parity (bin size, `nm`, Device OS and firmware version) after the first CI run.

## Stop and report (do not improvise) if
the compile action cannot work without a token, needs a `src/` change, or the workflow cannot fit in 80 lines.

## Report (write to `docs/work-orders/WO-2026-11-02-003-stage6-copilot-report.md`)
Model and reasoning actually used; interpreter line; files changed; line counts; the SHAs as pinned; anything unverified.

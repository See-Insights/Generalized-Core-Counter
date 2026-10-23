# VERIFIED WITH CONCERNS

**Reviewer:** Codex · **Model:** `gpt-5.6-sol` (probed 2026-10-20, §5) · **Reasoning:** high

**Test interpreters:** shell tests via `zsh`; bare Python tests via `python3`; all runs serial, one test at a time.

Review target: follow-up commit `8de7ddc0bad51c913577382e10b6e6050c939efe` on the reviewed `658b7cf`. The current branch tip adds documentation only after that target.

| Check | Result | Evidence |
|---|---|---|
| 1. `src/` unchanged | **PASS** | `git diff 658b7cf 8de7ddc --stat -- src` is empty. Both commits have Git `src` tree `fa31b3e26d5145d2fcb2d599f89e810af4be1a55` and path+content SHA-256 `7fc57df7a302712ea3f26ff37758ce455aaa6203808a7092d11636f5d06d190a`. Checks 1–7 and 9 therefore stand, including net `src/` `+368−348 = +20`, the hard ceiling; no ARM build was run. |
| 2. F1 closed | **PASS** | Guard below `SleepTeardown::run()` made the full suite **91/93**: dwell aborted on `state == FIRMWARE_UPDATE_STATE` (generated line 296), and wiring reported `the sleep-gate firmware-update check comes AFTER SleepTeardown::run( (or a direct teardown request) ...`. Guard just above the later direct requests failed both tests identically. Own mutation inside the non-READY result gate also failed both. Own mutation before `SleepTeardown::enter()` made the full suite **92/93**; `sleep_teardown_structural_test.py` reported `the lost-timestamp guard must run before the first disconnectRequested() read`. |
| 3. Re-edited tests / check 8 | **PASS** | Against `658b7cf`, the structural test retains every registration, record-only-handler, retired-state, guard transition/return, dwell-exit and ThrashGuard assertion; it only adds `SleepTeardown::run(` to the earliest teardown boundary (`firmware_update_wiring_structural_test.py:143-158`). The dwell runtime assertions are byte-identical at `7176048`, `658b7cf` and `8de7ddc` (tail SHA-256 `8d89d1f…`); only the wrapper now emits guard/gate/teardown in derived source order (`firmware_update_dwell_test.sh:38-50,164-197`). Missing any token fails explicitly. Neither test is weaker than the `7176048` pre-move version for the property. See C1. |
| 4. Suite and golden fixture | **PASS** | Clean detached target: **93/93** serial (**54 shell + 39 Python**). Both firmware tests and the golden test pass. `report_payload_golden.txt` is unchanged at `7176048`, `658b7cf` and `8de7ddc`, SHA-256 `07de1e1980e4a44e03420540e6c721a93a61b5d6392539219928420c048f0637`. |

## Findings

### C1 — Low — dwell order derivation does not fully exclude comments

- **Observation:** `firmware_update_dwell_test.sh:40-47` skips only full-line `//` comments. A missing guard, gate or `run(` produces the explicit three-token failure, and a full-line `//` occurrence is ignored; a block-comment or trailing-comment occurrence can satisfy the locator. Replacing the real run call with `/* SleepTeardown::run( removed mutation */` made the dwell test pass. The comment-stripping structural test rejected the same mutation with `handleSleepingState() has no SleepTeardown::run( call to order the check against`.
- **Inference:** the dwell test alone can give false assurance on comment-spoofed text, but the serial suite's structural complement closes the reproduced escape. This is a robustness concern, not a remaining F1 suite gap.

## Confirmations

- `8de7ddc` itself changes only `tests/firmware_update_dwell_test.sh` and `tests/firmware_update_wiring_structural_test.py` (`+47/−17`); the authorized verdict is the only main-worktree change.
- Mutations and generated test files stayed in detached worktrees from a local bare clone under `build-tmp/wo20261020-003-recheck/`; that review-created scratch was removed.
- No commit, push, stash, reset, checkout/restore, device, network or AWS access occurred; no ARM build was attempted.

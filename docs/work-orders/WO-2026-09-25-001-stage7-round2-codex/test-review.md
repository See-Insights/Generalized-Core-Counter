# Stage 7 round 2: host suite, mutations, and test hygiene

Agent/model: Codex / gpt-6-astra; reasoning ultra (inherited from parent).

## Result

The candidate passes **49/49 existing script tests: 25/25 shell via zsh; 24/24 Python via python3**. No baseline test was skipped. All four required mutations make at least one existing test fail. P2 (mutation iv) from round 1 is resolved. A P3 cleanup concern remains in the three publish test wrappers: each leaves an empty build-tmp directory.

## Execution and integrity

`run-review.py` copied real candidate source, libraries, tests, and referenced documents byte-for-byte into `/private/tmp/codex-wo-2026-09-25-001-stage7-round2-FbwtgJ/tests/candidate`. The snapshot included untracked source/tests. All 1,244 copied files matched original SHA-256 before execution. Two pre-existing tests use read-only Git queries; `GIT_DIR` referenced the original repository, `GIT_WORK_TREE` referenced the snapshot, and `GIT_OPTIONAL_LOCKS=0` prevented optional lock writes. Each `.sh` file ran with `zsh`; each `.py` with `python3`. Each full run had its own external TMPDIR.

Each mutation changed exactly one real candidate file in that snapshot. A `try/finally` rewrote the original bytes after each run; byte comparison, SHA-256 comparison, and a full snapshot hash audit passed after **every** mutation. The original repository was never mutated by this task, and final SHA-256 matched for every copied input. Mutations (i), (ii), and (iv) reproduce the round-1 patch semantics; (iii) was adapted to the round-3 `queuePermitsSleep` rename. All four adapted patches are retained with the logs.

The temporary candidate copy and every generated host binary, harness, TMPDIR, and build-tmp directory have been removed. Only requested evidence logs, patches, manifests, reports, and the reproducible runner remain outside the repository.

## Mutation table

Counts below are passing scripts/total scripts; nonzero exits are the expected mutation detections.

| Mutation | Full suite | sh via zsh | py via python3 | Failing test(s) | Restored byte-identically |
|---|---:|---:|---:|---|---|
| i-drop-with-ack | 48/49 | 24/25 | 24/24 | `publish_with_ack_queue_test.sh` | Yes |
| ii-remove-before-ack | 48/49 | 24/25 | 24/24 | `publish_with_ack_queue_test.sh` | Yes |
| iii-sleep-with-pending | 47/49 | 24/25 | 23/24 | `publish_ack_sleep_gate_structural_test.py`<br>`publish_delivery_budget_test.sh` | Yes |
| iv-remove-on-failure | 48/49 | 24/25 | 24/24 | `publish_with_ack_queue_test.sh` | Yes |

- **(i), drop WITH_ACK:** `FIDELITY CHECK FAILED: dispatch normalizes queued flags to explicit WITH_ACK with NO_ACK cleared`. The assertion is [tests/publish_with_ack_queue_test.sh:51](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/tests/publish_with_ack_queue_test.sh:51).
- **(ii), remove before ACK:** `FIDELITY CHECK FAILED: stateWait() has 2 removeFileNum() call(s); only the corrupted-file discard is allowed`. The assertion is [tests/publish_with_ack_queue_test.sh:118](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/tests/publish_with_ack_queue_test.sh:118).
- **(iii), remove the queue conjunct:** Python reports `the delivery gate's verdict must be one of the conjuncts that release the sleep gate`; zsh reports `the cloud-sync gate's release condition includes the queue term`. Assertions are [tests/publish_ack_sleep_gate_structural_test.py:92](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/tests/publish_ack_sleep_gate_structural_test.py:92) and [tests/publish_delivery_budget_test.sh:128](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/tests/publish_delivery_budget_test.sh:128).
- **(iv), discard the event on Future failure/ACK timeout:** `FIDELITY CHECK FAILED: the failure branch of statePublishWait() removes the queue file - a failed or unacknowledged publish must leave the event queued (offending line: fileQueue.removeFileNum(failedFileNum, false);)`. Failure-branch checks at [tests/publish_with_ack_queue_test.sh:81](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/tests/publish_with_ack_queue_test.sh:81) and assertion at [tests/publish_with_ack_queue_test.sh:98](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/tests/publish_with_ack_queue_test.sh:98) catch the adapted round-1 patch.

These detections come from checks against the production source. The queue C++ test is a host mirror anchored by those fidelity checks; the budget and counter C++ tests compile their production modules. This suite result does not substitute for the separate full-control-flow host reproductions and binary review.

Restored SHA-256:

- `lib/PublishQueuePosixRK/src/PublishQueuePosixRK.cpp`: `8e1d1a619ffabc55c3fa021fe85e10127501250988d220d9bca0701604940f7a` (mutations i, ii, iv).
- `src/state/State_Sleep.cpp`: `ba1093a9b15ea9651a85329542f783663d8d042c45336048cc767b7988aa19c3` (mutation iii).

## P3: three publish test scripts leave empty build-tmp

Independent runs began without build-tmp. Each of the following returned zero, removed its test binary, and left **an empty build-tmp directory**:

| Test | Creation | Cleanup | Observed final directory |
|---|---|---|---|
| publish_with_ack_queue_test.sh | line14 mkdir | line15 removes only binary | Exists, empty |
| publish_delivery_counters_test.sh | line17 mkdir | line18 removes only binary | Exists, empty |
| publish_delivery_budget_test.sh | line17 mkdir | line18 removes only binary | Exists, empty |

Evidence: [tests/publish_with_ack_queue_test.sh:14](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/tests/publish_with_ack_queue_test.sh:14), [tests/publish_delivery_counters_test.sh:17](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/tests/publish_delivery_counters_test.sh:17), [tests/publish_delivery_budget_test.sh:17](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/tests/publish_delivery_budget_test.sh:17). The complete alphabetically ordered baseline suite first creates the directory in `publish_delivery_budget_test.sh`; later tests reuse it. There is no directory cleanup in any of these three EXIT traps, so the same omission also applies after assertion failures. In contrast, the control `rtc_skew_test.sh` remembers whether it created the directory and uses `rmdir` on exit ([tests/rtc_skew_test.sh:68](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/tests/rtc_skew_test.sh:68) through line86); it independently returned zero and left no directory. If the directory already exists from another script, it correctly preserves it.

The workflow requires cleanup on completion, including failure paths ([AI_DEVELOPMENT_WORKFLOW.md:114](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/AI_DEVELOPMENT_WORKFLOW.md:114)), so this is a reproducible cleanup defect. Recommended eventual implementation: track whether each script created the directory, remove its binary in an EXIT/INT/TERM cleanup handler, and `rmdir` only a directory it created. No source or test fix was made during this review.

`hygiene-results.json` records the separate before/after runs. `baseline-artifact-audit.json` records the suite's final artifact state before the review harness cleaned it. Older tests' TMPDIR leftovers are filed separately in `out-of-scope-test-hygiene.md` and do not block this WO review.

## Evidence index

- `baseline-suite.log`, `baseline-results.json`: every command, output, exit status, interpreter, and timing.
- `*-suite.log`, `*-results.json`, `*.patch`: each mutation's complete 49-test run and exact patch.
- `mutation-results.json`: compact mutation matrix, hashes, byte-identical restoration.
- `hygiene-results.json`, `*-hygiene.log`: independent directory cleanup reproduction.
- `snapshot-sha256.json`, `final-restore-audit.json`: initial/final integrity.

# VERIFIED WITH CONCERNS

**Reviewer:** Codex · **Model:** `gpt-5.6-sol` (standard tier) · **Reasoning:** high

**Test interpreters:** shell tests via `zsh`; bare Python tests via `python3`; all suites run serially, one suite at a time.

Review target: the uncommitted working-tree change on `wo/2026-10-19-004-step6-p5-modem-health`, at `HEAD` `4234c08`, against the specified behavior base `db9e9e1`. The direct `git worktree add` against the review repository was blocked because its real Git metadata is outside the writable sandbox. I therefore made a local bare clone under the authorized Stage 7 scratch directory and created detached `db9e9e1` worktrees from that clone. The equivalence base object remained present, and no review-worktree Git metadata was changed.

| Check | Result | Evidence |
|---|---|---|
| 1. Byte-identical moves | **PASS** | The two thresholds (`10000UL`, `5000UL`), enum values, `markModemUnstable` and `maybeClearModemUnstable` match the extracted `db9e9e1` text byte-for-byte. The clean equivalence run's copy guard passed; a change to the recovery comparison failed as `COPY_MISMATCH`. There are 56 gross moved lines including separators (54 substantive lines). Differences are limited to linkage/declarations/includes and the specified callers. |
| 2. Three new functions | **PASS** | Manual line-by-line reading found no behavioral or field-write-order difference. Timeout order remains: saturating increment; `>= 2`; compute `reasonChanged`; write unstable then reason; conditional health log using the incremented count; conditional policy log then suppression write. Success still conditionally writes only count zero. Daily close still gates only on `modemUnstable`, writes unstable, suppression, reason and count in that order, then emits its one policy log; it has no result/time dependency and emits neither recovery log. |
| 3. Logs and conditions | **PASS** | All six required strings are byte-identical and the 176-case transcript comparison includes level and order. Conditions remain reason change (two unstable logs), first suppression (temporary-disable), unstable + successful connect + teardown `<= 5000` (recovered), suppressed during recovery (restored), and unstable at daily close (re-allowed). The only other latch-dependent log sites are the unchanged reconnect-deferred lines in `State_Report.cpp:242` and `:254`; no extra latch writer or dependent log exists. |
| 4. Equivalence harness | **CONCERN** | It is real: `tests/modem_health_equivalence_test.sh` reads old Sleep/Connect/Report text with `git show db9e9e1`, compiles old extracts and the real new owner separately, and byte-diffs state/log transcripts. Its 176 cases are 30 timeout, 6 success, 24 daily-close, 36 mark, and 80 maybe-clear cases. These include counts 0/1/2/3/255, all requested prior reasons and suppression states, success counts 0/1/255, daily set/unset states, all mark reason/reasonChanged/suppression branches, four connect results, and teardown 4999/5000/5001/10000/10001 ms. The mark cases verify elapsed passthrough at 12345; the slow threshold value is additionally pinned byte-for-byte and structurally. Two independent wrapper mutations failed with transcript diffs: success clearing only count 255, and timeout triggering only above the threshold. The concern is repository-history dependence; see F1. |
| 5. One owner | **PASS** | Bare-name source scan and the structural test find all four field writes only in `src/power/ModemHealth.cpp:11-12,30,45-47,50,57,67,73-76`. Reads remain in Connect (`State_Connect.cpp:423`) and Report (`State_Report.cpp:200`), plus the existing Sleep policy reads. `session` remains declared in `StateMachine.h:61-64`. An injected Connect writer failed with its file and offending line named. |
| 6. Linkage and symbols | **PASS** | Fresh Device OS 6.4.1 Boron final link succeeded. Repository plus Device OS/library search found no name collision; each external function has one definition. Header `#pragma once`, the enum definition, and the namespace-scope `constexpr` slow threshold are ODR-safe; the other thresholds remain translation-unit-local constants. `MODEM_UNSTABLE_SLOW_TEARDOWN_MS` is defined in the header and used only by Sleep. All five new external functions are present in the ELF. |
| 7. Text delta | **PASS** | Fresh deterministic builds are `151764/1094/2228 -> 151748/1094/2228` (`text/data/bss`), exactly -16 text. New owner symbols total +248 B: clear 36, mark 92, success 16, timeout 32, maybe-clear 72. Removed/shrunk symbols total -260 B: old anonymous mark 72, Sleep handler 7048->6968 (-80), Connect 2412->2320 (-92), Report 992->976 (-16). Symbol subtotal is -12 B; section fill/alignment supplies the remaining -4 B. Compiler-local `CSWTCH` names renumber with equal sizes. Unlike the supplied run, this build has identical `__tcf_*` and `Buffer::operator=` size multisets; this does not alter the complete accounting. Static-initialization stubs are the same 22 before and after, with no new stub. |
| 8. Retained layout | **PASS** | The complete `.backup` section is `0x98` bytes and its address/size/symbol table is identical before and after (including every object, mapping entry and linker boundary). `retainedBoot` is `0x2003f420`; hibernate fields are `0x2003f480/488/48c/490`; signature is `0x2003f494`; end is `0x2003f498`. Neither owner file contains `retained`. Separate probes fail as `ModemHealth.cpp must not mention retained` and `ModemHealth.h must not mention retained`. |
| 9. Tests and budget | **CONCERN** | Clean baseline 87/87 and candidate 89/89, serially. Unchanged `conn_phase_modem_off_test.sh` passes. The re-pointed daily-clear test retains its six old mutants and adds the dropped-owner-call mutant; all seven are caught. The unchanged golden fixture hashes `07de1e1980e4a44e03420540e6c721a93a61b5d6392539219928420c048f0637` and its test passes. Tracked `git diff --check` and explicit no-index checks for every untracked file are clean. Source budget passes; test volume exceeds the guideline as required to be flagged, see F2. |

**Retained outcome: A**

## Equivalence reading

`noteConnectTimeout()` is equivalent to old `markModemUnstableFromConnectTimeout()`: both saturate at 255, test the post-increment count at `>= 2`, write the same fields in the same order, define reason change against numeric reason 2, emit health before policy, and interpolate the current post-increment count. The only source-shape difference is the specified external call to the common mark routine.

`noteConnectSuccess()` reproduces the old `if (count != 0) count = 0` block exactly. `clearAtDailyClose()` reproduces the Report block exactly, including field-write order and log-after-writes order. It is not recovery: it neither checks connect success nor teardown time, and it emits neither `MODEM_HEALTH: recovered` nor `MODEM_POLICY: standby restored`.

No field-write-order difference, log-order difference, or other behavioral difference was found.

## ELF and retained details

The full retained comparison includes `retainedLoopForensics` `0x2003f400`; `retainedBoot` `0x2003f420`; thrash fields `0x2003f440/442/444`; `ledOffTime` `0x2003f448`; trend fields `0x2003f450/454/458`; PMIC fields from `0x2003f460` through `0x2003f47a`; the four hibernate fields; and the signature. All sizes, addresses, local/global bindings, section/mapping entries, start/end aliases and section bytes are unchanged.

## Findings

### F1 - Low - history-dependent equivalence fixture

- **Observation:** `tests/modem_health_equivalence_test.sh:15,20-22` defaults the base to `db9e9e1` and obtains all old sources with `git show`. The comparison is strong when that object exists, as it did in this review.
- **Inference:** a shallow clone that omits `db9e9e1`, or an exported source tree with no Git database, fails before testing behavior. A frozen fixture is more portable. It would add roughly 87 extracted legacy source lines plus provenance/hash validation, and would create a second copy that must be protected from accidental refresh or drift. A pragmatic design is a checked-in frozen fixture with its `db9e9e1` hash, optionally cross-checked against Git when the object is available.

### F2 - Advisory - test volume above guideline

- **Observation:** tests add 387 lines and remove 27, net +360, versus the +175 guideline. Most of the excess is the 346-line new equivalence/structural harness and stubs; the re-pointed existing test is +41/-27.
- **Inference:** this raises maintenance cost, but the tests exercise previously unpinned connect-timeout and 5000 ms recovery behavior and have demonstrated mutation sensitivity. The work order explicitly makes this a flag, not a failure.

## Budget versus actual

| Budget | Actual | Result |
|---|---|---|
| `src/` non-move net <= +20 hard; tests +175 guideline | `src/`: +101/-89 overall, with +56/-56 exact moves excluded => +45/-33 non-move, **net +12**. State files: Sleep 1725->1670, Connect 754->731, Report 333->328. Tests: +387/-27, **net +360**. | Source **PASS** by 8 lines; tests **CONCERN**, 185 lines over guideline. |

## Confirmations

- The review worktree's pre-existing source/test bytes match the start-of-review checksum; its status is unchanged except for this authorized verdict file.
- All mutations, builds, logs, generated extracts and detached worktrees were confined to `build-tmp/wo20261019-004-stage7/`. Only that Stage 7 scratch directory, created by this review, was removed. No pre-existing scratch material was removed.
- No commit, push, stash, reset, checkout/restore, device access, network access or AWS access was performed.

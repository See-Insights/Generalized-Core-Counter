# WO-2026-10-21-002: Sleep S-3: extract the cloud-operations gate (behavior-neutral, option D)

**Author:** architect, 2026-10-21 · **Status:** ready (Step 0 first)
**Branch:** `wo/2026-10-21-002-sleep-s3-cloud-gate`, cut from the S-2 branch tip `8d9695e` (PR #130), which stacks it on S-1 and S-2. The PR's base is `wo/2026-10-21-001-sleep-s2-execution`, retargeted as the earlier PRs merge.
**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §3 Stages 6–8; §12.2–12.4; §13 (b), (c), (g), (j)
**Spec source:** `WO-2026-10-20-002-step0-addendum-report.md` (round 2) Q9 row 2 and sequence item 3; WO-2026-10-20-002 rulings 1–8; WO-2026-10-21-001's option D ruling, which applies to S-3 to S-6. **This WO cites `src/state/State_Sleep.cpp` at `8d9695e` (1,348 lines).** Re-check every range.

## Plain goal

Move the cloud-operations gate (the bounded wait for the publish queue, ledgers, OTA check and webhook before disconnecting) out of `handleSleepingState()` into one named function in `State_Sleep.cpp`, with its state in one file-level struct. **No behavior change:** the same waits, budgets, alerts 43, 44, 42 and 40, log lines and resets, pass by pass.

## What changes (at `8d9695e`; re-check every range)

- **The gate block:** `:596–812`. It covers the queue-aware budget, the blocker mask, the progress and blocked logs, the timeout alerts and resets, the release log and `markServiceEnd`, and the not-connected reset.
- **The gate state, ten function statics:**
  - `operationsCompleteLogged`, `gateWasBlocking`, `lastGateStatusLogMs`, `lastGateBlockerMask`, `lastGateBlockerBeforeRelease` and `gateReleaseReasonKnown` (`:483–488`);
  - `cloudSyncStartMs` (`:513`);
  - `cloudSyncBudgetMs`, `cloudSyncMaxQueueDepth` and `lastQueueDepth` (`:544–546`).
- **Their resets stay where they are, unchanged:**
  - on entry, `:493–498` and `:561–563`;
  - on the CONNECTED+OPEN abort, `:525`;
  - on the firmware and occupancy priority exits, `:580` and `:591`.

  They change only to name the struct field.
- **No move:** the helpers `computeCloudSyncTimeoutMs`, `gateBlockerReasonLabel` and the `kGate*` constants (`:51–119`) are already file-local.

## Design (option D; do not widen)

1. One file-local function in the anonymous namespace, for example `cloudGatePass(bool useNetworkStandbyEffective)`, replaces `:596–812` at the same point.
2. **The ten statics become one file-static struct.** Only the gate function and the reset sites above write it, and a structural test checks that.
3. **Nothing else changes:** no new file or header, and no change to `SleepTeardown`, the S-2 functions or the order of the priority checks.

## Step 0 (Claude Code, read-only, §13 (c))

- **Re-check every range;** mark each holds, moved or wrong. Name every reader and writer of the ten fields by bare name.
- **Size:** prototype the change and count it the way the S-2 prototype was counted. **≤ +20:** dispatch Stage 6 without coming back to the architect. **Over +20:** stop and report.
- **Fixture growth:** state the expected growth (WO-2026-10-21-001 C1 ruling).
- **Ruling 8:** list every test anchored on text or line order in the changed code, run the suite on the prototype, and re-anchor whatever fails. Known to read gate text: the firmware-update ordering and dwell tests, and `sleep_breadcrumb_sequence_test.py`. If `firmware_update_dwell_test.sh` is touched, it skips block and trailing comments (S-1 C1).
- **Report:** at most 150 lines, committed on the WO branch.

## Equivalence (freeze the output, not the code)

- **Extend the shared harness** (ruling 7). Record `tests/fixtures/sleep_cloud_gate_transcript.txt` once from `8d9695e`, with that hash in its header. The harness extracts the new function verbatim by marker (`COPY_MISMATCH`).
- **Scenarios** (cover the edges, not a full cross-product):
  - each blocker alone and in combination (queue, ledger, webhook, update);
  - elapsed just under, at and over the budget;
  - the queue-depth growth that raises the budget;
  - the ledger-timeout floor;
  - release after blocking, and release with no block;
  - not connected;
  - the firmware and occupancy exits mid-gate.
- **Freeze, per pass:**
  - the alerts;
  - the log lines, character for character;
  - the ten fields after the pass;
  - `markServiceEnd`;
  - the transitions;
  - the `Particle.process` and watchdog calls.

## Budget

- **`src/`: at most +20 net non-move lines, hard.** If Stage 6 goes over, stop and report; don't compress (§12.3).
- **Tests:** guideline +100 (this extends the harness). Record the actual and flag it at about twice the guideline.

## Tests and checks

- The transcript matches exactly, the golden payload test is unchanged, and the suite reports `N/N`, run serially.
- Re-anchored tests each fail their mutation.
- **Mutations.** Each must fail the transcript or the named test aimed at it:
  1. Raise alert 44 before 43.
  2. Use the budget at the elapsed-equals-budget edge, off by one.
  3. Skip the `lastQueueDepth` progress update.
  4. Drop the `cloudSyncStartMs` reset on the occupancy exit.
- **Structural:** the ten fields are written only in the gate function and at the listed reset sites. No `retained` keyword is added.
- **`nm` retained check:** expect `.backup` unchanged. ELF delta explained by symbol.

## Agents and models

- **Step 0:** Claude Code (controller), read-only, in its own session.
- **Stage 6:** Copilot, Sonnet tier, **medium**. It's a single contiguous block becoming one function with one struct, and the harness pattern now exists twice.
- **Controller, before Stage 7 (§13 (g)):** regenerate the transcript and confirm it is byte-identical; reproduce the build, `nm`, the mutations and the suite, serially.
- **Stage 7:** Codex `gpt-5.6-sol`, **high**. It checks:
  - provenance and edge coverage;
  - the moved block is byte-identical apart from field access;
  - one owner;
  - the reset sites are unchanged;
  - the ruling-8 list is complete.

  Argon is not built: support is being dropped, per the recovery-plan item.
- **Stage 8:** pre-approved on VERIFIED, or on VERIFIED WITH CONCERNS when every concern is Low and already closed by another test, with net `src/` ≤ +20. Any High or Medium finding comes to the architect. At most two rounds.
- **PR:** marked **"do not merge until v45 is cut and #128 and #130 are merged"**.

## For the simplify series (record, don't do)

`gateBlockerReasonLabel()` is a 16-case switch spelling out every combination of four flags (`:81–119`, about 39 lines). Building the label from the four flags would be a candidate for the necessity review. It changes log text, so it needs its own WO.

## Closing record

- Budget versus actual.
- The transcript hash and its provenance.
- ELF by symbol.
- The retained outcome.
- The ruling-8 table.

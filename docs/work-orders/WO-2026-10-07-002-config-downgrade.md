# WO-2026-10-07-002: Step 6 WO 1a, stop the config/downgrade overwrite

**Base:** main 41ad238. The branch `wo/2026-10-07-002-config-downgrade` was cut from `73f752e` (PR #71, docs only; `src/`, `lib/` and `tests/` are identical to 41ad238).
**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §3 Stages 5–8. Role restrictions are in §2; the size and goal rules are in §12.2–§12.4.
**Recorded by:** Claude Code, verbatim from the architect's WO and its amendments (2026-10-07).

## Plain goal

The configured connection mode is never changed by a downgrade. The device works out the mode it actually uses from the configured mode plus any active downgrade, so when the downgrade ends, it goes back to exactly what the ledger says.

## Size budget

+15 to +35 net src/ lines, with moved lines counted separately using `git diff --color-moved`. Going over +35 means stop and report. No compressed code to fit the budget (§12.3).

## Agents and models

- **Step 0, citation check:** Claude Code, in its own session. This is mechanical checking against source, so Sonnet tier at default reasoning is enough.
- **Implementation (Stage 6):** Copilot. *Amended:* Sonnet tier, **high** reasoning, because classifying each reader is judgement, not mechanics. If Copilot can't be set to high, Claude Code does the reader classification itself, in its own session, and hands Copilot the table.
- **Verification (Stage 7):** Codex, high reasoning. Overwrite bugs hide in ordering (config apply versus downgrade versus boot), and that's exactly what an independent check should look for.

Claude Code resolves each model ID from its CLI at dispatch time (§5).

## Step 0 (done)

See `docs/work-orders/WO-2026-10-07-002-step0-report.md`:
- **Citations:** 0 wrong; Main :1583 → :1585.
- **History:** there was never a split between the configured and effective modes.
- **Estimate:** +8 to +20.
- **Verdict:** STOP on the owner rule, then the architect's ruling of PROCEED (§7).
- **Migration check:** passes (§8).

## Architect's ruling and amendments (2026-10-07)

**1. Do deletions count against the one-owner rule? No: PROCEED.** The rule is about where the logic lives. After the fix, the mode in use is worked out in one place. Removing the two battery writes and fixing the comparison in config apply retires the overwrite, which is the plain goal itself.

**2. Which owner? PowerManager, chosen deliberately.** The budget is net src/ lines. Pointing a reader at a different getter is a one-for-one replacement, so it adds nothing net: the diff will be wide, but the net budget still holds. Putting the derivation in the persistence accessor would make storage read battery state. That's the crossed ownership Step 6 exists to remove, and WO 3 would only have to move it again.

To make the wide change safe, rename the raw getter. `SystemConfig::get_connectionMode()` becomes `get_configuredConnectionMode()`, and PowerManager gains a getter for the mode in use. The compiler then fails at every one of the ~30 readers, so none can be missed, and each one is deliberately pointed at one of the two getters.

**Owner:** PowerManager works out the mode in use. The derivation reproduces the downgrade condition at `BatteryAuthorityCommand.cpp:83` exactly, including the occupancy-mode condition. There is no behaviour change other than ending the overwrite and the flip-flop.

**Changes:**
- Rename the raw getter to `get_configuredConnectionMode()`.
- Add the PowerManager getter for the mode in use.
- Delete the writes at `BatteryAuthorityCommand.cpp:83` and `:90`.
- `ConfigApply.cpp:448–449` compares the ledger with the configured mode only, and no longer clears `lowBatteryMode`, since that flag belongs to battery. Codex confirms that nothing else relied on config apply clearing it.

**Reader table:** the PR includes a table of every reader the rename broke, with file:line and which getter it now uses. A reader gets the configured getter only if it reports or compares configuration. Everything that connects gets the mode in use.

**Budget:** +35 net src/ lines stays the cap. Report three numbers, net lines, moved lines and replaced call-site lines, so the width of the diff is visible.

**Migration:** a device that is downgraded at OTA time has INTERMITTENT stored as its configured mode. After the fix, the comparison at :448 rewrites the ledger value at the first config apply. Claude Code confirmed that config apply runs on every cloud connection (Step 0 report §8). Before rollout, Chip checks fleet device-status for `battery.lowBatteryMode: true`.

## Implementation (Copilot)

- The configured mode is written only by applying configuration.
- The mode used for connecting is worked out as configured plus downgrade, in the one owner (PowerManager).
- Add a host test that checks the configured mode survives downgrade, recovery, a configuration re-apply and a restart, plus the re-apply-while-downgraded case that flip-flops today.
- No new status fields and no alert changes. If the change turns out to touch how an alert is raised or cleared, stop: that would need `docs/reference/alert-codes.md` updated in the same PR.

## Verification (Codex, Stage 7)

- The mandatory linkage check.
- A local toolchain build. Run `make clean-user` between build types (§2).
- Tests reported as `N/N (sh via zsh, py via python3)` (§10).
- Verify the binary, not the source (§12.5).
- A retirement check: no remaining write to the configured mode outside configuration apply.

## Bench (Chip)

The host test is the primary proof. On Dev-14, Chip confirms that normal behaviour is unchanged, using the log lines from the Step 0 report. The physical low-battery trigger runs only if it's convenient; verification is not held for a multi-day drain.

## Rules

- Agents may open a PR. They do not merge. Only Chip merges (§2).
- The two-round rule applies (§12.4): two rounds that each add a mechanism, or two rounds without VERIFIED, mean stop and restate the goal.

## Architect's decisions after dispatch (2026-10-07)

1. **The mode in use is confirmed** as the configured mode, except INTERMITTENT while sensor mode is OCCUPANCY, the configured mode is KEEP_ALIVE and `lowBatteryMode` is set. The `!flag` guard on the downgrade branch, so the downgrade fires only once, is confirmed.
2. **Labels and logs show the mode in use.** Only the config hash and the config-apply comparison read the configured mode, so a downgrade no longer changes the config hash.
3. **`lowBatteryMode` rule, option (b):** the battery check keeps today's set and clear conditions unchanged (`BatteryAuthorityCommand.cpp:80-99`, including the clears when the configured mode leaves KEEP_ALIVE or the sensor mode leaves OCCUPANCY). Config apply no longer clears the flag. The mode in use comes from the derivation. The flag means "downgrade active", and the failsafe (`Generalized-Core-Counter.cpp:2664`) and the status ledger read it that way. Renaming or redefining it is out of scope; if it is worth doing, it belongs in WO 3.
4. **No version bump in this WO.** The suggestion is to bundle 1a, 1b and 1c into v39 after 1c; that is Chip's call. Dev-14's bench build is identified by the commit in the closing record.

**Checks added for Stage 7 (Codex):**
- `lowBatteryMode` is persisted, so a downgraded device stays INTERMITTENT across a restart.
- Every side effect of the old write at `:83` (a disconnect, a log line, a publish) still happens exactly once when the flag is set.
- A downgrade no longer changes the config hash.
- List every reader of `lowBatteryMode`, including the status ledger's `battery.lowBatteryMode`, and confirm none relied on config apply clearing it.
- **The `:91` rewrite (Chip):** the operator sets the ledger to INTERMITTENT, or any non-KEEP_ALIVE value, while the device is downgraded. In the old code, config apply's clear ended the downgrade at once. In the new code, the mode in use changes at once, but the flag is cleared by the next battery check, with a different log line. Confirm the end state is the same, and measure how long the flag stays set between the apply and the next `BatteryAuthority::commit()`, during which the failsafe's hard stages stay blocked.

## Stage 7 round 1 and the architect's decisions (2026-10-07)

The Stage 7 round 1 verdict is `docs/work-orders/WO-2026-10-07-002-stage7-verdict.md`: **NOT VERIFIED**. It reviewed `0e94f81`. Fourteen of the sixteen checks passed; checks 9 and 14 failed.

- **Finding 1, the flag gap (check 9): fix in round 2.** It is a recovery regression. After an operator override, the flag can stay set across sleep, and indefinitely in CONNECTED mode, because the battery checks that would clear it run only while disconnected or after a wake. Meanwhile it blocks the failsafe's hard stages and shows a stale downgrade in the status ledger.
- **Finding 2, migration (check 14): accepted, no code.** A device that is downgraded at OTA time has the flag cleared by setup's battery check before the first config apply. It then runs KEEP_ALIVE until its next battery check while disconnected re-downgrades it. Chip's pre-rollout fleet check for `battery.lowBatteryMode: true` shows whether any device is in that state.

**Round 2 (the first rework round):**
- **Scope:** one PowerManager getter, "downgrade active". It is true when sensor mode is OCCUPANCY, the configured mode is KEEP_ALIVE and `lowBatteryMode` is set; it is the same condition the mode-in-use derivation uses. The three readers that act on the downgrade are re-pointed to it: the failsafe's hard-stage block (`Generalized-Core-Counter.cpp:2664`), its diagnostic mirror (`ConnectivityFailsafeTest.cpp:186`) and the status ledger's `battery.lowBatteryMode` (`DeviceStatusPublisher.cpp:255`). Nothing else: no new battery checks, no clearing at config apply, no migration code.
- **Budget:** at most +10 net `src/` lines for this round. The WO total stays well under +35.
- **Implementation:** Copilot, Sonnet tier, medium reasoning.
- **Verification:** Codex, standard model, high reasoning. It re-runs check 9, confirms all sixteen checks still pass, and records check 14 as accepted.
- **Two-round rule:** if round 2 does not reach VERIFIED, stop. Any further edge case is written down and accepted rather than coded.

## Closing record (2026-10-07)

**Result: VERIFIED** at Stage 7 round 2 (`docs/work-orders/WO-2026-10-07-002-stage7-round2-verdict.md`). All sixteen checks pass; check 14 (migration) is ACCEPTED. It reached VERIFIED in the second round, within the two-round rule.

### Rounds

| Round | Implementer | Verifier | Verdict |
|---|---|---|---|
| 1 (Stage 6) + controller edit | Copilot, `claude-sonnet-5.5`, high | Codex, `gpt-6-astra` (top), high | NOT VERIFIED: check 9 (flag gap) and check 14 (migration) failed |
| 2 (one getter, three readers) | Copilot, `claude-sonnet-5.5`, medium | Codex, `gpt-5.6-sol` (standard), high | VERIFIED |

- **Why the top model for round 1's verification:** Chip chose it for this WO because the reader classification and the `lowBatteryMode` interactions had to be exact. Round 2's narrow scope went back to the standard model.
- **Model substitution in round 2:** the dispatch first named `gpt-6.1-sol`, which the API rejected before any work ("not supported when using Codex with a ChatGPT account"; `gpt-6-sol` and `gpt-6-luna` were rejected the same way). It was re-sent unchanged on `gpt-5.6-sol`. The note is in the dispatch header.
- **Controller edit (Claude Code, Chip pre-authorized; not a round):** deleted the now-uncalled `BatteryAuthority::clearLowBatteryMode()` (definition and declaration) and the unused `power/BatteryAuthority.h` include in `ConfigApply.cpp`, and updated three comments that named the function. That is **−5 net code lines**. Text/data/bss were unchanged at 150940 / 1090 / 2196, because the linker had already dropped the uncalled function. `nm` shows `clearLowBatteryMode`: 0. Committed in `0e94f81`.

### Budget versus actual (figures from the Stage 7 verdicts)

| Item | Budget | Raised to (reason) | Actual net `src/` lines | Tests |
|---|---|---|---|---|
| Round 1, including the controller edit (−5) | +35 cap | — | **+3** (48 added, 45 removed) | 66/66 → 68/68; six targeted mutations caught |
| Round 2 | +10 cap | — | **+1** (10 added, 9 removed) | 68/68; 64-case branch matrix; ten mutations caught |
| **WO total** (`73f752e` → worktree) | +15 to +35 | — | **+4** (52 added, 48 removed) | 68/68 (sh via zsh, py via python3) |

- **Moved lines:** 0 in every scope (`git diff --color-moved=zebra`).
- **Replaced call-site lines:** 32 reader replacements (29 in round 1, 3 in round 2) plus 4 accessor-rename lines. The diff is wide but nearly net-neutral, as the architect predicted.
- **The WO came in under its own +15 floor.** The estimate assumed new derivation code; most of the change turned out to be one-for-one replacements and deletions.
- **Final ARM build** (Boron, Device OS 6.4.1, fresh `BUILD_PATH_BASE`): text/data/bss 150964 / 1090 / 2196, which is +120 text over `73f752e`'s 150844.

### Step 0 citation table

Step 0 report §1 holds the full table. In summary: every WO 1a citation in `docs/work-orders/2026-10-06-step6-ownership-codex-report.md` **HOLDS**, except one.

| Report line | Citation | Result |
|---|---|---|
| 16 | Main:1583 (setup commit) | **MOVED** to `src/Generalized-Core-Counter.cpp:1585`. `:1583` is the voltage read. |
| 45 / 74 | Main:2655-2671 (failsafe low-battery block) | HOLDS. The range is loose; the logic is at `:2661-2664`. |
| 67 | "connection policy derives effective mode from configured plus downgrade" | **No such code existed.** It is a proposal, not a citation (see below). |

All the others hold: `ConfigApply.cpp:445-455`, `BatteryAuthorityCommand.cpp:76-100`, `MyPersistentData.cpp:57,132,145,199,390`, `State_Report.cpp:179,231`, `State_Modes.cpp:79`, `State_Sleep.cpp:1620,1689,1719`, Main `:355-382` and `:2612-2620`.

### Corrections to the record

- **Codex ownership report (2026-10-06):** its statement that "connection policy derives effective mode from configured plus downgrade" described no existing code. Until this WO, nothing worked out a mode in use; there was one stored mode, and the downgrade overwrote it. `ConnectivityPolicy.h` holds timing constants only. Main `:1583` → `:1585`.
- **Step 0 report §8 (migration):** it said a device downgraded at OTA time would be reconciled at its first config apply. Stage 7 round 1 (check 14) found that setup's battery check runs first and clears the flag through the stale-clear branch. The device then runs KEEP_ALIVE until a battery check while disconnected re-downgrades it. The architect accepted this with no code; Chip's pre-rollout fleet check for `battery.lowBatteryMode: true` covers it.

### Known, accepted path differences

1. **The flag is cleared later after an operator override.** When the ledger moves a downgraded device off KEEP_ALIVE (or sensor mode leaves OCCUPANCY), the raw `lowBatteryMode` flag now clears at the next `BatteryAuthority::commit()`, not at config apply, and the log shows "Battery recovery: clearing lowBatteryMode" in place of the config-apply clear. Until then the stale flag is **inert**: the mode in use, the failsafe hard-stage block and the status ledger's `lowBatteryMode` all read `PowerManager::downgradeActive()` (round 2, verified on every path in check 9).
2. **Migration** (above). ACCEPTED, no code.
3. **The status ledger's `lowBatteryMode`** now publishes the derived "downgrade active" value, not the raw persisted flag. The key name is unchanged. The flag means "downgrade active"; renaming or redefining it belongs in WO 3.

### Remaining for Chip

- **Stage 8:** review and commit round 2 and the records.
- **Bench on Dev-14:** confirm normal behaviour is unchanged, using the Step 0 report §5 log lines. The physical low-battery trigger is optional. The build is identified by the commit in this record.
- **Before rollout:** check fleet device-status for `battery.lowBatteryMode: true`.
- **Release:** no version bump in this WO. The suggestion is v39, bundling 1a, 1b and 1c (Chip's call).

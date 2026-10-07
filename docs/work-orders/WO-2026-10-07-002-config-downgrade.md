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

## Closing record (to be completed)

- Budget versus actual: net, moved, and replaced call-site lines, plus any raise with its reason.
- Step 0's citation table (Step 0 report §1).
- Correction to the Codex ownership report: no code derived the mode in use. Also record Main :1583 → :1585.

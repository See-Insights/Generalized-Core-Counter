AGENT: Copilot · MODEL: claude-opus-5.5 · REASONING: high (still escalated after failed rounds, §5)
AUTHORIZATION SCOPE:

**Authorized:** edit `src/power/PmicFaultMonitor.cpp` (the single line at ~342 only) and the tests the change requires (`tests/pmic_fault_monitor_test.cpp`, its `.sh` only if a scenario is added to its list); run the host suite and the local ARM release build. **Scratch directory:** `build-tmp/wo20261102-001-stage6r3/`; remove only it.
**Not authorized:** anything else in `src/`, `docs/`, `lib/`; commits, pushes, merges, branch changes, stash, reset or checkout; deleting anything you didn't create, including `build-tmp/` itself.
**SIZE BUDGET:** `src/` total for the WO stays **net +30**; this round must add **0** net `src/` lines (a change to the existing line, no new line).

# Stage 6 round 3: WO-2026-11-02-001, Stage 7 round 2 blocker (`WO-2026-11-02-001-stage7-round2-verdict.md`, items 2, 3, 5)

Same worktree and branch; the round-2 diff is uncommitted and is your starting point. Binding spec: the WO incl. Amendment 1, and the architect rulings: P:342 is accepted **on the condition that the counter is set and reset to 0 within the same `pollAndRemediate` call and can never carry into the fault-escalation path.**

**The defect (verify it yourself first):** `PmicFaultMonitor.cpp:342` sets `consecutiveFaults = 1` when alert 22 is current and `chargeStatus != 0`. If `remediationInProgress` is true at that moment (the latch's own level-1 cycle is running), the `if (remediationInProgress)` branch (~343-362) runs and the clearing branch (`else if (consecutiveFaults > 0)`, ~363-380) is skipped, so the synthetic 1 survives the call. Consequences: (a) a later first real charge fault does `consecutiveFaults++` to 2 and requests a level-1 cycle on its first sighting; (b) a later fault-free poll with `chargeStatus == 0` enters the clearing branch and clears alert 22 while the charger still reads Not Charging.

**Fix (minimal, same line, 0 net lines):** set the counter only when no remediation cycle is running, i.e. add `!remediationInProgress` to the condition at ~342 (or the equivalent that guarantees the counter is set only on a call where the clearing branch at ~363 will run and reset it to 0). Then alert 22 simply stays raised until the cycle completes and a later poll with `chargeStatus != 0` sets and resets the counter in one call. Keep the same-line comment (update its wording if needed, still one line). If you find a path where the counter can still carry (a fault bit on the same call, `immediateResetConsumed`, anything), say which and stop rather than add lines.

**Tests (the gap Codex named):** add to scenario 7 (or a new scenario 10, your choice, kept small): with alert 22 current and the latch cycle running (`remediationInProgress`), a poll with `chargeStatus == 1` must **not** clear alert 22 and must not leave a carried counter: then (i) a fault-free `chargeStatus == 0` poll after the cycle completes does **not** clear alert 22, and (ii) a first real charge fault on the next fault poll does **not** request a level-1 cycle on its first sighting (it needs the usual 2 consecutive faults). Mutation (copy in `$TMPDIR`, never `src/`): remove the `!remediationInProgress` term; the new test must fail. Re-run the earlier mutation set that still applies.

**Suite and build:** full suite as one sequential loop, output to `/dev/null`, `N/N (sh via zsh, py via python3)` (round 2: 108/108); local ARM release build per §3 with a fresh `BUILD_PATH_BASE`; sizes; `git diff --numstat -- src` total (must stay +30 net).

**Report (final message):** the verified defect and the exact edit (file:line); test and mutation results; suite and build; `src/` arithmetic; model and reasoning level actually used; deviations or "none".

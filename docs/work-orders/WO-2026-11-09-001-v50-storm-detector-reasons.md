# WO-2026-11-09-001: boot-storm detector counts PANIC and UNKNOWN resets (v50.2)

**Author:** Claude Code, 9 Nov 2026 · **Status:** DISPATCHED (architect rulings, 9 Nov 2026) · **Branch:** `wo/2026-11-09-001-v50-storm-detector-reasons`, stacked on `integration/v50-ab1805` @ 1d1d5af7a22dc47094a4c23b515b348bd8ae153d; the PR is a **draft against `integration/v50-ab1805`, held unmerged until v50.1 ships** (as with I21). Ships in **v50.2**. Never merge.
**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §3 Stages 5-8; §4 (SHA rule); §10 (CI discovery); §12 · **Agents:** Stage 6 Copilot medium (the report states the level actually run); Stage 7 Codex `gpt-5.6-sol` **xhigh**, 90-minute box, at most two rounds.
**Inputs:** `docs/handover/2026-11-09-storm-detector-and-5096-rulings-needed.md` (the finding and the rulings; also in the readiness-review notes, item 4); `src/Generalized-Core-Counter.cpp:338-352` (the switch) and `:357-393` (its uses); `src/persist/RetainedBoot.h`; `tests/retained_boot_structural_test.py:120-123` (the text-only test that passed over the defect).
**Budget:** `src/` at most **+8 net** (removals count); stop and report rather than trim style.

## Plain goal
In the real firmware only reset reasons `NONE` (0) and `WATCHDOG` (60) count toward a boot storm. `PANIC` (130) and `UNKNOWN` (10) are listed in the switch, but behind `#ifdef RESET_REASON_PANIC` and `#ifdef RESET_REASON_UNKNOWN`, and in Device OS 6.6.0 these are **enumerators, not macros**, so `#ifdef` is false and the cases are compiled out (`RESET_REASON_USER_APPLICATION` does not exist at all). A panic loop or an unknown-reason loop therefore never trips the storm protection (the 600 s ULP holdoff and the alert). Proven by a Device OS 6.6.0 build of the integration head with `#pragma message` probes at the switch (all six names `NOT_A_MACRO`; make rc 0). Fix it, and add a test that checks what actually compiles.

## Rulings that bind this WO (architect, 9 Nov 2026; not reopened)
1. **Fix in v50.2** as a small test-first WO: direct `case RESET_REASON_PANIC:` and `case RESET_REASON_UNKNOWN:` labels; **drop `USER_APPLICATION`**; a **compile-level test** (built against the real Device OS header, **or** failing on any `#ifdef` guarding an enumerator). Budget small.
2. **`UNKNOWN` counts: yes.**
3. **The behaviour change is wanted:** panic and unknown-reason loops now trip the 600 s storm holdoff.
4. **No PANIC bench variant;** bench image 5096 (reason 60) is fine as built.
5. **Cleanup-series principle (recorded in the recovery plan):** structural tests of `#ifdef`-guarded code must check what compiles, not what the source text says.

## The change
1. **`src/`:** the qualifying decision uses direct cases for `RESET_REASON_NONE`, `RESET_REASON_PANIC`, `RESET_REASON_WATCHDOG`, `RESET_REASON_UNKNOWN`, with no preprocessor guards and no `USER_APPLICATION`. **Preferred shape:** a small pure inline function in an existing header (for example `src/persist/RetainedBoot.h`) returning whether a reset reason qualifies, called from the one place in `setup()`; it lets the test compile the real function against the real header. If that does not fit the budget, keep the switch inline and let the test extract the real switch text. Nothing else changes: the early-boot requirement (`previousBootFailedEarly`), the count, the window arithmetic, the trip, the holdoff and the alert are untouched.
2. **Tests (compile-level; the point of the WO):**
   a. A host test that includes the **real Device OS `system_defs.h`** (the CI host job checks out Device OS 6.4.1 headers under `DEVICE_OS_ROOT`; locally use the toolchain copy, as `tests/sleep_config_leak_test.sh` does) and compiles the real decision (the function, or the extracted switch) over every enumerator of `System_Reset_Reason`: it must qualify exactly `NONE`, `PANIC`, `WATCHDOG`, `UNKNOWN` and must **not** qualify `PIN_RESET` (20), `POWER_MANAGEMENT` (30), `POWER_DOWN` (40), `POWER_BROWNOUT` (50), `UPDATE` (70), `UPDATE_ERROR`, `UPDATE_TIMEOUT`, `FACTORY_RESET`, `SAFE_MODE`, `DFU_MODE`, `USER` (140), `CONFIG_UPDATE` (150). The test must work in CI (6.4.1 headers) and locally (6.6.0): if an enumerator is absent in one version, say how it is handled.
   b. A **generalised structural check**: parse the real `system_defs.h` for every enumerator name, then fail on any `#ifdef`, `#ifndef`, `#if defined(...)`, `#elif defined(...)` in `src/` (not `lib/`) that is guarded on one of those names. Step 0 reports every existing hit (there may be other latent cases); each hit is listed in the report and **not fixed here** unless it is the storm switch (stop and report: it goes to the architect).
   c. **Mutations (each must fail; back up the file, mutate, restore from the backup with cp; never `git checkout`/`git restore` while your work is uncommitted):** `PANIC` case removed; `UNKNOWN` case removed; an `#ifdef RESET_REASON_PANIC` guard put back around the case; `USER_APPLICATION` re-added behind an `#ifdef`; `PIN_RESET` added to the qualifying set; the function's result inverted.
3. The existing text-only `tests/retained_boot_structural_test.py` keeps passing (update its expected text only as the change forces; say what changed).

## Step 0 (read-only, inside Stage 6; stop and report if any answer breaks the plan)
1. Re-cite the switch and its three uses on this head; confirm `RESET_REASON_PANIC` (130), `RESET_REASON_UNKNOWN` (10), `RESET_REASON_NONE` (0), `RESET_REASON_WATCHDOG` (60) are enumerators in both the CI header set (6.4.1) and 6.6.0.
2. How existing host tests locate and include Device OS headers (`DEVICE_OS_ROOT`; the `sleep_config_leak_test.sh` pattern) and whether `system_defs.h` compiles standalone on the host with the stubs available (it includes `hal_platform.h`; a preprocess in the controller's probe aborted on `hal_platform_config.h`: say what the test needs to stub or include).
3. Every `#ifdef`/`#ifndef`/`#if defined` in `src/` guarding a name that is an enumerator in the real Device OS headers (item 2b's scan), with file:line.
4. Whether counting `PANIC` and `UNKNOWN` interacts with any other consumer of `qualifiesForStormCount` or the storm counters (`bootStormLastResetReason`, the alert, the forensics).
5. The net `src/` lines; if over +8, stop and report the cut.

## Bench
None required beyond 5096 (already built; reason 60). A panic or unknown loop is covered by the compile-level test; no hardware variant (architect ruling 4).

## Protected / not in scope
The storm window and threshold, the holdoff duration and bracketing (the S2 fix), the persisted and `retained` layouts, S1-S4, I21, L, `lib/`.

## Rollback
Revert the commit: v50.1 behaviour (reasons 0 and 60 only) returns.

## Amendment 1 (architect, 9 Nov 2026, after Stage 7 round 1 NOT VERIFIED on 8c82f13; test-only, no `src/` change)
1. **F1 (Medium), approved, the simpler option:** `tests/storm_reason_qualifies_test.sh` extracts the span from the `bool qualifiesForStormCount` declaration to its first consumer (`if (previousBootFailedEarly && qualifiesForStormCount)`), and fails unless that span contains **exactly one** `switch` and **no other executable code** (only the declaration, comments, the one switch, and whitespace). That one switch is what gets compiled and run. A second switch, an assignment, a call or a loop in the span fails the test.
2. **F2 (Medium), approved:** `tests/reset_reason_guard_structural_test.py` coalesces backslash-continued lines into one logical directive before matching, keeping the starting line number in the report.
3. **F3 (Low), NOT fixed; documented limits** (comment block at the top of the scan and in the report): macro aliases (`#define ALIAS RESET_REASON_PANIC` then `#if ALIAS`) and enumerator names inside quoted arguments are not handled; only enumerators declared in `system_defs.h` are covered (not other HAL or system headers); brace counting and the span check are not comment- or string-aware beyond what the scan already blanks. The scan is a guardrail, not a parser.
4. Mutations to show (own, by the controller): a correct decoy switch before the live switch with `RESET_REASON_POWER_MANAGEMENT` added to the live one must fail the test; a split `#if defined( \` newline `RESET_REASON_PANIC)` guard in `src/` must fail the scan.
5. Stage 7: Codex xhigh round 2, the last; a failure goes to the architect. Draft PR against `integration/v50-ab1805`, HELD UNMERGED until v50.1 ships (ships v50.2), like I21.

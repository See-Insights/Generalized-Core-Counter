AGENT: Codex · MODEL: gpt-5.6-sol (standard tier; confirmed with a one-line probe on 2026-10-19, §5) · REASONING: high
AUTHORIZATION SCOPE:

**Authorized:**
- Review the uncommitted diff in this worktree (branch `wo/2026-10-19-002-step6-p3-report-publishing`; main `dd90842`).
- Run the host suite and the tests; run local ARM builds in scratch copies.
- Write temporary files under **`build-tmp/wo20261019-002-stage7/`** or `$TMPDIR`, and remove **only** what you create.
- Mutate only in scratch copies.
- Write your verdict to `docs/work-orders/WO-2026-10-19-002-stage7-verdict.md` (the only file you may add in the repo).

**Not authorized:**
- Changes outside your scratch locations and the verdict file.
- Commits, pushes, stash, reset, checkout or restore.
- Device, network or AWS access.

# Stage 7 dispatch: WO-2026-10-19-002 (Step 6 P-3: move report publishing and the daily cleanup out of main)

**Goal, in plain language:** the hourly report's publishing code moves out of `Generalized-Core-Counter.cpp`: five functions (`isAutoClearAfterReportAlert`, `publishData`, `publishHibernateWakeForensics`, `UbidotsHandler`, `publishDiagnosticSafe`) and two constants into a new `src/cloud/ReportPublisher.{h,cpp}`, and `dailyCleanup()` into `src/state/State_Report.cpp`, its only caller. **Unchanged.** This is the product's core output: the webhook payload must stay byte-for-byte identical.

**Binding spec:** `docs/work-orders/WO-2026-10-19-002-step6-p3-report-publishing-move.md` (rules, budget, Claude Code's citation re-check at `dd90842`, the golden fixture hash). Standard since P-2: no `retained` keyword in the new files, with negative checks for the `.cpp` and the `.h`.

**Inputs:** `docs/work-orders/WO-2026-10-19-002-stage6-copilot-dispatch.md`, and Copilot's report (not in the repo; the figures below are Claude Code's reproduction). Treat every claim as unverified.

**What changed** (uncommitted):
- **`src/`:** new `cloud/ReportPublisher.{h,cpp}` (340 lines in the `.cpp`); `dailyCleanup()` added at the end of `state/State_Report.cpp` (31 lines); `Generalized-Core-Counter.cpp` 1978 → 1596 lines (the moved functions, the two `REPORT_FORENSICS_*` constants and five forward declarations removed; one include added). Claimed net **+12** (395 added, 383 removed; limit +20, hard).
- **Tests:** ten re-pointed (`alert19_clears_after_report_test.sh`, `clock_resync_wiring_test.py`, `daily_cleanup_boundary_test.py`, `hibernate_wake_diagnostics_test.sh`, `pir_sensor_reset_totals_test.sh`, `power_consumer_regression_test.sh`, `report_payload_fields_test.py`, `report_payload_golden_test.sh`, `reset_wiring_test.sh`, `watchdog_alert_code_test.sh`), +34/−29, and a new `report_publisher_move_structural_test.py` (87 lines); about +92 in all (guideline +150).
- **Claude Code's reproduction (6.4.1 Boron, fresh `BUILD_PATH_BASE`s; the "before" is the P-2 tree, whose `src/` equals main `dd90842`):** suite **86/86** (baseline 85; sh via zsh, py via python3); fixture `tests/fixtures/report_payload_golden.txt` sha256 `07de1e1980e4a44e03420540e6c721a93a61b5d6392539219928420c048f0637` unchanged; text/data/bss `151756/1094/2220 → 151820/1094/2228` (**+64 text, +8 bss**); the whole `.backup` section (24 symbols) identical (`retainedBoot` `0x2003f420`, signature `0x2003f494`). Symbol-level: two **new** static-initialization stubs, `_GLOBAL__sub_I__Z11publishDatax` (24 B) and `_GLOBAL__sub_I__Z20handleReportingStatev` (12 B); `publishStartupStatus` 540 → 548 and `publishWatchdogForensics` 156 → 164; `LocalTimePosixTimezone::operator=` clone 220 → 148; `__tcf_1` 2 → 24; `CSWTCH` tables +72 new / −44 gone. Mutations I ran in scratch copies: dropping `WITH_ACK` from the report publish (caught by `publish_with_ack_structural_test.py` and `report_payload_golden_test.sh`); swapping the alerts/resets names in the occupancy layout (caught by `report_payload_golden_test.sh`; `report_payload_fields_test.py` does not catch it); `dailyCleanup()` ahead of `publishData()` (caught by `daily_cleanup_boundary_test.py`, `standby_latch_daily_clear_test.sh`, the new structural test); a `retained` probe in the new `.cpp` and in the `.h` (caught by the new structural test).

## Checks (each one: PASS / FAIL / CONCERN, with evidence)

1. **Every moved body is byte-identical,** with its comments: the five functions, `dailyCleanup`, and the two constants (Copilot says `isAutoClearAfterReportAlert` stays `static`). The "Shared notes for report payload construction" doc block stayed in main (Copilot's deviation): confirm it is not part of any function range. Anything beyond linkage, declarations and includes is a FAIL.
2. **The payload is byte-for-byte unchanged.** The golden test passes all vectors with the fixture's hash unchanged and no diff to the fixture or the test's markers (`static const char *batteryContext[7] = {` … the `PublishQueuePosix::instance().publish(webhookName, data, PRIVATE | WITH_ACK)` line); the test reads the new file; its mutants are still caught. **`WITH_ACK` is still present on the report, hibernate-wake and diagnostic publishes** and `publish_with_ack_structural_test.py` still fails if any is removed (mutate each in a scratch copy).
3. **Call sites and order are intact.** In `State_Report.cpp`: `publishData(due ? boundary - 1 : 0);` still precedes `dailyCleanup();` still precedes the restart code (`SystemConfig::set_lastDailyCleanup(now)` etc.). `setup()` still registers `UbidotsHandler` (`Particle.subscribe(responseTopic, UbidotsHandler)`) and calls `publishHibernateWakeForensics` through the new header. Other callers (`State_Connect.cpp`, `MyPersistentData.cpp`) still resolve `publishDiagnosticSafe`. Bare-name search as well as qualified.
4. **Linkage and declarations.** Which functions are external, which internal; each declared and defined once; the default-argument declarations (`publishData` with `= 0` in `State_Common.h`, `publishDiagnosticSafe` in `State_Common.h` and `MyPersistentData.cpp:51`) unchanged; no ODR or duplicate definition; the final link succeeds.
5. **Static initialization (the new stubs).** Identify exactly which objects `_GLOBAL__sub_I__Z11publishDatax` (in `ReportPublisher.cpp`) and `_GLOBAL__sub_I__Z20handleReportingStatev` (in `State_Report.cpp`) construct, and compare with what `HEAD`'s `Generalized-Core-Counter.cpp` initialized for the same objects. State whether the move changes the order, timing or set of dynamic initializers, and whether any cross-translation-unit initialization-order dependence is introduced (including the `REPORT_FORENSICS_*` constants, the function-local statics such as `batteryContext`, and `PublishQueuePosix`).
6. **The +64 B text and +8 B bss are explained by symbol** (`nm -C -S`, both builds; the figures above are from Claude Code's reproduction; Copilot itemised +80 and left about 16 B unexplained; account for the whole delta, including alignment and the library clones that changed size). Confirm there is no new function beyond the moved set and the two stubs, and no logic change.
7. **Retained layout under the narrowed rule.** Reproduce the whole `.backup` comparison; state the outcome: (A) nothing moved, (B) report-only moved, (C) anything else (STOP). Confirm no `retained` keyword in the new files or in the moved `dailyCleanup` text in `State_Report.cpp`, and that the new structural test fails with a named message for a probe in the `.cpp` and in the `.h`.
8. **The tests were re-pointed, not weakened.** For each of the ten, diff it: each assertion is unchanged apart from the file it reads, except the declared one (`daily_cleanup_boundary_test.py` now checks the `publishData` definition in `ReportPublisher.cpp` omits the default argument, while still requiring exactly one default-argument declaration, in `State_Common.h`). Then, in scratch copies: (a) change the transient-alert list in `isAutoClearAfterReportAlert`; (b) re-define one moved function in main; (c) remove the `due`-gated restart or reorder it relative to `dailyCleanup()`; (d) change one `BREADCRUMB_REPORT_*` constant in `publishData`. Report which tests catch each, honestly, including none.
9. **Golden, suite and budget.** The full suite `N/N (sh via zsh, py via python3)` (baseline 85); `git diff --check` clean; non-move `src/` lines ≤ +20 (hard); main `wc -l` before (1978) and after (1596).

## Verdict (write it to the file above, then end with a short summary as your final message)

- **Header:** one of VERIFIED / VERIFIED WITH CONCERNS / NOT VERIFIED; the model and reasoning used; the test interpreter line.
- **A table of checks 1–9,** and, on its own line, **the retained outcome from check 7 (A, B or C)**.
- **Findings,** with file:line and severity, separating observation from inference.
- **Budget versus actual,** as one row.
- **Confirmations:** the tree matches the start, and you removed only your own scratch files.

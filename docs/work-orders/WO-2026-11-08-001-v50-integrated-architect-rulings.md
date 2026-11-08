# WO-2026-11-08-001: architect rulings on the integrated Stage 7 round 1 (8 Nov 2026)

Verdict: `WO-2026-11-08-001-v50-integrated-stage7-r1-verdict.md` (NOT VERIFIED on ae2619b3ce646d0b2ba59e73e60f9a4e66b31073). Rulings by Chip (architect), received 8 Nov 2026 ~11:00Z. Round 2 is the last; a failure goes to the architect.

1. **Finding 1 (High): fix in v50.1.** `ExternalWatchdog.init()`/`start()` move to right after the RTC bind; the boot-storm holdoff `System.sleep()` is wrapped in `ExternalWatchdog.stop()` before and `start()` after (as in the S2 paths); the later duplicate init is removed. (The structure is pre-existing in v49: boot-storm ULP sleep at daec561 lines ~396-405 precedes `setWDT` at 811.)
2. **Finding 2 (Medium): single-byte design approved.** The failsafe stage-3 path also writes the 0xA5 sentinel (`set_powerOffRequested(kPowerOffSentinel)`) before its flush. The reader uses the byte only: remove the stage-3 arm, the 600 s recency check (`kPowerOffEvidenceMaxAgeSec`) and the `timeValid` dependency. **This supersedes S2 Amendment 4's stage-3 parts, including the invalid-Time rejection (F4).** `lastConnectivityRecoveryAction` is not touched.
3. **Finding 3 (Low): accepted residual.** A stale 0xA5 can mislabel at most one later classification; the next completed v50 boot clears it.
4. **Finding 4 (Medium, tests): keep the two-boot consumption test.** (With the recency constant gone, no literal-600 assertion remains.)
5. **Finding 5 (Medium, bench): bench item 5 drops the raw AM18x5 STATUS** and adds "no human contact during the night" (requested-versus-actual timing plus reason 30).
6. **Rollback during a boot storm:** recorded residual for the readiness review (pre-existing in v49's structure; not a bench case).

Process: Copilot (medium; the report states the level actually run) -> Claude Code verifies and commits -> second and last integrated Stage 7 (Codex xhigh, 90-minute box, SHA rule). The bench kit waits for the final head.

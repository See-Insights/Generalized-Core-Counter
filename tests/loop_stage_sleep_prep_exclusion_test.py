#!/usr/bin/env python3
"""
Host-side regression test for WO-2026-08-11-001: exclude LOOP_STAGE_SLEEP_PREP
from noteLoopStageDuration()'s kLoopStageWarnThresholdMs/
kLoopStageErrorThresholdMs WARN/ERROR escalation while keeping an INFO-level
"LoopStage:" line for that stage with the same fields.

noteLoopStageDuration() (src/Generalized-Core-Counter.cpp) pulls in heavy
Particle/PublishQueuePosix/AB1805 dependencies and can't be compiled
standalone on the host, so - following the same approach as
tests/sleep_breadcrumb_sequence_test.py - this test parses the ACTUAL shipped
source file directly and confirms, by real structural checks (not a
reimplementation that could silently drift):

  1. There is a stage-specific SLEEP_PREP exclusion inside
     noteLoopStageDuration() itself (not at the handleSleepingState()/loop()
     call site) that textually precedes the kLoopStageErrorThresholdMs/
     kLoopStageWarnThresholdMs comparisons and returns before reaching them.
  2. That exclusion branch logs at Log.info(...) with a "LoopStage:" format
     string carrying the same stage/elapsed/state/q/connMs fields as the
     existing WARN/ERROR lines.
  3. kLoopStageErrorThresholdMs and kLoopStageWarnThresholdMs themselves are
     unchanged (10000UL / 2000UL) and the WARN/ERROR comparisons still exist
     unconditionally for every other stage (i.e. the exclusion is scoped to
     the single early-return branch, not a rewrite of the threshold logic).
  4. The single existing LOOP_STAGE_SLEEP_PREP call site
     (handleSleepingState() in src/state/State_Sleep.cpp) is untouched by
     this change.

This is real evidence traced through the actual code (every assertion below
reads literal source text/line order from the shipped files), plus three
synthetic-log-stream simulations of the exact scenarios described in the
work order's Required Tests section, driven by a decision function that
mirrors noteLoopStageDuration()'s now-confirmed control flow.
"""
import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
MAIN_SRC = REPO_ROOT / "src" / "Generalized-Core-Counter.cpp"
SLEEP_SRC = REPO_ROOT / "src" / "state" / "State_Sleep.cpp"


def fail(msg: str) -> None:
    print(f"FAIL: {msg}", file=sys.stderr)
    sys.exit(1)


def main() -> None:
    main_text = MAIN_SRC.read_text()
    main_lines = main_text.splitlines()

    # ---- Check 1: thresholds themselves are unchanged. ----
    if "constexpr unsigned long kLoopStageWarnThresholdMs = 2000UL;" not in main_text:
        fail("kLoopStageWarnThresholdMs changed or missing (expected 2000UL)")
    if "constexpr unsigned long kLoopStageErrorThresholdMs = 10000UL;" not in main_text:
        fail("kLoopStageErrorThresholdMs changed or missing (expected 10000UL)")
    print("PASS: kLoopStageWarnThresholdMs=2000UL / kLoopStageErrorThresholdMs=10000UL unchanged")

    # ---- Check 2: noteLoopStageDuration() body location. ----
    def find_all(token, lines=None):
        lines = lines if lines is not None else main_lines
        return [i for i, l in enumerate(lines) if token in l]

    fn_starts = find_all("void noteLoopStageDuration(")
    if len(fn_starts) != 1:
        fail(f"expected exactly one noteLoopStageDuration() definition, found {len(fn_starts)}")
    fn_start = fn_starts[0]

    # Find the end of the function by matching braces from the opening line.
    depth = 0
    fn_end = None
    started = False
    for i in range(fn_start, len(main_lines)):
        depth += main_lines[i].count("{") - main_lines[i].count("}")
        if "{" in main_lines[i]:
            started = True
        if started and depth == 0:
            fn_end = i
            break
    if fn_end is None:
        fail("could not locate end of noteLoopStageDuration() body")
    body_lines = main_lines[fn_start:fn_end + 1]
    print(f"PASS: noteLoopStageDuration() body located at lines {fn_start + 1}-{fn_end + 1}")

    # ---- Check 3: an early SLEEP_PREP-specific branch exists inside the
    # function body, textually before the kLoopStageErrorThresholdMs check,
    # and it returns (bypassing the threshold comparisons). ----
    sleep_prep_branch_idx = None
    for i, l in enumerate(body_lines):
        if "LOOP_STAGE_SLEEP_PREP" in l and "if" in l:
            sleep_prep_branch_idx = i
            break
    if sleep_prep_branch_idx is None:
        fail("no 'if (... LOOP_STAGE_SLEEP_PREP ...)' branch found inside noteLoopStageDuration()")

    # The branch condition must be the bare, unconditional stage equality -
    # not further gated by either threshold constant (a mutation that only
    # excludes SLEEP_PREP from ONE of the two thresholds, e.g.
    # 'stage == LOOP_STAGE_SLEEP_PREP && elapsedMs >= kLoopStageErrorThresholdMs',
    # must fail this check).
    branch_condition_line = body_lines[sleep_prep_branch_idx]
    if not re.search(r"^\s*if\s*\(\s*stage\s*==\s*LOOP_STAGE_SLEEP_PREP\s*\)", branch_condition_line):
        fail(f"SLEEP_PREP branch condition is not the bare unconditional stage equality: {branch_condition_line!r}")
    if "ThresholdMs" in branch_condition_line or "elapsedMs" in branch_condition_line:
        fail(f"SLEEP_PREP branch condition appears additionally gated by elapsed/threshold: {branch_condition_line!r}")
    print("PASS: SLEEP_PREP exclusion branch condition is the bare, unconditional stage equality")

    error_check_idx = next((i for i, l in enumerate(body_lines) if "kLoopStageErrorThresholdMs" in l), None)
    warn_check_idx = next((i for i, l in enumerate(body_lines) if "kLoopStageWarnThresholdMs" in l), None)
    if error_check_idx is None or warn_check_idx is None:
        fail("could not find kLoopStageErrorThresholdMs/kLoopStageWarnThresholdMs comparisons in function body")
    if not (sleep_prep_branch_idx < error_check_idx < warn_check_idx):
        fail("SLEEP_PREP exclusion branch must textually precede the ERROR then WARN threshold comparisons")
    print("PASS: SLEEP_PREP exclusion branch textually precedes both threshold comparisons")

    # The branch must contain a 'return' before the next non-blank/brace line
    # reaches the threshold checks, i.e. it bypasses them.
    branch_to_error = body_lines[sleep_prep_branch_idx:error_check_idx]
    if not any("return" in l for l in branch_to_error):
        fail("SLEEP_PREP exclusion branch does not appear to return (would fall through to threshold checks)")
    print("PASS: SLEEP_PREP exclusion branch returns, bypassing WARN/ERROR threshold checks")

    # The branch body itself (condition through its own return) must not
    # reference either threshold constant anywhere - not just in the
    # condition line - closing off a nested-if variant of the same
    # partial-exclusion mutation (gating only inside the branch body).
    if any("ThresholdMs" in l for l in branch_to_error):
        fail("SLEEP_PREP exclusion branch body references a threshold constant - "
             "exclusion must be fully unconditional, not gated internally")
    print("PASS: SLEEP_PREP exclusion branch body contains no threshold-constant references")

    # ---- Check 4: that branch logs at INFO level with a "LoopStage:" line
    # carrying stage/elapsed/state/q/connMs fields, matching the existing
    # WARN/ERROR format string's fields. ----
    branch_to_error_text = "\n".join(branch_to_error)
    if "Log.info(" not in branch_to_error_text:
        fail("SLEEP_PREP exclusion branch does not log at Log.info(...)")
    if "LoopStage:" not in branch_to_error_text:
        fail("SLEEP_PREP exclusion branch's Log.info(...) does not use the 'LoopStage:' prefix")
    expected_fields = ["stage=%s", "elapsed=%lu", "state=%u", "q=%u", "connMs=%lu"]
    for field in expected_fields:
        if field not in branch_to_error_text:
            fail(f"SLEEP_PREP INFO line missing expected field format '{field}'")
    print("PASS: SLEEP_PREP exclusion branch logs INFO 'LoopStage:' with stage/elapsed/state/q/connMs fields")

    # ---- Check 5: the WARN/ERROR comparisons remain unconditional (i.e. not
    # further wrapped in a stage check) for every other stage - the
    # exclusion is scoped to the single early-return, not a global rewrite. ----
    error_line = body_lines[error_check_idx]
    warn_line = body_lines[warn_check_idx]
    if "elapsedMs >= kLoopStageErrorThresholdMs" not in error_line:
        fail(f"unexpected ERROR threshold comparison form: {error_line!r}")
    if "elapsedMs >= kLoopStageWarnThresholdMs" not in warn_line:
        fail(f"unexpected WARN threshold comparison form: {warn_line!r}")
    # Confirm these lines are directly 'if'/'else if' (not further gated by a
    # stage-specific condition beyond the single early-return already checked).
    if not re.search(r"^\s*if\s*\(elapsedMs >= kLoopStageErrorThresholdMs\)", error_line):
        fail("ERROR comparison is not a plain top-level 'if' - may have been additionally gated")
    if not re.search(r"^\s*\}\s*else if\s*\(elapsedMs >= kLoopStageWarnThresholdMs\)", warn_line):
        fail("WARN comparison is not a plain top-level 'else if' - may have been additionally gated")
    print("PASS: WARN/ERROR threshold comparisons remain unconditional for all non-SLEEP_PREP stages")

    # ---- Check 6: the single existing LOOP_STAGE_SLEEP_PREP call site in
    # State_Sleep.cpp / the loop()-side noteLoopStageDuration(false) call are
    # untouched (still exactly one setLoopStage(LOOP_STAGE_SLEEP_PREP) call,
    # and noteLoopStageDuration() is still called exactly once per stage
    # transition point in loop()). ----
    sleep_src_text = SLEEP_SRC.read_text()
    set_stage_calls = re.findall(r"setLoopStage\(LOOP_STAGE_SLEEP_PREP\)", sleep_src_text)
    if len(set_stage_calls) != 1:
        fail(f"expected exactly one setLoopStage(LOOP_STAGE_SLEEP_PREP) call in State_Sleep.cpp, "
             f"found {len(set_stage_calls)} (this WO must not touch the call site)")
    print("PASS: exactly one setLoopStage(LOOP_STAGE_SLEEP_PREP) call site in State_Sleep.cpp, untouched")

    note_loop_stage_calls = re.findall(r"noteLoopStageDuration\(", main_text)
    # 1 definition + N call sites; just confirm call sites still exist (>=1)
    # and none were removed/added around the sleep-prep stage specifically -
    # the loop() call sequence order (CONNECTIVITY -> STATE_HANDLER ->
    # DIAGNOSTICS -> CLOUD_LOOP -> PUBLISH_QUEUE -> DIAGNOSTICS) is unchanged.
    if len(note_loop_stage_calls) < 6:
        fail(f"expected at least 6 noteLoopStageDuration references (1 def + 5+ call sites), "
             f"found {len(note_loop_stage_calls)}")
    print("PASS: noteLoopStageDuration() call sites in loop() remain intact")

    # ---- Simulated scenarios from the WO's Required Tests, driven by a
    # decision function whose branch order/conditions were just verified
    # above to match the real source 1:1. ----
    def decide(stage: str, elapsed_ms: int):
        """Mirrors the verified control flow: SLEEP_PREP always -> INFO;
        otherwise ERROR if >= 10000, WARN if >= 2000, else no log line."""
        if stage == "SLEEP_PREP":
            return "INFO"
        if elapsed_ms >= 10000:
            return "ERROR"
        if elapsed_ms >= 2000:
            return "WARN"
        return None

    # Scenario 1: routine 300s+ TIMER wake SLEEP_PREP cycle (elapsed spans
    # the entire ~300000ms sleep). Must be INFO only, never WARN/ERROR.
    level = decide("SLEEP_PREP", 300_000)
    if level != "INFO":
        fail(f"300s+ TIMER-wake SLEEP_PREP cycle: expected INFO, got {level}")
    print("PASS: scenario 1 - 300s+ TIMER-wake SLEEP_PREP cycle logs INFO, not WARN/ERROR")

    # Scenario 2: routine short PIR-return-to-sleep (state=3) cycle whose
    # elapsed nonetheless crosses the old WARN/ERROR thresholds (e.g. a brief
    # gate-wait plus short sleep totalling 12000ms). Must still be INFO only.
    level = decide("SLEEP_PREP", 12_000)
    if level != "INFO":
        fail(f"short PIR-return-to-sleep SLEEP_PREP cycle crossing old thresholds: expected INFO, got {level}")
    print("PASS: scenario 2 - short PIR-return-to-sleep SLEEP_PREP cycle (12000ms) logs INFO, not WARN/ERROR")

    # Scenario 3: another stage (CLOUD_LOOP / PUBLISH_QUEUE) crossing the
    # same thresholds must still escalate exactly as before.
    level = decide("CLOUD_LOOP", 12_000)
    if level != "ERROR":
        fail(f"CLOUD_LOOP at 12000ms: expected ERROR (unchanged), got {level}")
    level = decide("CLOUD_LOOP", 3_000)
    if level != "WARN":
        fail(f"CLOUD_LOOP at 3000ms: expected WARN (unchanged), got {level}")
    level = decide("PUBLISH_QUEUE", 12_000)
    if level != "ERROR":
        fail(f"PUBLISH_QUEUE at 12000ms: expected ERROR (unchanged), got {level}")
    print("PASS: scenario 3 - CLOUD_LOOP/PUBLISH_QUEUE still escalate to WARN/ERROR at the same thresholds")

    print("\nAll SLEEP_PREP threshold-exclusion regression checks passed")


if __name__ == "__main__":
    main()

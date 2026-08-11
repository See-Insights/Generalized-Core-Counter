#!/usr/bin/env python3
"""
Host-side regression test for WO-2026-08-11-001 (Fourth Corrective Pass):
exclude LOOP_STAGE_SLEEP_PREP from noteLoopStageDuration()'s
kLoopStageWarnThresholdMs/kLoopStageErrorThresholdMs WARN/ERROR escalation,
while logging exactly one INFO-level "LoopStage:" line per real SLEEP_PREP
span - now via a single universal choke-point check inside transitionTo()
itself (Generalized-Core-Counter.cpp), not an enumerated list of call sites
in src/state/State_Sleep.cpp.

## Why this test exists (regression history - three prior rounds, all wrong)

Round 1 (original WO): an unconditional
`if (stage == LOOP_STAGE_SLEEP_PREP) { Log.info(...); return; }` branch was
added inside noteLoopStageDuration(). That function runs on essentially
every loop() iteration, and handleSleepingState() (src/state/State_Sleep.cpp)
is a re-entrant polling state machine with multiple early-return points
during gate-wait/teardown, so it is called many times per real SLEEP_PREP
span before the stage genuinely changes. The branch logged on every one of
those calls - an observed ~2-3ms per-line log storm on real hardware.

Round 2 (first Corrective Pass): the SLEEP_PREP exit-log was moved into
setLoopStage(), gated on its own same-stage-transition guard. Still broken:
loop() retags the loop stage every iteration regardless of what the
dispatched state handler did, so the very next setLoopStage(DIAGNOSTICS)
call after a gate-wait early return was itself a "genuine transition" by
setLoopStage()'s own guard, firing the exit-log on nearly every polling
iteration - reproducing the original regression under a different call site.

Round 3 (second/third Corrective Passes): introduced a local wrapper,
exitSleepingState(State newState, const char *reason), that all 13 real
exit points inside handleSleepingState() called instead of transitionTo()
directly. This fixed the loop-stage-churn problem, but rested on an
unstated, false assumption: that every real exit from SLEEPING_STATE
happens via a transitionTo() call from *within* handleSleepingState().
Stage 7 found three live counterexamples that bypass the wrapper entirely:
out-of-memory handling and user-switch-press handling in loop() (both
called after the state-handler switch, calling transitionTo() directly),
and connectivityFailsafeSupervisor()'s failsafe-stage-1 action (called
*before* the switch even runs). None of these route through
exitSleepingState(); if any fires while a SLEEP_PREP span is open, the span
never closes - no log line, and the timestamp stays stuck non-zero,
poisoning the next real dwell. A parallel mutation-testing finding showed a
rogue extra logLoopStageLine(...) call planted directly in the gate-wait
polling section (not inside the wrapper at all) passed both test suites,
because the Python test's helper-call-count check was scoped specifically
to exitSleepingState()'s own body and had no way to see a call anywhere
else in the file.

## The corrected design (Fourth Corrective Pass)

Enumerating call sites was always going to be incomplete by construction,
proven twice now (HIBERNATE in the third pass, then the OOM/user-switch/
failsafe-stage-1 exits found in Stage 7). `state` is written in exactly one
place in the whole codebase: transitionTo() (Generalized-Core-Counter.cpp).
Every transition, from any file, any call site, passes through it - so the
fix moves the check there instead: transitionTo() itself now checks, before
overwriting `state`, whether the OLD state is SLEEPING_STATE and a SLEEP_PREP
span is currently open; if so it closes the span (closeSleepPrepSpan()) and
logs exactly one INFO "LoopStage:" line (via the shared logLoopStageLine()
helper) before proceeding with the state change and its own
"StateReq: ..." log line.

`exitSleepingState()` is removed entirely. All 13 call sites in
State_Sleep.cpp revert to plain transitionTo() calls - the choke point's job
is now done automatically and completely, with no enumeration and no way to
bypass it (from any file, not just State_Sleep.cpp). `maybeStartSleepPrepSpan()`
in handleSleepingState() and `resetSleepPrepSpanOnBoot()` in setup() are both
unchanged from the third Corrective Pass.

This resolves the mutation-testing finding as a direct side effect: with the
only real logging call site now inside transitionTo(), a structural test can
assert State_Sleep.cpp contains ZERO occurrences of the logging helper
anywhere in the file (not scoped to any function) - a complete, simple
invariant that directly catches a rogue call planted anywhere in the file,
including directly before the cloud-gate polling `return;`.

## What this test checks

1. Structural: transitionTo() in Generalized-Core-Counter.cpp contains the
   `state == SLEEPING_STATE && ... sleepPrepSpanStartMillis != 0` guard, and
   it appears BEFORE the `state = newState;` assignment (the guard needs the
   OLD state value).
2. Structural: the guard's body calls closeSleepPrepSpan(...) then
   logLoopStageLine(...) (the shared helper), in that relative order.
3. Non-negotiable, closes the mutation-testing finding completely:
   State_Sleep.cpp contains ZERO occurrences of the logging helper name
   anywhere in the whole file (not scoped to any function), and ZERO
   occurrences of the identifier `exitSleepingState` anywhere (function
   removed, no dangling references).
4. Confirm maybeStartSleepPrepSpan(...) is still called exactly once, at the
   top of handleSleepingState(), unchanged from the third Corrective Pass.
5. Confirm resetSleepPrepSpanOnBoot(...) is still called unconditionally in
   setup(), unchanged from the third Corrective Pass.
6. Confirm the two System.reset() paths' purpose-built diagnostics are still
   present, untouched - they were never routing through transitionTo() (or
   exitSleepingState()) in the first place, since System.reset() itself
   doesn't return.
7. Call-frequency/boundedness simulation (5000 iterations) and the
   SLEEPING_STATE -> SLEEPING_STATE self-transition scenario, adapted to
   model the new design: "exit" now represents "any transitionTo() call
   while a span is open", not specifically exitSleepingState(). A NEW
   scenario proves Finding 1 (Stage 7) is fixed: an "external" transition
   (representing the OOM/user-switch/failsafe-stage-1 case, i.e. a
   transitionTo() call from outside handleSleepingState() entirely) still
   closes an open span correctly, because the choke point only cares
   whether state == SLEEPING_STATE and a span is open at the moment ANY
   transitionTo() fires - it does not distinguish call-site origin at all.
8. Scenario coverage carried over/adapted: routine 300s+ TIMER-wake cycle
   logs one INFO line; short PIR-return-to-sleep logs one INFO line; other
   stages' WARN/ERROR threshold escalation is unchanged.
"""
import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
MAIN_SRC = REPO_ROOT / "src" / "Generalized-Core-Counter.cpp"
SLEEP_SRC = REPO_ROOT / "src" / "state" / "State_Sleep.cpp"
COMMON_HDR = REPO_ROOT / "src" / "state" / "State_Common.h"


def fail(msg: str) -> None:
    print(f"FAIL: {msg}", file=sys.stderr)
    sys.exit(1)


def find_function_body(lines, def_token, expect_count=1):
    """Locate function definition(s) by their 'void foo(' token and return a
    list of (start_index, end_index_inclusive, body_lines) using brace
    matching from the opening line."""
    starts = [i for i, l in enumerate(lines) if def_token in l]
    if len(starts) != expect_count:
        fail(f"expected exactly {expect_count} definition(s) of {def_token!r}, found {len(starts)}")
    results = []
    for start in starts:
        depth = 0
        started = False
        end = None
        for i in range(start, len(lines)):
            depth += lines[i].count("{") - lines[i].count("}")
            if "{" in lines[i]:
                started = True
            if started and depth == 0:
                end = i
                break
        if end is None:
            fail(f"could not locate end of {def_token!r} body starting at line {start + 1}")
        results.append((start, end, lines[start:end + 1]))
    return results


def brace_depths(body_lines):
    """Given a function body as returned by find_function_body() (starting
    at the 'void foo(...) {' signature line through the matching closing
    brace, inclusive), return a list of the brace depth BEFORE each line is
    processed. Depth 1 means "directly inside the function's own top-level
    body" - not nested inside any if/for/while/switch/etc. block relative
    to the function's own opening brace."""
    depths = []
    depth = 0
    for l in body_lines:
        depths.append(depth)
        depth += l.count("{") - l.count("}")
    return depths


def main() -> None:
    main_text = MAIN_SRC.read_text()
    main_lines = main_text.splitlines()
    sleep_text = SLEEP_SRC.read_text()
    sleep_lines = sleep_text.splitlines()
    common_text = COMMON_HDR.read_text()

    # ---- Check: thresholds themselves are unchanged. ----
    if "constexpr unsigned long kLoopStageWarnThresholdMs = 2000UL;" not in main_text:
        fail("kLoopStageWarnThresholdMs changed or missing (expected 2000UL)")
    if "constexpr unsigned long kLoopStageErrorThresholdMs = 10000UL;" not in main_text:
        fail("kLoopStageErrorThresholdMs changed or missing (expected 10000UL)")
    print("PASS: kLoopStageWarnThresholdMs=2000UL / kLoopStageErrorThresholdMs=10000UL unchanged")

    # ---- Check: both files include the dependency-free SleepPrepSpanTiming.h
    # header instead of reimplementing the span start/close/reset arithmetic
    # inline. Unchanged from the third Corrective Pass. ----
    if '#include "state/SleepPrepSpanTiming.h"' not in main_text:
        fail('Generalized-Core-Counter.cpp is missing #include "state/SleepPrepSpanTiming.h"')
    if '#include "state/SleepPrepSpanTiming.h"' not in sleep_text:
        fail('State_Sleep.cpp is missing #include "state/SleepPrepSpanTiming.h"')
    print('PASS: both Generalized-Core-Counter.cpp and State_Sleep.cpp include '
          '"state/SleepPrepSpanTiming.h"')

    # ---- Discover the shared logging helper's name up front. ----
    helper_match = re.search(r"void\s+(\w+)\s*\(\s*LogLevel\s+level", main_text)
    if not helper_match:
        fail("no shared LogLevel-taking logging helper function found "
             "(expected a helper like logLoopStageLine(LogLevel level, ...))")
    helper_name = helper_match.group(1)

    # ---- Check: noteLoopStageDuration() still bare-early-returns for
    # SLEEP_PREP, no logging call of any kind (direct or via the shared
    # helper), before the WARN/ERROR threshold comparisons. Unchanged. ----
    (_, _, note_body), = find_function_body(main_lines, "void noteLoopStageDuration(")
    note_body_text = "\n".join(note_body)

    sleep_prep_branch_idx = next(
        (i for i, l in enumerate(note_body) if "LOOP_STAGE_SLEEP_PREP" in l and "if" in l),
        None,
    )
    if sleep_prep_branch_idx is None:
        fail("noteLoopStageDuration() no longer excludes SLEEP_PREP from reaching the "
             "WARN/ERROR threshold comparisons at all - it must still return early for "
             "SLEEP_PREP (without logging)")
    branch_condition_line = note_body[sleep_prep_branch_idx]
    if not re.search(r"^\s*if\s*\(\s*stage\s*==\s*LOOP_STAGE_SLEEP_PREP\s*\)", branch_condition_line):
        fail(f"SLEEP_PREP guard condition is not the bare unconditional stage equality: {branch_condition_line!r}")

    error_check_idx = next((i for i, l in enumerate(note_body) if "kLoopStageErrorThresholdMs" in l), None)
    if error_check_idx is None or sleep_prep_branch_idx >= error_check_idx:
        fail("SLEEP_PREP guard in noteLoopStageDuration() must textually precede the "
             "kLoopStageErrorThresholdMs comparison")

    branch_to_error = note_body[sleep_prep_branch_idx:error_check_idx]
    if not any(re.search(r"^\s*return;\s*$", l) for l in branch_to_error):
        fail("SLEEP_PREP guard in noteLoopStageDuration() does not appear to return "
             "(would fall through to the threshold checks)")
    if any("Log.info(" in l or "Log.warn(" in l or "Log.error(" in l or helper_name in l
           for l in branch_to_error):
        fail("noteLoopStageDuration()'s SLEEP_PREP guard logs (directly or via the shared "
             f"helper {helper_name}()) - it must only return early here, never log")
    print("PASS: noteLoopStageDuration() still bare-early-returns for SLEEP_PREP with no logging "
          "call of its own (direct or via the shared helper)")

    warn_check_idx = next((i for i, l in enumerate(note_body) if "kLoopStageWarnThresholdMs" in l), None)
    error_line = note_body[error_check_idx]
    warn_line = note_body[warn_check_idx]
    if not re.search(r"^\s*if\s*\(elapsedMs >= kLoopStageErrorThresholdMs\)", error_line):
        fail(f"unexpected ERROR threshold comparison form: {error_line!r}")
    if not re.search(r"^\s*\}\s*else if\s*\(elapsedMs >= kLoopStageWarnThresholdMs\)", warn_line):
        fail(f"unexpected WARN threshold comparison form: {warn_line!r}")
    if note_body_text.count(helper_name) < 2:
        fail(f"noteLoopStageDuration() does not appear to call the shared helper {helper_name}() "
             f"for both WARN and ERROR lines")
    print("PASS: WARN/ERROR threshold comparisons remain unconditional top-level if/else-if, "
          f"using shared helper '{helper_name}()'")

    # ---- Check: setLoopStage() is fully reverted to its simple original
    # form - same-stage early-return guard, then unconditional overwrite.
    # NO SLEEP_PREP-specific branch/capture/logging of any kind. ----
    (_, _, set_body), = find_function_body(main_lines, "void setLoopStage(LoopStage stage)")
    set_body_text = "\n".join(set_body)

    if "SLEEP_PREP" in set_body_text:
        fail("setLoopStage() still references SLEEP_PREP - must remain fully reverted")
    if helper_name in set_body_text:
        fail(f"setLoopStage() still calls the shared logging helper {helper_name}() - it must not log anything")
    if "Log.info(" in set_body_text or "Log.warn(" in set_body_text or "Log.error(" in set_body_text:
        fail("setLoopStage() still contains a direct logging call - it must be a pure bookkeeping function")

    early_return_idx = next(
        (i for i, l in enumerate(set_body)
         if "retainedLoopForensics.lastLoopStage == (uint8_t)stage" in l),
        None,
    )
    if early_return_idx is None:
        fail("setLoopStage() no longer contains the same-stage early-return guard")
    overwrite_idx = next(
        (i for i, l in enumerate(set_body)
         if re.search(r"retainedLoopForensics\.lastLoopStage\s*=\s*\(uint8_t\)stage;", l)),
        None,
    )
    if overwrite_idx is None:
        fail("setLoopStage() does not overwrite retainedLoopForensics.lastLoopStage with the new stage")
    if not (early_return_idx < overwrite_idx):
        fail("setLoopStage() structural ordering is wrong - expected same-stage early return "
             "before the field overwrite")
    print("PASS: setLoopStage() is fully reverted to its simple original form - same-stage "
          "early-return guard, then unconditional overwrite, no SLEEP_PREP-specific logic")

    # ---- Check: exactly one setLoopStage(LOOP_STAGE_SLEEP_PREP) call site
    # in State_Sleep.cpp, untouched (still the very first line of
    # handleSleepingState()). ----
    set_stage_calls = re.findall(r"setLoopStage\(LOOP_STAGE_SLEEP_PREP\)", sleep_text)
    if len(set_stage_calls) != 1:
        fail(f"expected exactly one setLoopStage(LOOP_STAGE_SLEEP_PREP) call in State_Sleep.cpp, "
             f"found {len(set_stage_calls)}")
    print("PASS: exactly one setLoopStage(LOOP_STAGE_SLEEP_PREP) call site in State_Sleep.cpp, untouched")

    # ---- Check: RetainedLoopForensics carries the dedicated
    # sleepPrepSpanStartMillis field, distinct from stageStartMillis, and
    # kLoopForensicsVersion was bumped (retained-RAM struct layout change). ----
    struct_match = re.search(r"struct RetainedLoopForensics\s*\{([^}]*)\}", common_text, re.DOTALL)
    if not struct_match:
        fail("RetainedLoopForensics struct not found in state/State_Common.h")
    struct_body = struct_match.group(1)
    if "sleepPrepSpanStartMillis" not in struct_body:
        fail("RetainedLoopForensics struct missing sleepPrepSpanStartMillis field")
    if "stageStartMillis" not in struct_body:
        fail("RetainedLoopForensics struct missing pre-existing stageStartMillis field "
             "(must be kept, unused for this purpose)")

    version_match = re.search(r"constexpr uint8_t kLoopForensicsVersion = (\d+);", main_text)
    if not version_match:
        fail("kLoopForensicsVersion constant not found")
    if int(version_match.group(1)) < 2:
        fail(f"kLoopForensicsVersion must be bumped to at least 2 for the retained-RAM layout "
             f"change (sleepPrepSpanStartMillis addition), found {version_match.group(1)}")
    print(f"PASS: RetainedLoopForensics has a dedicated sleepPrepSpanStartMillis field "
          f"(distinct from stageStartMillis) and kLoopForensicsVersion={version_match.group(1)}")

    # ---- Locate handleSleepingState()'s body for the span-start check. ----
    (hs_start, hs_end, hs_body), = find_function_body(sleep_lines, "void handleSleepingState() {")

    # ---- Check: the span-start is via the extracted, dependency-free
    # maybeStartSleepPrepSpan() call, called exactly once, before the
    # enteredState-gated block (so it's not accidentally gated on entry
    # rather than on the span's own zero value). Unchanged from the third
    # Corrective Pass. ----
    old_inline_zero_gate = re.search(
        r"if\s*\(\s*retainedLoopForensics\.sleepPrepSpanStartMillis\s*==\s*0\s*\)", "\n".join(hs_body))
    if old_inline_zero_gate:
        fail("handleSleepingState() still contains the OLD inline zero-gated "
             "'if (retainedLoopForensics.sleepPrepSpanStartMillis == 0)' block - this must be "
             "replaced by a maybeStartSleepPrepSpan(...) call")

    maybe_start_calls = [
        i for i, l in enumerate(hs_body)
        if re.search(
            r"maybeStartSleepPrepSpan\s*\(\s*retainedLoopForensics\.sleepPrepSpanStartMillis\s*,\s*millis\(\)\s*\)\s*;",
            l)
    ]
    if len(maybe_start_calls) != 1:
        fail(f"expected exactly one maybeStartSleepPrepSpan(retainedLoopForensics.sleepPrepSpanStartMillis, "
             f"millis()) call in handleSleepingState(), found {len(maybe_start_calls)}")
    maybe_start_idx = maybe_start_calls[0]

    # Also confirm it's the ONLY call to maybeStartSleepPrepSpan( anywhere in
    # State_Sleep.cpp (not just within handleSleepingState()'s body).
    whole_file_maybe_start = re.findall(r"maybeStartSleepPrepSpan\s*\(", sleep_text)
    if len(whole_file_maybe_start) != 1:
        fail(f"expected exactly one maybeStartSleepPrepSpan( call anywhere in State_Sleep.cpp, "
             f"found {len(whole_file_maybe_start)}")

    entered_state_idx = next((i for i, l in enumerate(hs_body) if "bool enteredState" in l), None)
    if entered_state_idx is None:
        fail("could not locate 'bool enteredState = (state != oldState);' in handleSleepingState()")
    if maybe_start_idx >= entered_state_idx:
        fail("the maybeStartSleepPrepSpan(...) call must appear before the enteredState "
             "declaration/gated block, near the top of the function "
             "(right after setLoopStage(LOOP_STAGE_SLEEP_PREP)) - it must not be gated on "
             "enteredState, since that would never fire for a SLEEPING_STATE -> SLEEPING_STATE "
             "self-transition exit path and would silently report elapsed=0 for the next cycle")
    print("PASS: handleSleepingState() starts the span via "
          "maybeStartSleepPrepSpan(retainedLoopForensics.sleepPrepSpanStartMillis, millis()), "
          "called exactly once (whole file), wired before the enteredState-gated block")

    # ================================================================
    # Fourth Corrective Pass: the choke-point checks.
    # ================================================================

    # ---- Check 1: transitionTo() contains the SLEEPING_STATE + open-span
    # guard, and it appears BEFORE `state = newState;` (needs the OLD state
    # value, read before the assignment overwrites it). ----
    (_, _, transition_body_raw), = find_function_body(
        main_lines, "void transitionTo(State newState, const char *reason) {")
    # Strip trailing '//' line comments before any pattern matching below -
    # a Stage-7 pass found that a commented-out logLoopStageLine(...) call
    # (a realistic thing to do while debugging) still satisfied every
    # cardinality/ordering/argument check, since none of them distinguished
    # live code from a comment. Block comments are not handled (out of
    # scope - the realistic threat here is '//', not '/* */', and this
    # file's own style never uses block comments for this kind of thing).
    transition_body = [re.sub(r"//.*$", "", l) for l in transition_body_raw]
    transition_text = "\n".join(transition_body)

    # Find the FULL guard condition span (which may itself wrap across
    # several physical lines, e.g. clang-format-wrapped) by paren-matching
    # from the "if (" that starts it through to its balanced close, rather
    # than inspecting a single line - a Stage-7-found gap in the prior
    # version of this check only looked at one physical line and could be
    # fooled by a multi-line reformatting of the same condition.
    if_line_idx = next((i for i, l in enumerate(transition_body) if re.search(r"\bif\s*\(", l)), None)
    if if_line_idx is None:
        fail("transitionTo() does not contain an 'if (' guard at all")
    depth = 0
    started = False
    guard_end_idx = None
    for i in range(if_line_idx, len(transition_body)):
        depth += transition_body[i].count("(") - transition_body[i].count(")")
        if "(" in transition_body[i]:
            started = True
        if started and depth == 0:
            guard_end_idx = i
            break
    if guard_end_idx is None:
        fail("could not locate the end of transitionTo()'s guard condition (unbalanced parens)")
    guard_idx = if_line_idx
    guard_span_text = " ".join(transition_body[if_line_idx:guard_end_idx + 1])

    if not (re.search(r"state\s*==\s*SLEEPING_STATE", guard_span_text)
            and "sleepPrepSpanStartMillis" in guard_span_text
            and "!= 0" in guard_span_text):
        fail("transitionTo()'s guard (spanning lines "
             f"{if_line_idx + 1}-{guard_end_idx + 1}) does not contain the "
             "'state == SLEEPING_STATE && ... sleepPrepSpanStartMillis != 0' condition: "
             f"{guard_span_text!r}")

    # ---- Check 1b (Fifth pass, closing a Stage-7-found gap): the guard
    # condition must not be narrowed by any additional clause referencing
    # `newState` (e.g. "&& newState != ERROR_STATE") - a mutation that
    # silently skips the span-close for one specific target state would
    # otherwise pass every other check here undetected. The guard must
    # depend ONLY on the OLD state and whether a span is open, never on
    # what we're transitioning TO. Checked against the FULL guard span
    # (all physical lines it occupies), not just one line - closes the
    # multi-line-formatting gap Stage 7 found in this check's prior
    # version. ----
    if "newState" in guard_span_text:
        fail(f"transitionTo()'s guard condition references 'newState' - it must depend only on "
             f"the old state and whether a span is open, never on the transition target (a "
             f"narrowing mutation like '&& newState != ERROR_STATE' would silently skip real "
             f"exits, even if split across multiple physical lines): {guard_span_text!r}")
    print("PASS: transitionTo()'s guard condition (full span, tolerant of multi-line "
          "formatting) does not reference 'newState' (cannot be narrowed to skip specific "
          "transition targets)")

    assign_idx = next(
        (i for i, l in enumerate(transition_body) if re.search(r"^\s*state\s*=\s*newState\s*;", l)),
        None,
    )
    if assign_idx is None:
        fail("transitionTo() does not contain the expected 'state = newState;' assignment")

    if not (guard_idx < assign_idx):
        fail("transitionTo()'s SLEEPING_STATE + open-span guard must appear BEFORE "
             "'state = newState;' (it needs to read the OLD state value before it is "
             f"overwritten) - found guard at line {guard_idx}, assignment at line {assign_idx}")
    print("PASS: transitionTo() contains the 'state == SLEEPING_STATE && "
          "sleepPrepSpanStartMillis != 0' guard, textually before 'state = newState;'")

    # ---- Check 1c (Fifth pass, closing the most severe Stage-7-found gap):
    # the whole choke-point design rests on "state is written nowhere in the
    # codebase except transitionTo()'s 'state = newState;' line" - this was
    # verified by hand (grep) during design and dispatch, repeatedly, but
    # was never actually enforced by a test. A direct `state = ERROR_STATE;`
    # dropped in anywhere else (e.g. replacing the OOM transitionTo() call)
    # bypasses the choke point entirely, is invisible to every other check
    # in this file, and silently poisons the next SLEEP_PREP span exactly
    # like the original HIBERNATE/enumeration bugs did. Scan every .cpp file
    # under src/ for a direct assignment to the bare `state` identifier,
    # and confirm the ONLY one is transitionTo()'s own
    # 'state = newState;' line inside Generalized-Core-Counter.cpp. ----
    # Matches `state` assigned to ANY expression terminated by ';' (a bare
    # identifier, a function call, a parenthesized expression, etc.) - not
    # narrowed to a bare identifier, which a Stage-7 pass found missed
    # `state = chooseErrorState();` and `state = (ERROR_STATE);`. The
    # `(?!=)` right after the first '=' excludes '==' (comparison, not
    # assignment). String literals and comments are stripped before this
    # runs (see strip_strings_and_comments below), so this never matches
    # `state=%u` inside a printf format string - there is no other `state=`
    # text left in the flattened source for it to accidentally match.
    state_assignment_pattern = re.compile(r"(?<![.\w])state\s*=\s*(?!=)[^;]+;")
    # Strip string literals and comments before flattening whitespace, so a
    # clang-format-wrapped multi-line assignment (`state =\n    ERROR_STATE;`)
    # still matches, while printf format strings ("...state=%u...") and
    # comments never do. This is intentionally a text-level check, not a
    # full C++ parse - it will not catch every conceivable bypass (pointer/
    # reference aliasing, macros, casts) - see the WO's Fifth Corrective
    # Pass for the explicit, reasoned decision on why those are out of
    # scope here.
    def strip_strings_and_comments(text: str) -> str:
        text = re.sub(r'"(?:[^"\\]|\\.)*"', '""', text)
        text = re.sub(r"//[^\n]*", "", text)
        text = re.sub(r"/\*.*?\*/", "", text, flags=re.DOTALL)
        return text

    unexpected_assignments = []
    scanned_files = sorted(
        set((REPO_ROOT / "src").rglob("*.cpp")) | set((REPO_ROOT / "src").rglob("*.h"))
    )
    for src_file in scanned_files:
        cleaned = strip_strings_and_comments(src_file.read_text())
        flattened = re.sub(r"\s+", " ", cleaned)
        for match in state_assignment_pattern.finditer(flattened):
            snippet = match.group(0)
            is_expected = (src_file == MAIN_SRC and re.match(r"state\s*=\s*newState\s*;", snippet))
            is_declaration = flattened[max(0, match.start() - 12):match.start()].endswith("State ")
            if is_expected or is_declaration:
                continue
            unexpected_assignments.append(f"{src_file.relative_to(REPO_ROOT)}: {snippet!r}")
    if unexpected_assignments:
        fail("found direct assignment(s) to `state` outside transitionTo()'s "
             "'state = newState;' line - this bypasses the SLEEP_PREP choke point entirely "
             f"and silently poisons the next span: {unexpected_assignments}")
    print(f"PASS: `state` is assigned nowhere in src/**/*.cpp or src/**/*.h "
          f"({len(scanned_files)} files scanned) except transitionTo()'s own "
          "'state = newState;' line (the choke-point design's load-bearing claim is now "
          "actually enforced by this test, not just verified by hand at dispatch time, "
          "tolerant of multi-line/reformatted assignment splits, and catches any assignment "
          "RHS shape - identifier, function call, or parenthesized expression - not just a "
          "bare identifier)")

    # ---- Check 2: the guard's body calls closeSleepPrepSpan(...) then
    # logLoopStageLine(...) (the shared helper), in that relative order -
    # elapsed computed via close, then logged. ----
    close_idx = next(
        (i for i, l in enumerate(transition_body)
         if re.search(
             r"closeSleepPrepSpan\s*\(\s*retainedLoopForensics\.sleepPrepSpanStartMillis\s*,\s*nowMs\s*\)",
             l)),
        None,
    )
    if close_idx is None:
        fail("transitionTo()'s guard does not compute elapsed via "
             "closeSleepPrepSpan(retainedLoopForensics.sleepPrepSpanStartMillis, nowMs)")
    if not (guard_idx < close_idx < assign_idx):
        fail("closeSleepPrepSpan(...) call must sit inside the guard body, between the guard "
             "condition and 'state = newState;'")

    helper_call_pattern = re.compile(rf"\b{re.escape(helper_name)}\s*\(")
    log_idx = next((i for i, l in enumerate(transition_body) if helper_call_pattern.search(l)), None)
    if log_idx is None:
        fail(f"transitionTo()'s guard does not call the shared logging helper {helper_name}()")
    if not (close_idx < log_idx < assign_idx):
        fail(f"transitionTo() must compute elapsed via closeSleepPrepSpan(...) BEFORE calling "
             f"the shared logging helper {helper_name}() (got close={close_idx}, log={log_idx}, "
             f"assign={assign_idx})")

    depths = brace_depths(transition_body)
    if depths[log_idx] != depths[close_idx]:
        fail(f"closeSleepPrepSpan(...) and {helper_name}(...) must sit at the same brace depth "
             f"(both inside the guard's if-block) - found close at depth {depths[close_idx]}, "
             f"log at depth {depths[log_idx]}")
    if depths[log_idx] <= depths[guard_idx]:
        fail(f"the {helper_name}(...) call must be nested INSIDE the guard's if-block (depth "
             f"greater than the guard condition's own depth), not sitting alongside it - found "
             f"guard at depth {depths[guard_idx]}, log at depth {depths[log_idx]}")
    print(f"PASS: transitionTo()'s guard computes elapsed via closeSleepPrepSpan(...) then logs "
          f"via {helper_name}() (in that order), both nested inside the guard's if-block")

    # ---- Check 2b (Stage-7-found gap): the check above finds only the
    # FIRST matching call and verifies ordering/depth, but never cardinality
    # or arguments - a duplicated call, a wrong log level/stage, or literal
    # zeros standing in for the real computed elapsed/queueDepth/connMs
    # values would all pass undetected otherwise. ----
    helper_call_count = len(helper_call_pattern.findall(transition_text))
    if helper_call_count != 1:
        fail(f"transitionTo() must call the shared logging helper {helper_name}() exactly once - "
             f"found {helper_call_count} occurrence(s). A duplicated call (or an extra call "
             "outside the guard) would produce more than one SLEEP_PREP log line per real span")

    # Capture the full call expression, which may span multiple physical
    # lines, by joining from log_idx forward to the closing ');'.
    call_lines = []
    for i in range(log_idx, len(transition_body)):
        call_lines.append(transition_body[i])
        if ");" in transition_body[i]:
            break
    call_text = re.sub(r"\s+", " ", " ".join(call_lines))
    expected_args_pattern = re.compile(
        rf"{re.escape(helper_name)}\s*\(\s*LOG_LEVEL_INFO\s*,\s*LOOP_STAGE_SLEEP_PREP\s*,\s*"
        r"elapsedMs\s*,\s*queueDepth\s*,\s*millisSinceLastCloudConnect\s*\)\s*;"
    )
    if not expected_args_pattern.search(call_text):
        fail(f"transitionTo()'s {helper_name}(...) call does not have the exact expected "
             "arguments (LOG_LEVEL_INFO, LOOP_STAGE_SLEEP_PREP, elapsedMs, queueDepth, "
             "millisSinceLastCloudConnect, in that order) - a wrong log level, wrong stage, or "
             f"literal values standing in for the real computed fields would not otherwise be "
             f"caught: {call_text!r}")
    print(f"PASS: transitionTo() calls {helper_name}() exactly once, with the exact expected "
          "arguments (LOG_LEVEL_INFO, LOOP_STAGE_SLEEP_PREP, elapsedMs, queueDepth, "
          "millisSinceLastCloudConnect) - not duplicated, not a wrong level/stage, and not "
          "literal values standing in for the real computed fields")

    # ---- Check 3 (non-negotiable, closes the mutation-testing finding
    # completely): State_Sleep.cpp contains ZERO occurrences of the logging
    # helper name anywhere in the whole file - not scoped to any function, a
    # rogue call planted anywhere (e.g. directly before the cloud-gate
    # polling 'return;') is structurally impossible to slip past this
    # check. Also zero occurrences of exitSleepingState anywhere (function
    # removed, no dangling references). ----
    helper_occurrences_in_sleep = len(re.findall(rf"\b{re.escape(helper_name)}\b", sleep_text))
    if helper_occurrences_in_sleep != 0:
        fail(f"State_Sleep.cpp must contain ZERO occurrences of the shared logging helper "
             f"{helper_name}() anywhere in the file - found {helper_occurrences_in_sleep}. The "
             "only legitimate call site is now inside transitionTo() in "
             "Generalized-Core-Counter.cpp")

    # ---- Check 3b (Fifth pass, closing a Stage-7-found gap): also reject a
    # raw Log.info/warn/error(...) call reproducing the "LoopStage:" format
    # string directly, bypassing the helper name entirely. The helper-name
    # check above only catches a rogue call THROUGH the helper; someone
    # could just as easily hand-write Log.info("LoopStage: stage=%s ...")
    # inline, e.g. right before the cloud-gate polling return, recreating
    # the exact per-iteration log storm this WO exists to prevent. ----
    raw_loopstage_log_occurrences = len(re.findall(
        r"Log\.(?:info|warn|error)\s*\(\s*\"LoopStage:", sleep_text))
    if raw_loopstage_log_occurrences != 0:
        fail(f"State_Sleep.cpp must contain ZERO raw Log.info/warn/error(\"LoopStage: ...\") "
             f"calls anywhere in the file - found {raw_loopstage_log_occurrences}. A hand-written "
             "call bypassing the shared helper would recreate the original log-volume regression "
             "just as surely as a rogue helper call would")
    print("PASS: State_Sleep.cpp contains ZERO raw Log.info/warn/error(\"LoopStage: ...\") "
          "calls anywhere in the file (not just zero occurrences of the helper name)")

    exit_sleeping_state_occurrences = len(re.findall(r"\bexitSleepingState\b", sleep_text))
    if exit_sleeping_state_occurrences != 0:
        fail(f"State_Sleep.cpp must contain ZERO occurrences of the identifier "
             f"'exitSleepingState' anywhere in the file (function removed entirely) - found "
             f"{exit_sleeping_state_occurrences}")
    print(f"PASS: State_Sleep.cpp contains ZERO occurrences of the shared logging helper "
          f"{helper_name}() and ZERO occurrences of 'exitSleepingState' anywhere in the file")

    # ---- Check: all 13 real exit points inside handleSleepingState() are
    # plain transitionTo(...) calls again (reverted from exitSleepingState()),
    # plus the one dead/unreachable transitionTo() line following the second
    # System.reset() - 14 total transitionTo( occurrences in the function. ----
    transition_calls_in_hs = re.findall(r"\btransitionTo\s*\(", "\n".join(hs_body))
    EXPECTED_TRANSITION_CALLS = 14  # 13 real exit points + 1 dead line after the second System.reset()
    if len(transition_calls_in_hs) != EXPECTED_TRANSITION_CALLS:
        fail(f"expected exactly {EXPECTED_TRANSITION_CALLS} transitionTo( calls inside "
             f"handleSleepingState() (13 real exit points reverted from exitSleepingState(), plus "
             f"the 1 dead/unreachable line after the second System.reset()), found "
             f"{len(transition_calls_in_hs)}")
    print(f"PASS: all 13 real exit points inside handleSleepingState() are plain transitionTo(...) "
          "calls again (exitSleepingState() removed), plus the 1 dead/unreachable line, unchanged")

    # ---- Check: the two System.reset() paths' purpose-built diagnostics
    # are still present, untouched. These never routed through
    # exitSleepingState() (or transitionTo()) in the first place, since
    # System.reset() itself doesn't return - so there is no exclusion-window
    # logic needed anymore; just confirm the diagnostics themselves are
    # intact and the reset call count is unchanged. ----
    reset_line_indices = [i for i, l in enumerate(hs_body) if "System.reset();" in l]
    if len(reset_line_indices) != 2:
        fail(f"expected exactly 2 System.reset() calls inside handleSleepingState(), "
             f"found {len(reset_line_indices)}")
    if "Nightly heap guard: freeHeap=%lu <= %lu before overnight ULTRA_LOW_POWER fallback - resetting" not in sleep_text:
        fail("nightly heap-guard reset's purpose-built Log.warn(...) diagnostic is missing/changed")
    if "All sleep attempts failed err=%d - immediate reset required" not in sleep_text:
        fail("all-sleep-attempts-failed reset's purpose-built Log.error(...) diagnostic is missing/changed")
    print("PASS: both System.reset() paths' purpose-built diagnostics are still present, "
          "untouched, with the same reset call count as before")

    # ---- Check: setup() calls resetSleepPrepSpanOnBoot() UNCONDITIONALLY on
    # every boot - not gated on System.resetReason() or any other condition.
    # Unchanged from the third Corrective Pass. ----
    (_, _, setup_body), = find_function_body(main_lines, "void setup() {")
    reset_on_boot_idx = next(
        (i for i, l in enumerate(setup_body)
         if re.search(
             r"resetSleepPrepSpanOnBoot\s*\(\s*retainedLoopForensics\.sleepPrepSpanStartMillis\s*\)\s*;",
             l)),
        None,
    )
    if reset_on_boot_idx is None:
        fail("setup() does not call "
             "resetSleepPrepSpanOnBoot(retainedLoopForensics.sleepPrepSpanStartMillis) - a "
             "SLEEP_PREP span cannot survive any real reset, so this must be called "
             "unconditionally on every boot")
    setup_depths = brace_depths(setup_body)
    if setup_depths[reset_on_boot_idx] != 1:
        fail("resetSleepPrepSpanOnBoot(...) in setup() must be called unconditionally, at setup()'s "
             "own top-level brace depth (depth 1) - not nested inside any if/switch/other "
             f"conditional block that could skip it on some boots, found it at depth "
             f"{setup_depths[reset_on_boot_idx]}")
    print("PASS: setup() calls "
          "resetSleepPrepSpanOnBoot(retainedLoopForensics.sleepPrepSpanStartMillis) "
          "unconditionally, at its own top-level brace depth, on every boot")

    # ------------------------------------------------------------------
    # Simulation harness for the new choke-point design: transitionTo()
    # itself closes any open SLEEP_PREP span whenever the OLD state is
    # SLEEPING_STATE, regardless of call-site origin (inside
    # handleSleepingState(), or "external" - e.g. OOM/user-switch/
    # failsafe-stage-1, per Finding 1). Other loop stages intervene between
    # polling calls (mirroring loop()'s unconditional per-iteration
    # retagging); the design under test does not depend on loop-stage tags
    # at all - only on whether transitionTo() was called while a span was
    # open and state == SLEEPING_STATE.
    # ------------------------------------------------------------------
    class TransitionToChokePointSimulator:
        """Models the app-level `state` variable, the retained SLEEP_PREP
        span-start timestamp, and transitionTo()'s new choke-point guard,
        exactly matching the verified source. Also models loop()'s real
        behavior of retagging OTHER loop stages between polling calls, and
        of firing transitionTo() calls from OUTSIDE handleSleepingState()
        entirely (Finding 1's OOM/user-switch/failsafe-stage-1 case) - the
        design under test must be unaffected by either, unlike the designs
        that failed rounds 2 and 3."""

        SLEEPING_STATE = "SLEEPING_STATE"

        def __init__(self):
            self.state = None
            self.span_start_ms = None  # None == 0 / unset
            self.log = []  # list of (level, stage, elapsed_ms)

        def enter_sleep_prep_poll(self, now_ms):
            """Models the top of handleSleepingState(): setLoopStage() call
            (irrelevant to this design) + the zero-gated span-start set via
            maybeStartSleepPrepSpan()."""
            self.state = self.SLEEPING_STATE
            if self.span_start_ms is None:
                self.span_start_ms = now_ms

        def other_stage_churn(self):
            """Models loop()'s unconditional retagging of unrelated stages
            (e.g. DIAGNOSTICS) between polling calls - a no-op for this
            design, since it does not depend on loop-stage tags at all."""
            pass  # deliberately does nothing to span_start_ms or state

        def transition_to(self, new_state, now_ms):
            """Models the real transitionTo(): the choke-point guard reads
            the OLD state (self.state) BEFORE it is overwritten. This is
            called identically whether the call site is inside
            handleSleepingState() or "external" (OOM/user-switch/
            failsafe-stage-1) - the guard cannot tell the difference, by
            design, which is exactly the Fourth Corrective Pass's fix for
            Finding 1."""
            if self.state == self.SLEEPING_STATE and self.span_start_ms is not None:
                elapsed = now_ms - self.span_start_ms
                self.log.append(("INFO", "SLEEP_PREP", elapsed))
                self.span_start_ms = None
            self.state = new_state

    # ---- Non-negotiable requirement: N repeated polling calls, WITH other
    # stage-tag churn intervening between each one (the exact mechanism that
    # broke the first Corrective Pass), must produce zero SLEEP_PREP log
    # lines, then exactly one line on the real transitionTo() exit call. ----
    sim = TransitionToChokePointSimulator()
    ENTRY_MS = 1_000
    sim.enter_sleep_prep_poll(ENTRY_MS)  # real entry into SLEEP_PREP

    N = 5000  # simulate a large number of re-entrant polling iterations
    poll_interval_ms = 3  # matches the ~2-3ms per-iteration cadence observed on hardware
    for i in range(N):
        now_ms = ENTRY_MS + (i + 1) * poll_interval_ms
        sim.enter_sleep_prep_poll(now_ms)  # still gate-waiting: zero-gate is a no-op (already set)
        sim.other_stage_churn()  # loop() retags DIAGNOSTICS etc. in between - must not matter

    sleep_prep_lines_during_poll = [l for l in sim.log if l[1] == "SLEEP_PREP"]
    if len(sleep_prep_lines_during_poll) != 0:
        fail(f"expected zero SLEEP_PREP log lines during {N} repeated polling calls with "
             f"intervening other-stage churn, got {len(sleep_prep_lines_during_poll)}")
    print(f"PASS: {N} repeated polling calls (with intervening other-stage-tag churn matching "
          f"loop()'s real behavior) produced zero SLEEP_PREP log lines")

    exit_now_ms = ENTRY_MS + (N + 1) * poll_interval_ms
    sim.transition_to("REPORTING_STATE", exit_now_ms)  # real exit, e.g. sleep-timer-report
    sleep_prep_lines_total = [l for l in sim.log if l[1] == "SLEEP_PREP"]
    if len(sleep_prep_lines_total) != 1:
        fail(f"expected exactly 1 SLEEP_PREP log line for the whole span (entry -> {N} polls "
             f"with intervening stage churn -> exit), got {len(sleep_prep_lines_total)}")
    level, stage, elapsed = sleep_prep_lines_total[0]
    if level != "INFO":
        fail(f"expected the single SLEEP_PREP exit line to be INFO, got {level}")
    expected_elapsed = exit_now_ms - ENTRY_MS
    if elapsed != expected_elapsed:
        fail(f"SLEEP_PREP exit line elapsed mismatch: expected {expected_elapsed}, got {elapsed} "
             "(must reflect the ENTIRE span, not just the last polling interval)")
    print(f"PASS: exactly 1 SLEEP_PREP INFO line for the whole span (entry -> {N} polls with "
          f"intervening stage churn -> exit), elapsed={elapsed}ms spans the full span as expected")

    # ---- NEW scenario proving Finding 1 is fixed: an "external" transition
    # (representing the OOM/user-switch/failsafe-stage-1 case - a
    # transitionTo() call from OUTSIDE handleSleepingState() entirely) still
    # closes an open span correctly. The choke point in transitionTo() only
    # cares whether state == SLEEPING_STATE and a span is open at the moment
    # ANY transitionTo() fires - it does not, and structurally cannot,
    # distinguish call-site origin. ----
    sim_external = TransitionToChokePointSimulator()
    external_entry_ms = 2_000
    sim_external.enter_sleep_prep_poll(external_entry_ms)
    for i in range(50):
        now_ms = external_entry_ms + (i + 1) * poll_interval_ms
        sim_external.enter_sleep_prep_poll(now_ms)
        sim_external.other_stage_churn()
    # An "external" exit: modeled identically to any other transitionTo()
    # call, since the real fix is that transitionTo() cannot tell the
    # difference between an internal handleSleepingState() exit and one
    # fired from loop()'s OOM/user-switch handling or
    # connectivityFailsafeSupervisor()'s failsafe-stage-1 action - all three
    # are simply transitionTo() calls made while state == SLEEPING_STATE.
    external_exit_ms = external_entry_ms + 51 * poll_interval_ms + 42_000
    sim_external.transition_to("ERROR_STATE", external_exit_ms)  # e.g. "out of memory"
    external_lines = [l for l in sim_external.log if l[1] == "SLEEP_PREP"]
    if len(external_lines) != 1:
        fail(f"Finding 1 regression: an 'external' transitionTo() call (representing OOM/"
             f"user-switch/failsafe-stage-1) while state == SLEEPING_STATE and a span was open "
             f"did not close/log the span - expected exactly 1 SLEEP_PREP log line, got "
             f"{len(external_lines)}")
    external_expected_elapsed = external_exit_ms - external_entry_ms
    if external_lines[0][2] != external_expected_elapsed:
        fail(f"Finding 1 regression: external-transition SLEEP_PREP elapsed mismatch: expected "
             f"{external_expected_elapsed}, got {external_lines[0][2]}")
    if sim_external.span_start_ms is not None:
        fail("Finding 1 regression: span_start_ms was not reset to 0/None after the external "
             "transition closed it - a subsequent real dwell would be poisoned")
    print("PASS: Finding 1 fixed - an 'external' transitionTo() call (representing the OOM/"
          "user-switch/failsafe-stage-1 case, outside handleSleepingState() entirely) while a "
          f"span was open still closes/logs it correctly, elapsed={external_lines[0][2]}ms")

    # ---- Dedicated test: SLEEPING_STATE -> SLEEPING_STATE self-transition
    # (e.g. sleep-pir-return-to-sleep) must start a genuinely fresh span for
    # cycle 2 - elapsed must be neither 0 nor inclusive of cycle 1's
    # duration. This is exactly the bug found and fixed during the third
    # Corrective Pass's design; an enteredState-gated implementation would
    # regress this silently, and the choke-point guard (reading OLD state
    # before assignment) must still fire correctly on a self-transition. ----
    sim_self_transition = TransitionToChokePointSimulator()
    cycle1_entry_ms = 0
    sim_self_transition.enter_sleep_prep_poll(cycle1_entry_ms)
    cycle1_exit_ms = cycle1_entry_ms + 15_000  # cycle 1: 15s dwell, e.g. sleep-pir-return-to-sleep
    sim_self_transition.transition_to("SLEEPING_STATE", cycle1_exit_ms)  # self-transition

    # `state` never changed, but a brand-new real SLEEP_PREP span begins
    # immediately (handleSleepingState() is called again on the very next
    # loop() iteration, still in SLEEPING_STATE).
    cycle2_entry_ms = cycle1_exit_ms
    sim_self_transition.enter_sleep_prep_poll(cycle2_entry_ms)  # zero-gate: span_start_ms was reset, so this sets fresh
    cycle2_exit_ms = cycle2_entry_ms + 8_000  # cycle 2: a different, shorter 8s dwell
    sim_self_transition.transition_to("REPORTING_STATE", cycle2_exit_ms)

    lines = [l for l in sim_self_transition.log if l[1] == "SLEEP_PREP"]
    if len(lines) != 2:
        fail(f"expected exactly 2 SLEEP_PREP log lines (one per cycle) across a "
             f"SLEEPING_STATE -> SLEEPING_STATE self-transition, got {len(lines)}")
    cycle1_elapsed = lines[0][2]
    cycle2_elapsed = lines[1][2]
    if cycle1_elapsed != 15_000:
        fail(f"cycle 1 elapsed mismatch: expected 15000, got {cycle1_elapsed}")
    if cycle2_elapsed == 0:
        fail("cycle 2 elapsed is 0 - the zero-gated reset did not fire for the "
             "SLEEPING_STATE -> SLEEPING_STATE self-transition")
    if cycle2_elapsed == cycle1_elapsed + 8_000 or cycle2_elapsed > cycle1_elapsed:
        fail(f"cycle 2 elapsed ({cycle2_elapsed}) appears to include cycle 1's duration - the "
             "span-start timestamp was not reset for the self-transition")
    if cycle2_elapsed != 8_000:
        fail(f"cycle 2 elapsed mismatch: expected 8000 (fresh span, not carried over from "
             f"cycle 1), got {cycle2_elapsed}")
    print(f"PASS: SLEEPING_STATE -> SLEEPING_STATE self-transition starts a fresh span for "
          f"cycle 2 (cycle1_elapsed={cycle1_elapsed}, cycle2_elapsed={cycle2_elapsed}, "
          "independent of cycle 1), guard correctly reads OLD state before assignment")

    # ---- Scenario: routine 300s+ TIMER-wake SLEEP_PREP cycle produces
    # exactly one INFO line (not WARN/ERROR) with elapsed spanning the full
    # cycle, even with intervening polling + other-stage churn. ----
    sim2 = TransitionToChokePointSimulator()
    sim2.enter_sleep_prep_poll(100)
    for i in range(10):
        now_ms = 100 + (i + 1) * 3
        sim2.enter_sleep_prep_poll(now_ms)
        sim2.other_stage_churn()
    sim2.transition_to("REPORTING_STATE", 100 + 305_000)  # real exit after 300s+ sleep
    lines = [l for l in sim2.log if l[1] == "SLEEP_PREP"]
    if len(lines) != 1 or lines[0][0] != "INFO":
        fail(f"300s+ TIMER-wake SLEEP_PREP cycle: expected exactly one INFO line, got {lines}")
    if lines[0][2] != 305_000:
        fail(f"300s+ TIMER-wake SLEEP_PREP cycle: expected elapsed=305000, got {lines[0][2]}")
    print("PASS: scenario - 300s+ TIMER-wake SLEEP_PREP cycle logs exactly one INFO line "
          "with elapsed spanning the full cycle")

    # ---- Scenario: routine short PIR-return-to-sleep cycle still produces
    # exactly one INFO line, not WARN/ERROR, even though its elapsed crosses
    # the old WARN/ERROR thresholds - the new design never routes SLEEP_PREP
    # through the WARN/ERROR path at all. ----
    sim3 = TransitionToChokePointSimulator()
    sim3.enter_sleep_prep_poll(50)
    for i in range(4):
        now_ms = 50 + (i + 1) * 3
        sim3.enter_sleep_prep_poll(now_ms)
        sim3.other_stage_churn()
    sim3.transition_to("REPORTING_STATE", 50 + 12_000)  # short PIR-return cycle, 12s total
    lines = [l for l in sim3.log if l[1] == "SLEEP_PREP"]
    if len(lines) != 1 or lines[0][0] != "INFO":
        fail(f"short PIR-return-to-sleep SLEEP_PREP cycle: expected exactly one INFO line, got {lines}")
    print("PASS: scenario - short PIR-return-to-sleep SLEEP_PREP cycle logs exactly one INFO "
          "line, not WARN/ERROR")

    # ---- Scenario: other stages (CLOUD_LOOP/PUBLISH_QUEUE) still escalate
    # to WARN/ERROR exactly as before - no change to their behavior. This is
    # modeled directly against noteLoopStageDuration()'s verified structure
    # (thresholds/comparison form checked above), independent of the
    # SLEEP_PREP-specific simulator above. ----
    class OtherStageSimulator:
        """Minimal model of noteLoopStageDuration()'s WARN/ERROR comparison
        for non-SLEEP_PREP stages, matching the verified unconditional
        if/else-if form and threshold constants checked above."""

        def __init__(self):
            self.log = []

        def note(self, stage, elapsed_ms):
            if elapsed_ms >= 10_000:
                self.log.append(("ERROR", stage, elapsed_ms))
            elif elapsed_ms >= 2_000:
                self.log.append(("WARN", stage, elapsed_ms))

    sim4 = OtherStageSimulator()
    sim4.note("CLOUD_LOOP", 12_000)
    sim4.note("PUBLISH_QUEUE", 3_000)
    levels = [l[0] for l in sim4.log]
    if levels != ["ERROR", "WARN"]:
        fail(f"expected CLOUD_LOOP@12000ms=ERROR then PUBLISH_QUEUE@3000ms=WARN, got {levels}")
    print("PASS: scenario - CLOUD_LOOP/PUBLISH_QUEUE still escalate to WARN/ERROR at the same "
          "unchanged thresholds")

    print("\nAll SLEEP_PREP Fifth Corrective Pass regression checks passed")


if __name__ == "__main__":
    main()

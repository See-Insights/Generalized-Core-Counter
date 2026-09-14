#!/bin/zsh
set -euo pipefail

repo_root="${0:A:h:h}"
binary="$TMPDIR/hibernate_wake_diagnostics_test"

# --- Part 1: compile and run the host-side pure-logic test ---
clang++ -std=c++17 -Wall -Wextra -pedantic \
  -I"$repo_root/src" \
  "$repo_root/tests/hibernate_wake_diagnostics_test.cpp" \
  -o "$binary"
"$binary"

# --- Part 2: fidelity checks against the real source, so the host-side
# mirror above cannot silently drift from
# src/Generalized-Core-Counter.cpp without this test failing. ---
src="$repo_root/src/Generalized-Core-Counter.cpp"

check() {
  local desc="$1"
  local pattern="$2"
  if ! grep -q -- "$pattern" "$src"; then
    echo "FIDELITY CHECK FAILED: $desc (pattern not found: $pattern)" >&2
    exit 1
  fi
}

check "gate still requires retainedHibernatePending" "if (retainedHibernatePending) {"
check "gate still requires RESET_REASON_POWER_MANAGEMENT" "reason == RESET_REASON_POWER_MANAGEMENT &&"
check "gate still requires AB1805 ALARM wake" "wakeReason == AB1805::WakeReason::ALARM &&"
check "gate still requires rtcReadOk" "rtcReadOk &&"
check "gate still requires retainedHibernateRtcBefore > 0" "retainedHibernateRtcBefore > 0 &&"
check "gate still requires retainedHibernateRequestedSleep > 0" "retainedHibernateRequestedSleep > 0 &&"
check "gate still requires rtc ordering" "rtcTime >= retainedHibernateRtcBefore) {"
check "success path still sets startupHibernateStatusReady" "startupHibernateStatusReady = true;"
check "existing serial Log.info line for success is unchanged" \
  "Log.info(\"HibernateWake: reason=%s req=%lu actual=%lu err=%ld count=%lu\","
check "existing serial Log.info line for failure is unchanged" \
  "Log.info(\"HibernateWake: pending=1 reason=%d wake=%s rtcOk=%d\","

# --- INPUT-MAPPING fidelity checks (NOT behavioural) ---
# Finding 2 (Stage 7): the host tests above exercise HibernateWakeDiagnostics'
# pure functions with GateInputs the TEST constructs itself - they cannot
# observe whether Generalized-Core-Counter.cpp populates GateInputs from the
# CORRECT local variable at each field. That mapping cannot be exercised on
# the host without either (a) stubbing AB1805/Time/PublishQueuePosix deeply
# enough to run setup() itself - which would just re-implement setup() as a
# parallel "mirror" and could drift independently of it - or (b) linking and
# running the real firmware on a Boron, which the hard constraints for this
# WO forbid ("no device operation"). Given that, the six lines below are
# checked for their EXACT right-hand-side expression, one line per
# GateInputs field, so a wrong-variable or hardcoded-literal substitution
# (e.g. the B2 mutation `gateInputs.rtcReadOk = true;`) is caught textually
# even though it cannot be caught by executing the mapping. This is honestly
# a source-fidelity check, not a behavioural one - it is called out as such
# here and in the Work Order/report rather than mislabeled.
check "gateInputs.resetReasonIsPowerManagement maps from reset reason, not a literal" \
  "gateInputs.resetReasonIsPowerManagement = (reason == RESET_REASON_POWER_MANAGEMENT);"
check "gateInputs.wakeReasonIsAlarm maps from AB1805 wake reason, not a literal" \
  "gateInputs.wakeReasonIsAlarm = (wakeReason == AB1805::WakeReason::ALARM);"
check "gateInputs.rtcReadOk maps from the real rtcReadOk local, not a literal" \
  "gateInputs.rtcReadOk = rtcReadOk;"
check "gateInputs.rtcBefore maps from retainedHibernateRtcBefore, not a literal" \
  "gateInputs.rtcBefore = (int64_t)retainedHibernateRtcBefore;"
check "gateInputs.requestedSleepSec maps from retainedHibernateRequestedSleep, not a literal" \
  "gateInputs.requestedSleepSec = retainedHibernateRequestedSleep;"
check "gateInputs.rtcAtWake maps from rtcTime, not a literal" \
  "gateInputs.rtcAtWake = (int64_t)rtcTime;"

# --- CALL-SITE ARGUMENT fidelity checks (NOT behavioural) ---
# Stage 7 pass 3: buildEventFields()'s INTERNAL field mapping is already
# covered behaviourally (see hibernate_wake_diagnostics_test.cpp's
# testBuildEventFieldsOn{Success,Failure}ForwardsEveryForensicInputFaithfully),
# but that only proves the HELPER forwards whatever arguments it is GIVEN -
# it cannot observe whether the CALL SITE below passes the correct
# production values (reason, startupHibernateWakeReason,
# retainedHibernateCount, startupHibernateActualSleepSec,
# startupHibernateSleepErrorSec) rather than a literal/wrong variable. Same
# host-testability limit as the GateInputs mapping above: exercising this
# call site behaviourally would require stubbing AB1805/Time/
# PublishQueuePosix deeply enough to run setup() itself, which would just
# re-implement setup() as a parallel mirror. So, same as above, this is an
# exact-text, non-behavioural source-fidelity check on the two call-site
# lines - not a substitute for behavioural coverage, just the next
# boundary out from what was already covered.
check "buildEventFields() call forwards reason/wakeReason/count, not literals" \
  "gateInputs, reason, startupHibernateWakeReason, retainedHibernateCount,"
check "buildEventFields() call forwards actual/error sleep values, not literals" \
  "startupHibernateActualSleepSec, startupHibernateSleepErrorSec);"

check "eventFields are built via the shared, host-tested buildEventFields(), not hand-duplicated" \
  "const HibernateWakeDiagnostics::EventFields eventFields = HibernateWakeDiagnostics::buildEventFields("

# --- publishHibernateWakeForensics() INTERNAL WIRING fidelity check
# (NOT behavioural) ---
# Next boundary out again: buildEventPayload()'s field-to-JSON emission IS
# already covered BEHAVIOURALLY (both
# testBuildEventFieldsOnSuccessForwardsEveryForensicInputFaithfully and
# testBuildEventFieldsOnFailureForwardsEveryForensicInputFaithfully call
# buildEventPayload() directly and assert every field's distinctive value
# appears correctly formatted in the resulting JSON string). What remains
# unprotected is publishHibernateWakeForensics() itself - the parameter
# `fields` it receives could, at that call site, be swapped for a
# default-constructed EventFields{} (or any other value) before being
# passed into buildEventPayload(), and no test would notice, because
# publishHibernateWakeForensics() cannot be compiled/run standalone on the
# host: it lives inside the same monolithic Generalized-Core-Counter.cpp
# as the rest of setup(), and extracting it would require the same
# depth of AB1805/Time/PublishQueuePosix stubbing already ruled out above
# as creating a parallel, drift-prone mirror rather than real coverage.
# So, same as the two check groups above: an exact-text, non-behavioural
# source-fidelity check that `fields` (the actual parameter) - not a
# literal or reconstructed struct - is what reaches buildEventPayload().
check "publishHibernateWakeForensics() forwards its fields parameter into buildEventPayload(), not a reconstructed/literal EventFields" \
  "const int written = HibernateWakeDiagnostics::buildEventPayload(payload, sizeof(payload), fields);"

check "new forensic event is queued via PublishQueuePosix, mirroring publishWatchdogForensics" \
  "PublishQueuePosix::instance().publish(\"hibernate_wake\", payload, PRIVATE))"

# Requirement: no preceding hibernate -> no event. The publish call must be
# textually inside the `if (retainedHibernatePending)` block, i.e. it must
# appear before the block's closing `retainedHibernatePending = false;`
# reset and after the block's opening line, with no unmatched `}` between -
# approximated here by checking the call appears strictly between those two
# anchors in the file.
awk '
  /if \(retainedHibernatePending\) \{/ { start=NR }
  start && /publishHibernateWakeForensics\(eventFields\);/ && !call { call=NR }
  start && /^  retainedHibernatePending = false;/ && !reset { reset=NR; exit }
  END {
    if (!start || !call || !reset) {
      print "FIDELITY CHECK FAILED: could not locate all three anchors" > "/dev/stderr";
      exit 1;
    }
    if (!(start < call && call < reset)) {
      print "FIDELITY CHECK FAILED: publishHibernateWakeForensics() call is not gated inside if (retainedHibernatePending) before it is cleared" > "/dev/stderr";
      exit 1;
    }
  }
' "$src"

echo "Fidelity checks passed: gate conditions unchanged, new hibernate_wake event mirrors publishWatchdogForensics and stays gated on retainedHibernatePending"

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
# mirror above cannot silently drift from production without this test
# failing. WO-2026-09-16 Step 2 split the production logic across two
# files - the classification itself moved into time/HibernateCycle.cpp,
# while Generalized-Core-Counter.cpp only consumes its WakeVerdict now -
# so this Part 2 checks both. ---
gcc_src="$repo_root/src/Generalized-Core-Counter.cpp"
hc_src="$repo_root/src/time/HibernateCycle.cpp"

check() {
  local desc="$1"
  local pattern="$2"
  local file="$3"
  if ! grep -q -- "$pattern" "$file"; then
    echo "FIDELITY CHECK FAILED: $desc (pattern not found in $file: $pattern)" >&2
    exit 1
  fi
}

# --- GCC.cpp: the gate now consumes classifyWake()'s WakeVerdict; it does
# not classify anything itself (WO-2026-09-14-002 Step 1 got to one
# decision site; WO-2026-09-16 Step 2 moved that one site's classification
# logic out to HibernateCycle, absorbing HibernateWakeDiagnostics'
# classifyGateArm() rather than continuing to call it - see HibernateCycle.cpp's
# own checks below for that absorbed logic). ---
check "classifyWake() is called exactly where PIN_RESET classification used to run, before setWDT()" \
  "const HibernateCycle::WakeVerdict wakeVerdict = HibernateCycle::classifyWake(reason, ab1805);" \
  "$gcc_src"
check "gate now decides via wakeVerdict.hibernateGateEvaluated, not retainedHibernatePending directly" \
  "if (wakeVerdict.hibernateGateEvaluated) {" \
  "$gcc_src"
check "success arm still checked via wakeVerdict.gateArm == GateArm::kNone" \
  "if (wakeVerdict.gateArm == HibernateWakeDiagnostics::GateArm::kNone) {" \
  "$gcc_src"
check "success path still sets startupHibernateStatusReady" "startupHibernateStatusReady = true;" "$gcc_src"
check "existing serial Log.info line for success is unchanged" \
  "Log.info(\"HibernateWake: reason=%s req=%lu actual=%lu err=%ld count=%lu\"," \
  "$gcc_src"
check "existing serial Log.info line for failure is unchanged" \
  "Log.info(\"HibernateWake: pending=1 reason=%d wake=%s rtcOk=%d\"," \
  "$gcc_src"
check "hibernate cycle is un-armed via HibernateCycle::abandon(), not a raw retainedHibernatePending write" \
  "HibernateCycle::abandon();" \
  "$gcc_src"

# --- CALL-SITE ARGUMENT fidelity checks (NOT behavioural) ---
# Stage 7 pass 3 (original WO-2026-08-29-001 round), still applicable in
# the new shape: buildEventFields()'s INTERNAL field mapping is already
# covered behaviourally (see hibernate_wake_diagnostics_test.cpp's
# testBuildEventFieldsOn{Success,Failure}ForwardsEveryForensicInputFaithfully),
# but that only proves the HELPER forwards whatever arguments it is GIVEN -
# it cannot observe whether the CALL SITE passes the correct production
# values (wakeVerdict.gateArm/gateInputs/hibernateCount,
# startupHibernateWakeReason, startupHibernateActualSleepSec,
# startupHibernateSleepErrorSec) rather than a literal/wrong variable.
# Exercising this call site behaviourally would require stubbing
# AB1805/Time/PublishQueuePosix deeply enough to run setup() itself, which
# would just re-implement setup() as a parallel mirror - so, as before,
# this is an exact-text, non-behavioural source-fidelity check.
check "buildEventFields() call forwards wakeVerdict.gateArm/gateInputs and reason/wakeReason, not literals" \
  "wakeVerdict.gateArm, wakeVerdict.gateInputs, reason, startupHibernateWakeReason," \
  "$gcc_src"
check "buildEventFields() call forwards wakeVerdict.hibernateCount and actual/error sleep values, not literals" \
  "wakeVerdict.hibernateCount, startupHibernateActualSleepSec, startupHibernateSleepErrorSec);" \
  "$gcc_src"
check "eventFields are built via the shared, host-tested buildEventFields(), not hand-duplicated" \
  "const HibernateWakeDiagnostics::EventFields eventFields = HibernateWakeDiagnostics::buildEventFields(" \
  "$gcc_src"

# --- publishHibernateWakeForensics() INTERNAL WIRING fidelity check
# (NOT behavioural) --- unchanged from prior steps: this function itself
# was not touched.
check "publishHibernateWakeForensics() forwards its fields parameter into buildEventPayload(), not a reconstructed/literal EventFields" \
  "const int written = HibernateWakeDiagnostics::buildEventPayload(payload, sizeof(payload), fields);" \
  "$gcc_src"
check "new forensic event is queued via PublishQueuePosix, mirroring publishWatchdogForensics" \
  "PublishQueuePosix::instance().publish(\"hibernate_wake\", payload, PRIVATE))" \
  "$gcc_src"

# Requirement: no preceding hibernate -> no event. The publish call must be
# textually inside the `if (wakeVerdict.hibernateGateEvaluated)` block, i.e.
# it must appear before HibernateCycle::abandon() and after the block's
# opening line, with no unmatched `}` between - approximated here by
# checking the call appears strictly between those two anchors in the file.
awk '
  /if \(wakeVerdict\.hibernateGateEvaluated\) \{/ { start=NR }
  start && /publishHibernateWakeForensics\(eventFields\);/ && !call { call=NR }
  start && /HibernateCycle::abandon\(\);/ && !reset { reset=NR; exit }
  END {
    if (!start || !call || !reset) {
      print "FIDELITY CHECK FAILED: could not locate all three anchors" > "/dev/stderr";
      exit 1;
    }
    if (!(start < call && call < reset)) {
      print "FIDELITY CHECK FAILED: publishHibernateWakeForensics() call is not gated inside if (wakeVerdict.hibernateGateEvaluated) before HibernateCycle::abandon()" > "/dev/stderr";
      exit 1;
    }
  }
' "$gcc_src"

# --- HibernateCycle.cpp: the six-condition classification and its
# GateInputs mapping live here now (WO-2026-09-16 Step 2 absorbed
# HibernateWakeDiagnostics::classifyGateArm() rather than continuing to
# call it - "absorb, don't wrap"). Same INPUT-MAPPING fidelity rationale
# as the prior step: this cannot be exercised behaviourally on the host
# without stubbing AB1805 deeply enough to reconstruct a parallel mirror,
# so these are exact-text checks, one per GateInputs field, matching this
# test's own classifyGateArmMirror() field-for-field. ---
check "in.resetReasonIsPowerManagement maps from osResetReason, not a literal" \
  "in.resetReasonIsPowerManagement = (osResetReason == RESET_REASON_POWER_MANAGEMENT);" \
  "$hc_src"
check "in.wakeReasonIsAlarm maps from the AB1805 wake reason, not a literal" \
  "in.wakeReasonIsAlarm = (wakeReason == AB1805::WakeReason::ALARM);" \
  "$hc_src"
check "in.rtcReadOk maps from the real RTC read outcome, not a literal" \
  "in.rtcReadOk = v.rtcReadOk;" \
  "$hc_src"
check "in.rtcBefore maps from the retained retainedHibernateRtcBefore field, not a literal" \
  "in.rtcBefore = (int64_t)retainedHibernateRtcBefore;" \
  "$hc_src"
check "in.requestedSleepSec maps from the retained retainedHibernateRequestedSleep field, not a literal" \
  "in.requestedSleepSec = retainedHibernateRequestedSleep;" \
  "$hc_src"
check "in.rtcAtWake maps from the real RTC read outcome, not a literal" \
  "in.rtcAtWake = v.rtcAtWake;" \
  "$hc_src"

# The six-condition classification itself must still be present, in the
# same order the test's classifyGateArmMirror() exercises - one check per
# arm, same pattern the removed classifyGateArm() was checked by before it
# moved here.
check "kResetReason check present" "v.gateArm = GateArm::kResetReason;" "$hc_src"
check "kWakeReason check present" "v.gateArm = GateArm::kWakeReason;" "$hc_src"
check "kRtcRead check present" "v.gateArm = GateArm::kRtcRead;" "$hc_src"
check "kRtcBeforeZero check present" "v.gateArm = GateArm::kRtcBeforeZero;" "$hc_src"
check "kRequestedZero check present" "v.gateArm = GateArm::kRequestedZero;" "$hc_src"
check "kRtcOrder check present" "v.gateArm = GateArm::kRtcOrder;" "$hc_src"

echo "Fidelity checks passed: gate conditions unchanged, classification absorbed into HibernateCycle.cpp, new hibernate_wake event mirrors publishWatchdogForensics and stays gated on wakeVerdict.hibernateGateEvaluated"

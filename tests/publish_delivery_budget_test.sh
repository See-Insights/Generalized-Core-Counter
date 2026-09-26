#!/bin/zsh
set -euo pipefail

# WO-2026-09-25-001, Stage 5 decision 6 / acceptance criterion 11:
# the bounded delivery wait.
#
# Part 1 compiles and runs the REAL decision module
# (src/cloud/PublishDeliveryBudget.cpp) with the REAL policy constant on the host.
# Part 2 checks the verdict is actually applied at every gate that can sleep or
# tear the cloud connection down, that an in-flight publish is never abandoned,
# and that RAM-queue events reach flash before the device sleeps.

repo_root="${0:A:h:h}"
tmp_dir="$repo_root/build-tmp"
binary="$tmp_dir/publish_delivery_budget_test_bin"

mkdir -p "$tmp_dir"
cleanup() { rm -f "$binary"; }
trap cleanup EXIT

# --- Part 1: behavioural test against the shipped module --------------------
# ConnectivityPolicy.h uses time_t for two unrelated constants; -include ctime
# supplies it on the host. Nothing else in that header or in
# PublishDeliveryBudget.cpp needs Device OS.
clang++ -std=c++17 -Wall -Wextra -pedantic -Werror \
  -include ctime \
  -I"$repo_root/src" \
  -I"$repo_root/tests/stubs" \
  "$repo_root/tests/publish_delivery_budget_test.cpp" \
  "$repo_root/src/cloud/PublishDeliveryBudget.cpp" \
  -o "$binary"
"$binary"

# --- Part 2: the verdict is applied at every gate ---------------------------
policy_hdr="$repo_root/src/power/ConnectivityPolicy.h"
gate_src="$repo_root/src/cloud/PublishDeliveryGate.cpp"
idle_src="$repo_root/src/state/State_Idle.cpp"
sleep_src="$repo_root/src/state/State_Sleep.cpp"
queue_hdr="$repo_root/lib/PublishQueuePosixRK/src/PublishQueuePosixRK.h"
queue_src="$repo_root/lib/PublishQueuePosixRK/src/PublishQueuePosixRK.cpp"

check() {
  local desc="$1"
  local pattern="$2"
  local file="$3"
  if ! grep -q -F -- "$pattern" "$file"; then
    echo "GATE CHECK FAILED: $desc (pattern not found in $file: $pattern)" >&2
    exit 1
  fi
}

refute() {
  local desc="$1"
  local pattern="$2"
  local file="$3"
  if grep -q -F -- "$pattern" "$file"; then
    echo "GATE CHECK FAILED: $desc (pattern MUST NOT appear in $file: $pattern)" >&2
    exit 1
  fi
}

# The budget is a single named constant in the existing policy location.
check "the delivery budget lives in ConnectivityPolicy" \
  "constexpr unsigned long PUBLISH_DELIVERY_BUDGET_MS = 90000UL;" "$policy_hdr"
check "the in-flight hold cap lives in ConnectivityPolicy" \
  "constexpr unsigned long PUBLISH_IN_FLIGHT_HOLD_MAX_MS = 25000UL;" "$policy_hdr"
check "the budget module reads that constant, not a literal" \
  "return ConnectivityPolicy::PUBLISH_DELIVERY_BUDGET_MS;" \
  "$repo_root/src/cloud/PublishDeliveryBudget.cpp"

# "No publish in flight" comes from the queue's own state.
check "the queue exposes its in-flight state" \
  "bool getPublishInFlight() const { return publishInFlight; };" "$queue_hdr"
check "the queue marks an accepted dispatch in flight" "publishInFlight = true;" "$queue_src"
check "the queue clears in flight only after the result is processed" \
  "publishInFlight = false;" "$queue_src"
check "the gate reads the queue's in-flight state" \
  "in.publishInFlight = PublishQueuePosix::instance().getPublishInFlight();" "$gate_src"
check "the gate reads the queue's own sleep verdict" \
  "in.queueSleepSafe = PublishQueuePosix::instance().getCanSleep();" "$gate_src"

# publishInFlight must be set inside the accepted-dispatch branch of stateWait()
# (a real Particle.publish() was started), not unconditionally.
awk '
  /^void PublishQueuePosix::stateWait\(\)/ { inFn=1; next }
  inFn && /^void PublishQueuePosix::/ { inFn=0 }
  inFn {
    n++
    if ($0 ~ /if \(BackgroundPublishRK::instance\(\)\.publish\(/ && !dispatch) dispatch=n
    if ($0 ~ /publishInFlight = true;/ && !mark) mark=n
    if ($0 ~ /PubqDispatch: e=%s/ && !reject) reject=n
  }
  END {
    if (!dispatch || !mark || !reject) { print "GATE CHECK FAILED: could not locate the stateWait() dispatch anchors" > "/dev/stderr"; exit 1 }
    if (!(dispatch < mark && mark < reject)) { print "GATE CHECK FAILED: publishInFlight is not set inside the accepted-dispatch branch" > "/dev/stderr"; exit 1 }
  }
' "$queue_src"

# publishInFlight must be cleared only after statePublishWait() has acted on the
# result, so a failed RAM event cannot be lost between the worker completing the
# future and the application requeueing/persisting it.
awk '
  /^void PublishQueuePosix::statePublishWait\(\)/ { inFn=1; next }
  inFn && /^void PublishQueuePosix::/ { inFn=0 }
  inFn && /^PublishQueuePosix::PublishQueuePosix\(\)/ { inFn=0 }
  inFn {
    n++
    if ($0 ~ /ramQueue\.push_front\(curEvent\);/ && !requeue) requeue=n
    if ($0 ~ /writeQueueToFiles\(\);/ && !persist) persist=n
    if ($0 ~ /publishInFlight = false;/ && !clear) clear=n
  }
  END {
    if (!requeue || !persist || !clear) { print "GATE CHECK FAILED: could not locate the statePublishWait() in-flight anchors" > "/dev/stderr"; exit 1 }
    if (!(requeue < clear && persist < clear)) { print "GATE CHECK FAILED: in-flight is cleared before the failed event is requeued/persisted" > "/dev/stderr"; exit 1 }
  }
' "$queue_src"

# All three gates that can sleep or disconnect use the one shared verdict, and
# none of them reads the raw queue verdict for that decision any more.
check "the IDLE low-power sleep gate uses the bounded delivery gate" \
  "bool canSleepGate = PublishDeliveryGate::queuePermitsSleep();" "$idle_src"
check "the IDLE connectivity ceiling uses the bounded delivery gate" \
  "queueCanSleep = PublishDeliveryGate::queuePermitsSleep();" "$idle_src"
check "the IDLE ceiling still treats a blocking queue as meaningful work" \
  "const bool noMeaningfulWorkRemains = !updatesPending && queueCanSleep;" "$idle_src"
check "the SLEEPING_STATE cloud-sync gate uses the bounded delivery gate" \
  "const bool queuePermitsSleep = PublishDeliveryGate::queuePermitsSleep();" "$sleep_src"
check "the cloud-sync gate's release condition includes the queue term" \
  "bool allComplete = queuePermitsSleep && ledgersSynced && updatesChecked && webhookConfirmed;" \
  "$sleep_src"
refute "the IDLE gates must not go back to the unbounded raw queue verdict" \
  "canSleepGate = PublishQueuePosix::instance().getCanSleep();" "$idle_src"

# A gate timeout must never tear the connection down with an attempt outstanding.
check "the cloud-sync gate holds for an in-flight publish past its budget" \
  "if (PublishDeliveryGate::publishInFlight()) {" "$sleep_src"
check "that hold is bounded by the policy cap" \
  "if (holdElapsedMs < ConnectivityPolicy::PUBLISH_IN_FLIGHT_HOLD_MAX_MS) {" "$sleep_src"
awk '
  /if \(PublishDeliveryGate::publishInFlight\(\)\) \{/ && !hold { hold=NR }
  /Log\.warn\("GateFail: reason=/ && !gatefail { gatefail=NR }
  END {
    if (!hold || !gatefail) { print "GATE CHECK FAILED: could not locate the in-flight hold / GateFail anchors" > "/dev/stderr"; exit 1 }
    if (!(hold < gatefail)) { print "GATE CHECK FAILED: the in-flight hold must run before the gate gives up and tears down" > "/dev/stderr"; exit 1 }
  }
' "$sleep_src"

# Every budget expiry is logged at INFO with the queued count, the elapsed time
# and whether a publish was in flight.
check "each budget expiry is logged" \
  'Log.info("DeliveryBudget: expired budget=%lums elapsed=%lums qn=%u inflight=%d",' \
  "$gate_src"
check "the expiry log fires once per episode" "if (verdict.expiryFirstObserved) {" "$gate_src"

# Before sleeping with events queued, the RAM queue is moved to flash and
# sleptWithQueued is counted - on every sleep-commit path.
check "the sleep commit flushes the RAM queue to flash" \
  "PublishQueuePosix::instance().writeQueueToFiles();" "$sleep_src"
check "the sleep commit counts a carry-over" \
  "PublishDeliveryCounters::noteSleptWithQueued();" "$sleep_src"
awk '
  /^void commitDeliveryAccountingBeforeSleep\(uint16_t qDepth, unsigned long awakeMs\) \{/ { inFn=1; next }
  inFn && /^\}/ { inFn=0 }
  inFn {
    n++
    if ($0 ~ /if \(qDepth > 0\) \{/ && !guard) guard=n
    if ($0 ~ /writeQueueToFiles\(\);/ && !flush) flush=n
    if ($0 ~ /noteSleptWithQueued\(\);/ && !slept) slept=n
    if ($0 ~ /noteQueuedAtSleep\(qDepth\);/ && !snap) snap=n
    if ($0 ~ /CycleDelivery: awake=/ && !logline) logline=n
  }
  END {
    if (!guard || !flush || !slept || !snap || !logline) { print "GATE CHECK FAILED: the shared sleep-commit helper is missing a step" > "/dev/stderr"; exit 1 }
    if (!(guard < flush && flush < slept)) { print "GATE CHECK FAILED: the RAM->flash flush must happen inside the queued-events branch, before the counter" > "/dev/stderr"; exit 1 }
    if (!(slept < snap && snap < logline)) { print "GATE CHECK FAILED: the sleep-commit helper must snapshot then log" > "/dev/stderr"; exit 1 }
  }
' "$sleep_src"

echo "Gate checks passed: the delivery budget bounds the wait at every sleep/teardown gate, an in-flight publish is never abandoned, expiries are logged, and queued events reach flash before sleep"

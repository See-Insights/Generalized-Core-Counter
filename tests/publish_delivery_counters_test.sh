#!/bin/zsh
set -euo pipefail

# WO-2026-09-25-001, Stage 5 decision 2 / acceptance criterion 4:
# publish delivery counters (attempted / acknowledged / failed / retried /
# queued-at-sleep).
#
# Part 1 compiles and runs the REAL src/cloud/PublishDeliveryCounters.cpp on the
# host (`retained` defined away, stub Particle.h).
# Part 2 checks the increment sites are wired to the production code paths -
# the host test can prove the accounting is right but not that anything calls it.

repo_root="${0:A:h:h}"
tmp_dir="$repo_root/build-tmp"
binary="$tmp_dir/publish_delivery_counters_test_bin"

mkdir -p "$tmp_dir"
cleanup() { rm -f "$binary"; }
trap cleanup EXIT

# --- Part 1: behavioural test against the shipped module --------------------
# -Dretained= removes the only Particle-ism in PublishDeliveryCounters.cpp.
# On the host the block is ordinary static storage, which is zero-initialized,
# so begin() sees a mismatched magic exactly as it would on a power-loss boot.
clang++ -std=c++17 -Wall -Wextra -pedantic -Werror \
  -Dretained= \
  -I"$repo_root/src" \
  -I"$repo_root/tests/stubs" \
  "$repo_root/tests/publish_delivery_counters_test.cpp" \
  "$repo_root/src/cloud/PublishDeliveryCounters.cpp" \
  -o "$binary"
"$binary"

# --- Part 2: increment-site wiring -----------------------------------------
queue_src="$repo_root/lib/PublishQueuePosixRK/src/PublishQueuePosixRK.cpp"
queue_hdr="$repo_root/lib/PublishQueuePosixRK/src/PublishQueuePosixRK.h"
app_src="$repo_root/src/Generalized-Core-Counter.cpp"
sleep_src="$repo_root/src/state/State_Sleep.cpp"
status_src="$repo_root/src/cloud/DeviceStatusPublisher.cpp"
counters_src="$repo_root/src/cloud/PublishDeliveryCounters.cpp"

check() {
  local desc="$1"
  local pattern="$2"
  local file="$3"
  if ! grep -q -F -- "$pattern" "$file"; then
    echo "WIRING CHECK FAILED: $desc (pattern not found in $file: $pattern)" >&2
    exit 1
  fi
}

refute() {
  local desc="$1"
  local pattern="$2"
  local file="$3"
  if grep -q -F -- "$pattern" "$file"; then
    echo "WIRING CHECK FAILED: $desc (pattern MUST NOT appear in $file: $pattern)" >&2
    exit 1
  fi
}

# The queue exposes both hooks, and both are invoked from the application
# thread - stateWait()/statePublishWait() - not from the background publish
# worker. That is what lets the counters use plain unlocked increments.
check "queue exposes the dispatch hook" "withPublishAttemptUserCallback" "$queue_hdr"
check "queue exposes the result hook" "withPublishResultUserCallback" "$queue_hdr"
check "attempt hook is fired with the effective send flags" \
  "publishAttemptUserCallback(curEvent->eventName, (uint32_t)sendFlags.value());" \
  "$queue_src"
check "result hook is fired with the Future's own result" \
  "publishResultUserCallback(curEvent->eventName, publishSuccess);" \
  "$queue_src"

# The attempt hook must fire only on an ACCEPTED dispatch (a Particle.publish()
# that actually started), i.e. inside the `if (...publish(...))` true branch and
# before the rejection `else` that logs PubqDispatch.
awk '
  /^void PublishQueuePosix::stateWait\(\)/ { inFn=1; next }
  inFn && /^void PublishQueuePosix::/ { inFn=0 }
  inFn {
    n++
    if ($0 ~ /if \(BackgroundPublishRK::instance\(\)\.publish\(/ && !dispatch) dispatch=n
    if ($0 ~ /publishAttemptUserCallback\(curEvent->eventName/ && !hook) hook=n
    if ($0 ~ /PubqDispatch: e=%s/ && !reject) reject=n
  }
  END {
    if (!dispatch || !hook || !reject) { print "WIRING CHECK FAILED: could not locate the stateWait() dispatch anchors" > "/dev/stderr"; exit 1 }
    if (!(dispatch < hook && hook < reject)) { print "WIRING CHECK FAILED: the attempt hook is not inside the accepted-dispatch branch" > "/dev/stderr"; exit 1 }
  }
' "$queue_src"

# The result hook must fire BEFORE the removal/retry decision acts on it, so a
# mutation that removes the entry early still gets counted honestly.
awk '
  /^void PublishQueuePosix::statePublishWait\(\)/ { inFn=1; next }
  inFn && /^void PublishQueuePosix::/ { inFn=0 }
  inFn && /^PublishQueuePosix::PublishQueuePosix\(\)/ { inFn=0 }
  inFn {
    n++
    if ($0 ~ /publishResultUserCallback\(curEvent->eventName, publishSuccess\);/ && !hook) hook=n
    if ($0 ~ /if \(publishSuccess\) \{/ && !success) success=n
  }
  END {
    if (!hook || !success) { print "WIRING CHECK FAILED: could not locate the statePublishWait() result anchors" > "/dev/stderr"; exit 1 }
    if (!(hook < success)) { print "WIRING CHECK FAILED: the result hook does not precede the removal decision" > "/dev/stderr"; exit 1 }
  }
' "$queue_src"

# Application registration: begin() before setup(), both hooks bound to the
# counter module.
check "retained block is validated at boot" "PublishDeliveryCounters::begin();" "$app_src"
check "attempt hook is registered" ".withPublishAttemptUserCallback(" "$app_src"
check "result hook is registered" ".withPublishResultUserCallback(" "$app_src"
check "attempt hook increments 'attempted'" "PublishDeliveryCounters::noteAttempt();" "$app_src"
check "result hook increments 'acknowledged'/'failed'" "PublishDeliveryCounters::noteResult(acknowledged);" "$app_src"

awk '
  /PublishDeliveryCounters::begin\(\);/ && !begun { begun=NR }
  /\.withFileQueueSize\(800\)/ && !inst { inst=NR }
  END {
    if (!begun || !inst) { print "WIRING CHECK FAILED: could not locate setup() anchors" > "/dev/stderr"; exit 1 }
    if (!(begun < inst)) { print "WIRING CHECK FAILED: begin() must run before the publish queue is configured" > "/dev/stderr"; exit 1 }
  }
' "$app_src"

# queued-at-sleep is recorded at the sleep commit point, from the same queue
# depth the cycle stats use.
check "queued-at-sleep is recorded at the sleep commit point" \
  "PublishDeliveryCounters::noteQueuedAtSleep(qDepth);" \
  "$sleep_src"
check "qDepth is the real queue depth" \
  "const uint16_t qDepth = (uint16_t)PublishQueuePosix::instance().getNumEvents();" \
  "$sleep_src"
check "carry-over sleeps are counted at the same commit point" \
  "PublishDeliveryCounters::noteSleptWithQueued();" \
  "$sleep_src"

# Acceptance criterion 6: awake time per cycle must be readable from a plain USB
# capture, so this line is unconditional (the existing "CYCLE end awake=" line
# is gated on verboseMode).
check "awake time and counters are logged once per cycle" \
  "Log.info(\"CycleDelivery: awake=%lums a=%u k=%u f=%u r=%u q=%u\"," \
  "$sleep_src"

# WO-2026-09-25-001 decision 7 (Stage 7 finding P2): a successful Boron overnight
# HIBERNATE resets the MCU without returning, so the accounting has to be
# committed on BOTH real sleep-commit paths - before System.sleep(), not after.
# One shared helper serves both; each System.sleep() call that commits this
# cycle must be preceded by it.
awk '
  /^void commitDeliveryAccountingBeforeSleep\(/ { helper=NR }
  /commitDeliveryAccountingBeforeSleep\($/ { calls++; if (!firstCall) firstCall=NR }
  /commitDeliveryAccountingBeforeSleep\(qDepth,/ { calls++; if (!firstCall) firstCall=NR }
  /System\.sleep\(config\)/ { if (!firstSleep) firstSleep=NR }
  /SystemSleepResult hibernateResult = System\.sleep\(config\)/ { hibernate=NR }
  END {
    if (!helper) { print "WIRING CHECK FAILED: the shared sleep-commit helper is missing" > "/dev/stderr"; exit 1 }
    if (calls < 2) { printf "WIRING CHECK FAILED: the sleep-commit helper is called %d time(s); both the HIBERNATE and the ULTRA_LOW_POWER path must call it\n", calls > "/dev/stderr"; exit 1 }
    if (!hibernate) { print "WIRING CHECK FAILED: could not locate the HIBERNATE System.sleep() call" > "/dev/stderr"; exit 1 }
    if (!(firstCall < hibernate)) { print "WIRING CHECK FAILED: the HIBERNATE path commits the delivery accounting AFTER System.sleep(), which never returns" > "/dev/stderr"; exit 1 }
  }
' "$sleep_src"

# The counters module must reconcile an attempt interrupted by a reset, so
# a == k + f + abandoned holds across a watchdog/pin/software reset.
check "an outstanding attempt is recorded" "retainedPublishDelivery.attemptOutstanding = 1;" "$counters_src"
check "boot counts an interrupted attempt as abandoned" "bump(retainedPublishDelivery.abandoned);" "$counters_src"
awk '
  /^void begin\(\) \{/ { inFn=1; next }
  inFn && /^\}/ { inFn=0 }
  inFn {
    n++
    if ($0 ~ /attemptOutstanding\) \{/ && !guard) guard=n
    if ($0 ~ /bump\(retainedPublishDelivery\.abandoned\);/ && !bump) bump=n
    if ($0 ~ /retryPending = 1;/ && !retry) retry=n
  }
  END {
    if (!guard || !bump || !retry) { print "WIRING CHECK FAILED: begin() does not reconcile an interrupted attempt into abandoned + retryPending" > "/dev/stderr"; exit 1 }
    if (!(guard < bump && bump < retry)) { print "WIRING CHECK FAILED: the abandoned reconciliation is not inside the outstanding-attempt branch" > "/dev/stderr"; exit 1 }
  }
' "$counters_src"

# Criterion 4 as amended by Stage 5 decision 5: the counters ride in the
# `status` event, NOT in the device-status ledger payload.
check "status event reads the counters, not literals" \
  "const PublishDeliveryCounters::Snapshot delivery = PublishDeliveryCounters::snapshot();" \
  "$app_src"

# Both build variants of the event (ENABLE_PMIC_FORENSICS 1 and 0) must carry
# the object, and both must be in the snprintf() that feeds publish("status").
delivery_format=',\"d\":{\"a\":%u,\"k\":%u,\"f\":%u,\"r\":%u,\"q\":%u,\"s\":%u,\"b\":%u}'
variant_count=$(grep -c -F -- "$delivery_format" "$app_src" || true)
if [[ "$variant_count" != "2" ]]; then
  echo "WIRING CHECK FAILED: expected the delivery-counter object in both status-event format strings (PMIC and non-PMIC), found $variant_count" >&2
  exit 1
fi
for field in attempted acknowledged failed retried queuedAtSleep sleptWithQueued abandoned; do
  arg_count=$(grep -c -F -- "(unsigned)delivery.$field," "$app_src" || true)
  if [[ "$arg_count" != "2" ]]; then
    echo "WIRING CHECK FAILED: counter '$field' is not passed to both status-event snprintf() calls (found $arg_count)" >&2
    exit 1
  fi
done
check "the counters go out on the status event" \
  "PublishQueuePosix::instance().publish(\"status\", status, PRIVATE);" \
  "$app_src"

# Decision 5: the ledger payload format is unchanged - a mutation that puts the
# counters back into the ledger must fail here.
refute "the ledger payload must not open a delivery object" \
  "writerBase.name(\"d\")" "$status_src"
for key in a k f r q s b; do
  refute "the ledger payload must not emit delivery key '$key'" \
    "writerBase.name(\"$key\")" "$status_src"
done
refute "the ledger payload must not read the counters at all" \
  "PublishDeliveryCounters" "$status_src"

# The counters must live in their own retained block: no SysData / CurrentData /
# SensorData layout may be involved (WO-2026-09-25-001 Stage 6 constraint).
check "counters live in a dedicated retained block" "retained RetainedPublishDelivery retainedPublishDelivery" "$counters_src"
for forbidden in 'SysData::' 'CurrentData::' 'SensorData::' 'MyPersistentData.h'; do
  if grep -q -F -- "$forbidden" "$counters_src" || grep -q -F -- "$forbidden" "$repo_root/src/cloud/PublishDeliveryCounters.h"; then
    echo "WIRING CHECK FAILED: PublishDeliveryCounters references $forbidden - persisted layouts must not be involved" >&2
    exit 1
  fi
done

echo "Wiring checks passed: attempt/result hooks fire on the application thread at the right sites, the sleep commit is captured on both sleep paths, interrupted attempts are reconciled as abandoned, all seven counters reach the status event, and none reaches the ledger payload"

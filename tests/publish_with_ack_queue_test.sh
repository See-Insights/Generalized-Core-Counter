#!/bin/zsh
set -euo pipefail

# WO-2026-09-25-001 (fix B): explicit WITH_ACK on every queued send, and queue
# removal only after an acknowledged publish Future.
#
# Part 1 runs the host mirror (tests/publish_with_ack_queue_test.cpp).
# Part 2 anchors that mirror to the real library source, so neither can drift.

repo_root="${0:A:h:h}"
tmp_dir="$repo_root/build-tmp"
binary="$tmp_dir/publish_with_ack_queue_test_bin"

mkdir -p "$tmp_dir"
cleanup() { rm -f "$binary"; }
trap cleanup EXIT

# --- Part 1: host mirror ----------------------------------------------------
clang++ -std=c++17 -Wall -Wextra -pedantic \
  "$repo_root/tests/publish_with_ack_queue_test.cpp" \
  -o "$binary"
"$binary"

# --- Part 2: fidelity checks against the real source ------------------------
queue_src="$repo_root/lib/PublishQueuePosixRK/src/PublishQueuePosixRK.cpp"

check() {
  local desc="$1"
  local pattern="$2"
  local file="$3"
  if ! grep -q -F -- "$pattern" "$file"; then
    echo "FIDELITY CHECK FAILED: $desc (pattern not found in $file: $pattern)" >&2
    exit 1
  fi
}

refute() {
  local desc="$1"
  local pattern="$2"
  local file="$3"
  if grep -q -F -- "$pattern" "$file"; then
    echo "FIDELITY CHECK FAILED: $desc (pattern unexpectedly present in $file: $pattern)" >&2
    exit 1
  fi
}

# --- Mutation (i): drop WITH_ACK -------------------------------------------
# The normalization must clear NO_ACK (which takes precedence in Device OS,
# communication/src/publisher.cpp:49-57) and set WITH_ACK, at DISPATCH so that
# events already persisted by an older PRIVATE-only build are covered too.
check "dispatch normalizes queued flags to explicit WITH_ACK with NO_ACK cleared" \
  "const PublishFlags sendFlags = (curEvent->flags & ~PublishFlags(NO_ACK)) | WITH_ACK;" \
  "$queue_src"
check "the normalized sendFlags - not the queued flags - are what is published" \
  "if (BackgroundPublishRK::instance().publish(curEvent->eventName, curEvent->eventData, sendFlags," \
  "$queue_src"
refute "the un-normalized PRIVATE-only dispatch must not come back" \
  "BackgroundPublishRK::instance().publish(curEvent->eventName, curEvent->eventData, curEvent->flags," \
  "$queue_src"

# --- Mutation (ii): remove the queue entry before the acknowledgment --------
# statePublishWait() must (a) refuse to act at all until the Future completed,
# and (b) perform every removal INSIDE the publishSuccess branch.
awk '
  /^void PublishQueuePosix::statePublishWait\(\)/ { inFn=1; next }
  inFn && /^void PublishQueuePosix::/ { inFn=0 }
  inFn && /^PublishQueuePosix::PublishQueuePosix\(\)/ { inFn=0 }
  inFn {
    n++
    if ($0 ~ /if \(!publishComplete\)/ && !guard) guard=n
    if ($0 ~ /if \(publishSuccess\) \{/ && !success) success=n
    if ($0 ~ /^    else \{/ && success && !elseLine) elseLine=n
    if ($0 ~ /fileQueue\.removeFileNum\(fileNum, false\);/ && !removeFile) removeFile=n
    if ($0 ~ /durationMs = waitBetweenPublish;/ && !dequeue) dequeue=n
    if ($0 ~ /ramQueue\.push_front\(curEvent\);/ && !requeue) requeue=n
    if ($0 ~ /durationMs = waitAfterFailure;/ && !backoff) backoff=n
    # Mutation (iv): any statement that discards the event must be confined to
    # the success branch. The failure branch may free the RAM copy of an event
    # that is already on flash (delete curEvent / curEvent = NULL), but it must
    # never touch the file queue or forget which file the event came from.
    if (elseLine) {
      if ($0 ~ /removeFileNum\(/) { badRemove=n; badRemoveText=$0 }
      if ($0 ~ /getFileFromQueue\(/) { badDequeue=n; badDequeueText=$0 }
      if ($0 ~ /curFileNum = 0;/) { badForget=n; badForgetText=$0 }
    }
  }
  END {
    if (!guard)  { print "FIDELITY CHECK FAILED: statePublishWait() no longer early-returns on !publishComplete" > "/dev/stderr"; exit 1 }
    if (!success){ print "FIDELITY CHECK FAILED: statePublishWait() no longer branches on publishSuccess" > "/dev/stderr"; exit 1 }
    if (!elseLine){ print "FIDELITY CHECK FAILED: statePublishWait() has no retry (else) branch" > "/dev/stderr"; exit 1 }
    if (!removeFile || !dequeue) { print "FIDELITY CHECK FAILED: statePublishWait() removal statements not found" > "/dev/stderr"; exit 1 }
    if (!requeue || !backoff) { print "FIDELITY CHECK FAILED: statePublishWait() retry statements not found" > "/dev/stderr"; exit 1 }
    if (!(guard < success)) { print "FIDELITY CHECK FAILED: the !publishComplete guard no longer precedes the publishSuccess branch" > "/dev/stderr"; exit 1 }
    if (!(success < removeFile && removeFile < elseLine)) { print "FIDELITY CHECK FAILED: file removal is not inside the publishSuccess branch" > "/dev/stderr"; exit 1 }
    if (!(success < dequeue && dequeue < elseLine)) { print "FIDELITY CHECK FAILED: event dequeue is not inside the publishSuccess branch" > "/dev/stderr"; exit 1 }
    if (!(elseLine < requeue)) { print "FIDELITY CHECK FAILED: the RAM requeue is not inside the retry branch" > "/dev/stderr"; exit 1 }
    if (!(elseLine < backoff)) { print "FIDELITY CHECK FAILED: waitAfterFailure backoff is not inside the retry branch" > "/dev/stderr"; exit 1 }
    if (badRemove) { printf "FIDELITY CHECK FAILED: the failure branch of statePublishWait() removes the queue file - a failed or unacknowledged publish must leave the event queued (offending line:%s)\n", badRemoveText > "/dev/stderr"; exit 1 }
    if (badDequeue) { printf "FIDELITY CHECK FAILED: the failure branch of statePublishWait() dequeues from the file queue - a failed or unacknowledged publish must leave the event queued (offending line:%s)\n", badDequeueText > "/dev/stderr"; exit 1 }
    if (badForget) { printf "FIDELITY CHECK FAILED: the failure branch of statePublishWait() clears curFileNum, losing the event'"'"'s file identity (offending line:%s)\n", badForgetText > "/dev/stderr"; exit 1 }
  }
' "$queue_src"

# A dispatch must not remove anything. The ONLY removal allowed inside
# stateWait() is the corrupted-file discard, which happens when readQueueFile()
# returned null - i.e. for a file that was never dispatched at all.
awk '
  /^void PublishQueuePosix::stateWait\(\)/ { inFn=1; next }
  inFn && /^void PublishQueuePosix::/ { inFn=0 }
  inFn {
    n++
    if ($0 ~ /discarding corrupted file/) discard=n
    if ($0 ~ /removeFileNum\(/) { removals++; if (!firstRemoval) firstRemoval=n }
    if ($0 ~ /delete curEvent;/) deletes++
  }
  END {
    if (deletes) { print "FIDELITY CHECK FAILED: stateWait() deletes the in-flight event on dispatch" > "/dev/stderr"; exit 1 }
    if (removals != 1) { printf "FIDELITY CHECK FAILED: stateWait() has %d removeFileNum() call(s); only the corrupted-file discard is allowed\n", removals > "/dev/stderr"; exit 1 }
    if (!discard || !(discard < firstRemoval)) { print "FIDELITY CHECK FAILED: stateWait()'"'"'s only removal is not the corrupted-file discard" > "/dev/stderr"; exit 1 }
  }
' "$queue_src"

# The success flag the removal branch reads must be the Future'"'"'s own result,
# not a hard-coded true.
check "publishCompleteCallback records the Future result verbatim" \
  "publishSuccess = succeeded;" \
  "$queue_src"
refute "publishSuccess must not be hard-coded true" \
  "publishSuccess = true;" \
  "$queue_src"

# --- Mutation (iii): sleep with unacknowledged events queued ---------------
# The library must not report itself sleep-safe while an event is in flight.
check "dispatch marks the queue not sleep-safe" "canSleep = false;" "$queue_src"
check "sleep-safety is derived from the queue depth, not assumed" \
  "canSleep = (getNumEvents() == 0);" \
  "$queue_src"

# Stage 4 warning: setPausePublishing() makes the queue report itself sleep-safe
# with events still pending (PublishQueuePosixRK.cpp:290-293). The application
# must never call it.
if grep -rn --include='*.cpp' --include='*.h' -F 'setPausePublishing(' "$repo_root/src" >/dev/null 2>&1; then
  echo "FIDELITY CHECK FAILED: application code calls setPausePublishing(), which reports the queue sleep-safe with events pending" >&2
  exit 1
fi

echo "Fidelity checks passed: queued sends are normalized to explicit WITH_ACK, removal is gated on an acknowledged Future, and the queue is never sleep-safe with an unacknowledged event"

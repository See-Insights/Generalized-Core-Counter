#!/usr/bin/env python3
"""
Regression/evidence test for WO-2026-08-29-002 items 6/7/8/9 - the app-side
resync gate, recurring RTC write-back, sync-recency trust signal, and the
clock-trust telemetry.

`Generalized-Core-Counter.cpp` and `cloud/DeviceStatusPublisher.cpp` cannot
be compiled standalone on the host (heavy Particle/AB1805/PublishQueue/
Ledger dependencies, no full-file stub harness - see tests/README.md for the
established precedent of tracing real shipped source directly in this
situation instead of hand-mirroring the logic, as
tests/boron_soc_commit_test.py and tests/sleep_breadcrumb_sequence_test.py
already do for other Work Orders). The monotonic-gate/trust *arithmetic*
itself is covered by tests/clock_trust_test.cpp, which compiles and runs the
real `src/time/ClockTrust.h` on the host with zero stubbing. This test
instead traces the actual shipped source to prove:

1. No new one-shot/per-boot latch was introduced to guard the RTC
   write-back. `AB1805::loop()`'s pre-existing `timeSet` latch
   (lib/AB1805_RK, untouched - vendored) is the exact defect this Work
   Order works around: it fires at most once per boot and then blocks
   every later correction. The 2nd review fix removed `clockResyncPending`
   entirely (see point 8 below) - there is no request/pending bookkeeping
   left to reintroduce that kind of latch.
2. `ab1805.setRtcFromSystem()` (item 7) is called whenever
   `Particle.timeSyncedLast()` is observed to be nonzero and to have
   advanced past the value the RTC was last written for - never gated on
   `Particle.syncTimeDone()`, which only means "no longer pending" and
   races with `timeSyncedLast()` actually advancing (see point 8).
3. `sysStatus.set_lastTimeSync(Time.now())` (item 6's Finding-1 fix) is
   stamped at that same observed-advance point, and `dailyCleanup()` no
   longer stamps it at request time (the old defect: stamping with the very
   clock under suspicion, before any sync had completed).
4. `get_lastTimeSync()` (previously zero callers) now has at least one real
   caller.
5. The item-8 gated consumer (`lastConnectionAgeSec`) requires
   `isClockTrusted()`, not `Time.isValid()` alone.
6. `isClockTrusted()` is defined in terms of
   `ClockTrust::isTrustedWrapAware()`, and `checkClockResync()`'s
   request-side gate in terms of `ClockTrust::shouldResyncWrapAware()` -
   i.e. the app wires the tested pure logic in, rather than
   re-implementing (and risking drift from) its own copy. The assertions
   below require the WRAP-AWARE variants specifically and actively reject
   the plain `isTrusted()`/`shouldResync()`: those are wrap-unsafe, and
   using them would reintroduce the Round-4 review Finding 4 defect where
   an ancient sync reads as fresh after a full ~49.7-day millis() wrap.
7. Item 9: the per-cycle `TimeDiag:` line and the ledger status payload
   (`DeviceStatusPublisher.cpp`, `LedgerPayloadStatus:` - see WO budget note,
   896-byte capacity) both carry the trust verdict and sync recency.
8. Review fix (2nd round): the 1st round's fix (gate the RTC write-back on
   `Particle.syncTimeDone()` plus a captured request-time
   `timeSyncedLast()` snapshot) was itself defective, because the system
   thread runs in this project (default since Device OS 6.2.0) and a sync
   request can complete asynchronously, arbitrarily fast - even before the
   very next `loop()` iteration observes it. `syncTimeDone()` could go true
   while `timeSyncedLast()` had not yet advanced, get misclassified as
   "did not complete", and permanently lose its only path to the RTC write
   for that sync, silently reintroducing the bug. The fix: the RTC
   write-back is now a pure function of OBSERVED `timeSyncedLast()` state
   (`lastRtcWriteSyncedLastMs` tracks the value the RTC was last written
   for), decoupled entirely from request/pending bookkeeping -
   `clockResyncPending` is removed. This also means Device-OS-initiated
   syncs at cloud handshake (which this app never explicitly requests) now
   correctly drive an RTC write too.
9. Review fix (1st round, retained): item 8's full Finding-4 setter scope.
   Two sites (`lastCountTime`, which has zero consumers) are safely gated on
   `isClockTrusted()`. Three sites (`lastAlertTime` x2, `lastHookResponse`)
   and one additional site found during that review's consumer analysis
   (`occupancyStartTime`) are deliberately left ungated and documented in
   place, because their fields are read elsewhere as "0 == never happened"
   control-flow sentinels, not just recorded values.
10. Round 4 review, Finding 1 (HIGH): the RTC write-back's tracked value
    (`lastRtcWriteSyncedLastMs`) must only advance AFTER a CONFIRMED
    successful `ab1805.setRtcFromSystem()` call - never before/unconditionally
    - so a failed write is retried on the next check rather than being
    permanently (and silently) treated as done.
11. Round 4 review, Finding 2 (HIGH): `Particle.timeSyncedLast()` must not be
    called directly by `checkClockResync()`, `isClockTrusted()`,
    `logTimeDiag()`, or the status-payload publisher - all must go through
    the non-blocking `observedTimeSyncedLastMs()` accessor.
12. Round 4 review, Finding 3 (HIGH): `setup()`'s time-validation gate must
    force `CONNECTING_STATE` not only when `!Time.isValid()`, but also when
    this device has never (across its whole persisted history) completed a
    confirmed sync (`sysStatus.get_lastTimeSync() == 0`), so a wrong RTC
    that reads `Time.isValid()==true` cannot reach a hibernate-capable state
    without ever having been forced to connect.
13. Round 4 review, Finding 4 (MEDIUM): the request- and write-back-side
    gates must use the wrap-aware `ClockTrust::shouldResyncWrapAware()` /
    `ClockTrust::isTrustedWrapAware()`, not the plain (wrap-unsafe)
    variants, so a sync more than one millis() rollover (~49.7 days) old
    cannot be misread as fresh.
14. Round 4 review, Finding 6 (MEDIUM): `State_Modes.cpp`'s COUNTING-mode
    `lastCountTime` write (previously missed by the item-8 sweep) must also
    be gated on `isClockTrusted()`.
"""
import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
APP_SRC = REPO_ROOT / "src" / "Generalized-Core-Counter.cpp"
STATUS_SRC = REPO_ROOT / "src" / "cloud" / "DeviceStatusPublisher.cpp"
COMMON_HDR = REPO_ROOT / "src" / "state" / "State_Common.h"
AB1805_CPP = REPO_ROOT / "lib" / "AB1805_RK" / "src" / "AB1805_RK.cpp"
MODES_SRC = REPO_ROOT / "src" / "state" / "State_Modes.cpp"


def fail(msg: str) -> None:
    print(f"FAIL: {msg}", file=sys.stderr)
    sys.exit(1)


def extract_function(text: str, signature_pattern: str, name: str) -> str:
    match = re.search(signature_pattern, text)
    if not match:
        fail(f"could not locate {name}() in {APP_SRC.name}")
    start = match.start()
    # The signature pattern's own trailing `\{` already matched the
    # function's opening brace, so match.end() - 1 is its position - do NOT
    # search for the next "{" from here, or a nested block's brace (e.g. an
    # `if` immediately inside the function) would be used instead, truncating
    # the extracted body at that nested block's closing brace.
    brace_start = match.end() - 1
    depth = 0
    i = brace_start
    while i < len(text):
        if text[i] == "{":
            depth += 1
        elif text[i] == "}":
            depth -= 1
            if depth == 0:
                return text[start:i + 1]
        i += 1
    fail(f"could not find matching closing brace for {name}()")
    return ""  # unreachable, satisfies type checkers


def strip_line_comments(text: str) -> str:
    """Removes `//`-prefixed comment lines before doing substring checks, so
    explanatory prose that documents an old/removed pattern (e.g. this
    file's own review-history comments mentioning `clockResyncPending`)
    does not trip a "must not exist in code" check."""
    return "\n".join(
        line for line in text.splitlines() if not line.strip().startswith("//")
    )


def main() -> None:
    app_text = APP_SRC.read_text()
    status_text = STATUS_SRC.read_text()
    common_text = COMMON_HDR.read_text()
    ab1805_text = AB1805_CPP.read_text()

    # --- Confirm the vendored AB1805::loop() latch is untouched (out of ---
    # --- scope per the Work Order) and still exists as documented.      ---
    if "lib/AB1805_RK" not in str(AB1805_CPP):
        fail("sanity check misconfigured")
    if "bool timeSet" not in (REPO_ROOT / "lib" / "AB1805_RK" / "src" / "AB1805_RK.h").read_text():
        fail("AB1805_RK.h's timeSet latch member appears to have been modified - out of scope")
    if not re.search(r"if\s*\(!timeSet\s*&&\s*Time\.isValid\(\)", ab1805_text):
        fail("AB1805::loop()'s pre-existing !timeSet gate appears to have been modified - out of scope")

    # --- 1/2/3: checkClockResync() body. ---
    check_fn = extract_function(
        app_text, r"void\s+checkClockResync\s*\(\s*\)\s*\{", "checkClockResync"
    )

    if "ab1805.setRtcFromSystem()" not in check_fn:
        fail("checkClockResync() must call ab1805.setRtcFromSystem() (item 7)")

    # --- 2nd review fix: RTC write-back must be a pure function of ---
    # --- OBSERVED timeSyncedLast() state, not of request/pending    ---
    # --- bookkeeping (clockResyncPending/syncTimeDone() raced and    ---
    # --- could silently drop the write - see the Work Order review  ---
    # --- history). No request/pending machinery may gate the write. ---
    if "clockResyncPending" in strip_line_comments(app_text):
        fail(
            "clockResyncPending must be fully removed (2nd review fix): the "
            "RTC write-back must not depend on request/pending bookkeeping, "
            "which races against the system thread and can silently drop "
            "the write-back"
        )
    if "Particle.syncTimeDone()" in check_fn:
        fail(
            "checkClockResync() must not use Particle.syncTimeDone() to gate "
            "the RTC write-back (2nd review fix) - it only means 'no longer "
            "pending', which races with Particle.timeSyncedLast() actually "
            "advancing and can misclassify a genuine, fast completion as "
            "failure"
        )
    if "lastRtcWriteSyncedLastMs" not in check_fn:
        fail(
            "checkClockResync() must track the timeSyncedLast() value the "
            "RTC was last written for (lastRtcWriteSyncedLastMs) and write "
            "the RTC whenever Particle.timeSyncedLast() is nonzero and has "
            "advanced past it - regardless of who caused the sync (2nd "
            "review fix, also required so Device-OS-initiated handshake "
            "syncs drive an RTC write)"
        )

    write_back_gate = re.search(
        r"ClockTrust::shouldAttemptRtcWriteNow\(\s*lastSyncMs\s*,\s*"
        r"lastRtcWriteSyncedLastMs\s*,\s*nowMs\s*,\s*lastRtcWriteFailedAttemptMs\s*,\s*"
        r"lastRtcWriteFailedSyncMs\s*\)",
        check_fn,
    )
    if not write_back_gate:
        fail(
            "checkClockResync()'s write-back gate must call "
            "ClockTrust::shouldAttemptRtcWriteNow(lastSyncMs, "
            "lastRtcWriteSyncedLastMs, nowMs, lastRtcWriteFailedAttemptMs, "
            "lastRtcWriteFailedSyncMs) - a pure function shared with the "
            "host tests (Round 5 cleanup task 4, Stage 7 finding 7: "
            "re-deriving the condition inline let the host tests exercise "
            "only a hand-written mirror of it, which could silently drift "
            "from the real gate) - combining the observed-state advance "
            "check (not request completion) AND the FAILED-write retry "
            "pacing keyed to the specific value that failed (Round 5 "
            "cleanup task 2 / Stage 7 finding 5, corrected Round 6 second "
            "follow-up / Stage 7 pass 4: a floor keyed to ANY attempt, "
            "including successes, wrongly withheld a distinct new sync "
            "value from the RTC if it arrived soon after a prior success)"
        )
    if "lastRtcWriteFailedAttemptMs = nowMs" not in check_fn:
        fail(
            "checkClockResync() must record lastRtcWriteFailedAttemptMs = "
            "nowMs ONLY on a FAILED write attempt (never on success, never "
            "unconditionally before the attempt) so canRetryRtcWriteNow() "
            "paces retries of that SAME failed value without ever delaying "
            "a distinct new sync value (Round 6 second follow-up)"
        )
    if "lastRtcWriteFailedSyncMs = lastSyncMs" not in check_fn:
        fail(
            "checkClockResync() must record lastRtcWriteFailedSyncMs = "
            "lastSyncMs alongside lastRtcWriteFailedAttemptMs on a FAILED "
            "write, so shouldAttemptRtcWriteNow() can tell a retry of the "
            "SAME failed value apart from a distinct new one (Round 6 "
            "second follow-up)"
        )
    if "lastRtcWriteAttemptMs = nowMs" in check_fn:
        fail(
            "checkClockResync() must NOT set lastRtcWriteAttemptMs = nowMs "
            "unconditionally before every write attempt (the Round 5/Stage "
            "7 pass 4 defect: this keyed the retry floor to successes too, "
            "wrongly withholding a distinct new sync value that arrived "
            "soon after a prior successful write) - only "
            "lastRtcWriteFailedAttemptMs, set exclusively in the failure "
            "branch, may pace retries (Round 6 second follow-up)"
        )

    # Round 4 review, Finding 1: the tracked value must be updated ONLY
    # AFTER a CONFIRMED successful RTC write (rtcUpdated == true) - never
    # unconditionally, and never before ab1805.setRtcFromSystem() is even
    # called. A failed write must leave the tracked value untouched so the
    # next check retries. This is the INVERSE of the ordering this test used
    # to require (the pre-Finding-1 defect updated the tracked value BEFORE
    # calling setRtcFromSystem(), which is exactly what let a failed write be
    # silently treated as permanently done).
    gate_idx = write_back_gate.start()
    check_fn_stripped = strip_line_comments(check_fn)
    write_back_gate_stripped = re.search(
        r"ClockTrust::shouldAttemptRtcWriteNow\(\s*lastSyncMs\s*,\s*"
        r"lastRtcWriteSyncedLastMs\s*,\s*nowMs\s*,\s*lastRtcWriteFailedAttemptMs\s*,\s*"
        r"lastRtcWriteFailedSyncMs\s*\)",
        check_fn_stripped,
    )
    gate_idx = write_back_gate_stripped.start() if write_back_gate_stripped else gate_idx
    rtc_idx = check_fn_stripped.index("ab1805.setRtcFromSystem()")
    tracked_update_idx = check_fn_stripped.index("lastRtcWriteSyncedLastMs = lastSyncMs")
    stamp_idx = check_fn_stripped.index("sysStatus.set_lastTimeSync(Time.now())")
    if not (gate_idx < rtc_idx < tracked_update_idx < stamp_idx):
        fail(
            "checkClockResync() must check the write-back gate, THEN call "
            "setRtcFromSystem(), THEN (only on confirmed success) update "
            "lastRtcWriteSyncedLastMs, THEN stamp lastTimeSync - found order "
            f"gate={gate_idx}, setRtcFromSystem={rtc_idx}, "
            f"trackedUpdate={tracked_update_idx}, set_lastTimeSync={stamp_idx} "
            "(Finding 1, Round 4 review: updating the tracked value before/"
            "regardless of the write's success permanently consumes a "
            "FAILED write, blocking retry for up to 24h)"
        )

    # Round 6 (second follow-up), Stage 7 pass 4 regression check: neither
    # failure-tracking assignment may appear BEFORE ab1805.setRtcFromSystem()
    # is called (i.e. unconditionally, before the outcome is known) - that
    # is exactly the Stage-7-pass-4 defect (recording an attempt timestamp
    # regardless of success, which re-arms the retry floor on every write
    # and wrongly delays a later distinct sync value).
    failed_attempt_idx = check_fn_stripped.find("lastRtcWriteFailedAttemptMs = nowMs")
    failed_sync_idx = check_fn_stripped.find("lastRtcWriteFailedSyncMs = lastSyncMs")
    if failed_attempt_idx != -1 and failed_attempt_idx < rtc_idx:
        fail(
            "lastRtcWriteFailedAttemptMs = nowMs must not appear before "
            "ab1805.setRtcFromSystem() is called - it must only be set in "
            "the failed-write else branch, AFTER the outcome is known "
            "(Round 6 second follow-up regression check, Stage 7 pass 4)"
        )
    if failed_sync_idx != -1 and failed_sync_idx < rtc_idx:
        fail(
            "lastRtcWriteFailedSyncMs = lastSyncMs must not appear before "
            "ab1805.setRtcFromSystem() is called - it must only be set in "
            "the failed-write else branch, AFTER the outcome is known "
            "(Round 6 second follow-up regression check, Stage 7 pass 4)"
        )

    # The tracked-value update and the lastTimeSync stamp must both be
    # gated behind the write's confirmed-success branch, not merely follow
    # it textually (a `rtcUpdated = true;` unconditional assignment right
    # before an unguarded update would satisfy the ordering check above
    # while still reintroducing Finding 1). Require an explicit
    # `if (rtcUpdated)` guard wrapping both statements.
    confirm_guard = re.search(
        r"if\s*\(\s*rtcUpdated\s*\)\s*\{([^}]*)\}",
        check_fn,
        re.DOTALL,
    )
    if not confirm_guard:
        fail(
            "checkClockResync() must guard the tracked-value update and "
            "lastTimeSync stamp behind `if (rtcUpdated)` (Finding 1) so a "
            "failed write does not advance either"
        )
    guarded_body = confirm_guard.group(1)
    if "lastRtcWriteSyncedLastMs = lastSyncMs" not in guarded_body:
        fail("the lastRtcWriteSyncedLastMs update must be inside the `if (rtcUpdated)` guard (Finding 1)")
    if "sysStatus.set_lastTimeSync(Time.now())" not in guarded_body:
        fail("the sysStatus.set_lastTimeSync() stamp must be inside the `if (rtcUpdated)` guard (Finding 1)")

    # Round 6 (second follow-up), Stage 7 pass 4: the failure-only tracking
    # writes must be OUTSIDE the success guard (i.e. in the paired `else`),
    # never alongside the success-path updates above - putting them inside
    # the `if (rtcUpdated)` guard would mean a SUCCESS also arms the retry
    # floor, reintroducing the exact defect this round fixes (a distinct
    # new sync withheld because a recent SUCCESSFUL write set the floor).
    if "lastRtcWriteFailedAttemptMs = nowMs" in guarded_body:
        fail(
            "lastRtcWriteFailedAttemptMs must NOT be set inside the "
            "`if (rtcUpdated)` (success) guard - it must only be set on "
            "the FAILED-write else branch, or a successful write would "
            "re-arm the retry floor and wrongly delay a later distinct "
            "sync value (Round 6 second follow-up regression check)"
        )
    if "lastRtcWriteFailedSyncMs = lastSyncMs" in guarded_body:
        fail(
            "lastRtcWriteFailedSyncMs must NOT be set inside the "
            "`if (rtcUpdated)` (success) guard - it must only be set on "
            "the FAILED-write else branch (Round 6 second follow-up "
            "regression check)"
        )
    else_guard = re.search(
        r"if\s*\(\s*rtcUpdated\s*\)\s*\{[^}]*\}\s*else\s*\{([^}]*)\}",
        check_fn,
        re.DOTALL,
    )
    if not else_guard:
        fail(
            "checkClockResync() must have an `else` branch paired with the "
            "`if (rtcUpdated)` guard that records the failure-only "
            "lastRtcWriteFailedAttemptMs/lastRtcWriteFailedSyncMs tracking "
            "(Round 6 second follow-up)"
        )
    else_body = else_guard.group(1)
    if "lastRtcWriteFailedAttemptMs = nowMs" not in else_body:
        fail(
            "the `else` (failed-write) branch must set "
            "lastRtcWriteFailedAttemptMs = nowMs (Round 6 second follow-up)"
        )
    if "lastRtcWriteFailedSyncMs = lastSyncMs" not in else_body:
        fail(
            "the `else` (failed-write) branch must set "
            "lastRtcWriteFailedSyncMs = lastSyncMs (Round 6 second "
            "follow-up)"
        )

    # Round 5 cleanup task 3 (Stage 7 finding 3): the confirmed-write branch
    # must flag a deferred status republish so the ledger eventually shows
    # trusted=true after the corrective sync succeeds (the status payload
    # published on connect necessarily read trusted=false, before this
    # sync). Must go through Cloud's new public entry point, not touch
    # pendingStatusPublish directly (that would bypass the encapsulation
    # this task explicitly asked for) and must not publish synchronously
    # (no direct writeDeviceStatusToCloud() call here - only the deferred
    # loop should ever do that for this branch).
    if "Cloud::instance().requestStatusPublish(" not in guarded_body:
        fail(
            "checkClockResync()'s confirmed-write branch (inside "
            "`if (rtcUpdated)`) must call "
            "Cloud::instance().requestStatusPublish(...) so the status "
            "payload eventually republishes trusted=true (Round 5 cleanup "
            "task 3, Stage 7 finding 3) - it must NOT call "
            "writeDeviceStatusToCloud() directly (synchronous) or touch "
            "pendingStatusPublish directly (bypasses the new Cloud API)"
        )
    if "pendingStatusPublish" in strip_line_comments(check_fn):
        fail(
            "checkClockResync() must not touch Cloud's pendingStatusPublish "
            "flag directly - it must go through the public "
            "requestStatusPublish() entry point (Round 5 cleanup task 3)"
        )

    if "ClockTrust::shouldResyncWrapAware(" not in check_fn:
        fail(
            "checkClockResync()'s request-side gate must use "
            "ClockTrust::shouldResyncWrapAware(), not the plain "
            "ClockTrust::shouldResync() (Finding 4, Round 4 review: the "
            "plain gate can misread an ancient sync as fresh after a full "
            "millis() rollover)"
        )

    # --- Round 4 review, Finding 2: no direct, potentially-blocking calls. ---
    if "Particle.timeSyncedLast()" in check_fn:
        fail(
            "checkClockResync() must not call Particle.timeSyncedLast() "
            "directly (Finding 2, Round 4 review) - it can block the "
            "calling thread mid-connect; use observedTimeSyncedLastMs()"
        )
    if "observedTimeSyncedLastMs()" not in check_fn:
        fail("checkClockResync() must read the sync timestamp via observedTimeSyncedLastMs() (Finding 2)")

    # --- Retry pacing on the request side (kept from the 1st review fix). ---
    if "ClockTrust::canRetryResyncNow(" not in check_fn:
        fail(
            "checkClockResync()'s automatic-retry gate must be paced by "
            "ClockTrust::canRetryResyncNow() so a device whose sync never "
            "completes does not re-request on every loop() iteration"
        )
    if "clockResyncLastAttemptMs" not in app_text:
        fail("checkClockResync()/requestClockResync() must track clockResyncLastAttemptMs for retry pacing")

    # --- dailyCleanup() must no longer stamp lastTimeSync at request time. ---
    daily_fn = extract_function(
        app_text, r"void\s+dailyCleanup\s*\(\s*\)\s*\{", "dailyCleanup"
    )
    daily_code_lines = [
        line for line in daily_fn.splitlines() if not line.strip().startswith("//")
    ]
    daily_code_text = "\n".join(daily_code_lines)
    if "sysStatus.set_lastTimeSync" in daily_code_text:
        fail(
            "dailyCleanup() must not stamp sysStatus.set_lastTimeSync() directly "
            "(Finding 1's request-time-stamp defect) - completion must be "
            "recorded exactly once, by checkClockResync()"
        )
    if "requestClockResync(" not in daily_code_text:
        fail("dailyCleanup() should route its forced sync through requestClockResync()")

    # --- Round 5 cleanup task 4 (Stage 7 finding 7): requestClockResync()
    # itself must actually call Particle.syncTime() - a pure caller-side
    # check (checkClockResync() calling requestClockResync()) would still
    # pass if requestClockResync() stopped issuing the request altogether,
    # since nothing observes Device OS state from the caller's side. This
    # closes that gap by extracting requestClockResync()'s own body and
    # requiring the real API call textually within it, and requiring the
    # pacing bookkeeping (clockResyncLastAttemptMs) to be updated in the
    # same function, not merely referenced elsewhere.
    request_fn = extract_function(
        app_text,
        r"void\s+requestClockResync\s*\(\s*const\s+char\s*\*\s*\w+\s*\)\s*\{",
        "requestClockResync",
    )
    request_code_text = strip_line_comments(request_fn)
    if "Particle.syncTime()" not in request_code_text:
        fail(
            "requestClockResync() must actually call Particle.syncTime() "
            "(Round 5 cleanup task 4, Stage 7 finding 7: a caller-side-only "
            "check would still pass if this function stopped issuing the "
            "request)"
        )
    if "clockResyncLastAttemptMs = millis()" not in request_code_text:
        fail(
            "requestClockResync() must stamp clockResyncLastAttemptMs = "
            "millis() itself so the retry floor is paced from the actual "
            "request, not merely referenced elsewhere in the file"
        )

    # --- get_lastTimeSync() must have gained a real caller. Real code only -
    # --- explanatory comments mentioning the name (e.g. this file's own    -
    # --- review-history prose) must not inflate the count, or the check   -
    # --- would still pass even if the actual callers were deleted.        -
    get_last_time_sync_callers = len(
        re.findall(r"get_lastTimeSync\(\)", strip_line_comments(app_text))
    ) + len(re.findall(r"get_lastTimeSync\(\)", strip_line_comments(status_text)))
    if get_last_time_sync_callers < 1:
        fail("sysStatus.get_lastTimeSync() must have at least one real (non-comment) caller (item 6)")

    # --- item 8: the confirmed regression site must require isClockTrusted(). ---
    lastconn_match = re.search(
        r"const\s+long\s+lastConnectionAgeSec\s*=\s*\n\s*\((.*?)\)\s*\n\s*\?",
        app_text,
        re.DOTALL,
    )
    if not lastconn_match:
        fail("could not locate the lastConnectionAgeSec computation")
    lastconn_condition = lastconn_match.group(1)
    if "isClockTrusted()" not in lastconn_condition:
        fail(
            "lastConnectionAgeSec's gating condition must require isClockTrusted() "
            f"(Finding 3: Time.isValid() alone is not sufficient); found: "
            f"{lastconn_condition!r}"
        )
    if re.search(r"(?<!is)Time\.isValid\(\)", lastconn_condition):
        fail(
            "lastConnectionAgeSec's gating condition should no longer reference "
            f"a bare Time.isValid() check; found: {lastconn_condition!r}"
        )

    # --- isClockTrusted() must be defined in terms of ClockTrust::isTrustedWrapAware(). ---
    trusted_fn = extract_function(
        app_text, r"bool\s+isClockTrusted\s*\(\s*\)\s*\{", "isClockTrusted"
    )
    if "ClockTrust::isTrustedWrapAware(" not in trusted_fn:
        fail(
            "isClockTrusted() must delegate to ClockTrust::isTrustedWrapAware(), "
            "not a hand-rolled copy or the plain (wrap-unsafe) isTrusted() "
            "(Finding 4, Round 4 review)"
        )
    if "Particle.timeSyncedLast()" in trusted_fn:
        fail(
            "isClockTrusted() must not call Particle.timeSyncedLast() directly "
            "(Finding 2, Round 4 review) - use observedTimeSyncedLastMs()"
        )
    if "observedTimeSyncedLastMs()" not in trusted_fn:
        fail("isClockTrusted() must read the sync timestamp via observedTimeSyncedLastMs() (Finding 2)")
    if "isClockTrusted();" not in common_text and "bool isClockTrusted();" not in common_text:
        fail("isClockTrusted() must be declared in state/State_Common.h so other translation units (DeviceStatusPublisher.cpp) can call it")

    # --- item 9: TimeDiag line carries the trust verdict + sync recency. ---
    time_diag_fn = extract_function(
        app_text, r"void\s+logTimeDiag\s*\(bool\s+isOpen\)\s*\{", "logTimeDiag"
    )
    if "trusted=%d" not in time_diag_fn or "syncAgeMs=%lu" not in time_diag_fn:
        fail("logTimeDiag()'s TimeDiag: line must report trusted=/syncAgeMs= (item 9)")
    if "isClockTrusted()" not in time_diag_fn:
        fail("logTimeDiag() must compute its trust field via isClockTrusted()")
    if "Particle.timeSyncedLast()" in strip_line_comments(time_diag_fn):
        fail(
            "logTimeDiag() must not call Particle.timeSyncedLast() directly "
            "(Finding 2, Round 4 review) - use observedTimeSyncedLastMs()"
        )

    # Round 6 (Stage 7 finding 1): the reported syncAgeMs= value must come
    # from reportedSyncAgeMs() - the wrap-aware, sentinel-bearing telemetry
    # function - not a raw ClockTrust::elapsedMs() recomputation, which
    # would silently reintroduce both defects (uptime misreported as sync
    # age before any sync, and a small deceptively-fresh value after a full
    # millis() wrap).
    time_diag_fn_stripped = strip_line_comments(time_diag_fn)
    if "reportedSyncAgeMs()" not in time_diag_fn_stripped:
        fail(
            "logTimeDiag() must report syncAgeMs via reportedSyncAgeMs() "
            "(Round 6, Stage 7 finding 1) - not a raw ClockTrust::elapsedMs() "
            "recomputation, which can misreport uptime as sync age before "
            "any sync, or wrap back around to a small stale-looking value "
            "after a full millis() rollover"
        )
    if "ClockTrust::elapsedMs(" in time_diag_fn_stripped:
        fail(
            "logTimeDiag() must not call ClockTrust::elapsedMs() directly to "
            "compute the reported sync age (Round 6, Stage 7 finding 1) - "
            "that raw computation is exactly the defect reportedSyncAgeMs() "
            "fixes; use reportedSyncAgeMs() instead"
        )

    # --- item 9: ledger status payload carries the same fields, within  ---
    # --- budget. Scoped to the actual JSON-writer block that emits the   ---
    # --- "clock" object (not a file-wide substring match, which comments ---
    # --- or unrelated code could satisfy) - anchored on the              ---
    # --- writerBase.name("clock").beginObject()/...endObject() pair.     ---
    clock_block_match = re.search(
        r'writerBase\.name\("clock"\)\.beginObject\(\)(.*?)writerBase\.endObject\(\)',
        status_text,
        re.DOTALL,
    )
    if not clock_block_match:
        fail("DeviceStatusPublisher.cpp's ledger status payload must add a clock trust object (item 9)")
    clock_block = clock_block_match.group(1)
    if "isClockTrusted()" not in clock_block:
        fail("the ledger status payload's clock object must use isClockTrusted(), not a re-derived copy")
    if "syncAgeSec" not in clock_block or "lastSyncEpoch" not in clock_block:
        fail("the ledger status payload's clock object must report syncAgeSec and lastSyncEpoch (item 9)")
    if "Particle.timeSyncedLast()" in strip_line_comments(status_text):
        fail(
            "DeviceStatusPublisher.cpp must not call Particle.timeSyncedLast() "
            "directly (Finding 2, Round 4 review) - use observedTimeSyncedLastMs()"
        )

    # Round 6 (Stage 7 finding 1): syncAgeSec must be derived from
    # reportedSyncAgeMs()'s sentinel, not a raw elapsedMs()-based
    # computation gated only on "== 0" (which is exactly the pre-fix
    # pattern that misreported a small value after a full millis() wrap).
    status_block_stripped = strip_line_comments(status_text)
    if "reportedSyncAgeMs()" not in status_block_stripped:
        fail(
            "DeviceStatusPublisher.cpp must derive syncAgeSec from "
            "reportedSyncAgeMs() (Round 6, Stage 7 finding 1), not by "
            "calling observedTimeSyncedLastMs() and ClockTrust::elapsedMs() "
            "directly - the wrap-unsafe pattern this finding fixes"
        )
    if "ClockTrust::kReportedSyncAgeUnavailableMs" not in status_block_stripped:
        fail(
            "DeviceStatusPublisher.cpp must compare against "
            "ClockTrust::kReportedSyncAgeUnavailableMs to decide when to "
            "report syncAgeSec's -1 sentinel (Round 6, Stage 7 finding 1)"
        )
    if "DEVICE_STATUS_PAYLOAD_CAPACITY" not in status_text:
        fail("sanity check: DEVICE_STATUS_PAYLOAD_CAPACITY reference missing from DeviceStatusPublisher.cpp")

    # --- Change 2 (review fix): item 8's full Finding-4 setter scope. ---
    # Two sites are safe to gate (lastCountTime has zero consumers anywhere
    # in the codebase, so writing a 0 sentinel when untrusted changes only
    # the recorded value, never control flow):
    persistent_text = (REPO_ROOT / "src" / "MyPersistentData.cpp").read_text()
    sleep_text = (REPO_ROOT / "src" / "state" / "State_Sleep.cpp").read_text()

    if "current.set_lastCountTime(isClockTrusted() ? Time.now() : 0)" not in persistent_text:
        fail(
            "MyPersistentData.cpp's resetEverything() must gate "
            "set_lastCountTime() on isClockTrusted() (Change 2: lastCountTime "
            "has no consumers, so this is a safe, in-scope Finding-4 fix)"
        )
    if "current.set_lastCountTime(isClockTrusted() ? Time.now() : 0)" not in sleep_text:
        fail(
            "State_Sleep.cpp's PIR-wake counting path must gate "
            "set_lastCountTime() on isClockTrusted() (Change 2)"
        )

    # --- Round 4 review, Finding 6: State_Modes.cpp's COUNTING-mode ---
    # --- lastCountTime write (missed by the earlier item-8 sweep) must  ---
    # --- also be gated, identically to the other two lastCountTime      ---
    # --- sites above (same field, same "no consumers" analysis).        ---
    modes_text = MODES_SRC.read_text()
    if "current.set_lastCountTime(isClockTrusted() ? Time.now() : 0)" not in modes_text:
        fail(
            "State_Modes.cpp's COUNTING-mode handler must gate "
            "set_lastCountTime() on isClockTrusted() (Finding 6, Round 4 "
            "review)"
        )

    # Three sites must be DELIBERATELY left ungated, because their shared
    # fields (lastAlertTime, lastHookResponse) are read elsewhere as
    # control-flow sentinels ("0 == never happened"), where writing 0 due to
    # an untrusted clock would silently change branching (bypassing
    # State_Report.cpp's alert-40 escalation cooldown, or suppressing
    # State_Error.cpp's corrective-reset logic) rather than merely recording
    # a different value. Confirm each site (a) still writes Time.now()
    # unconditionally and (b) carries a comment documenting why, so a future
    # editor does not "finish the job" here without redoing this analysis.
    unchanged_sites = [
        (
            persistent_text,
            "MyPersistentData.cpp",
            "set_lastAlertTime(Time.now());",
            "raiseAlert",
        ),
        (
            app_text,
            APP_SRC.name,
            "current.set_lastAlertTime(Time.now());",
            "boot-storm alert (bootStormAlertPending)",
        ),
        (
            app_text,
            APP_SRC.name,
            "sysStatus.set_lastHookResponse(Time.now());",
            "webhook response handler",
        ),
    ]
    for text, filename, needle, context in unchanged_sites:
        if needle not in text:
            fail(f"{context} in {filename} must still call `{needle}` unconditionally (deliberately ungated - see Change 2 analysis)")
        idx = text.index(needle)
        preceding = text[max(0, idx - 900):idx]
        if "WO-2026-08-29-002" not in preceding or "NOT gated" not in preceding:
            fail(
                f"{context} in {filename} must carry a comment documenting why "
                "it is deliberately NOT gated on isClockTrusted() (Change 2 "
                "review requirement - avoids a future editor 'finishing the "
                "job' without redoing the consumer analysis)"
            )

    # occupancyStartTime (State_Sleep.cpp PIR-wake path) must likewise still
    # write Time.now() unconditionally and be documented as deliberately
    # ungated (validate()'s occupied-with-occupancyStartTime==0 forced-
    # unoccupied side effect, and State_Modes.cpp's duration-since-start
    # calculation would produce a garbage span).
    occ_needle = "current.set_occupancyStartTime(Time.now());"
    if occ_needle not in sleep_text:
        fail("State_Sleep.cpp's PIR-wake occupancy-start path must still call set_occupancyStartTime(Time.now()) unconditionally")
    occ_idx = sleep_text.index(occ_needle)
    occ_preceding = sleep_text[max(0, occ_idx - 900):occ_idx]
    if "WO-2026-08-29-002" not in occ_preceding or "NOT gated" not in occ_preceding:
        fail("State_Sleep.cpp's occupancyStartTime write must carry a comment documenting why it is deliberately NOT gated (Change 2)")

    # --- Round 4 review, Finding 3: setup()'s time-validation gate must ---
    # --- also force CONNECTING_STATE when this device has never (across ---
    # --- its whole persisted history) completed a confirmed sync, not   ---
    # --- only when Time.isValid() is false. Anchored on the actual      ---
    # --- "Validate time and configure local time converter" gate, not a ---
    # --- file-wide substring match.                                     ---
    #
    # SCOPE NARROWED 2026-08-31: Round 5 added a third OR term
    # (`hibernateWakeWithoutConfirmedSyncThisBoot`, gated on
    # `System.resetReason() == RESET_REASON_POWER_MANAGEMENT`) that has
    # been REVERTED per the Work Order's "SCOPE NARROWED 2026-08-31"
    # section - it fired on every hibernate wake (not just the skewed-RTC
    # case), making the opening-hour alert-40 suppression in the mutually
    # exclusive `else` branch below unreachable on every normal overnight
    # wake. This check is therefore back to the plain two-term Round-4
    # gate, PLUS a new check that the gate does NOT reintroduce a
    # resetReason/RESET_REASON_POWER_MANAGEMENT term (which would make the
    # alert-40 `else` branch unreachable again).
    setup_gate_match = re.search(
        r"if\s*\(\s*!Time\.isValid\(\)\s*\|\|\s*(\w+)\s*\)\s*\{",
        app_text,
    )
    if not setup_gate_match:
        fail(
            "setup()'s time-validation gate must be "
            "`if (!Time.isValid() || <never-confirmed-sync-flag>)` (Finding "
            "3, Round 4 review) - a bare `if (!Time.isValid())` no longer "
            "guarantees a connect attempt for a wrong RTC that reads valid"
        )
    never_synced_flag = setup_gate_match.group(1)
    flag_def_match = re.search(
        re.escape(never_synced_flag) + r"\s*=\s*\(sysStatus\.get_lastTimeSync\(\)\s*==\s*0\)",
        app_text,
    )
    if not flag_def_match:
        fail(
            f"setup()'s time-validation gate flag ({never_synced_flag}) must be "
            "defined as (sysStatus.get_lastTimeSync() == 0) (Finding 3) - "
            "NOT Particle.timeSyncedLast() == 0, which always reads 0 "
            "immediately after every boot/HIBERNATE wake and would force a "
            "connect on every wake, defeating low-power connection scheduling"
        )
    if flag_def_match.start() >= setup_gate_match.start():
        fail(f"{never_synced_flag} must be defined BEFORE setup()'s time-validation gate uses it")
    gate_to_flag_gap = app_text[flag_def_match.end():setup_gate_match.start()]
    if "Particle.timeSyncedLast()" in gate_to_flag_gap:
        fail("setup()'s Finding-3 gate must not also reference Particle.timeSyncedLast() between the flag definition and the gate")
    transition_after_gate = app_text[setup_gate_match.end():setup_gate_match.end() + 200]
    if "CONNECTING_STATE" not in transition_after_gate:
        fail("setup()'s Finding-3 gate must transitionTo(CONNECTING_STATE, ...) when triggered")

    # SCOPE NARROWED 2026-08-31 regression guard: the gate's condition
    # itself (the `if (...)` expression only, not the whole `if` body -
    # RESET_REASON_POWER_MANAGEMENT legitimately appears a few lines later,
    # inside the `else` branch, for the alert-40 suppression check) must
    # not reference RESET_REASON_POWER_MANAGEMENT or isClockTrusted() -
    # reintroducing either would resurrect the Round-5 regression that made
    # the alert-40 suppression `else` branch unreachable on every hibernate
    # wake.
    setup_gate_condition = app_text[setup_gate_match.start():setup_gate_match.end()]
    if "RESET_REASON_POWER_MANAGEMENT" in setup_gate_condition or "isClockTrusted" in setup_gate_condition:
        fail(
            "setup()'s time-validation gate condition must not reference "
            "RESET_REASON_POWER_MANAGEMENT or isClockTrusted() (SCOPE "
            "NARROWED 2026-08-31 regression guard) - either would make the "
            "opening-hour alert-40 suppression in the mutually exclusive "
            "`else` branch unreachable on every hibernate wake, exactly "
            "the Round-5 regression that was reverted"
        )

    # The alert-40 suppression itself must still be present in the `else`
    # branch immediately following the gate, and still exclusively gated
    # on RESET_REASON_POWER_MANAGEMENT there (proving that branch is
    # reachable again - it can only be reached when the gate above did NOT
    # fire, i.e. the if/else are still mutually exclusive as before Round 5).
    else_branch_after_gate = app_text[setup_gate_match.end():setup_gate_match.end() + 1500]
    if "} else {" not in else_branch_after_gate:
        fail("setup()'s time-validation gate must retain its mutually exclusive `else` branch")
    if "RESET_REASON_POWER_MANAGEMENT" not in else_branch_after_gate or "suppressAlert40ThisSession" not in else_branch_after_gate:
        fail(
            "setup()'s `else` branch must still contain the opening-hour "
            "alert-40 suppression path gated on RESET_REASON_POWER_MANAGEMENT "
            "(SCOPE NARROWED 2026-08-31 regression guard) - this branch is "
            "only reachable when the time-validation gate above does NOT "
            "fire, so its continued presence here demonstrates the Round-5 "
            "unreachability regression is fixed"
        )

    print(
        "PASS: checkClockResync()/dailyCleanup()/isClockTrusted()/logTimeDiag()/"
        "DeviceStatusPublisher.cpp/setup()/State_Modes.cpp wire ClockTrust's "
        "wrap-aware monotonic gate and trust signal correctly, record RTC "
        "write-back + lastTimeSync ONLY at confirmed-successful-write time "
        "(Finding 1), never call the potentially-blocking "
        "Particle.timeSyncedLast() directly (Finding 2), guarantee a connect "
        "opportunity for a device that has never completed a confirmed sync "
        "(Finding 3), stay correct across a full millis() rollover (Finding "
        "4), gate the full Finding-4 setter scope including State_Modes.cpp "
        "(Finding 6), give get_lastTimeSync() a real (non-comment) caller, "
        "and publish the item-9 clock-trust telemetry - with no new per-boot "
        "latch reintroduced and the vendored AB1805::loop() latch left "
        "untouched."
    )


if __name__ == "__main__":
    main()

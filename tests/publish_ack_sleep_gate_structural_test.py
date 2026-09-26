#!/usr/bin/env python3
"""WO-2026-09-25-001 acceptance criterion 3: the device must not sleep, and must
not tear down the cloud connection, while an unacknowledged event is still
queued - except when the existing cloud-sync gate times out, which must be
logged with the event count and the elapsed time.

The gate lives in ``handleSleepingState()`` (src/state/State_Sleep.cpp) and the
low-power entry into it lives in ``handleIdleState()`` (src/state/State_Idle.cpp).
Neither can be compiled standalone on the host - both pull in Particle,
PublishQueuePosix, AB1805 and the persistence layer - so this test parses the
shipped source directly, in the style of the repository's other structural
tests.

Stage 7 mutation this file is built to catch: (iii) allow sleep with
unacknowledged events queued.

WO-2026-09-25-001 Stage 5 decision 6 (Stage 7 finding P1): the gate is bounded.
All three sites now take their verdict from ``PublishDeliveryGate::queuePermitsSleep()``,
which permits sleep when the queue is sleep-safe, or the device is offline, or a
bounded delivery budget has expired with no publish in flight. The budget cannot
be defeated by keeping the queue non-empty, and it can never release the gate
while an attempt is outstanding.
"""

import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
SLEEP_SRC = REPO / "src" / "state" / "State_Sleep.cpp"
IDLE_SRC = REPO / "src" / "state" / "State_Idle.cpp"
QUEUE_SRC = REPO / "lib" / "PublishQueuePosixRK" / "src" / "PublishQueuePosixRK.cpp"

failures = []


def fail(message):
    failures.append(message)


def require(text, needle, description, source_name):
    if needle not in text:
        fail(f"{description} (not found in {source_name}: {needle!r})")


def index_of(text, needle):
    return text.find(needle)


def function_body(text, signature, source_name):
    """Return the body of the function whose definition starts with `signature`."""
    start = text.find(signature)
    if start < 0:
        fail(f"could not find {signature!r} in {source_name}")
        return ""
    brace = text.find("{", start)
    if brace < 0:
        fail(f"could not find the opening brace of {signature!r} in {source_name}")
        return ""
    depth = 0
    for i in range(brace, len(text)):
        if text[i] == "{":
            depth += 1
        elif text[i] == "}":
            depth -= 1
            if depth == 0:
                return text[brace : i + 1]
    fail(f"unbalanced braces after {signature!r} in {source_name}")
    return ""


def main():
    sleep_text = SLEEP_SRC.read_text()
    idle_text = IDLE_SRC.read_text()
    queue_text = QUEUE_SRC.read_text()

    # ---------------------------------------------------------------
    # 1. The sleep gate blocks on the publish queue.
    # ---------------------------------------------------------------
    require(
        sleep_text,
        "bool queueEmpty = PublishQueuePosix::instance().getCanSleep();",
        "the sleep gate must read the queue's own sleep-safety verdict",
        "State_Sleep.cpp",
    )
    require(
        sleep_text,
        "const bool queuePermitsSleep = PublishDeliveryGate::queuePermitsSleep();",
        "the sleep gate's queue term must come from the bounded delivery gate",
        "State_Sleep.cpp",
    )
    require(
        sleep_text,
        "bool allComplete = queuePermitsSleep && ledgersSynced && updatesChecked && webhookConfirmed;",
        "the delivery gate's verdict must be one of the conjuncts that release the sleep gate",
        "State_Sleep.cpp",
    )
    require(
        sleep_text,
        "if (!allComplete) {",
        "the gate must branch on allComplete",
        "State_Sleep.cpp",
    )

    # While the gate is blocked and inside budget, the handler must RETURN -
    # i.e. stay in SLEEPING_STATE without disconnecting - rather than fall
    # through to the disconnect request.
    gate_block_start = index_of(sleep_text, "if (elapsedMs < cloudSyncBudgetMs) {")
    gate_return = index_of(sleep_text, "return; // Stay in SLEEPING_STATE until complete or timeout")
    if gate_block_start < 0 or gate_return < 0 or not gate_block_start < gate_return:
        fail(
            "the in-budget branch of the cloud-sync gate must return and stay in "
            "SLEEPING_STATE instead of proceeding to disconnect"
        )

    # The gate must only be entered before the disconnect is requested.
    require(
        sleep_text,
        "if (Particle.connected() && !disconnectRequested) {",
        "the cloud-operations gate must run before the disconnect is requested",
        "State_Sleep.cpp",
    )

    # ---------------------------------------------------------------
    # 1b. Decision 6: the wait is bounded, and the bound can never fire while a
    #     publish attempt is outstanding.
    # ---------------------------------------------------------------
    budget_src = (REPO / "src" / "cloud" / "PublishDeliveryBudget.cpp").read_text()
    require(
        budget_src,
        "if (in.queueSleepSafe) {",
        "the budget must be cancelled when the queue drains",
        "PublishDeliveryBudget.cpp",
    )
    require(
        budget_src,
        "if (!in.cloudConnected) {",
        "offline time must not consume the delivery budget",
        "PublishDeliveryBudget.cpp",
    )
    require(
        budget_src,
        "out.sleepPermitted = out.budgetExpired && !in.publishInFlight;",
        "an expired budget must only permit sleep when no publish is in flight",
        "PublishDeliveryBudget.cpp",
    )

    # The teardown path must hold - bounded - rather than abandon an outstanding
    # attempt, and the hold must keep the thrash guard and watchdog fed.
    hold_start = index_of(sleep_text, "if (PublishDeliveryGate::publishInFlight()) {")
    hold_return = index_of(sleep_text, "return; // Never tear down while an attempt is outstanding")
    gatefail_line = index_of(sleep_text, 'Log.warn("GateFail: reason=%s')
    if hold_start < 0 or hold_return < 0 or gatefail_line < 0:
        fail("the teardown path must hold while a publish attempt is in flight")
    elif not hold_start < hold_return < gatefail_line:
        fail("the in-flight hold must sit between the budget expiry and the teardown")
    require(
        sleep_text,
        "ConnectivityPolicy::PUBLISH_IN_FLIGHT_HOLD_MAX_MS",
        "the in-flight hold must itself be bounded by a named policy constant",
        "State_Sleep.cpp",
    )
    require(
        sleep_text,
        'thrashGuard.markProgress("PUBLISH_ACK_HOLD");',
        "the in-flight hold must report progress so the thrash guard does not fire",
        "State_Sleep.cpp",
    )

    # ---------------------------------------------------------------
    # 2. The one permitted exception - the gate timing out - must be logged
    #    with the event count and the elapsed time.
    # ---------------------------------------------------------------
    gatefail = re.search(r'Log\.warn\("GateFail:[^"]*"', sleep_text)
    if not gatefail:
        fail("the gate timeout must still emit a GateFail warning")
    else:
        fmt = gatefail.group(0)
        for token in ("reason=%s", "elapsed=%lu", "qn=%u", "timeout=%lu"):
            if token not in fmt:
                fail(f"GateFail must report {token} (got {fmt})")

    alert43 = re.search(r'Log\.warn\("SLEEP: Publish queue not empty[^"]*"', sleep_text)
    if not alert43:
        fail("a gate timeout with events still queued must log the queue blocker")
    else:
        fmt = alert43.group(0)
        if "%u event(s) unacknowledged" not in fmt:
            fail(f"the queue-blocked timeout log must state the event count (got {fmt})")
        if "after %lu ms" not in fmt:
            fail(f"the queue-blocked timeout log must state the elapsed time (got {fmt})")
    require(
        sleep_text,
        "RecoveryState::raiseAlert(43);",
        "a gate timeout with events still queued must still raise alert 43",
        "State_Sleep.cpp",
    )

    # ---------------------------------------------------------------
    # 3. IDLE must not hand the device to SLEEPING_STATE while connected with
    #    an unacknowledged event queued.
    # ---------------------------------------------------------------
    idle_body = function_body(idle_text, "void handleIdleState(", "State_Idle.cpp")
    require(
        idle_body,
        "canSleepGate = PublishDeliveryGate::queuePermitsSleep();",
        "IDLE must consult the bounded delivery gate before entering SLEEPING_STATE",
        "State_Idle.cpp",
    )
    require(
        idle_body,
        "if (!updatesPending && canSleepGate) {",
        "the low-power sleep transition must be gated on canSleepGate",
        "State_Idle.cpp",
    )
    low_power_gate = index_of(idle_body, "if (!updatesPending && canSleepGate) {")
    low_power_transition = index_of(idle_body, 'transitionTo(SLEEPING_STATE, "low power idle");')
    if low_power_gate < 0 or low_power_transition < 0 or not low_power_gate < low_power_transition:
        fail("the low-power SLEEPING_STATE transition must sit inside the canSleepGate branch")

    # The IDLE connectivity ceiling force-tears-down and sleeps; it must only do
    # so when there is no meaningful work left, which includes the queue.
    require(
        idle_body,
        "const bool noMeaningfulWorkRemains = !updatesPending && queueCanSleep;",
        "the IDLE ceiling must treat a non-empty queue as meaningful work",
        "State_Idle.cpp",
    )
    require(
        idle_body,
        "queueCanSleep = PublishDeliveryGate::queuePermitsSleep();",
        "the IDLE ceiling's queue term must come from the bounded delivery gate",
        "State_Idle.cpp",
    )

    # All three sites must share one verdict, so a queue that never drains can
    # never pin the device awake at any of them.
    for name, text in (("State_Sleep.cpp", sleep_text), ("State_Idle.cpp", idle_text)):
        if "PublishDeliveryGate::queuePermitsSleep()" not in text:
            fail(f"{name} must take its sleep verdict from the bounded delivery gate")
    if idle_text.count("PublishDeliveryGate::queuePermitsSleep()") < 2:
        fail(
            "both IDLE sites (the low-power sleep entry and the connectivity ceiling) "
            "must use the bounded delivery gate"
        )
    ceiling_gate = index_of(idle_body, "connectivityPowered &&\n      noMeaningfulWorkRemains &&")
    if ceiling_gate < 0:
        fail("the IDLE ceiling must require noMeaningfulWorkRemains before forcing teardown")

    # ---------------------------------------------------------------
    # 4. The queue's own verdict must stay honest.
    # ---------------------------------------------------------------
    state_wait = function_body(queue_text, "void PublishQueuePosix::stateWait()", "PublishQueuePosixRK.cpp")
    if "canSleep = false;" not in state_wait:
        fail("stateWait() must mark the queue not sleep-safe once an event is dispatched")
    if "canSleep = (getNumEvents() == 0);" not in state_wait:
        fail("stateWait() must derive sleep safety from the queue depth")

    # Stage 4 warning: while pausePublishing is set the queue reports itself
    # sleep-safe even with events pending, so the application must never use it.
    for path in sorted((REPO / "src").rglob("*.cpp")) + sorted((REPO / "src").rglob("*.h")):
        if "setPausePublishing(" in path.read_text():
            fail(
                f"{path.relative_to(REPO)} calls setPausePublishing(), which reports the "
                "queue sleep-safe with events still pending"
            )

    if failures:
        for message in failures:
            print(f"FAIL: {message}", file=sys.stderr)
        print(f"publish_ack_sleep_gate_structural_test: {len(failures)} failure(s)", file=sys.stderr)
        return 1

    print(
        "publish_ack_sleep_gate_structural_test: sleep and cloud teardown stay blocked while "
        "an unacknowledged event is queued, the wait is bounded and never releases with a "
        "publish in flight, and the gate timeout logs count and elapsed time"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())

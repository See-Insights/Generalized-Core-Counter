#!/usr/bin/env python3
"""WO-2026-09-25-001 Stage 5 decision 5: `status` event byte budget.

Decision 5 moved the publish delivery counters out of the device-status ledger
payload and into the ``status`` event built by ``publishStartupStatus()``
(src/Generalized-Core-Counter.cpp) and published through
``PublishQueuePosix::instance().publish("status", status, PRIVATE)``.

That event has to fit under three separate ceilings:

  1. ``char status[1024]`` - the app's own ``snprintf`` buffer. Exceeding it
     truncates: safe for memory, but the event becomes invalid JSON.
  2. ``particle::protocol::MAX_EVENT_DATA_LENGTH`` = 1024 - the Device OS 6.4.1
     event-data limit (``communication/inc/protocol_defs.h:94``).
  3. PublishQueuePosixRK's own ceiling, which is that *same* constant:
     ``PublishQueuePosixRK.cpp:106`` rejects the publish outright when
     ``strlen(eventData) > particle::protocol::MAX_EVENT_DATA_LENGTH``.
     (The "255 bytes maximum, 622 bytes in system firmware 0.8.0-rc.4 and
     later" figure in the library's doc comments - and the "~622-byte event
     ceiling" comment in src/power/PowerDiagnostics.cpp - is stale prose from
     an older Device OS; it is not what this library enforces.)

Like tests/device_status_payload_budget_test.py, this parses the real format
strings out of the shipped source rather than mirroring them, so a new field
cannot be added without updating the width table below (the test fails with
"unbudgeted field" instead of silently going stale). It then renders real JSON
with placeholder values of the budgeted width, so punctuation is measured
rather than arithmetic-modelled.

Three profiles are reported:

  observed    - a typical in-service cycle. NOTE: this is a modelled profile,
                not a device capture; no full `status` payload capture exists
                in this repository. It is the field-width profile a healthy
                Dev-14 cycle produces.
  deployed    - every field at the widest value this firmware can actually
                produce. This is the binding worst case.
  structural  - every field at its C type / buffer maximum, reported for the
                record.
"""

import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
APP_SRC = REPO / "src" / "Generalized-Core-Counter.cpp"
VERSION_SRC = REPO / "src" / "Version.cpp"
QUEUE_SRC = REPO / "lib" / "PublishQueuePosixRK" / "src" / "PublishQueuePosixRK.cpp"
LEDGER_SRC = REPO / "src" / "cloud" / "DeviceStatusPublisher.cpp"

# Device OS 6.4.1 communication/inc/protocol_defs.h:94. Asserted against the
# toolchain header when one is present (see check_device_os_limit()).
MAX_EVENT_DATA_LENGTH = 1024

OBSERVED, DEPLOYED, STRUCTURAL = 0, 1, 2

# Rendered width (JSON text, including the quotes of a string value) of every
# field, keyed by its JSON key: (observed, deployed, structural).
#
# None means "read FIRMWARE_VERSION from src/Version.cpp".
WIDTHS = {
    # FIRMWARE_VERSION string literal; structural is the 32-char ceiling this
    # test enforces on that literal below.
    "version": (None, None, 34),
    # System.resetReason() - Device OS RESET_REASON_* values run to 140
    "resetReason": (2, 3, 11),
    # System.resetReasonData() is uint32_t
    "resetReasonData": (1, 10, 10),
    # RecoveryState::get_alertCode() is int8_t
    "alert": (1, 4, 4),
    # time_t epoch
    "lastAlert": (10, 10, 11),
    # System.freeMemory() on a Boron (256 KB SRAM) never reaches 7 digits
    "freeHeap": (6, 6, 10),
    # AppBreadcrumb enum stored as uint8_t
    "appBreadcrumb": (2, 3, 3),
    # uint32_t millis() snapshot
    "appBreadcrumbMs": (6, 10, 10),
    # RecoveryState::get_watchdogResetCount() is uint16_t
    "watchdogResetCount": (1, 5, 5),
    "lastWatchdogBreadcrumb": (1, 3, 3),  # uint8_t
    "lastWatchdogUptimeMs": (1, 10, 10),  # uint32_t
    "lastWatchdogResetReasonData": (1, 10, 10),  # uint32_t
    # --- PMIC forensics variant only (ENABLE_PMIC_FORENSICS defaults to 1) ---
    "pmicAnomalyCount": (1, 5, 5),  # uint16_t
    # %.2f of a float: "-1.00" when unset, "100.00" at full charge. The
    # structural figure is the widest %.2f a float can render (FLT_MAX),
    # reachable only from a corrupt float.
    "lastPmicAnomalySoc": (5, 6, 42),
    "lastPmicAnomalyChargeStatus": (1, 3, 3),  # uint8_t
    "lastPmicAnomalyAgeSec": (1, 10, 10),  # uint32_t
    "lastPmicAnomalyPowerSource": (1, 3, 3),  # uint8_t
    "lastPmicAnomalyVbusStatus": (1, 3, 3),  # uint8_t
    # ------------------------------------------------------------------------
    "failsafeStage": (1, 3, 3),  # uint8_t
    "failsafeCount": (1, 3, 3),  # uint8_t
    "failsafeLastAction": (1, 10, 11),  # time_t
    "lastConnectionAgeSec": (4, 10, 11),  # long, or the -1 sentinel
    "failsafeTest": (1, 1, 1),  # 0 or 1
    "failsafeTestMode": (1, 1, 1),  # 0 or 1
    # WO-2026-09-25-001 delivery counters - uint16_t saturating at
    # PublishDeliveryCounters::kCounterMax (65535), so five digits is both the
    # deployed and the structural maximum.
    "a": (3, 5, 5),
    "k": (3, 5, 5),
    "f": (1, 5, 5),
    "r": (1, 5, 5),
    "q": (1, 5, 5),
    "s": (1, 5, 5),
    "b": (1, 5, 5),
}

# Widths for the two optional trailing field groups, which are built by their
# own snprintf() calls into fixed buffers and appended with %s%s.
HIBERNATE_WIDTHS = {
    # ab1805WakeReasonNameLocal(): longest is "DEEP_POWER_DOWN"
    "wakeReason": (9, 17, 17),
    "actualSleep": (4, 10, 10),  # uint32_t seconds
    "sleepError": (2, 11, 11),  # signed seconds delta
    "hibernateCount": (3, 10, 10),  # uint32_t
}

PIN_RESET_WIDTHS = {
    # ab1805WakeReasonNameLocal(): longest is "DEEP_POWER_DOWN"
    "ab1805WakeReason": (10, 17, 17),
    # "AB1805_PIN" or "NONE"
    "watchdogSource": (6, 12, 12),
}

DELIVERY_KEYS = ("a", "k", "f", "r", "q", "s", "b")
DELIVERY_FORMAT = '"d":{"a":%u,"k":%u,"f":%u,"r":%u,"q":%u,"s":%u,"b":%u}'

failures = []


def fail(message):
    failures.append(message)


def unescape(literal):
    return literal.replace('\\"', '"').replace("\\\\", "\\")


def read_function(text, signature):
    start = text.find(signature)
    if start < 0:
        fail(f"could not find {signature!r} in {APP_SRC.name}")
        return ""
    depth = 0
    for index in range(start, len(text)):
        if text[index] == "{":
            depth += 1
        elif text[index] == "}":
            depth -= 1
            if depth == 0:
                return text[start : index + 1]
    fail(f"could not find the end of {signature!r}")
    return ""


def format_literals(body, anchor):
    """Return every concatenation-free C string literal passed as the format
    argument of an snprintf() whose target is `anchor`."""
    literals = []
    for match in re.finditer(
        r"snprintf\(\s*" + re.escape(anchor) + r"\s*,\s*sizeof\([^)]*\)\s*,\s*\"((?:[^\"\\]|\\.)*)\"",
        body,
    ):
        literals.append(unescape(match.group(1)))
    return literals


def render(fmt, widths, which, version_width, trailing=()):
    """Render a printf format string as JSON, replacing every conversion with a
    placeholder of the budgeted width. `trailing` supplies the rendered text for
    the trailing %s conversions, in order.

    Budgeted widths include the quotes of a string value; when the format itself
    already wraps the conversion in quotes (`"key":"%s"`), those two bytes are
    subtracted so they are not counted twice."""
    specs = list(re.finditer(r"%(?:\.\d+)?l{0,2}[dufs]", fmt))
    if not specs:
        fail(f"no conversions found in format {fmt[:40]!r}")
        return ""

    trailing = list(trailing)
    first_trailing = len(specs) - len(trailing)
    out = []
    cursor = 0
    for index, spec in enumerate(specs):
        out.append(fmt[cursor : spec.start()])
        cursor = spec.end()
        if trailing and index >= first_trailing:
            out.append(trailing[index - first_trailing])
            continue
        prefix = fmt[: spec.start()]
        name_match = None
        for name_match in re.finditer(r'"([A-Za-z0-9]+)"\s*:', prefix):
            pass
        if not name_match:
            fail(f"could not attribute conversion {spec.group(0)!r} to a JSON key")
            continue
        key = name_match.group(1)
        if key not in widths:
            fail(f"unbudgeted field {key!r} in the status event - add it to the width table")
            continue
        width = widths[key][which]
        if width is None:
            width = version_width
        quoted = prefix.endswith('"') and fmt[spec.end() : spec.end() + 1] == '"'
        out.append("x" * (width - 2 if quoted else width))
    out.append(fmt[cursor:])
    return "".join(out)


def main():
    text = APP_SRC.read_text()
    body = read_function(text, "void publishStartupStatus() {")
    if not body:
        for message in failures:
            print(f"FAIL: {message}", file=sys.stderr)
        return 1

    version_match = re.search(r'FIRMWARE_VERSION\s*=\s*"([^"]*)"', VERSION_SRC.read_text())
    if not version_match:
        fail("could not read FIRMWARE_VERSION from src/Version.cpp")
        version_literal = ""
    else:
        version_literal = version_match.group(1)
    if len(version_literal) > 32:
        fail(
            f"FIRMWARE_VERSION is {len(version_literal)} chars, above the 32-char ceiling this "
            "budget assumes"
        )
    version_width = len(version_literal) + 2  # quotes

    status_formats = format_literals(body, "status")
    if len(status_formats) != 2:
        fail(
            f"expected 2 status snprintf() format strings (PMIC and non-PMIC), found "
            f"{len(status_formats)}"
        )
        for message in failures:
            print(f"FAIL: {message}", file=sys.stderr)
        return 1
    pmic_format, plain_format = status_formats

    hibernate_formats = format_literals(body, "hibernateFields")
    pin_reset_formats = format_literals(body, "pinResetAb1805Fields")
    if len(hibernate_formats) != 1 or len(pin_reset_formats) != 1:
        fail("expected exactly one hibernateFields and one pinResetAb1805Fields format string")
        for message in failures:
            print(f"FAIL: {message}", file=sys.stderr)
        return 1

    # Criterion 4: the counters are in the status event, in both build variants.
    for label, fmt in (("PMIC", pmic_format), ("non-PMIC", plain_format)):
        if DELIVERY_FORMAT not in fmt:
            fail(
                f"the {label} status event format is missing the delivery-counter object "
                f"{DELIVERY_FORMAT!r}"
            )

    # Decision 5: and they are NOT in the device-status ledger payload.
    ledger = LEDGER_SRC.read_text()
    if 'writerBase.name("d")' in ledger:
        fail(
            'the device-status ledger payload emits a "d" object - Stage 5 decision 5 requires '
            "the delivery counters in the status event only, leaving the ledger payload format "
            "unchanged"
        )
    for key in DELIVERY_KEYS:
        if f'writerBase.name("{key}")' in ledger:
            fail(
                f'the device-status ledger payload emits delivery counter "{key}" - Stage 5 '
                "decision 5 requires the ledger payload format to be unchanged"
            )
    if "PublishDeliveryCounters" in ledger:
        fail(
            "src/cloud/DeviceStatusPublisher.cpp still references PublishDeliveryCounters - the "
            "ledger payload must not read the counters at all (Stage 5 decision 5)"
        )

    # The counters must be read from the module, not hard-coded.
    if "PublishDeliveryCounters::snapshot();" not in body:
        fail("publishStartupStatus() does not read PublishDeliveryCounters::snapshot()")
    for field in (
        "attempted",
        "acknowledged",
        "failed",
        "retried",
        "queuedAtSleep",
        "sleptWithQueued",
        "abandoned",
    ):
        if f"delivery.{field}" not in body:
            fail(f"the status event does not publish the {field!r} counter")

    # The library ceiling is MAX_EVENT_DATA_LENGTH, not the stale ~622 prose.
    queue = QUEUE_SRC.read_text()
    if "strlen(eventData) > particle::protocol::MAX_EVENT_DATA_LENGTH" not in queue:
        fail(
            "PublishQueuePosixRK no longer rejects on "
            "particle::protocol::MAX_EVENT_DATA_LENGTH - re-derive the event-size ceiling"
        )

    buffer_match = re.search(r"char status\[(\d+)\];", body)
    if not buffer_match:
        fail("could not read the status[] buffer size")
        for message in failures:
            print(f"FAIL: {message}", file=sys.stderr)
        return 1
    buffer_size = int(buffer_match.group(1))

    results = {}
    for which, label in ((OBSERVED, "observed"), (DEPLOYED, "deployed"), (STRUCTURAL, "structural")):
        hibernate = render(hibernate_formats[0], HIBERNATE_WIDTHS, which, version_width)
        pin_reset = render(pin_reset_formats[0], PIN_RESET_WIDTHS, which, version_width)
        if len(hibernate) > 191:
            fail(f"hibernateFields ({label}) renders {len(hibernate)} B, above its 192 B buffer")
        if len(pin_reset) > 159:
            fail(
                f"pinResetAb1805Fields ({label}) renders {len(pin_reset)} B, above its 160 B buffer"
            )
        for variant, fmt in (("pmic", pmic_format), ("plain", plain_format)):
            # Worst case assumes both optional groups are present; the observed
            # profile is a hibernate wake, which is the common in-service case.
            appended = (hibernate, pin_reset) if which != OBSERVED else (hibernate, "")
            results[(label, variant)] = len(render(fmt, WIDTHS, which, version_width, appended))

    # ENABLE_PMIC_FORENSICS defaults to 1 (src/BuildProfile.h:109), so the pmic
    # variant is the shipped one and the binding figure.
    limit = min(buffer_size - 1, MAX_EVENT_DATA_LENGTH)

    print(f"status event limits")
    print(f"  app snprintf buffer (char status[])   : {buffer_size} bytes ({buffer_size - 1} usable)")
    print(f"  Device OS 6.4.1 MAX_EVENT_DATA_LENGTH : {MAX_EVENT_DATA_LENGTH} bytes")
    print(f"  PublishQueuePosixRK ceiling           : {MAX_EVENT_DATA_LENGTH} bytes (same constant)")
    print(f"  binding limit                         : {limit} bytes")
    print(f"status event size, with the delivery counters (shipped PMIC-forensics variant)")
    for label in ("observed", "deployed", "structural"):
        size = results[(label, "pmic")]
        print(f"  {label:<10} : {size:>5} bytes   headroom {limit - size:>5} bytes")
    print(f"status event size, ENABLE_PMIC_FORENSICS=0 variant")
    for label in ("observed", "deployed", "structural"):
        size = results[(label, "plain")]
        print(f"  {label:<10} : {size:>5} bytes   headroom {limit - size:>5} bytes")

    counter_cost = (
        len(DELIVERY_FORMAT)
        - len("%u") * len(DELIVERY_KEYS)
        + 5 * len(DELIVERY_KEYS)
        + 1
    )  # + leading comma
    print(f"delivery-counter object cost, worst case: {counter_cost} bytes")

    for label in ("observed", "deployed"):
        for variant in ("pmic", "plain"):
            size = results[(label, variant)]
            if size > limit:
                fail(
                    f"the {label} {variant} status event is {size} B, above the binding "
                    f"{limit} B limit - the delivery counters do not fit"
                )

    for variant in ("pmic", "plain"):
        size = results[("structural", variant)]
        if size > limit:
            print(
                f"NOTE: the structural worst case of the {variant} variant ({size} B) exceeds the "
                f"{limit} B limit. Reachable only from a corrupt float in lastPmicAnomalySoc "
                "(%.2f of FLT_MAX is 42 characters); every real value is at most 6."
            )

    if failures:
        for message in failures:
            print(f"FAIL: {message}", file=sys.stderr)
        print(f"status_event_payload_budget_test: {len(failures)} failure(s)", file=sys.stderr)
        return 1

    print(
        "status_event_payload_budget_test: the delivery counters are in the status event, absent "
        f"from the ledger payload, and fit the {limit}-byte limit"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())

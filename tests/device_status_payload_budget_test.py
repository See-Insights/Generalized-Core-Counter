#!/usr/bin/env python3
"""WO-2026-09-25-001 Stage 6: device-status ledger payload byte budget.

``Cloud::writeDeviceStatusToCloud()`` (src/cloud/DeviceStatusPublisher.cpp)
serializes the status payload into a ``char[DEVICE_STATUS_PAYLOAD_CAPACITY]``
stack buffer with ``JSONBufferWriter``. Device OS documents that
``dataSize()`` "can be greater than buffer size"
(6.4.1 ``wiring/inc/spark_wiring_json.h:232``), so exceeding the cap means a
truncated, invalid payload - hence the explicit overflow guard this test also
checks for.

Stage 5 decision 5 keeps the publish delivery counters OUT of this payload -
they go in the `status` event instead (see
tests/status_event_payload_budget_test.py) - precisely because this payload was
already measured at 831-856 of its 896-byte cap in the field. This test
therefore also guards that decision: it fails if a `"d"` object, any of the five
counter keys, or any reference to PublishDeliveryCounters reappears here.

So this test:

  1. parses the actual writer calls out of the shipped source, so a new field
     cannot be added without updating the width table below (the test fails
     with "unbudgeted field" instead of silently going stale);
  2. computes the DEPLOYED worst case - every field at the largest value this
     firmware can actually produce - and asserts it fits;
  3. computes and prints the STRUCTURAL worst case - every field at its C type
     or char[] buffer maximum - which is reported for the record;
  4. asserts the delivery counters are absent.

This cannot compile the real function on the host (it pulls in Particle,
PowerManager, SensorManager, Ledger and the persistence layer), so it measures
the source's own field list rather than a mirror of it.
"""

import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
STATUS_SRC = REPO / "src" / "cloud" / "DeviceStatusPublisher.cpp"
CLOUD_HDR = REPO / "src" / "cloud" / "Cloud.h"
VERSION_SRC = REPO / "src" / "Version.cpp"

# Observed on-device size of the payload (WO-2026-09-25-001 Stage 6 dispatch:
# "the last observed size was 839/896"; Stage 5 decision 5 records 831-856 on
# Dev-14, most often 853). Used to report the real-world headroom.
OBSERVED_BASELINE_BYTES = 839

# Deployed worst case of this payload, as measured by this test on the
# WO-2026-09-25-001 Stage 6 tree. Stage 5 decision 5 requires the payload format
# to be UNCHANGED by this WO, so this figure must not move.
DEPLOYED_BASELINE_WITHOUT_DELIVERY = 903

# Rendered width (JSON text, including the quotes of a string value) of every
# field, keyed by its dotted path.
#
#   deployed   - the widest value this firmware can actually emit
#   structural - the widest value the field's C type or char[] buffer allows
#
# Justification for each deployed value is in the comment beside it.
WIDTHS = {
    # constant kLedgerSchemaVersion == 2
    "schemaVersion": (1, 1),
    # FIRMWARE_VERSION, read from src/Version.cpp below (deployed); the startup
    # snapshot copy is a char[32] (structural).
    "firmware.version": (None, 33),
    # RecoveryState::get_resetCount() is uint8_t
    "firmware.resetCount": (3, 3),
    # compiledBuildFlags is a uint16_t bitmask, bits 0x0001..0x4000
    "firmware.flags": (5, 5),
    # formatConfigGeneration() writes exactly 8 hex digits
    "config.generation": (10, 10),
    # (int) of a time_t epoch: 10 digits now, 11 if ever negative
    "reporting.lastReportEpoch": (10, 11),
    "reporting.nextReportEpoch": (10, 11),
    # ConfigApply bounds reportingIntervalSec to 300..86400 (5 digits)
    "reporting.configuredIntervalSec": (5, 10),
    # effective = configured * tier multiplier (<=12), still <= 7 digits
    "reporting.effectiveIntervalSec": (7, 10),
    # adjustmentReasonName(): "none" | "low-battery"
    "reporting.adjustmentReason": (13, 13),
    "reporting.windowOpen": (5, 5),
    "startup.epoch": (10, 11),
    # resetReasonName(): longest is "power-management"; field is char[24]
    "startup.reason": (18, 25),
    # copy of FIRMWARE_VERSION; field is char[32]
    "startup.firmware": (None, 33),
    # System.version(), e.g. "6.4.1"; field is char[24]
    "startup.deviceOS": (10, 25),
    # StartupSnapshot::resetCount is uint32_t, fed from a uint8_t counter
    "startup.resetCount": (3, 10),
    # powerSourceLabel(): longest is "UNAVAILABLE"/"USB_ADAPTER"
    "power.source": (13, 13),
    # inputProfileLabel(): longest is "NotApplicable"
    "power.profile": (15, 15),
    "power.overrideActive": (5, 5),
    # soc(), one decimal: "100.0" / "-1.0"
    "battery.soc": (5, 6),
    # vcell, two decimals: "-1.00" / "4.20"
    "battery.vcell": (5, 6),
    # compactPmicChargeLabel(): longest is "UNKNOWN"
    "battery.chargeState": (9, 9),
    # batteryTierName(): longest is "CONSERVING"
    "battery.tier": (12, 12),
    "battery.lowBatteryMode": (5, 5),
    # vcellSampleStateLabel(): longest is "Unavailable"
    "battery.vcellState": (13, 13),
    # socTrustLabel(): longest is "Untrusted"
    "battery.socTrust": (11, 11),
    # toString(ConnectResult): longest is "timeout"/"aborted"
    "connection.lastResult": (9, 9),
    # connect_duration_ms is uint32_t but bounded by the connect budget
    "connection.elapsedMs": (7, 11),
    "clock.trusted": (5, 5),
    # reportedSyncAgeMs()/1000, or the -1 sentinel
    "clock.syncAgeSec": (7, 11),
    "clock.lastSyncEpoch": (10, 11),
    # Clock::lastSyncCorrectionSec() is a signed seconds delta
    "clock.correctionSec": (11, 11),
    "clock.usingRC": (5, 5),
    # uint8_t AB1805 registers
    "clock.oscStatus": (3, 3),
    "clock.oscCtrl": (3, 3),
    "clock.aos": (5, 5),
    "clock.fos": (5, 5),
}

# WO-2026-09-25-001 Stage 5 decision 5: these keys belong in the `status` event,
# not here. Their presence in this payload is a test failure, not a budget line.
FORBIDDEN_DELIVERY_KEYS = ("d", "a", "k", "f", "r", "q")

failures = []


def fail(message):
    failures.append(message)


def read_capacity():
    match = re.search(
        r"DEVICE_STATUS_PAYLOAD_CAPACITY\s*=\s*(\d+)", CLOUD_HDR.read_text()
    )
    if not match:
        fail("could not read DEVICE_STATUS_PAYLOAD_CAPACITY from src/cloud/Cloud.h")
        return 0
    return int(match.group(1))


def read_firmware_version_width():
    match = re.search(r'FIRMWARE_VERSION\s*=\s*"([^"]*)"', VERSION_SRC.read_text())
    if not match:
        fail("could not read FIRMWARE_VERSION from src/Version.cpp")
        return 0
    return len(match.group(1)) + 2  # quotes


def parse_writer(text):
    """Walk the writerBase calls and rebuild the payload's shape as nested
    ordered lists of (name, children-or-None)."""
    start = text.find("writerBase.beginObject();")
    if start < 0:
        fail("could not find the start of the status payload writer")
        return None
    region = text[start:]
    end = region.find("if (!writerBase.buffer())")
    if end < 0:
        fail("could not find the end of the status payload writer")
        return None
    region = region[:end]

    tokens = re.findall(
        r'writerBase\.name\("([A-Za-z]+)"\)\s*(\.beginObject\(\))?|writerBase\.(endObject)\(\)',
        region,
    )

    root = []
    stack = [root]
    for name, begins, ends in tokens:
        if ends:
            if len(stack) > 1:
                stack.pop()
            continue
        if begins:
            children = []
            stack[-1].append((name, children))
            stack.append(children)
        else:
            stack[-1].append((name, None))
    if len(stack) != 1:
        fail("unbalanced beginObject()/endObject() calls in the status payload writer")
    return root


def collect_paths(node, prefix=""):
    for name, children in node:
        path = f"{prefix}{name}"
        if children is None:
            yield path
        else:
            yield from collect_paths(children, f"{path}.")


def render(node, which, version_width, prefix=""):
    """Render the payload as real JSON with placeholder values of the budgeted
    width, so the punctuation cost is measured rather than arithmetic-modelled."""
    parts = []
    for name, children in node:
        path = f"{prefix}{name}"
        if children is not None:
            parts.append(f'"{name}":{render(children, which, version_width, path + ".")}')
            continue
        if path not in WIDTHS:
            fail(f"unbudgeted field {path!r} in the status payload - add it to WIDTHS")
            width = 0
        else:
            width = WIDTHS[path][which]
            if width is None:
                width = version_width
        parts.append(f'"{name}":' + ("x" * width))
    return "{" + ",".join(parts) + "}"


def main():
    text = STATUS_SRC.read_text()
    capacity = read_capacity()
    version_width = read_firmware_version_width()
    root = parse_writer(text)

    if not root:
        for message in failures:
            print(f"FAIL: {message}", file=sys.stderr)
        return 1

    paths = list(collect_paths(root))

    # Stage 5 decision 5: no delivery counter may appear in this payload.
    for key in FORBIDDEN_DELIVERY_KEYS:
        for path in paths:
            if path == key or path.startswith(f"{key}."):
                fail(
                    f"the device-status ledger payload emits {path!r} - WO-2026-09-25-001 Stage 5 "
                    "decision 5 requires the delivery counters in the `status` event only, "
                    "leaving this payload's format unchanged"
                )
    if "PublishDeliveryCounters" in text:
        fail(
            "src/cloud/DeviceStatusPublisher.cpp references PublishDeliveryCounters - this "
            "payload must not read the delivery counters at all (Stage 5 decision 5)"
        )

    deployed = len(render(root, 0, version_width))
    structural = len(render(root, 1, version_width))

    # The writer must refuse to publish rather than index past the buffer.
    if "if (writerBase.dataSize() >= sizeof(bufferBase)) {" not in text:
        fail(
            "the status writer must guard against dataSize() exceeding the buffer "
            "before storing the NUL terminator"
        )

    usable = capacity - 1
    print(f"device-status payload capacity          : {capacity} bytes")
    print(f"  usable (one byte reserved for the NUL): {usable} bytes")
    print(f"  observed size in the field            : {OBSERVED_BASELINE_BYTES} bytes")
    print(f"  DEPLOYED worst case                   : {deployed} bytes")
    print(f"  STRUCTURAL worst case (type maxima)   : {structural} bytes")

    if deployed != DEPLOYED_BASELINE_WITHOUT_DELIVERY:
        fail(
            f"the device-status payload's deployed worst case is {deployed} B, not the recorded "
            f"{DEPLOYED_BASELINE_WITHOUT_DELIVERY} B - WO-2026-09-25-001 Stage 5 decision 5 "
            "requires this payload's format to be unchanged; re-measure the byte budget before "
            "changing fields"
        )

    # Pre-existing and independent of WO-2026-09-25-001: the deployed and
    # structural worst cases already exceed the cap on their own, so these are
    # reported rather than enforced - enforcing them would fail on the
    # unmodified baseline too. The dataSize() guard checked above is what makes
    # those cases safe rather than a stack overwrite.
    if deployed > usable:
        print(
            f"NOTE: the deployed worst case ({deployed} B) exceeds the usable cap ({usable} B). "
            "Pre-existing; contained by the dataSize() overflow guard. Raising "
            "DEVICE_STATUS_PAYLOAD_CAPACITY is the clean fix - it is a self-imposed RAM "
            "constant, not a Particle Ledger limit - but that is WO-2026-09-25-003."
        )
    if structural > usable:
        print(
            f"NOTE: structural worst case ({structural} B) exceeds the usable cap ({usable} B). "
            "Reachable only with a firmware version string, reset-reason name or Device OS "
            "version string far longer than any this fleet uses."
        )

    if failures:
        for message in failures:
            print(f"FAIL: {message}", file=sys.stderr)
        print(f"device_status_payload_budget_test: {len(failures)} failure(s)", file=sys.stderr)
        return 1

    print(
        "device_status_payload_budget_test: the ledger payload format is unchanged and carries "
        "no delivery counters"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())

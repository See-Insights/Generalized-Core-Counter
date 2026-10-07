#!/usr/bin/env python3
"""WO-2026-10-02-001 items D and E: the report payload's new numeric fields.

Behavioural where it can be: the two real snprintf format strings are
extracted from publishData() and rendered with each field's worst-case
value, so the assertions are about the bytes that actually go on the wire,
not about the source text.

The Ubidots rule (Stage 5 decision 3) is that every new top-level key must
be an UNQUOTED JSON number, because Ubidots auto-creates a variable per new
key and a quoted value would create a text variable instead.
"""

import json
import os
import re
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
APP = os.path.join(REPO, "src", "Generalized-Core-Counter.cpp")

NEW_KEYS = ("fh", "lfb", "cyc", "slp")
# WO-2026-10-04-001 item D: the cell voltage. Unquoted like the rest, but a
# %.2f rather than a %lu, so it is checked separately from NEW_KEYS.
VC_KEY = "vc"
BUFFER_BYTES = 256

# Worst-case rendered value per key, taken from the type that feeds it.
#
#   hourly/daily    uint16_t CurrentReadings counters           -> 65535
#   occupancy       the 0/1 ternary                             -> 1
#   dailyoccupancy  uint32_t seconds / 60                       -> 71582788
#   battery         clamped to [0,100] just above the snprintf  -> 100.00
#   key1            the longest batteryContext string
#   temp            float internal temperature; the sensor range is far
#                   inside +/-999.99 (NaN and Inf print shorter)
#   alerts          int8_t alert code                           -> -128
#   resets          uint8_t reset count                         -> 255
#   connecttime     uint16_t seconds                            -> 65535
#   fh / lfb        uint32_t, but bounded by the nRF52840's 256 KB of RAM
#   cyc / slp       uint32_t RAM counters, bounded only by the type
#   vc              cell voltage; SensorManager only caches a plausible
#                   sample (2.5 V < vcell < 5.0 V, not NaN) and publishData()
#                   starts from 0.0f, so %.2f renders at most "5.00"
#   timestamp       UTC epoch seconds, plus the format's literal "000"
WORST_VALUE = {
    "hourly": "65535",
    "daily": "65535",
    "occupancy": "1",
    "dailyoccupancy": "71582788",
    "battery": "100.00",
    "temp": "-999.99",
    "alerts": "-128",
    "resets": "255",
    "connecttime": "65535",
    "fh": "262144",
    "lfb": "262144",
    "cyc": "4294967295",
    "slp": "4294967295",
    "vc": "5.00",
    "timestamp": "9999999999",
}

FIELD_RE = re.compile(r'"(\w+)":(%[-0-9.]*(?:lu|i|d|f|s)|"%s")')

failures = []


def check(ok, message):
    if ok:
        print("  ok   %s" % message)
    else:
        print("  FAIL %s" % message)
        failures.append(message)


def battery_contexts(text):
    m = re.search(r"batteryContext\[7\]\s*=\s*\{(.*?)\};", text, re.S)
    if not m:
        sys.exit("EXTRACT_FAILED: batteryContext table not found")
    return re.findall(r'"([^"]*)"', m.group(1))


def formats(text):
    """The two report format strings, in source order, un-escaped."""
    start = text.index("void publishData(")
    end = text.index("const char *webhookName", start)
    body = text[start:end]
    found = re.findall(r'"(\{\\"[^\n]*?)",\n', body)
    if len(found) != 2:
        sys.exit("EXTRACT_FAILED: expected 2 report format strings, found %d" % len(found))
    return [f.replace('\\"', '"') for f in found]


def conversions(fmt):
    return dict((m.group(1), m.group(2)) for m in FIELD_RE.finditer(fmt))


def render(fmt, longest_context):
    """Substitute each field's worst-case value into the real format."""
    out = []
    pos = 0
    for m in FIELD_RE.finditer(fmt):
        key, conv = m.group(1), m.group(2)
        if conv == '"%s"':
            value = '"%s"' % longest_context
        else:
            if key not in WORST_VALUE:
                sys.exit("EXTRACT_FAILED: no worst-case value defined for %r" % key)
            value = WORST_VALUE[key]
        out.append(fmt[pos:m.start()])
        out.append('"%s":%s' % (key, value))
        pos = m.end()
    out.append(fmt[pos:])
    return "".join(out)


def main():
    text = open(APP).read()
    longest_context = max(battery_contexts(text), key=len)
    fmts = formats(text)
    names = ("occupancy", "counting")

    buf = re.search(r"char data\[(\d+)\];", text)
    check(buf is not None and int(buf.group(1)) == BUFFER_BYTES,
          "the payload buffer is char data[%d]" % BUFFER_BYTES)

    for name, fmt in zip(names, fmts):
        print("")
        print("--- %s format ---" % name)

        keys_in_order = [m.group(1) for m in FIELD_RE.finditer(fmt)]
        conv = conversions(fmt)
        rendered = render(fmt, longest_context)
        parsed = json.loads(rendered)

        check(len(keys_in_order) == len(set(keys_in_order)), "%s: no duplicated key" % name)

        for key in NEW_KEYS:
            check(conv.get(key) == "%lu",
                  "%s: %r is an unquoted %%lu conversion in the format string (found %r)"
                  % (name, key, conv.get(key)))
            check(isinstance(parsed.get(key), int) and not isinstance(parsed.get(key), bool),
                  "%s: %r parses as an unquoted JSON number" % (name, key))

        check(conv.get(VC_KEY) == "%.2f",
              "%s: %r is an unquoted %%.2f conversion in the format string (found %r)"
              % (name, VC_KEY, conv.get(VC_KEY)))
        check(isinstance(parsed.get(VC_KEY), float),
              "%s: %r parses as an unquoted JSON number" % (name, VC_KEY))

        string_keys = sorted(k for k, v in parsed.items() if isinstance(v, str))
        check(string_keys == ["key1"],
              "%s: the only string-valued key is still key1 (found %s)"
              % (name, string_keys or "none"))

        for key in ("battery", "key1", "temp", "alerts", "resets", "connecttime", "timestamp"):
            check(key in parsed, "%s: pre-existing key %r is unchanged" % (name, key))

        size = len(rendered) + 1  # snprintf's terminating NUL
        print("       maximum payload: %d bytes incl. NUL (buffer %d, longest key1 %r)"
              % (size, BUFFER_BYTES, longest_context))
        check(size <= BUFFER_BYTES,
              "%s: the maximum payload (%d bytes) fits char data[%d]"
              % (name, size, BUFFER_BYTES))

    print("")
    print("--- the values behind the fields ---")
    check("HAL_Core_Runtime_Info(&rtInfo, nullptr);" in text,
          "lfb comes from HAL_Core_Runtime_Info()")
    check("rtInfo.size = sizeof(rtInfo);" in text,
          "runtime_info_t.size is set to sizeof(runtime_info_t) before the call")
    check("rtInfo.largest_free_block_heap" in text,
          "lfb reads largest_free_block_heap")
    start = text.index("void publishData(time_t stampOverride) {")
    body = text[start:text.index("const char *webhookName", start)]
    check("System.freeMemory()" in body, "fh is System.freeMemory()")
    check("AwakeCycles::cycles" in body and "AwakeCycles::sleeps" in body,
          "cyc and slp are the AwakeCycles counters "
          "(cyc >= slp is proven in tests/awake_cycle_counters_test.sh)")
    check("SensorManager::instance().cachedBatteryVoltage(vc)" in body,
          "vc comes from SensorManager::cachedBatteryVoltage()")
    check("float vc = 0.0f;" in body,
          "vc starts at 0.0f, so an implausible or unsampled cell voltage publishes 0.00")

    print("")
    if failures:
        print("FAILED: %d check(s)" % len(failures))
        return 1
    print("report_payload_fields_test passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())

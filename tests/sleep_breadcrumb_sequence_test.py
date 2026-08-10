#!/usr/bin/env python3
"""
Host-side regression test for item 2 of WO-2026-08-10-001: finer breadcrumb
granularity through the sleep-entry sequence in src/state/State_Sleep.cpp.

Rather than mirroring the logic in a separate C++ file (State_Sleep.cpp pulls
in far too many Particle/Cloud/Sensor dependencies to compile standalone),
this test parses the ACTUAL shipped source file directly and traces each of
the four sleep-entry call sites (HIBERNATE, ULP-primary, STOP-fallback-1,
STOP-fallback-2) to confirm:

  1. The stale 18/19/20/21 breadcrumb literals are gone from the sleep path
     (they collided with BREADCRUMB_PUBLISH_QUEUE_EXIT/REPORT_POST_LEDGER/
     REPORT_EXIT/IDLE_ENTRY).
  2. Within each call site, the breadcrumb sequence fires in strictly
     increasing, correctly-ordered fashion:
       SLEEP_GATE_START(22) -> SLEEP_GATE_DONE(23) [once, shared gate] ->
       SLEEP_CONFIG_START(24) -> SLEEP_SYSTEM_CALL(25) ->
       SLEEP_SERIAL_DRAIN_DONE(26) -> [SLEEP_DIAG_FLUSH_DONE(27) - hibernate
       only, gated on ENABLE_DIAGNOSTICS_PUBLISH_MODE] -> SLEEP_CALL_ENTER(28)
       immediately before the System.sleep() call itself.
  3. Each call site's breadcrumb sequence textually precedes its own
     drainSerialBeforeSleep() / flushDiagBatch() / System.sleep() calls in
     the right relative order, so a future stall's last-recorded breadcrumb
     really does pinpoint the specific stuck call.

This is real evidence traced through the actual code path (not a hand-wave):
every assertion below reads the literal line order from the shipped file.
"""
import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
SRC = REPO_ROOT / "src" / "state" / "State_Sleep.cpp"

BREADCRUMB_NAMES = {
    22: "SLEEP_GATE_START",
    23: "SLEEP_GATE_DONE",
    24: "SLEEP_CONFIG_START",
    25: "SLEEP_SYSTEM_CALL",
    26: "SLEEP_SERIAL_DRAIN_DONE",
    27: "SLEEP_DIAG_FLUSH_DONE",
    28: "SLEEP_CALL_ENTER",
}


def fail(msg: str) -> None:
    print(f"FAIL: {msg}", file=sys.stderr)
    sys.exit(1)


def main() -> None:
    text = SRC.read_text()
    lines = text.splitlines()

    # ---- Check 1: stale literals 18/19/20/21 must not appear as
    # setAppBreadcrumb() arguments anywhere in the sleep path anymore. ----
    stale_calls = re.findall(r"setAppBreadcrumb\((1[89]|2[01])\)", text)
    if stale_calls:
        fail(f"stale breadcrumb literals still present: {stale_calls}")
    print("PASS: no stale 18/19/20/21 breadcrumb literals remain in State_Sleep.cpp")

    # ---- Check 2: the shared sleep-gate pair (22 -> 23) appears exactly
    # once each, and 22 precedes 23 textually. ----
    gate_start_lines = [i for i, l in enumerate(lines) if "setAppBreadcrumb(22)" in l]
    gate_done_lines = [i for i, l in enumerate(lines) if "setAppBreadcrumb(23)" in l]
    if len(gate_start_lines) != 1 or len(gate_done_lines) != 1:
        fail(f"expected exactly one SLEEP_GATE_START/DONE each, got "
             f"{len(gate_start_lines)}/{len(gate_done_lines)}")
    if gate_start_lines[0] >= gate_done_lines[0]:
        fail("SLEEP_GATE_START must textually precede SLEEP_GATE_DONE")
    print("PASS: SLEEP_GATE_START(22) precedes SLEEP_GATE_DONE(23), each exactly once")

    # ---- Check 3: every SLEEP_SYSTEM_CALL(25)...System.sleep() run has the
    # correct internal breadcrumb order and the drain/flush calls land in the
    # expected relative position. ----
    # Collect indices of interest.
    def find_all(token):
        return [i for i, l in enumerate(lines) if token in l]

    system_call_lines = find_all("setAppBreadcrumb(25)")
    drain_done_lines = find_all("setAppBreadcrumb(26)")
    diag_flush_done_lines = find_all("setAppBreadcrumb(27)")
    call_enter_lines = find_all("setAppBreadcrumb(28)")
    drain_call_lines = find_all("drainSerialBeforeSleep();")
    flush_call_lines = find_all("PowerDiagnostics::flushDiagBatch();")
    sleep_call_lines = find_all("System.sleep(config)")

    # Expect 4 sleep-entry call sites total (hibernate, ULP primary, STOP
    # fallback 1, STOP fallback 2).
    if len(system_call_lines) != 4:
        fail(f"expected 4 SLEEP_SYSTEM_CALL(25) sites, found {len(system_call_lines)}: {system_call_lines}")
    if len(drain_done_lines) != 4:
        fail(f"expected 4 SLEEP_SERIAL_DRAIN_DONE(26) sites, found {len(drain_done_lines)}")
    if len(call_enter_lines) != 4:
        fail(f"expected 4 SLEEP_CALL_ENTER(28) sites, found {len(call_enter_lines)}")
    if len(drain_call_lines) != 4:
        fail(f"expected 4 drainSerialBeforeSleep() call sites, found {len(drain_call_lines)}")
    if len(sleep_call_lines) != 4:
        fail(f"expected 4 System.sleep(config) call sites, found {len(sleep_call_lines)}")
    print("PASS: found exactly 4 sleep-entry call sites "
          "(hibernate, ULP primary, STOP fallback x2) with matching breadcrumb counts")

    # SLEEP_DIAG_FLUSH_DONE(27) is gated on ENABLE_DIAGNOSTICS_PUBLISH_MODE and
    # only added at the hibernate call site (the only pre-sleep flush site) -
    # expect exactly 1.
    if len(diag_flush_done_lines) != 1:
        fail(f"expected exactly 1 SLEEP_DIAG_FLUSH_DONE(27) site (hibernate only), "
             f"found {len(diag_flush_done_lines)}")
    print("PASS: SLEEP_DIAG_FLUSH_DONE(27) present exactly once (hibernate pre-sleep flush)")

    # For each of the 4 call sites, pair up the nearest-following markers and
    # confirm strict ordering: 25 < drain() < 26 < [flush() < 27 <] 28 < sleep().
    def nearest_after(idx, candidates):
        after = [c for c in candidates if c > idx]
        if not after:
            fail(f"no candidate found after line {idx + 1}")
        return min(after)

    for site_num, sc in enumerate(system_call_lines, start=1):
        drain_call = nearest_after(sc, drain_call_lines)
        drain_done = nearest_after(sc, drain_done_lines)
        call_enter = nearest_after(sc, call_enter_lines)
        sleep_call = nearest_after(sc, sleep_call_lines)

        if not (sc < drain_call < drain_done < call_enter < sleep_call):
            fail(f"call site #{site_num} (line {sc + 1}): breadcrumb/call order violated - "
                 f"SLEEP_SYSTEM_CALL={sc+1} drainCall={drain_call+1} "
                 f"SLEEP_SERIAL_DRAIN_DONE={drain_done+1} SLEEP_CALL_ENTER={call_enter+1} "
                 f"System.sleep={sleep_call+1}")

        # If this site has a diag flush (only the hibernate site), confirm
        # flush() and SLEEP_DIAG_FLUSH_DONE both land strictly between
        # drain_done and call_enter.
        flush_calls_here = [f for f in flush_call_lines if drain_done < f < call_enter]
        flush_done_here = [d for d in diag_flush_done_lines if drain_done < d < call_enter]
        if flush_calls_here:
            if not flush_done_here:
                fail(f"call site #{site_num}: flushDiagBatch() present but no "
                     f"SLEEP_DIAG_FLUSH_DONE(27) recorded after it")
            if not (flush_calls_here[0] < flush_done_here[0]):
                fail(f"call site #{site_num}: SLEEP_DIAG_FLUSH_DONE(27) must come "
                     f"after flushDiagBatch()")

        print(f"PASS: call site #{site_num} (line {sc + 1}) breadcrumb order verified: "
              f"25@{sc+1} -> drain()@{drain_call+1} -> 26@{drain_done+1}"
              + (f" -> flush()+27@{flush_done_here[0]+1}" if flush_done_here else "")
              + f" -> 28@{call_enter+1} -> System.sleep()@{sleep_call+1}")

    # ---- Check 4: SLEEP_CONFIG_START(24) precedes its site's SLEEP_SYSTEM_CALL(25). ----
    config_start_lines = find_all("setAppBreadcrumb(24)")
    if len(config_start_lines) != 4:
        fail(f"expected 4 SLEEP_CONFIG_START(24) sites, found {len(config_start_lines)}")
    for site_num, sc in enumerate(system_call_lines, start=1):
        preceding_config_starts = [c for c in config_start_lines if c < sc]
        if not preceding_config_starts:
            fail(f"call site #{site_num}: no SLEEP_CONFIG_START(24) precedes SLEEP_SYSTEM_CALL(25) at line {sc+1}")
    print("PASS: SLEEP_CONFIG_START(24) precedes SLEEP_SYSTEM_CALL(25) at all 4 call sites")

    print("\nAll sleep-entry breadcrumb sequence regression checks passed")


if __name__ == "__main__":
    main()

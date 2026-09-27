#!/usr/bin/env python3
"""Host/source-fidelity test for WO-2026-09-24-001 daily cleanup boundaries."""

import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
STATE_REPORT = REPO_ROOT / "src" / "state" / "State_Report.cpp"
STATE_COMMON = REPO_ROOT / "src" / "state" / "State_Common.h"
STATE_MACHINE = REPO_ROOT / "src" / "state" / "StateMachine.h"
APP_MAIN = REPO_ROOT / "src" / "Generalized-Core-Counter.cpp"

SECONDS_PER_DAY = 86400


def fail(message: str) -> None:
    print(f"FAIL: {message}", file=sys.stderr)
    sys.exit(1)


def extract_function(text: str, signature_pattern: str, name: str) -> str:
    match = re.search(signature_pattern, text)
    if not match:
        fail(f"could not locate {name}")
    start = match.start()
    depth = 0
    for index in range(match.end() - 1, len(text)):
        if text[index] == "{":
            depth += 1
        elif text[index] == "}":
            depth -= 1
            if depth == 0:
                return text[start:index + 1]
    fail(f"could not find closing brace for {name}")
    return ""


def strip_comment_lines(text: str) -> str:
    return "\n".join(
        line for line in text.splitlines() if not line.strip().startswith("//")
    )


def local_boundary_utc(now: int, open_hour: int, close_hour: int) -> int:
    close = 24 if open_hour == close_hour else close_hour
    day_start = (now // SECONDS_PER_DAY) * SECONDS_PER_DAY
    boundary = day_start + (SECONDS_PER_DAY if close == 24 else close * 3600)
    if now < boundary:
        boundary -= SECONDS_PER_DAY
    return boundary


def cleanup_due(trusted: bool, now: int, last_cleanup: int, open_hour: int, close_hour: int):
    if not trusted:
        return False, last_cleanup, None
    boundary = local_boundary_utc(now, open_hour, close_hour)
    due = last_cleanup < boundary or last_cleanup > now
    return due, (now if due else last_cleanup), boundary


def close_session_seconds(session_start: int, boundary: int) -> int:
    return max(0, boundary - session_start)


def restart_session_start(boundary: int, original_session_start: int) -> int:
    return max(boundary, original_session_start)


def index_after(text: str, needle: str, previous: int, label: str) -> int:
    found = text.find(needle, previous + 1)
    if found == -1:
        fail(f"missing ordered token for {label}: {needle}")
    return found


def verify_source_shape() -> None:
    report_text = STATE_REPORT.read_text()
    common_text = STATE_COMMON.read_text()
    machine_text = STATE_MACHINE.read_text()
    app_text = APP_MAIN.read_text()
    report_code = strip_comment_lines(report_text)

    if "time/LocalTimeCache.h" in report_code or "LocalTimeCache::" in report_code:
        fail("State_Report.cpp daily boundary logic must not use LocalTimeCache")

    local_today = extract_function(
        report_text, r"time_t\s+localTodayAt\s*\(\s*uint8_t\s+\w+\s*\)\s*\{", "localTodayAt"
    )
    for required in (
        "LocalTimeConvert",
        "withCurrentTime()",
        "hour == 24",
        'nextDay(LocalTimeHMS("00:00:00"))',
        "atLocalTime",
    ):
        if required not in local_today:
            fail(f"localTodayAt() must compute live LocalTimeRK boundaries and handle hour 24; missing {required}")
    if "LocalTimeCache" in local_today:
        fail("localTodayAt() must never read LocalTimeCache")

    handle = extract_function(
        report_text, r"void\s+handleReportingState\s*\(\s*\)\s*\{", "handleReportingState"
    )
    handle_code = strip_comment_lines(handle)
    daily_cleanup = handle_code.find("dailyCleanup();")
    publish_calls = re.findall(r"\bpublishData\s*\(", handle_code)
    report_publish = re.search(r"\bpublishData\s*\(\s*due\s*\?\s*boundary\s*-\s*1\s*:\s*0\s*\)", handle_code)
    hourly_reset = handle_code.find("CurrentReadings::set_hourlyCount(0)")
    last_report_stamp = handle_code.find("SystemConfig::set_lastReport(now)")
    if -1 in (daily_cleanup, last_report_stamp, hourly_reset) or report_publish is None:
        fail("handleReportingState() must retain dailyCleanup(), closing-record publish, lastReport stamp, and hourlyCount reset")
    if len(publish_calls) != 1:
        fail("handleReportingState() must contain exactly one publishData() call per report")
    publish_pos = report_publish.start()

    ordered_tokens = [
        ("due default", "bool due = false"),
        ("trusted-clock gate", "if (Clock::isTrusted())"),
        ("open hour", "SystemConfig::get_openTime()"),
        ("close hour", "SystemConfig::get_closeTime()"),
        ("always-open normalization", "close = (openHour == closeHour) ? 24 : closeHour"),
        ("boundary computation", "localTodayAt(close)"),
        ("before-boundary condition", "if (now < boundary)"),
        ("before-boundary adjustment", "boundary -= 86400"),
        ("lastDailyCleanup read", "SystemConfig::get_lastDailyCleanup()"),
        ("due comparison", "due = (lastDailyCleanup < boundary || lastDailyCleanup > now)"),
        ("occupied snapshot", "wasOccupied = CurrentReadings::get_occupied()"),
        ("original session start capture", "CurrentReadings::get_occupancyStartTime()"),
        ("boundary-aware close", 'closeOccupancySessionSafely("daily-cleanup", boundary)'),
        ("sensor measurement", "measure.loop()"),
        ("battery measurement", "measure.batteryState()"),
        ("closing-record publish", "publishData(due ? boundary - 1 : 0)"),
        ("daily reset", "dailyCleanup();"),
        ("always-open restart", "close == 24 && wasOccupied"),
        ("restart occupied", "CurrentReadings::set_occupied(true)"),
        ("restart time", "CurrentReadings::set_occupancyStartTime(std::max(boundary, originalSessionStart))"),
        ("lastDailyCleanup stamp", "SystemConfig::set_lastDailyCleanup(now)"),
    ]
    position = -1
    for label, token in ordered_tokens:
        position = index_after(handle_code, token, position, label)

    # Stage 5 decision 7: the always-open restart must never start before the
    # person actually arrived, so an unconditional boundary start is rejected.
    if re.search(r"set_occupancyStartTime\s*\(\s*boundary\s*\)", handle_code):
        fail("always-open restart must use max(boundary, originalSessionStart), not boundary unconditionally")

    if handle_code.find("SystemConfig::get_lastReport()", 0, daily_cleanup) != -1:
        fail("daily boundary decision must not use lastReport")
    if not (handle_code.find("due = (lastDailyCleanup < boundary || lastDailyCleanup > now)") < publish_pos < daily_cleanup):
        fail("boundary due computation must precede the single publish, and publish must precede dailyCleanup()")
    if last_report_stamp < daily_cleanup:
        fail("lastReport must not be stamped by the daily boundary block")
    if not (publish_pos < daily_cleanup < last_report_stamp < hourly_reset):
        fail("report publish, daily cleanup, lastReport stamp, and hourlyCount reset ordering is wrong")
    if any(token in handle_code[:daily_cleanup] for token in ("getYear()", "getMonth()", "getDay()")):
        fail("daily boundary logic must not compare calendar Y/M/D fields")

    default_declarations = sum(
        text.count("publishData(time_t stampOverride = 0)")
        for text in (common_text, machine_text, app_text)
    )
    if default_declarations != 1 or "publishData(time_t stampOverride = 0)" not in common_text:
        fail("publishData() default argument must appear exactly once, in State_Common.h")
    if "void publishData(time_t stampOverride);" not in machine_text:
        fail("StateMachine.h must declare publishData(time_t stampOverride) without a default argument")
    if "void publishData(time_t stampOverride);" not in app_text:
        fail("Generalized-Core-Counter.cpp forward declaration must omit the default argument")

    publish_impl = extract_function(
        app_text, r"void\s+publishData\s*\(\s*time_t\s+\w+\s*\)\s*\{", "publishData"
    )
    if "stampOverride != 0 ? (unsigned long)stampOverride : nowStampSec" not in publish_impl:
        fail("publishData() must apply a nonzero stamp override to the occupancy payload timestamp")
    occupancy_branch = publish_impl.split("if (sensorMode == SystemConfig::OCCUPANCY)", 1)[1].split("} else {", 1)[0]
    counting_branch = publish_impl.split("} else {", 1)[1]
    if "stampOverride" not in occupancy_branch:
        fail("occupancy payload must use the stamp override")
    if "stampOverride" in counting_branch:
        fail("counting payload must not use the stamp override")
    if "const unsigned long timeStampValue = endOfPrevHourStampSec;" not in counting_branch:
        fail("counting payload timestamp must remain endOfPrevHourStampSec")

    close_helper = extract_function(
        common_text,
        r"inline\s+OccupancyCloseResult\s+closeOccupancySessionSafely\s*\(\s*const\s+char\s*\*\s*\w+\s*,\s*time_t\s+\w+\s*=\s*Time\.now\(\)\s*\)\s*\{",
        "closeOccupancySessionSafely",
    )
    for required in (
        "effectiveStart > closeAt",
        "effectiveStart = closeAt",
        "rawSessionSeconds = (long long)closeAt - (long long)effectiveStart",
        "sessionSeconds > 86400UL",
        "CurrentReadings::set_occupied(false)",
        "CurrentReadings::set_occupancyStartTime(0)",
    ):
        if required not in close_helper:
            fail(f"closeOccupancySessionSafely() must support boundary-aware close while preserving safety checks; missing {required}")

    for src_name in ("State_Idle.cpp", "State_Modes.cpp", "State_Sleep.cpp"):
        src = (REPO_ROOT / "src" / "state" / src_name).read_text()
        if re.search(r"closeOccupancySessionSafely\s*\(\s*\"[^\"]+\"\s*,", src):
            fail(f"{src_name} should keep existing default close-at-Time.now() behavior")


def verify_acceptance_model() -> None:
    close_22 = 22 * 3600
    just_after_close = 2 * SECONDS_PER_DAY + close_22 + 60

    due, stamped, boundary = cleanup_due(True, just_after_close, 0, 6, 22)
    assert due and stamped == just_after_close and boundary == 2 * SECONDS_PER_DAY + close_22
    due_again, stamped_again, _ = cleanup_due(True, just_after_close + 60, stamped, 6, 22)
    assert not due_again and stamped_again == stamped

    # Exactly at park close, the boundary is today's close (strict now < boundary),
    # so cleanup is due at that instant, not one report later.
    exactly_at_close = 2 * SECONDS_PER_DAY + close_22
    yesterday_stamp = 1 * SECONDS_PER_DAY + close_22 + 60
    due_exact, _, exact_boundary = cleanup_due(True, exactly_at_close, yesterday_stamp, 6, 22)
    assert due_exact and exact_boundary == exactly_at_close

    midnight_sensor_now = 5 * SECONDS_PER_DAY + 3600
    due_midnight, _, midnight_boundary = cleanup_due(True, midnight_sensor_now, 0, 7, 7)
    assert due_midnight and midnight_boundary == 5 * SECONDS_PER_DAY

    untrusted_due, untrusted_stamp, _ = cleanup_due(False, just_after_close, 0, 6, 22)
    assert not untrusted_due and untrusted_stamp == 0
    trusted_due, trusted_stamp, _ = cleanup_due(True, just_after_close, untrusted_stamp, 6, 22)
    assert trusted_due and trusted_stamp == just_after_close

    assert close_session_seconds(boundary - 300, boundary) == 300
    assert close_session_seconds(boundary + 300, boundary) == 0

    # Stage 5 decision 7: late always-open restart starts at max(boundary, sessionStart).
    assert restart_session_start(midnight_boundary, midnight_boundary - 600) == midnight_boundary
    assert restart_session_start(midnight_boundary, midnight_boundary + 1800) == midnight_boundary + 1800

    future_stamp_due, _, _ = cleanup_due(True, just_after_close, just_after_close + 3600, 6, 22)
    assert future_stamp_due

    mar_1_after_close = 59 * SECONDS_PER_DAY + close_22 + 60
    feb_28_before_close = 58 * SECONDS_PER_DAY + close_22 - 60
    month_rollover_due, _, month_boundary = cleanup_due(True, mar_1_after_close, feb_28_before_close, 6, 22)
    assert month_rollover_due and month_boundary == 59 * SECONDS_PER_DAY + close_22


def main() -> None:
    verify_source_shape()
    verify_acceptance_model()
    print("daily_cleanup_boundary_test: source fidelity and acceptance-model checks passed")


if __name__ == "__main__":
    main()

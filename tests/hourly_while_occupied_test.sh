#!/bin/zsh
set -euo pipefail

# WO-2026-10-02-002 (v33-HourlyWhileOccupied): the scheduled report goes out
# every hour whether or not the site is occupied.
#
# Part 1 lifts the REAL reportDueThisInterval() out of
# src/state/State_Common.h and compiles it into
# hourly_while_occupied_test.cpp, together with -D flags describing what the
# real occupied-branch of each of the three sites was found to do. The
# behavioral simulations there therefore run the production due rule through
# the production structure.
#
# Part 2 asserts the real sources against the work order directly: one shared
# due test (not three copies), the clock-interval rule, the three sites, the
# untouched unoccupied branches, and the protected v28/v32 behavior.
#
# Set HOURLY_WHILE_OCCUPIED_SRC_ROOT to point Part 1 and Part 2 at a copy of
# src/ (mutation testing never touches the production tree).

repo_root="${0:A:h:h}"
src_root="${HOURLY_WHILE_OCCUPIED_SRC_ROOT:-$repo_root/src}"
work_dir=$(mktemp -d)
trap 'rm -rf "$work_dir"' EXIT

binary="$work_dir/hourly_while_occupied_test"
helper_header="$work_dir/real_due_helper.h"

common_src="$src_root/state/State_Common.h"
sleep_src="$src_root/state/State_Sleep.cpp"
idle_src="$src_root/state/State_Idle.cpp"
report_src="$src_root/state/State_Report.cpp"
policy_src="$src_root/power/ConnectivityPolicy.h"

for f in "$common_src" "$sleep_src" "$idle_src" "$report_src" "$policy_src"; do
  [[ -f "$f" ]] || { echo "FAILED: missing source $f" >&2; exit 1; }
done

fail() {
  echo "FAILED: $1" >&2
  exit 1
}

# --- Shared extractor -----------------------------------------------------
# Brace matching is done on a scrubbed copy (line comments, block comments and
# string literals removed) so braces inside them cannot confuse it.
extract() {
  python3 - "$@" <<'PY'
import re
import sys


def scrub(text):
    """Blank out comments and string/char literals, preserving offsets."""
    out = list(text)
    i, n = 0, len(text)
    while i < n:
        c = text[i]
        if c == '/' and i + 1 < n and text[i + 1] == '/':
            while i < n and text[i] != '\n':
                out[i] = ' '
                i += 1
        elif c == '/' and i + 1 < n and text[i + 1] == '*':
            while i < n and not (text[i] == '*' and i + 1 < n and text[i + 1] == '/'):
                if text[i] != '\n':
                    out[i] = ' '
                i += 1
            for j in range(i, min(i + 2, n)):
                out[j] = ' '
            i += 2
        elif c in '"\'':
            quote = c
            out[i] = ' '
            i += 1
            while i < n and text[i] != quote:
                if text[i] == '\\':
                    out[i] = ' '
                    i += 1
                if i < n and text[i] != '\n':
                    out[i] = ' '
                i += 1
            if i < n:
                out[i] = ' '
                i += 1
        else:
            i += 1
    return "".join(out)


def block_after(text, scrubbed, start):
    """Return (body, end_index) for the brace block whose '{' follows start."""
    open_idx = scrubbed.find('{', start)
    if open_idx < 0:
        sys.exit("EXTRACT_FAILED: no opening brace after offset %d" % start)
    depth = 0
    for i in range(open_idx, len(scrubbed)):
        if scrubbed[i] == '{':
            depth += 1
        elif scrubbed[i] == '}':
            depth -= 1
            if depth == 0:
                return text[open_idx + 1:i], i
    sys.exit("EXTRACT_FAILED: unbalanced braces")


def find(scrubbed, needle, start=0):
    idx = scrubbed.find(needle, start)
    if idx < 0:
        sys.exit("EXTRACT_FAILED: %r not found" % needle)
    return idx


def occupancy_guard(text, scrubbed, before_idx):
    """Locate the occupancy+keep-alive guard that encloses before_idx.

    The guard nearest before the anchor label is the one that owns it.
    Returns (condition, then_branch, else_branch).
    """
    anchor = "if (SystemConfig::get_sensorMode() == SystemConfig::OCCUPANCY"
    idx = scrubbed.rfind(anchor, 0, before_idx)
    if idx < 0:
        sys.exit("EXTRACT_FAILED: occupancy guard not found before offset %d" % before_idx)
    cond_open = find(scrubbed, '(', idx)
    depth = 0
    cond_close = None
    for i in range(cond_open, len(scrubbed)):
        if scrubbed[i] == '(':
            depth += 1
        elif scrubbed[i] == ')':
            depth -= 1
            if depth == 0:
                cond_close = i
                break
    if cond_close is None:
        sys.exit("EXTRACT_FAILED: unbalanced guard condition")
    condition = text[cond_open + 1:cond_close]
    then_branch, then_end = block_after(text, scrubbed, cond_close)
    tail = scrubbed[then_end + 1:then_end + 40]
    else_branch = ""
    if tail.lstrip().startswith("else"):
        else_start = find(scrubbed, "else", then_end)
        else_branch, _ = block_after(text, scrubbed, else_start)
    return condition, then_branch, else_branch


mode = sys.argv[1]

if mode == "helper":
    text = open(sys.argv[2]).read()
    scrubbed = scrub(text)
    sig = "inline bool reportDueThisInterval()"
    idx = find(scrubbed, sig)
    body, end = block_after(text, scrubbed, idx + len(sig))
    sys.stdout.write("%s {%s}\n" % (sig, body))
    sys.exit(0)

if mode == "site1":
    text = open(sys.argv[2]).read()
    scrubbed = scrub(text)
    label = find(text, '"sleep-timer-occupied-suppress-report"')
    cond, then_branch, _ = occupancy_guard(text, scrubbed, label)
    flat_cond = " ".join(cond.split())
    gated = int("!reportDueThisInterval()" in flat_cond)
    keepalive = int("INTERMITTENT_KEEP_ALIVE" in cond and "get_occupied()" in cond)
    suppresses = int('transitionTo(SLEEPING_STATE, "sleep-timer-occupied-suppress-report")'
                     in " ".join(then_branch.split()))
    after = " ".join(text[label:label + 600].split())
    fallthrough = int('transitionTo(REPORTING_STATE, "sleep-timer-report")' in after)
    print("SITE1_GATED_ON_DUE=%d" % gated)
    print("SITE1_GUARD_INTACT=%d" % (keepalive and suppresses))
    print("SITE1_FALLTHROUGH_REPORTS=%d" % fallthrough)
    sys.exit(0)

if mode in ("site2", "site3"):
    text = open(sys.argv[2]).read()
    scrubbed = scrub(text)
    if mode == "site2":
        report_label = '"sleep-pir-overdue-report"'
        else_needle = "lastReport > 0 && (now - lastReport) >= intervalSec"
        prefix = "SITE2"
    else:
        report_label = '"report interval"'
        else_needle = "lastReport == 0 || (now - lastReport) >= intervalSec"
        prefix = "SITE3"
    label = find(text, report_label)
    cond, then_branch, else_branch = occupancy_guard(text, scrubbed, label)
    flat_then = " ".join(then_branch.split())
    keepalive = int("INTERMITTENT_KEEP_ALIVE" in cond and "get_occupied()" in cond)
    reports = int("reportDueThisInterval()" in flat_then
                  and ("transitionTo(REPORTING_STATE, %s)" % report_label) in flat_then
                  and "return;" in flat_then)
    else_ok = int(" ".join(else_needle.split()) in " ".join(else_branch.split()))
    print("%s_GUARD_INTACT=%d" % (prefix, keepalive))
    print("%s_REPORTS_WHEN_DUE=%d" % (prefix, reports))
    print("%s_ELSE_UNCHANGED=%d" % (prefix, else_ok))
    sys.exit(0)

if mode == "stale":
    text = open(sys.argv[2]).read()
    block = re.search(r"#if CONNECTIVITY_FAILSAFE_TEST_MODE(.*?)#else(.*?)#endif", text, re.S)
    if not block:
        sys.exit("EXTRACT_FAILED: CONNECTIVITY_FAILSAFE_TEST_MODE block not found")
    m = re.search(r"constexpr\s+time_t\s+CONNECTIVITY_FAILSAFE_STALE_SEC\s*=\s*([^;]+);", block.group(2))
    if not m:
        sys.exit("EXTRACT_FAILED: production CONNECTIVITY_FAILSAFE_STALE_SEC not found")
    expr = m.group(1).replace("L", "").strip()
    if not re.fullmatch(r"[0-9*+\s]+", expr):
        sys.exit("EXTRACT_FAILED: stale value is not a plain arithmetic literal")
    print(int(eval(expr)))
    sys.exit(0)

sys.exit("EXTRACT_FAILED: unknown mode %s" % mode)
PY
}

# --- The real due helper, verbatim ---------------------------------------
extract helper "$common_src" > "$helper_header"
grep -q "reportDueThisInterval" "$helper_header" || \
  fail "could not extract reportDueThisInterval() from $common_src"

echo "Real due test extracted from $common_src:"
sed 's/^/  /' "$helper_header"

# --- What each site actually does ----------------------------------------
eval "$(extract site1 "$sleep_src")"
eval "$(extract site2 "$sleep_src")"
eval "$(extract site3 "$idle_src")"
stale_sec=$(extract stale "$policy_src")

echo "Sites as found in the real sources:"
echo "  site1: gated-on-due=$SITE1_GATED_ON_DUE guard-intact=$SITE1_GUARD_INTACT falls-through=$SITE1_FALLTHROUGH_REPORTS"
echo "  site2: reports-when-due=$SITE2_REPORTS_WHEN_DUE guard-intact=$SITE2_GUARD_INTACT else-unchanged=$SITE2_ELSE_UNCHANGED"
echo "  site3: reports-when-due=$SITE3_REPORTS_WHEN_DUE guard-intact=$SITE3_GUARD_INTACT else-unchanged=$SITE3_ELSE_UNCHANGED"
echo "  v32 failsafe stale threshold: ${stale_sec}s"
echo ""

clang++ -std=c++17 -Wall -Wextra -pedantic -Werror \
  -DREAL_DUE_HELPER_HEADER="\"$helper_header\"" \
  -DSITE1_GATED_ON_DUE="$SITE1_GATED_ON_DUE" \
  -DSITE1_FALLTHROUGH_REPORTS="$SITE1_FALLTHROUGH_REPORTS" \
  -DSITE2_REPORTS_WHEN_DUE="$SITE2_REPORTS_WHEN_DUE" \
  -DSITE3_REPORTS_WHEN_DUE="$SITE3_REPORTS_WHEN_DUE" \
  -DREAL_STALE_SEC="$stale_sec" \
  "$repo_root/tests/hourly_while_occupied_test.cpp" \
  -o "$binary"

"$binary"

echo ""
echo "--- Part 2: the real sources against the work order ---"

# One shared due test, not three copies.
definitions=$(grep -rn "bool reportDueThisInterval()" "$src_root" | grep -c "inline bool reportDueThisInterval() {" || true)
(( definitions == 1 )) || fail "there must be exactly one definition of reportDueThisInterval(), found $definitions"
call_sites=$(grep -rn "reportDueThisInterval()" "$src_root" --include='*.cpp' | wc -l | tr -d ' ')
(( call_sites == 3 )) || fail "the due test must be used at exactly the three sites, found $call_sites call sites"
echo "OK: one shared due test, used at the three sites"

# The clock-interval rule, with the work order's inputs.
helper_flat=$(tr '\n' ' ' < "$helper_header" | tr -s ' ')
for needle in "Config::reportingIntervalSecForRuntime()" \
              "SystemConfig::get_lastReport()" \
              "Time.now()" \
              "lastReport == 0"; do
  [[ "$helper_flat" == *"$needle"* ]] || fail "the due test must use $needle"
done
[[ "$helper_flat" == *"Time.now() / interval != lastReport / interval"* ]] || \
  fail "the due test must be the clock-interval rule: now / interval != lastReport / interval"
echo "OK: the due test is lastReport == 0 || now / interval != lastReport / interval"

# The three sites.
(( SITE1_GUARD_INTACT == 1 )) || fail "site 1 must still be gated on occupancy mode + occupied + INTERMITTENT_KEEP_ALIVE"
(( SITE1_GATED_ON_DUE == 1 )) || fail "site 1 must suppress only when a report is NOT due (!reportDueThisInterval())"
(( SITE1_FALLTHROUGH_REPORTS == 1 )) || fail "site 1 must still fall through to sleep-timer-report"
(( SITE2_GUARD_INTACT == 1 )) || fail "site 2 must still be gated on occupancy mode + occupied + INTERMITTENT_KEEP_ALIVE"
(( SITE2_REPORTS_WHEN_DUE == 1 )) || fail "site 2 must report sleep-pir-overdue-report while occupied when due"
(( SITE3_GUARD_INTACT == 1 )) || fail "site 3 must still be gated on occupancy mode + occupied + INTERMITTENT_KEEP_ALIVE"
(( SITE3_REPORTS_WHEN_DUE == 1 )) || fail "site 3 must report \"report interval\" while occupied when due"
echo "OK: all three sites report when due while occupied"

# Unoccupied branches untouched.
(( SITE2_ELSE_UNCHANGED == 1 )) || fail "site 2's unoccupied branch must keep lastReport > 0 && (now - lastReport) >= intervalSec"
(( SITE3_ELSE_UNCHANGED == 1 )) || fail "site 3's unoccupied branch must keep lastReport == 0 || (now - lastReport) >= intervalSec"
echo "OK: the unoccupied rules at all three sites are unchanged"

# Protected behavior: occupancy-change reports and the return-to-sleep path.
for needle in "sleep-pir-occupancy-report" "sleep-occupancy-debounce-report" \
              "sleep-pir-return-to-sleep" "sleep-timer-report"; do
  grep -q "\"$needle\"" "$sleep_src" || fail "the protected transition $needle must still exist"
done
echo "OK: occupancy-change reports and the return-to-sleep path are untouched"

# The report path still closes a session only at the daily close.
closes=$(grep -c "closeOccupancySessionSafely(" "$report_src" || true)
(( closes == 1 )) || fail "State_Report.cpp must close a session exactly once (the daily close), found $closes"
grep -q 'closeOccupancySessionSafely("daily-cleanup"' "$report_src" || \
  fail "the only session close in the report path must be the daily close"
echo "OK: a scheduled report never closes or restarts the occupancy session"

echo ""
echo "hourly_while_occupied_test (real due test + real site structure) passed"

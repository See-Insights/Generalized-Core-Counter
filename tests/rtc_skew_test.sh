#!/bin/zsh
set -euo pipefail

# WO-2026-08-31-003: RTC skew bench hook (ENABLE_RTC_SKEW_TEST), Amendment A.
#
# This test operates on the REAL shipped source (not a copy or a re-typed
# mirror) throughout:
#
#   Step 1 - static structural checks against the real
#     src/Generalized-Core-Counter.cpp / src/BuildProfile.h:
#       - the bench-hook block is gated behind
#         `#if PLATFORM_ID == PLATFORM_BORON && ENABLE_RTC_SKEW_TEST`
#       - BuildProfile.h defaults ENABLE_RTC_SKEW_TEST to 0, with the same
#         0/1 #error guard every other flag carries, and the app emits a
#         #warning when it is enabled
#       - A.6 gap 1: the RTC-write call (`ab1805.setRtcFromTime(`) is
#         actually NESTED inside the `if (rtcSkewTestGuard.shouldRun())`
#         block, by brace-depth counting - not merely that the guard line
#         is present somewhere in the file. This is what makes "replace the
#         guarded if with an unconditional block" a check FAILURE, not a
#         silent pass.
#       - A.3: `Time.setTime(` does not appear anywhere inside the
#         bench-hook block.
#       - A.4: the write's return value (`rtcSkewWriteOk`) is branched on
#         before the "fired" log line, not discarded.
#       - Round 4 MEDIUM: `RtcSkewTest::rearmIfBuildTokenChanged(` actually
#         APPEARS in the production hook, and appears BEFORE
#         `RtcSkewTest::OneShotGuard rtcSkewTestGuard(` is constructed, by
#         line-number ordering. This is what makes "delete the production
#         re-arm call, leaving OneShotGuard directly over the retained
#         bool" - the exact round-2 composition that previously failed
#         Stage 7 - a check FAILURE instead of a silent pass; round 3's
#         suite proved the guard's own logic correct in isolation but
#         never proved production actually invokes the re-arm call before
#         consulting the guard.
#     This is a source-level check only - it is NOT the authoritative
#     "absent from the compiled artifact" proof (that requires a real
#     Boron build + nm, which this script does not perform - see the
#     Work Order report for the actual nm-based verification run).
#
#   Step 2 - A.6 gap 2: extracts isRtcTimeValidForHibernate()'s REAL
#     function body verbatim from src/state/State_Sleep.cpp (not a
#     reimplementation of its two bounds) and compiles it, unmodified,
#     into a host harness alongside the real RtcSkewTest::applySkew()/
#     chooseAnchor() from src/time/RtcSkewTest.h. This means a mutation
#     that changes the REAL isRtcTimeValidForHibernate() to always return
#     false is exercised by this test directly - not sidestepped by a
#     separate reimplementation of the bounds it happens to use today.
#     Also fixes the previously-broken sweep, which claimed to cover
#     2024-06-01 through 2034-06-01 but actually stopped at 2033-05-30
#     (fixed 31,536,000s/year steps drift against real calendar years via
#     leap days) - this version uses real calendar dates for every year in
#     the range, computed by `date`, not a fixed-step approximation.

repo_root="${0:A:h:h}"
app_src="$repo_root/src/Generalized-Core-Counter.cpp"
sleep_src="$repo_root/src/state/State_Sleep.cpp"
build_profile="$repo_root/src/BuildProfile.h"
rtc_skew_header="$repo_root/src/time/RtcSkewTest.h"

# A.5: visible, descriptively-named scratch directory - no leading dot on
# the directory OR the files inside it - cleaned up on every exit path
# (success, failure, or interrupt), not just the success path.
build_tmp_dir="$repo_root/build-tmp"
harness_src="$build_tmp_dir/rtc_skew_harness.cpp"
harness_bin="$build_tmp_dir/rtc_skew_harness_bin"

# Remember whether build-tmp/ already existed before this script touches it,
# so cleanup only removes the directory THIS SCRIPT created - never an
# existing directory that might hold unrelated files from something else.
build_tmp_dir_preexisted=0
if [[ -d "$build_tmp_dir" ]]; then
  build_tmp_dir_preexisted=1
fi

cleanup() {
  rm -f "$harness_src" "$harness_bin"
  # Only remove the directory if it did not exist before this script ran.
  # `rmdir` (not `rm -rf`) is deliberate: it fails harmlessly and silently
  # if anything else is still in there, so unrelated contents are never
  # blown away.
  if [[ "$build_tmp_dir_preexisted" -eq 0 ]]; then
    rmdir "$build_tmp_dir" 2>/dev/null || true
  fi
}
trap cleanup EXIT INT TERM

mkdir -p "$build_tmp_dir"

echo "--- Step 1: static structural checks (not the compiled-artifact proof - see header comment) ---"

if ! grep -q '#if PLATFORM_ID == PLATFORM_BORON && ENABLE_RTC_SKEW_TEST' "$app_src"; then
  echo "FAILED: bench hook is not gated behind '#if PLATFORM_ID == PLATFORM_BORON && ENABLE_RTC_SKEW_TEST' in $app_src" >&2
  exit 1
fi
echo "OK: bench hook block is gated behind PLATFORM_ID==PLATFORM_BORON && ENABLE_RTC_SKEW_TEST"

if ! awk '
  /#ifndef ENABLE_RTC_SKEW_TEST/ { infl=1 }
  infl && /#define ENABLE_RTC_SKEW_TEST 0/ { found=1 }
  infl && /#endif/ { infl=0 }
  END { exit(found ? 0 : 1) }
' "$build_profile"; then
  echo "FAILED: ENABLE_RTC_SKEW_TEST does not default to 0 in $build_profile" >&2
  exit 1
fi
echo "OK: ENABLE_RTC_SKEW_TEST defaults to 0 in BuildProfile.h"

if ! grep -q '#error "ENABLE_RTC_SKEW_TEST must be 0 or 1"' "$build_profile"; then
  echo "FAILED: ENABLE_RTC_SKEW_TEST is missing the 0/1 guard #error in $build_profile" >&2
  exit 1
fi
echo "OK: ENABLE_RTC_SKEW_TEST carries the same 0/1 #error guard as the other flags"

if ! grep -q '#warning "ENABLE_RTC_SKEW_TEST=1' "$app_src"; then
  echo "FAILED: no #warning for ENABLE_RTC_SKEW_TEST=1 in $app_src" >&2
  exit 1
fi
echo "OK: an enabled build emits a #warning, matching CONNECTIVITY_FAILSAFE_TEST_MODE's pattern"

# A.6 gap 1: prove the RTC-write call is nested INSIDE the
# `if (rtcSkewTestGuard.shouldRun())` block via brace-depth counting, so a
# mutation that replaces that guarded if with an unconditional block is
# actually caught (the write would then be found before the guard's own
# opening brace is ever seen, or the guard line would be gone entirely).
gap1_result=$(python3 - "$app_src" <<'PY'
import sys

path = sys.argv[1]
with open(path) as f:
    lines = f.readlines()

guard_line_idx = None
for i, line in enumerate(lines):
    if "if (rtcSkewTestGuard.shouldRun())" in line:
        guard_line_idx = i
        break

if guard_line_idx is None:
    print("FAIL: no 'if (rtcSkewTestGuard.shouldRun())' line found")
    sys.exit(0)

depth = 0
opened = False
write_call_seen_inside = False
end_idx = None
for i in range(guard_line_idx, len(lines)):
    line = lines[i]
    for ch in line:
        if ch == '{':
            depth += 1
            opened = True
        elif ch == '}':
            depth -= 1
    if opened and "ab1805.setRtcFromTime(" in line:
        write_call_seen_inside = True
    if opened and depth == 0:
        end_idx = i
        break

if end_idx is None:
    print("FAIL: guard block's closing brace was not found")
elif not write_call_seen_inside:
    print("FAIL: ab1805.setRtcFromTime( was not found nested inside the guarded if-block")
else:
    print("PASS")
PY
)
if [[ "$gap1_result" != "PASS" ]]; then
  echo "FAILED (A.6 gap 1): $gap1_result" >&2
  exit 1
fi
echo "OK (A.6 gap 1): ab1805.setRtcFromTime( is nested inside the guarded 'if (rtcSkewTestGuard.shouldRun())' block"

# A.3: Time.setTime( must not appear anywhere in the bench-hook block AS
# CODE (comment-only mentions, e.g. explaining its absence, don't count -
# strip full-line "//" comments before checking).
hook_block=$(awk '
  /#if PLATFORM_ID == PLATFORM_BORON && ENABLE_RTC_SKEW_TEST/ { infl++; if (infl==2) capture=1 }
  capture { print }
  capture && /#endif/ { if (infl==2) exit }
' "$app_src")
hook_block_code_only=$(echo "$hook_block" | grep -v '^[[:space:]]*//')
if echo "$hook_block_code_only" | grep -q 'Time\.setTime('; then
  echo "FAILED (A.3): Time.setTime( found inside the bench-hook block - the hook must write the RTC only" >&2
  exit 1
fi
echo "OK (A.3): Time.setTime( is absent from the bench-hook block"

# A.4: the write's return value must be checked (branched on), not discarded.
if ! echo "$hook_block" | grep -q 'if (rtcSkewWriteOk)'; then
  echo "FAILED (A.4): setRtcFromTime()'s return value does not appear to be checked before use" >&2
  exit 1
fi
echo "OK (A.4): setRtcFromTime()'s return value is checked before the write is treated as done"

# Round 4 MEDIUM: bind the production re-arm call
# (RtcSkewTest::rearmIfBuildTokenChanged() must appear, and must appear
# BEFORE RtcSkewTest::OneShotGuard is constructed. This is a STRUCTURAL
# ordering check, not merely "both strings exist somewhere in the file" -
# it catches deleting the re-arm call entirely, AND reordering it to after
# the guard is constructed, either of which reproduces the round-2
# composition (OneShotGuard directly over the retained bool, with no
# re-arm ever applied) that previously failed Stage 7 while every other
# check in this suite kept passing.
rearm_call_line=$(grep -vn '^[[:space:]]*//' "$app_src" | grep 'RtcSkewTest::rearmIfBuildTokenChanged(' | head -1 | cut -d: -f1)
guard_ctor_line=$(grep -vn '^[[:space:]]*//' "$app_src" | grep 'RtcSkewTest::OneShotGuard rtcSkewTestGuard(' | head -1 | cut -d: -f1)

if [[ -z "$rearm_call_line" ]]; then
  echo "FAILED (round 4 MEDIUM): RtcSkewTest::rearmIfBuildTokenChanged( call not found in $app_src - the production re-arm call may have been deleted" >&2
  exit 1
fi
if [[ -z "$guard_ctor_line" ]]; then
  echo "FAILED (round 4 MEDIUM): RtcSkewTest::OneShotGuard rtcSkewTestGuard( construction not found in $app_src" >&2
  exit 1
fi
if (( rearm_call_line >= guard_ctor_line )); then
  echo "FAILED (round 4 MEDIUM): RtcSkewTest::rearmIfBuildTokenChanged( (line $rearm_call_line) does not appear BEFORE RtcSkewTest::OneShotGuard rtcSkewTestGuard( is constructed (line $guard_ctor_line) - the re-arm must run before the guard is consulted" >&2
  exit 1
fi
echo "OK (round 4 MEDIUM): RtcSkewTest::rearmIfBuildTokenChanged( (line $rearm_call_line) appears before RtcSkewTest::OneShotGuard rtcSkewTestGuard( is constructed (line $guard_ctor_line)"

echo ""
echo "--- Step 2: A.6 gap 2 - isRtcTimeValidForHibernate() acceptance, using the REAL function body ---"

# Extract isRtcTimeValidForHibernate()'s real body VERBATIM from
# State_Sleep.cpp - not a reimplementation - so a mutation to the real
# predicate (e.g. "always return false") is exercised by this test
# directly.
real_predicate=$(python3 - "$sleep_src" <<'PY'
import sys

path = sys.argv[1]
with open(path) as f:
    lines = f.readlines()

start_idx = None
for i, line in enumerate(lines):
    if "bool isRtcTimeValidForHibernate(time_t rtcNow) {" in line:
        start_idx = i
        break

if start_idx is None:
    sys.exit("EXTRACT_FAILED: signature not found")

depth = 0
opened = False
end_idx = None
for i in range(start_idx, len(lines)):
    for ch in lines[i]:
        if ch == '{':
            depth += 1
            opened = True
        elif ch == '}':
            depth -= 1
    if opened and depth == 0:
        end_idx = i
        break

if end_idx is None:
    sys.exit("EXTRACT_FAILED: closing brace not found")

sys.stdout.write("".join(lines[start_idx:end_idx + 1]))
PY
)
if [[ -z "$real_predicate" ]]; then
  echo "FAILED: could not extract isRtcTimeValidForHibernate() verbatim from $sleep_src" >&2
  exit 1
fi
echo "Extracted the real isRtcTimeValidForHibernate() body verbatim from $sleep_src"

# Build the real calendar-date sweep (2024-06-01 through 2034-06-01
# inclusive, one anchor per year) using `date`, not a fixed-step
# approximation - this is the fix for the sweep bug (previously stopped at
# 2033-05-30 instead of 2034-06-01, a leap-year drift bug in the fixed
# 31,536,000s/year step).
sweep_anchors=()
for year in {2024..2034}; do
  epoch=$(TZ=UTC date -j -f "%Y-%m-%d %H:%M:%S" "${year}-06-01 00:00:00" +%s)
  sweep_anchors+=("$epoch")
done
echo "Sweep anchors (2024-06-01 .. 2034-06-01, one per year): ${sweep_anchors[*]}"

{
  echo '#include "time/RtcSkewTest.h"'
  echo '#include <cstdio>'
  echo '#include <cstdint>'
  echo '#include <ctime>'
  echo ""
  echo "// ---- REAL isRtcTimeValidForHibernate() body, extracted verbatim from"
  echo "// $sleep_src - NOT reimplemented (A.6 gap 2)."
  echo "$real_predicate"
  echo ""
  echo 'int main() {'
  echo '  bool allOk = true;'
  echo '  // The compiled-in fallback anchor (used when the pre-hook AB1805 reading'
  echo '  // is not sane) must itself land in range after the skew is applied.'
  echo '  {'
  echo '    time_t after = (time_t)RtcSkewTest::applySkew(RtcSkewTest::kFallbackAnchor);'
  echo '    bool ok = isRtcTimeValidForHibernate(after);'
  echo '    std::printf("fallback anchor=%lld after=%lld inRange=%d\\n", (long long)RtcSkewTest::kFallbackAnchor, (long long)after, ok);'
  echo '    allOk = allOk && ok;'
  echo '  }'
  echo '  // Sweep the real calendar-date anchors built above - all must be accepted,'
  echo '  // since the skew magnitude is far smaller than the device in-service'
  echo '  // lifetime margin at either end of the window.'
  for anchor in "${sweep_anchors[@]}"; do
    echo "  {"
    echo "    time_t anchor = ${anchor};"
    echo '    time_t after = (time_t)RtcSkewTest::applySkew(anchor);'
    echo '    bool ok = isRtcTimeValidForHibernate(after);'
    echo '    std::printf("sweep anchor=%lld after=%lld inRange=%d\\n", (long long)anchor, (long long)after, ok);'
    echo '    allOk = allOk && ok;'
    echo "  }"
  done
  echo '  // Sensitivity check: the harness must NOT be vacuously true. An anchor'
  echo '  // within the skew magnitude of the real kRtcMin must be REJECTED once'
  echo '  // skewed below it - proving isRtcTimeValidForHibernate() actually'
  echo '  // discriminates. (kRtcMin is not re-extracted separately here - this'
  echo '  // reuses the same compiled-in real predicate above.)'
  echo '  {'
  echo '    time_t anchor = (time_t)1704067200 + 100; // 100s after the real kRtcMin (2024-01-01 UTC)'
  echo '    time_t after = (time_t)RtcSkewTest::applySkew(anchor);'
  echo '    bool ok = isRtcTimeValidForHibernate(after);'
  echo '    std::printf("edge anchor=%lld after=%lld inRange=%d (expected 0)\\n", (long long)anchor, (long long)after, ok);'
  echo '    allOk = allOk && !ok;'
  echo '  }'
  echo '  if (!allOk) { std::printf("FAIL\\n"); return 1; }'
  echo '  std::printf("PASS\\n");'
  echo '  return 0;'
  echo '}'
} > "$harness_src"

if ! clang++ -std=c++17 -Wall -Wextra -pedantic -I"$repo_root/src" "$harness_src" -o "$harness_bin"; then
  echo "FAILED: harness compile failed (see above) - $harness_src retained for inspection until this script exits" >&2
  exit 1
fi

set +e
output=$("$harness_bin")
result=$?
set -e
echo "$output"

if [[ $result -ne 0 ]]; then
  echo "FAILED: isRtcTimeValidForHibernate() acceptance-range check failed" >&2
  exit 1
fi

echo ""
echo "RTC skew bench-hook test passed"

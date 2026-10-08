#!/bin/zsh
set -euo pipefail

# WO-2026-10-08-001 item B: Report's "already connected" branch refreshes
# lastConnection, so an online CONNECTED device does not reach the failsafe.
#
# The branch is extracted verbatim from src/state/State_Report.cpp and run in
# connected_online_last_connection_test.cpp. The offline branch is checked
# structurally: lastConnection is still written only by Connect there.
# A mutation (the refresh removed, on a COPY) must make the test fail at run
# time, not at compile time.
#
# Usage: connected_online_last_connection_test.sh [State_Report.cpp]

script="${0:A}"
repo_root="${script:h:h}"
report_src="${1:-$repo_root/src/state/State_Report.cpp}"
work="${TMPDIR:-/tmp}/connected_online_$$"
mkdir -p "$work"
trap 'rm -rf "$work"' EXIT

fail() {
  echo "FAILED: $1" >&2
  exit 1
}

stale_sec=$(python3 - "$repo_root/src/power/ConnectivityPolicy.h" <<'PY'
import re, sys
text = open(sys.argv[1]).read()
block = re.search(r"#if CONNECTIVITY_FAILSAFE_TEST_MODE(.*?)#else(.*?)#endif", text, re.S)
m = re.search(r"CONNECTIVITY_FAILSAFE_STALE_SEC\s*=\s*([^;]+);", block.group(2))
print(int(eval(m.group(1).replace("L", ""))))
PY
)

run_with() {
  # $1 = State_Report.cpp to extract the branch from
  python3 - "$1" "$work/already_connected_branch.inc" <<'PY'
import sys
src, out = sys.argv[1:3]
lines = open(src).readlines()
end = next((i for i, l in enumerate(lines) if '"already connected"' in l), None)
if end is None:
    sys.exit("EXTRACT_FAILED: the 'already connected' transition not found")
start = end
while start > 0 and lines[start].strip() != "} else {":
    start -= 1
open(out, "w").write("".join(lines[start + 1:end + 1]))
PY
  clang++ -std=c++17 -Wall -Wextra -pedantic -DREAL_STALE_SEC="$stale_sec" \
    -I"$work" "$repo_root/tests/connected_online_last_connection_test.cpp" \
    -o "$work/bin"
  "$work/bin"
}

echo "--- the real 'already connected' branch ---"
run_with "$report_src"

# The offline branch is unchanged: the only lastConnection write in Report is
# the one in the online branch; Connect still owns the offline write.
writes=$(grep -c "set_lastConnection(" "$report_src" || true)
(( writes == 1 )) || fail "State_Report.cpp must contain exactly one set_lastConnection() (the online branch), found $writes"
grep -q "set_lastConnection(Time.now())" "$repo_root/src/state/State_Connect.cpp" || \
  fail "State_Connect.cpp must still write lastConnection on a successful connect"
echo "OK: the offline branch does not write lastConnection; Connect still does"

[[ -n "${1:-}" ]] && { echo "connected_online_last_connection_test passed"; exit 0; }

echo "--- mutation: remove the refresh (on a copy) ---"
python3 - "$report_src" "$work/State_Report.mut.cpp" <<'PY'
import sys
text = open(sys.argv[1]).read()
old = "    SystemConfig::set_lastConnection(Time.now());\n    transitionTo(IDLE_STATE, \"already connected\");"
if text.count(old) != 1:
    sys.exit("MUTATION_FAILED: refresh not found exactly once")
open(sys.argv[2], "w").write(text.replace(old, "    transitionTo(IDLE_STATE, \"already connected\");"))
PY
if "$script" "$work/State_Report.mut.cpp" >/dev/null 2>"$work/err"; then
  fail "mutation 'remove the already-connected refresh' was NOT detected"
fi
grep -q "error:" "$work/err" && { cat "$work/err" >&2; fail "mutation broke the build instead of being caught"; }
echo "OK (mutation): 'remove the already-connected refresh' detected"

echo "connected_online_last_connection_test passed"

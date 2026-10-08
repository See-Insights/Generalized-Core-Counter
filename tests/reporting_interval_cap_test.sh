#!/bin/zsh
set -euo pipefail

# WO-2026-10-08-001 item T: reportingIntervalSec is capped at 65535 (the width
# of the uint16_t store) in ConfigApply, so a larger value is rejected instead
# of wrapping. The real validateRange() and the real reportingIntervalSec block
# are extracted from src/cloud/ConfigApply.cpp. A mutation (the 86400 bound
# restored, on a COPY) must fail at run time, not at compile time.
#
# Usage: reporting_interval_cap_test.sh [ConfigApply.cpp]

script="${0:A}"
repo_root="${script:h:h}"
config_src="${1:-$repo_root/src/cloud/ConfigApply.cpp}"
work="${TMPDIR:-/tmp}/reporting_interval_cap_$$"
mkdir -p "$work"
trap 'rm -rf "$work"' EXIT

fail() {
  echo "FAILED: $1" >&2
  exit 1
}

extract() {
  # $1 = file, $2 = literal on the block's first line, $3 = output
  python3 - "$1" "$2" "$3" <<'PY'
import sys
path, needle, out = sys.argv[1:4]
lines = open(path).readlines()
start = next((i for i, l in enumerate(lines) if needle in l), None)
if start is None:
    sys.exit("EXTRACT_FAILED: %r not found in %s" % (needle, path))
if needle.startswith("bool Cloud::validateRange"):
    start -= 1  # include the template<typename T> line
depth = 0
opened = False
for i in range(start, len(lines)):
    for ch in lines[i]:
        if ch == '{':
            depth += 1
            opened = True
        elif ch == '}':
            depth -= 1
    if opened and depth == 0:
        open(out, "w").write("".join(lines[start:i + 1]))
        break
else:
    sys.exit("EXTRACT_FAILED: no closing brace")
PY
}

run_with() {
  extract "$1" 'if (getMergedIntValue(defaultTiming, deviceTiming, "reportingIntervalSec", reportingInterval)) {' "$work/interval_block.inc"
  extract "$1" 'bool Cloud::validateRange(T value' "$work/validate_range.inc"
  grep -q set_reportingInterval "$work/interval_block.inc" || fail "could not extract the interval block"
  clang++ -std=c++17 -Wall -Wextra -pedantic -I"$work" \
    "$repo_root/tests/reporting_interval_cap_test.cpp" -o "$work/bin"
  "$work/bin"
}

run_with "$config_src"

[[ -n "${1:-}" ]] && exit 0

echo "--- mutation: restore the 86400 upper bound (on a copy) ---"
python3 - "$config_src" "$work/ConfigApply.mut.cpp" <<'PY'
import sys
text = open(sys.argv[1]).read()
old = "reportingInterval, 300, 65535,"
if text.count(old) != 1:
    sys.exit("MUTATION_FAILED: bound not found exactly once")
open(sys.argv[2], "w").write(text.replace(old, "reportingInterval, 300, 86400,"))
PY
if "$script" "$work/ConfigApply.mut.cpp" >/dev/null 2>"$work/err"; then
  fail "mutation 'restore the 86400 bound' was NOT detected"
fi
grep -q "error:" "$work/err" && { cat "$work/err" >&2; fail "mutation broke the build instead of being caught"; }
echo "OK (mutation): 'restore the 86400 bound' detected"

echo "reporting_interval_cap_test passed"

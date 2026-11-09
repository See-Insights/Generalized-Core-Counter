#!/bin/zsh
set -euo pipefail

# WO-2026-11-09-001: extract the REAL storm-qualifying switch from setup() in
# src/Generalized-Core-Counter.cpp (preprocessor lines and all), compile it
# against the REAL Device OS system_defs.h and check it over every
# RESET_REASON_* enumerator the header declares (the list is generated from the
# header, so a version that lacks one simply has fewer rows). Exactly NONE,
# PANIC, WATCHDOG and UNKNOWN qualify. DEVICE_OS_ROOT selects the header set;
# STORM_TEST_SRC_ROOT overrides src/ (used to show the test failing on old code).

tests_dir="${0:A:h}"
repo_root="${tests_dir:h}"
src_root="${STORM_TEST_SRC_ROOT:-$repo_root/src}"
os_root="${DEVICE_OS_ROOT:-$HOME/.particle/toolchains/deviceOS/6.4.1}"
defs="$os_root/services/inc/system_defs.h"
[[ -f "$defs" ]] || { echo "FAILED: missing $defs" >&2; exit 1; }

work="$(mktemp -d "${TMPDIR:-/tmp}/storm_reason_qualifies.XXXXXX")"
trap 'rm -rf "$work"' EXIT

python3 -B - "$src_root/Generalized-Core-Counter.cpp" "$work/decision.h" <<'PY'
import re, sys
text = open(sys.argv[1]).read()
start = text.index("bool qualifiesForStormCount = false;")
open_brace = text.index("switch (reason) {", start)
depth, i = 0, text.index("{", open_brace)
for j in range(i, len(text)):
    depth += {"{": 1, "}": -1}.get(text[j], 0)
    if depth == 0:
        break
switch = text[open_brace:j + 1]
with open(sys.argv[2], "w") as out:
    out.write('#include "system_defs.h"\ninline bool stormReasonQualifies(int reason) {\n'
              '  bool qualifiesForStormCount = false;\n' + switch + '\n  return qualifiesForStormCount;\n}\n')
PY

python3 -B - "$defs" "$work/reasons.h" <<'PY'
import re, sys
text = re.sub(r"/\*.*?\*/|//[^\n]*", "", open(sys.argv[1]).read(), flags=re.S)
body = re.search(r"typedef\s+enum\s+System_Reset_Reason\s*\{(.*?)\}", text, flags=re.S).group(1)
names = re.findall(r"\b(RESET_REASON_\w+)\s*(?:=|,|$)", body)
assert {"RESET_REASON_NONE", "RESET_REASON_PANIC", "RESET_REASON_WATCHDOG", "RESET_REASON_UNKNOWN"} <= set(names)
with open(sys.argv[2], "w") as out:
    for n in names:
        out.write(f'ROW({n})\n')
PY

c++ -std=c++14 -Wall -Wextra -Werror -pedantic -I"$work" -I"$src_root" \
  -I"$os_root/services/inc" -I"$os_root/hal/inc" -I"$os_root/hal/shared" \
  -I"$os_root/hal/src/gcc" -I"$os_root/wiring/inc" -I"$os_root/dynalib/inc" \
  "$tests_dir/storm_reason_qualifies_test.cpp" -o "$work/t"
"$work/t"

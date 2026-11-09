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
ship_os="$(sed -n 's/^deviceOS=//p' "$repo_root/project.properties")"   # the Device OS version we ship
os_root="${DEVICE_OS_ROOT:-$HOME/.particle/toolchains/deviceOS/$ship_os}"
defs="$os_root/services/inc/system_defs.h"
[[ -f "$defs" ]] || { echo "FAILED: missing $defs" >&2; exit 1; }
# The header tree must be the Device OS in project.properties (header tests check the version we ship).
have_os="$(sed -n 's/^#define SYSTEM_VERSION SYSTEM_VERSION_v//p' "$os_root/system/inc/system_version.h" | tr -d '[:space:]')"
[[ -n "$ship_os" && -n "$have_os" && "${ship_os//./}" == "$have_os" ]] || { echo "FAILED: Device OS headers under $os_root are v$have_os but project.properties ships $ship_os" >&2; exit 1; }

work="$(mktemp -d "${TMPDIR:-/tmp}/storm_reason_qualifies.XXXXXX")"
trap 'rm -rf "$work"' EXIT

python3 -B - "$src_root/Generalized-Core-Counter.cpp" "$work/decision.h" <<'PY'
import re, sys
def fail(msg):
    sys.exit("FAILED: storm decision span: " + msg)

text = open(sys.argv[1]).read()
start = text.find("bool qualifiesForStormCount")
end = text.find("if (previousBootFailedEarly && qualifiesForStormCount)", start) if start >= 0 else -1
if start < 0 or end < 0:
    fail("cannot find the qualifiesForStormCount declaration and its first consumer in setup()")
span = re.sub(r"/\*.*?\*/|//[^\n]*", "", text[start:end], flags=re.S)
switches = list(re.finditer(r"\bswitch\b", span))
if len(switches) != 1:
    fail(f"expected exactly one switch, found {len(switches)}")
open_brace = span.index("{", switches[0].start())
depth = 0
for j in range(open_brace, len(span)):
    depth += {"{": 1, "}": -1}.get(span[j], 0)
    if depth == 0:
        break
else:
    fail("unbalanced braces in the switch")
rest = span[:switches[0].start()] + span[j + 1:]
rest = re.sub(r"^\s*bool\s+qualifiesForStormCount\s*=\s*false\s*;", "", rest)
if rest.strip():
    fail("executable code besides the declaration and the one switch: " + " ".join(rest.split()))
switch = span[switches[0].start():j + 1]
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

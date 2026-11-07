#!/bin/zsh
set -euo pipefail

# WO-2026-11-05-005 (S3): the timed POWER_OFF at failsafe stage 3 and the ERROR_STATE hard recovery.
#
# Extracts the REAL powerOffForRecovery() helper and kPowerOffRecoverySec (power/ConnectivityFailsafe.{cpp,h}),
# the REAL connectivityFailsafeSupervisor() body and the REAL State_Error.cpp "case 3:" block, and compiles them
# against the ordered fakes in s3_timed_power_off_test.cpp. Also fails on any deepPowerDown / ab1805. use left in
# src/ outside the object definition, and (Python probes below) on text mutations of the sources.
#
# S3_SRC=<dir> reads a copy of src/ instead (used to run mutations without touching the real tree).

repo_root="${0:A:h:h}"
src="${S3_SRC:-$repo_root/src}"
work="${TMPDIR:-/tmp}/s3_timed_power_off_$$"
mkdir -p "$work"
trap 'rm -rf "$work"' EXIT

python3 - "$src" "$work" <<'PY'
import re, sys

src, work = sys.argv[1], sys.argv[2]
fs = open(f"{src}/power/ConnectivityFailsafe.cpp").read()
hdr = open(f"{src}/power/ConnectivityFailsafe.h").read()
err = open(f"{src}/state/State_Error.cpp").read()

def grab(text, pattern, what):
    m = re.search(pattern, text, re.S | re.M)
    if not m:
        sys.exit(f"EXTRACT_FAILED: {what}")
    return m.group(0)

open(f"{work}/const.h", "w").write(grab(hdr, r"^constexpr uint32_t kPowerOffRecoverySec = \d+;[^\n]*\n", "kPowerOffRecoverySec"))
open(f"{work}/helper.h", "w").write(grab(fs, r"^void powerOffForRecovery\(\) \{.*?^\}\n", "powerOffForRecovery()"))
open(f"{work}/supervisor.h", "w").write(grab(fs, r"^void connectivityFailsafeSupervisor\(\) \{.*?^\}\n", "connectivityFailsafeSupervisor()"))
open(f"{work}/case3.h", "w").write(grab(err, r"^  case 3:\n.*?^    break;\n", "State_Error case 3"))
PY

# No caller or helper may still reach the AB1805 object or its power-down (the object's definition is the only survivor).
if grep -rnE 'deepPowerDown|\bab1805\.' "$src" --include='*.cpp' --include='*.h' | grep -vE '^\S+:[0-9]+:\s*(//|\*)'; then
  echo "AB1805_REFERENCE_LEFT: deepPowerDown/ab1805. still referenced in src/" >&2
  exit 1
fi

clang++ -std=c++17 -Wall -Wextra -pedantic \
  -I "$src/time" \
  -DPOWER_OFF_CONST_H="\"$work/const.h\"" \
  -DHELPER_BODY_H="\"$work/helper.h\"" \
  -DSUPERVISOR_BODY_H="\"$work/supervisor.h\"" \
  -DCASE3_BODY_H="\"$work/case3.h\"" \
  "$repo_root/tests/s3_timed_power_off_test.cpp" \
  -o "$work/s3_timed_power_off"
"$work/s3_timed_power_off"

echo "s3_timed_power_off_test passed"

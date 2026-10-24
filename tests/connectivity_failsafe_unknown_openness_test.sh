#!/bin/zsh
set -euo pipefail

# WO-2026-10-24-001 (recovery item 18): the failsafe acts when openness is
# Unknown and skips only when it is Closed.
#
# The REAL connectivityFailsafeSupervisor() body is extracted from
# src/power/ConnectivityFailsafe.cpp, compared with the source (COPY_MISMATCH on
# any difference) and compiled against fakes in
# connectivity_failsafe_unknown_openness_test.cpp. The three timings are parsed
# from the production branch of src/power/ConnectivityPolicy.h.
#
# UNKNOWN_OPENNESS_TAMPER_COPY=1 alters the compiled copy to show COPY_MISMATCH.

repo_root="${0:A:h:h}"
src="$repo_root/src/power/ConnectivityFailsafe.cpp"
policy="$repo_root/src/power/ConnectivityPolicy.h"
work="${TMPDIR:-/tmp}/failsafe_unknown_openness_$$"
mkdir -p "$work"
trap 'rm -rf "$work"' EXIT

extract() {
  python3 - "$src" "$1" <<'PY'
import re, sys

app = open(sys.argv[1]).read()
m = re.search(r"^void connectivityFailsafeSupervisor\(\) \{.*?^\}\n", app, re.S | re.M)
if not m:
    sys.exit("EXTRACT_FAILED: connectivityFailsafeSupervisor() not found")
open(sys.argv[2], "w").write(m.group(0))
PY
}

extract "$work/body.h"
if [[ "${UNKNOWN_OPENNESS_TAMPER_COPY:-0}" == 1 ]]; then
  print -r -- "// tampered" >> "$work/body.h"
fi
extract "$work/fresh.h"
cmp -s "$work/body.h" "$work/fresh.h" || {
  echo "COPY_MISMATCH: the supervisor body compiled by the harness differs from $src" >&2
  exit 1
}

read_policy() {
  python3 - "$policy" "$1" <<'PY'
import re, sys

text = open(sys.argv[1]).read()
block = re.search(r"#if CONNECTIVITY_FAILSAFE_TEST_MODE(.*?)#else(.*?)#endif", text, re.S)
if not block:
    sys.exit("EXTRACT_FAILED: CONNECTIVITY_FAILSAFE_TEST_MODE block not found")
m = re.search(r"constexpr\s+time_t\s+%s\s*=\s*([^;]+);" % re.escape(sys.argv[2]), block.group(2))
if not m:
    sys.exit("EXTRACT_FAILED: %s not found" % sys.argv[2])
expr = m.group(1).replace("L", "").strip()
if not re.fullmatch(r"[0-9*+\s]+", expr):
    sys.exit("EXTRACT_FAILED: %s is not a plain arithmetic literal" % sys.argv[2])
print(int(eval(expr)))
PY
}

clang++ -std=c++17 -Wall -Wextra -pedantic \
  -DSUPERVISOR_BODY_H="\"$work/body.h\"" \
  -DREAL_STALE_SEC="$(read_policy CONNECTIVITY_FAILSAFE_STALE_SEC)" \
  -DREAL_COOLDOWN_SEC="$(read_policy CONNECTIVITY_FAILSAFE_COOLDOWN_SEC)" \
  -DREAL_JITTER_MAX_SEC="$(read_policy CONNECTIVITY_FAILSAFE_JITTER_MAX_SEC)" \
  "$repo_root/tests/connectivity_failsafe_unknown_openness_test.cpp" \
  -o "$work/unknown_openness"
"$work/unknown_openness"

echo "connectivity_failsafe_unknown_openness_test passed"

#!/bin/zsh
# WO-2026-11-11-003 - the compiled occupancy debounce default is the production
# value, 300000 ms (60000 is for test devices only, set in their ledgers).
# The real Config.h is compiled on the host; the two readers must still take
# their value from the constant rather than a literal.
set -euo pipefail

repo_root="${0:A:h:h}"
work="${TMPDIR:-/tmp}/debounce_default_test"
mkdir -p "$work/stub"

cat > "$work/stub/Particle.h" <<'H'
#include <ctime>
#include <cstdint>
H
cat > "$work/t.cpp" <<'CPP'
#include "Config.h"
#include <cstdio>
int main() {
  printf("%lu\n", (unsigned long)Config::DEFAULT_OCCUPANCY_DEBOUNCE_MS);
  return 0;
}
CPP
c++ -std=c++17 -I"$repo_root/src" -I"$work/stub" "$work/t.cpp" -o "$work/t"
got="$("$work/t")"
[[ "$got" == "300000" ]] || { print -u2 "FAIL: default debounce is $got, expected 300000"; exit 1; }

grep -q 'return DEFAULT_OCCUPANCY_DEBOUNCE_MS;' "$repo_root/src/Config.cpp" \
  || { print -u2 "FAIL: invalid-config fallback no longer uses the constant"; exit 1; }
grep -q 'debounceMs == 0 ? DEFAULT_OCCUPANCY_DEBOUNCE_MS' "$repo_root/src/Config.cpp" \
  || { print -u2 "FAIL: zero fallback no longer uses the constant"; exit 1; }
grep -q 'set_sensorSetting1(Config::DEFAULT_OCCUPANCY_DEBOUNCE_MS)' "$repo_root/src/MyPersistentData.cpp" \
  || { print -u2 "FAIL: new-device setting1 no longer uses the constant"; exit 1; }

print "PASS: occupancy debounce default is 300000"

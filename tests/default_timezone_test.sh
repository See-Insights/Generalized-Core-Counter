#!/bin/zsh
set -euo pipefail

# WO-2026-11-02-004 Amendment 2: asserts Config::DEFAULT_TIMEZONE exactly, that
# it fits sysStatusData::SysData::timeZoneStr and is accepted by the real
# SystemConfig::set_timeZoneStr. Mutations (on COPIES of Config.h, placed first
# on the include path) must fail the test: UTC0 and a truncated string.

repo_root="${0:A:h:h}"
work="${TMPDIR:-/tmp}/default_timezone_test_$$"
mkdir -p "$work"
trap 'rm -rf "$work"' EXIT

build() {
  local extra_inc="$1" out="$2" impl="${3:-$repo_root/src/MyPersistentData.cpp}"
  clang++ -std=c++17 -Wall -Wextra -pedantic ${extra_inc:+-I"$extra_inc"} \
    -I"$repo_root/tests/stubs/persistence_backend" -I"$repo_root/src" \
    "$repo_root/tests/default_timezone_test.cpp" \
    "$impl" -o "$out"
}

build "" "$work/clean"
"$work/clean"

mutate_and_expect_failure() {
  local name="$1" new="$2"
  mkdir -p "$work/$name"
  python3 - "$repo_root/src/Config.h" "$work/$name/Config.h" "$new" <<'PY'
import re, sys
src, dst, new = sys.argv[1:4]
text = open(src).read()
text, n = re.subn(r'(DEFAULT_TIMEZONE = )"[^"]*"', lambda m: m.group(1) + '"' + new + '"', text)
if n != 1:
    sys.exit("MUTATION_FAILED: DEFAULT_TIMEZONE not found exactly once")
open(dst, "w").write(text)
PY
  # A quoted include resolves beside the including file first, so the mutant
  # needs its own copy of the implementation to pick up the mutant Config.h.
  cp "$repo_root/src/MyPersistentData.cpp" "$work/$name/MyPersistentData.cpp"
  build "$work/$name" "$work/$name/bin" "$work/$name/MyPersistentData.cpp" 2>"$work/$name/build.log" \
    || { cat "$work/$name/build.log" >&2; echo "FAIL (harness): mutant '$name' did not compile" >&2; exit 1; }
  if "$work/$name/bin" >/dev/null 2>&1; then
    echo "FAIL (mutation): '$name' was NOT detected" >&2
    exit 1
  fi
  echo "OK (mutation): '$name' detected"
}

mutate_and_expect_failure utc0 "UTC0"
mutate_and_expect_failure truncated "EST5EDT,M3.2.0/2:00:00,M11.1.0"

echo "default_timezone_test passed"

#!/bin/zsh
set -euo pipefail

# WO-2026-11-03-003: bump_version.sh derives PRODUCT_VERSION as
# major * 100 + iteration (v<major>.<iteration>[-name]); the legacy dotless
# form v<NN>[-name] maps to NN. Exercises the script's real derive_product_version
# from a temp copy of the script; no environment hook.

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SCRIPT="$ROOT/bump_version.sh"
fail=0

# Source a temp copy of bump_version.sh cut off after the function definition
# (everything from the argument check on is dropped).
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
sed '/^if \[ "\$#" -lt 2 \]/,$d' "$SCRIPT" > "$WORK/derive_only.sh"
grep -q '^derive_product_version()' "$WORK/derive_only.sh" || { echo "FAIL: function missing from truncated copy"; exit 1; }

# bump_version.sh is bash; run its function in bash so the check matches production.
derive() {
  bash -c 'set -euo pipefail; source "$1"; derive_product_version "$2"' _ "$WORK/derive_only.sh" "$1"
}

expect() {
  local input="$1" want="$2" got
  if got="$(derive "$input")"; then
    [ "$got" = "$want" ] || { echo "FAIL: $input -> $got, want $want"; fail=1; }
  else
    echo "FAIL: $input rejected, want $want"; fail=1
  fi
}

reject() {
  local got
  if got="$(derive "$1")"; then
    echo "FAIL: $1 -> $got, want rejection"; fail=1
  fi
}

expect v49.1-Foo 4901
expect v49.2 4902
expect v48.90 4890
expect 49.1 4901
expect 49.2 4902
expect v49.1 4901
expect 48.90 4890
expect v08.05 805
expect v28-CloseBeforeSleep 28
expect v49 49   # legacy dotless form, kept for old release names

reject v100.1
reject v49.100
reject 49
reject 49.100
reject 100.1
reject v49.x
reject v4611686018427387904.48
reject v49.
reject ""

# The old BUMP_VERSION_SOURCE_ONLY hook must no longer matter.
rc=0
BUMP_VERSION_SOURCE_ONLY=1 bash "$SCRIPT" >/dev/null 2>&1 || rc=$?
[ "$rc" -eq 1 ] || { echo "FAIL: bump_version.sh with no arguments exited $rc, want 1"; fail=1; }

[ "$fail" -eq 0 ] || exit 1
echo "PASS: bump_version.sh version scheme"

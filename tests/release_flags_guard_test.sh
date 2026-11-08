#!/bin/zsh
set -euo pipefail

# WO-2026-11-08-004 (recovery item 28): a `v*` tag build must fail unless DEV_BUILD and
# CONNECTIVITY_FAILSAFE_TEST_MODE are 0. Extracts the workflow step's shell body (not a copy)
# and runs it against the real tree and against fixture copies of src/BuildProfile.h.
# WF may point at a mutated copy of build.yml (used by the WO's mutation checks).

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
WF="${WF:-$ROOT/.github/workflows/build.yml}"
fail=0
bad() { echo "FAIL: $1"; fail=1; }

# The step block: from its name line up to the next step.
step="$(awk '/^      - /{ if (on) exit } /^      - name: Release flags DEV_BUILD and CONNECTIVITY_FAILSAFE_TEST_MODE are 0/{on=1} on{print}' "$WF")"
[ -n "$step" ] || { echo "FAIL: release-flag guard step not found in build.yml"; exit 1; }

print -r -- "$step" | grep -qxF "        if: github.ref_type == 'tag'" || bad "the step has no tag-only if: guard"

# Order: the guard must come before the Compile step.
gl="$(grep -n 'Release flags DEV_BUILD and CONNECTIVITY_FAILSAFE_TEST_MODE are 0' "$WF" | head -n 1 | cut -d: -f1)"
cl="$(grep -n '^      - name: Compile' "$WF" | head -n 1 | cut -d: -f1)"
[ -n "$cl" ] && [ "$gl" -lt "$cl" ] || bad "the guard step is not before the Compile step"

# (f) No compile flags can slip in through the workflow without tripping this test.
compile="$(awk '/^      - /{ if (on) exit } /^      - name: Compile/{on=1} on{print}' "$WF")"
[ -n "$compile" ] || bad "Compile step not found"
if print -r -- "$compile" | grep -qiE 'EXTRA_CFLAGS|cflags|(^|[[:space:]])-D[A-Za-z_]'; then
  bad "the Compile step passes compiler flags (EXTRA_CFLAGS, -D or cflags); rule on it, then update this test"
fi

body="$(print -r -- "$step" | awk '/^        run: \|/{on=1; next} on{sub(/^          /,""); print}')"
[ -n "$body" ] || { echo "FAIL: step has no run body"; exit 1; }

tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT

# run_guard <dir>: runs the extracted body with <dir> as the checkout; sets rc and out.
run_guard() {
  rc=0
  out="$(cd "$1" && bash -c "$body" 2>&1)" || rc=$?
}

fixture() { # <name> <sed expr or ''> -> dir with a copy of src/BuildProfile.h
  local d="$tmp/$1"; mkdir -p "$d/src"
  cp "$ROOT"/src/BuildProfile.h "$d/src/BuildProfile.h"
  [ -z "$2" ] || sed -i.bak -e "$2" "$d/src/BuildProfile.h"
  rm -f "$d/src/BuildProfile.h.bak"
  print -r -- "$d"
}

expect_fail() { # <label> <dir> <name that must appear, or ''>
  run_guard "$2"
  [ "$rc" -ne 0 ] || bad "$1: guard passed, must fail"
  print -r -- "$out" | grep -q '::error::' || bad "$1: no ::error:: message"
  [ -z "$3" ] || print -r -- "$out" | grep '::error::' | grep -q "$3" || bad "$1: error does not name $3"
}

# (a) the real tree passes
run_guard "$ROOT"
[ "$rc" -eq 0 ] || bad "(a) real tree: guard failed: $out"

dev_sed='s/^#define DEV_BUILD 0$/#define DEV_BUILD 1/'
fs_sed='s/^#define CONNECTIVITY_FAILSAFE_TEST_MODE 0$/#define CONNECTIVITY_FAILSAFE_TEST_MODE 1/'

d="$(fixture dev "$dev_sed")"; grep -q '^#define DEV_BUILD 1$' "$d/src/BuildProfile.h" || bad "fixture dev not applied"
expect_fail "(b) DEV_BUILD 1" "$d" DEV_BUILD

d="$(fixture fs "$fs_sed")"; grep -q '^#define CONNECTIVITY_FAILSAFE_TEST_MODE 1$' "$d/src/BuildProfile.h" || bad "fixture fs not applied"
expect_fail "(c) CONNECTIVITY_FAILSAFE_TEST_MODE 1" "$d" CONNECTIVITY_FAILSAFE_TEST_MODE

d="$(fixture both "$dev_sed;$fs_sed")"
expect_fail "(d) both set" "$d" DEV_BUILD

d="$(fixture broken '1i\
#error fixture does not preprocess')"
expect_fail "(e) header does not preprocess" "$d" ''

# A header that preprocesses but leaves a macro undefined (prints its name, not 0) must fail too.
d="$(fixture undef 's/^#define DEV_BUILD 0$//')"
expect_fail "undefined DEV_BUILD" "$d" DEV_BUILD

# A third token (a macro expanding to two tokens) is not "two tokens": fail closed.
d="$(fixture extra 's/^#define DEV_BUILD 0$/#define DEV_BUILD 0 0/')"
expect_fail "extra preprocessor tokens" "$d" ''

[ "$fail" = 0 ] && echo "PASS: release_flags_guard_test" || exit 1

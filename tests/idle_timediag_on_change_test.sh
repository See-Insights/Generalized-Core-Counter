#!/bin/zsh
set -euo pipefail

# WO-2026-10-09-001: Idle's CONNECTED branch logs TimeDiag once on Idle entry
# and once per change of isOpen / openness / trusted / valid - never per pass.
#
# The head of the real handleIdleState() (from its opening brace through the
# CONNECTED branch) is extracted byte-for-byte from src/state/State_Idle.cpp,
# closed with "}" and compiled into idle_timediag_on_change_test.cpp, which
# stubs everything it touches. A second step re-reads src/ and fails with
# COPY_MISMATCH if the extracted block is not an exact copy.
# Set IDLE_TIMEDIAG_SRC_ROOT to a copy of src/ to test a mutant; the clean run
# rebuilds against deliberately broken copies and each must FAIL a targeted
# check at run time (a mutant that does not compile is a broken mutation).

self="${0:A}"
repo_root="${self:h:h}"
work="$(mktemp -d "${TMPDIR:-/tmp}/idle_timediag_on_change.XXXXXX")"
trap 'rm -rf "$work"' EXIT

START='void handleIdleState() {'
END='  // ********** Scheduled Mode Sampling **********'

extract() {
  # $1 = src root, $2 = output .inc
  python3 - "$1/state/State_Idle.cpp" "$2" "$START" "$END" <<'PY'
import sys
path, out, start, end = sys.argv[1:5]
text = open(path).read()
s = text.find(start)
e = text.find(end)
if s < 0 or e < 0 or e < s:
    sys.exit("EXTRACT_FAILED: markers not found in " + path)
body = text[s + len(start) - 1:e]
open(out, "w").write("void handleIdleState() " + body + "}\n")
PY
}

check_copy() {
  # $1 = src root, $2 = .inc; the .inc body must be an exact substring of src.
  python3 - "$1/state/State_Idle.cpp" "$2" <<'PY'
import sys
src = open(sys.argv[1]).read()
inc = open(sys.argv[2]).read()
assert inc.endswith("}\n")
if inc[:-2] not in src:
    sys.exit("COPY_MISMATCH: extracted Idle head differs from " + sys.argv[1])
PY
}

build_and_run() {
  # $1 = src root, $2 = dir; prints the run's output, returns its status
  local root="$1" dir="$2"
  mkdir -p "$dir"
  extract "$root" "$dir/extracted_idle_head.inc"
  check_copy "$root" "$dir/extracted_idle_head.inc"
  c++ -std=c++17 -Wall -Wextra -I"$dir" \
    "$repo_root/tests/idle_timediag_on_change_test.cpp" -o "$dir/test" 2>"$dir/build.log" \
    || { cat "$dir/build.log" >&2; echo "BUILD_FAILED" >&2; return 99; }
  "$dir/test"
}

src_root="${IDLE_TIMEDIAG_SRC_ROOT:-$repo_root/src}"
if [[ -n "${IDLE_TIMEDIAG_MUTANT_RUN:-}" ]]; then
  build_and_run "$src_root" "$work/run"
  exit $?
fi

echo "== clean run =="
build_and_run "$src_root" "$work/clean"

mutate_and_expect_failure() {
  # $1 = label, $2 = expected failing check text, $3 = python replace old, $4 = new
  local label="$1" expect="$2" old="$3" new="$4"
  local root="$work/mut_$RANDOM"
  mkdir -p "$root/state"
  cp "$src_root/state/State_Idle.cpp" "$root/state/State_Idle.cpp"
  python3 - "$root/state/State_Idle.cpp" "$old" "$new" <<'PY'
import sys
p, old, new = sys.argv[1:4]
t = open(p).read()
if t.count(old) != 1:
    sys.exit("MUTATION_NOT_APPLIED: " + old)
open(p, "w").write(t.replace(old, new))
PY
  local rc=0 out
  out="$(IDLE_TIMEDIAG_MUTANT_RUN=1 IDLE_TIMEDIAG_SRC_ROOT="$root" "$self" 2>&1)" || rc=$?
  if [[ $rc -eq 0 || $rc -eq 99 ]] || ! print -r -- "$out" | grep -q "FAIL: .*$expect"; then
    echo "MUTANT NOT CAUGHT: $label (rc=$rc)" >&2
    print -r -- "$out" >&2
    exit 1
  fi
  echo "mutant caught: $label"
}

mutate_and_expect_failure "unconditional call" "no TimeDiag" \
  'if (enteredIdle || timeDiagKey != lastTimeDiagKey) {' 'if (true) {'
mutate_and_expect_failure "trusted dropped from key" "trusted true->false" \
  '(isClockTrusted() ? 8 : 0) |' '0 |'
mutate_and_expect_failure "entry test after publishStateTransition" "re-entry" \
  'const bool enteredIdle = (state != oldState);
  if (enteredIdle) {
    publishStateTransition();' \
  'if (state != oldState) {
    publishStateTransition();
  }
  const bool enteredIdle = (state != oldState);
  if (false) {
    publishStateTransition();'

echo "idle_timediag_on_change_test: PASS"

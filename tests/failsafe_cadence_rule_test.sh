#!/bin/zsh
set -euo pipefail

# WO-2026-10-08-001 item C: the failsafe counts only overdue expected
# connections (the cadence rule in connectivityFailsafeSupervisor()).
#
# Part 1 extracts the real cadence-rule block from the supervisor, checks the
# copy byte-for-byte against the source (COPY_MISMATCH on any difference) and
# compiles it into failsafe_cadence_rule_test.cpp, in a production build and a
# test-mode build. Part 2 checks the real source structure (the block sits after
# the stale, stage and cooldown returns and before the connect-attempt check).
# Part 3 plants mutations on COPIES of the source; each must be caught by a
# behaviour or structure check at run time, not by a compile error.
#
# Usage: failsafe_cadence_rule_test.sh [src-root]   (default: this repo's src/)

script="${0:A}"
repo_root="${script:h:h}"
src_root="${1:-$repo_root/src}"
work="${TMPDIR:-/tmp}/failsafe_cadence_rule_$$"
mkdir -p "$work"
trap 'rm -rf "$work"' EXIT

fail() {
  echo "FAILED: $1" >&2
  exit 1
}

# Extracts the rule block to $work/rule_block.h, verifies the copy, writes
# $work/flags.txt and the ordering facts $work/order_ok.
extract() {
  python3 - "$1" "$work" <<'PY'
import re, sys

src_root, work = sys.argv[1], sys.argv[2]
path = src_root + "/Generalized-Core-Counter.cpp"
app = open(path).read()
policy = open(src_root + "/power/ConnectivityPolicy.h").read()

m = re.search(r"^void connectivityFailsafeSupervisor\(\) \{.*?^\}", app, re.S | re.M)
if not m:
    sys.exit("EXTRACT_FAILED: connectivityFailsafeSupervisor() not found")
body, body_start = m.group(0), m.start()

blk = re.search(r"#if CONNECTIVITY_FAILSAFE_TEST_MODE(.*?)#else(.*?)#endif", policy, re.S)
if not blk:
    sys.exit("EXTRACT_FAILED: policy test-mode block not found")

def stale(text):
    s = re.search(r"CONNECTIVITY_FAILSAFE_STALE_SEC\s*=\s*([^;]+);", text)
    return int(eval(s.group(1).replace("L", "")))

# The rule block: the first "#if !CONNECTIVITY_FAILSAFE_TEST_MODE ... #endif"
# that follows the stale-threshold return. No block means no rule (the 4 h
# behaviour checks then fail, which is the point of mutation 3).
stale_ret = body.find("connectionAgeSec < ConnectivityPolicy::CONNECTIVITY_FAILSAFE_STALE_SEC")
rule = re.compile(r"^#if !CONNECTIVITY_FAILSAFE_TEST_MODE\n.*?^#endif\n", re.S | re.M).search(body, max(stale_ret, 0))
block = rule.group(0) if rule else ""
if rule:
    start = body_start + rule.start()
    end = body_start + rule.end()
    if block != app[start:end] or block not in body:
        print("COPY_MISMATCH: rule block differs from the supervisor source", file=sys.stderr)
        sys.exit(1)
open(work + "/rule_block.h", "w").write(block)

def pos(text):
    return body.find(text)

# Order: stale return < stage-3 return < cooldown return < rule < connect-attempt check.
marks = [
    stale_ret,
    pos("if (currentStage >= 3) {"),
    pos("(now - lastAction) < requiredDelay"),
    rule.start() if rule else -1,
    pos("if (activeConnectAttemptWithinBudget())"),
]
order_ok = all(x >= 0 for x in marks) and marks == sorted(marks)
open(work + "/order_ok", "w").write("1" if order_ok else "0")
open(work + "/uses_sum", "w").write(
    "1" if re.search(r"STALE_SEC\s*\+\s*cadenceSec", block) else "0")

open(work + "/flags.txt", "w").write("%d %d" % (stale(blk.group(2)), stale(blk.group(1))))
PY
}

run_all() {
  # $1 = source root; builds and runs the real block in both build modes
  extract "$1"
  local real_stale test_stale
  read -r real_stale test_stale < "$work/flags.txt" || true
  for mode in 0 1; do
    clang++ -std=c++17 -Wall -Wextra -pedantic \
      -DREAL_STALE_SEC="$real_stale" -DTEST_STALE_SEC="$test_stale" \
      -DCONNECTIVITY_FAILSAFE_TEST_MODE=$mode \
      -DCADENCE_RULE_BLOCK_H="\"$work/rule_block.h\"" \
      -I"$1" "$repo_root/tests/failsafe_cadence_rule_test.cpp" \
      "$1/reporting/ReportingPolicy.cpp" -o "$work/real_block"
    "$work/real_block" || return 1
  done
}

structural() {
  extract "$1"
  [[ -s "$work/rule_block.h" ]] || { echo "the cadence rule is missing from the supervisor" >&2; return 1; }
  [[ "$(<"$work/order_ok")" == 1 ]] || { echo "rule is not below the stale, stage and cooldown returns and above the connect-attempt check" >&2; return 1; }
  [[ "$(<"$work/uses_sum")" == 1 ]] || { echo "rule does not use STALE_SEC + cadence" >&2; return 1; }
  grep -q "ReportingPolicyResolver::resolveRuntime" "$work/rule_block.h" || { echo "rule does not resolve the runtime cadence" >&2; return 1; }
}

echo "=== Part 1: the real block, production and test-mode builds ==="
run_all "$src_root"
echo ""
echo "=== Part 2: real-source structure ==="
structural "$src_root" || fail "the cadence rule is not in the supervisor the way the work order specifies"
echo "OK: the rule sits below the stage and cooldown returns, uses STALE_SEC + cadence, and is compiled out of the test build"

# When called on a mutated copy, stop here: the caller wants the exit status.
[[ -n "${1:-}" ]] && { echo "failsafe_cadence_rule_test passed"; exit 0; }

echo ""
echo "=== Part 3: mutations (planted on copies) ==="
plant() {
  # $1 = name, then pairs of: old literal, new literal
  local name="$1"; shift
  local dir="$work/mut_${name// /_}"
  mkdir -p "$dir"
  cp -R "$repo_root/src" "$dir/src"
  python3 - "$dir/src/Generalized-Core-Counter.cpp" "$@" <<'PY'
import sys
path, pairs = sys.argv[1], sys.argv[2:]
text = open(path).read()
for old, new in zip(pairs[::2], pairs[1::2]):
    if text.count(old) != 1:
        sys.exit("MUTATION_FAILED: literal not found exactly once: %r" % old)
    text = text.replace(old, new, 1)
open(path, "w").write(text)
PY
  if "$script" "$dir/src" >/dev/null 2>"$dir/err"; then
    rm -rf "$dir"
    fail "mutation '$name' was NOT detected"
  fi
  if grep -q "error:\|MUTATION_FAILED\|EXTRACT_FAILED" "$dir/err"; then
    cat "$dir/err" >&2
    fail "mutation '$name' broke the build or the mutation instead of being caught by a check"
  fi
  rm -rf "$dir"
  echo "OK (mutation): '$name' detected"
}

plant "STALE + cadence becomes cadence" \
  "connectionAgeSec < ConnectivityPolicy::CONNECTIVITY_FAILSAFE_STALE_SEC + cadenceSec" \
  "connectionAgeSec < cadenceSec"
plant "ignore the cadence >= STALE condition" \
  "  if (cadenceSec >= ConnectivityPolicy::CONNECTIVITY_FAILSAFE_STALE_SEC &&
      connectionAgeSec" \
  "  if (connectionAgeSec"
plant "drop the cadence rule" \
  "  if (cadenceSec >= ConnectivityPolicy::CONNECTIVITY_FAILSAFE_STALE_SEC &&
      connectionAgeSec < ConnectivityPolicy::CONNECTIVITY_FAILSAFE_STALE_SEC + cadenceSec) {
    return;
  }" \
  "  (void)cadenceSec;"
plant "remove the whole rule block" \
  "#if !CONNECTIVITY_FAILSAFE_TEST_MODE
  const time_t cadenceSec = (time_t)ReportingPolicyResolver::resolveRuntime(
      PowerManager::instance().soc(), now).effectiveIntervalSec;
  if (cadenceSec >= ConnectivityPolicy::CONNECTIVITY_FAILSAFE_STALE_SEC &&
      connectionAgeSec < ConnectivityPolicy::CONNECTIVITY_FAILSAFE_STALE_SEC + cadenceSec) {
    return;
  }
#endif

" \
  ""
plant "remove the test-mode guard" \
  "#if !CONNECTIVITY_FAILSAFE_TEST_MODE
  const time_t cadenceSec" \
  "#if 1
  const time_t cadenceSec"
plant "move the rule back above the stage-3 return" \
  "#if !CONNECTIVITY_FAILSAFE_TEST_MODE
  const time_t cadenceSec = (time_t)ReportingPolicyResolver::resolveRuntime(
      PowerManager::instance().soc(), now).effectiveIntervalSec;
  if (cadenceSec >= ConnectivityPolicy::CONNECTIVITY_FAILSAFE_STALE_SEC &&
      connectionAgeSec < ConnectivityPolicy::CONNECTIVITY_FAILSAFE_STALE_SEC + cadenceSec) {
    return;
  }
#endif

" \
  "" \
  "  uint8_t currentStage = RecoveryState::get_connectivityRecoveryStage();" \
  "#if !CONNECTIVITY_FAILSAFE_TEST_MODE
  const time_t cadenceSec = (time_t)ReportingPolicyResolver::resolveRuntime(
      PowerManager::instance().soc(), now).effectiveIntervalSec;
  if (cadenceSec >= ConnectivityPolicy::CONNECTIVITY_FAILSAFE_STALE_SEC &&
      connectionAgeSec < ConnectivityPolicy::CONNECTIVITY_FAILSAFE_STALE_SEC + cadenceSec) {
    return;
  }
#endif
  uint8_t currentStage = RecoveryState::get_connectivityRecoveryStage();"

echo ""
echo "failsafe_cadence_rule_test passed (real block in production + test-mode builds, structure, mutations)"

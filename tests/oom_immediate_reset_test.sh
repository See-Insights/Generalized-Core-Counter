#!/bin/zsh
# WO-2026-10-02-003 item B - an out-of-memory event must reset the device at
# once, with a cause code that says so.
#
# Behavioral test, following the firmware_update_dwell_test.sh pattern: the REAL
# out-of-memory block is extracted verbatim from `loop()` in
# src/Generalized-Core-Counter.cpp and compiled against a fake Particle /
# RecoveryState / state-machine surface, so the decision is executed rather than
# pattern-matched. The real src/ResetCause.h supplies the cause code.
#
# What it pins:
#   1. with outOfMemory >= 0 the block calls System.reset(RESET_CAUSE_OUT_OF_MEMORY);
#   2. it makes no transition to ERROR_STATE and raises no alert;
#   3. it does not depend on the reset count - it resets at 0, 3 and 200 alike
#      (3 was the threshold at which the old ERROR_STATE route suppressed the
#      reset and bounced back to Idle, leaving the heap exhausted);
#   4. with outOfMemory < 0 nothing happens.
#
# Mutation: a copy of the source with the old ERROR_STATE route restored
# (raiseAlert(14) + transitionTo(ERROR_STATE, "out of memory")) must fail the
# test. The mutation is applied to a copy in TMPDIR; the checked-in source is
# never written to.
set -euo pipefail

repo_root="${0:A:h:h}"
work="${TMPDIR:-/tmp}/oom_immediate_reset_test"
rm -rf "$work"
mkdir -p "$work"

extract_braced_block() {
  local file="$1"
  local marker="$2"
  awk -v marker="$marker" '
    !active && index($0, marker) { active = 1 }
    active {
      print
      opens = gsub(/\{/, "{")
      closes = gsub(/\}/, "}")
      depth += opens - closes
      if (seen_open && depth == 0) exit
      if (opens > 0) seen_open = 1
    }
  ' "$file"
}

# Builds and runs the test against the given copy of the application source.
# Returns the compiled binary's exit status (or 1 if the block is missing or
# the generated program does not compile).
build_and_run() {
  local app_src="$1"
  local tag="$2"
  local generated="$work/${tag}.cpp"
  local binary="$work/${tag}"

  local block
  block=$(extract_braced_block "$app_src" "if (outOfMemory >= 0) {")
  if [[ -z "$block" ]]; then
    echo "EXTRACTION FAILED: no \`if (outOfMemory >= 0) {\` block in $app_src" >&2
    return 1
  fi

  {
    cat <<'CPP'
#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "ResetCause.h"   // the REAL cause codes

// ----- Fake Particle / application surface -----------------------------------

enum State { INITIALIZATION_STATE, ERROR_STATE, IDLE_STATE, SLEEPING_STATE };

struct Transition { State target; std::string reason; };
std::vector<Transition> transitions;
void transitionTo(State target, const char *reason) {
  transitions.push_back({target, reason ? reason : ""});
}

std::vector<uint32_t> resets;
unsigned long delaysMs = 0;
void delay(unsigned long ms) { delaysMs += ms; }

struct FakeSystem {
  uint32_t freeHeap = 128;
  void reset(uint32_t data) { resets.push_back(data); }
  uint32_t freeMemory() const { return freeHeap; }
};
FakeSystem System;

struct FakeLog {
  void info(const char *, ...) {}
  void error(const char *, ...) {}
  void warn(const char *, ...) {}
};
FakeLog Log;

// Present so that restoring the removed ERROR_STATE route still compiles - and
// then fails the assertions below.
namespace RecoveryState {
std::vector<int> alerts;
uint8_t resetCount = 0;
inline void raiseAlert(int code) { alerts.push_back(code); }
inline uint8_t get_resetCount() { return resetCount; }
inline int8_t get_alertCode() { return alerts.empty() ? 0 : (int8_t)alerts.back(); }
} // namespace RecoveryState

int outOfMemory = -1;

void oomBlock() {
CPP
    echo "// ===== Extracted verbatim from src/Generalized-Core-Counter.cpp ====="
    echo "$block"
    cat <<'CPP'
}

// ----- Test driver ------------------------------------------------------------

static void resetWorld() {
  transitions.clear();
  resets.clear();
  RecoveryState::alerts.clear();
  delaysMs = 0;
}

static void expectImmediateReset(uint8_t resetCount, int param) {
  resetWorld();
  RecoveryState::resetCount = resetCount;
  outOfMemory = param;
  oomBlock();

  if (resets.size() != 1) {
    std::cerr << "FAIL: out-of-memory with resetCount=" << (int)resetCount
              << " issued " << resets.size() << " reset(s), expected exactly 1"
              << std::endl;
    std::exit(1);
  }
  if (resets[0] != (uint32_t)RESET_CAUSE_OUT_OF_MEMORY) {
    std::cerr << "FAIL: out-of-memory reset carried code " << resets[0]
              << ", expected RESET_CAUSE_OUT_OF_MEMORY ("
              << (uint32_t)RESET_CAUSE_OUT_OF_MEMORY << ")" << std::endl;
    std::exit(1);
  }
  if (!transitions.empty()) {
    std::cerr << "FAIL: out-of-memory transitioned to state "
              << (int)transitions[0].target << " (\"" << transitions[0].reason
              << "\") instead of resetting directly" << std::endl;
    std::exit(1);
  }
  if (!RecoveryState::alerts.empty()) {
    std::cerr << "FAIL: out-of-memory raised alert "
              << RecoveryState::alerts[0]
              << "; the ERROR_STATE route was removed in WO-2026-10-02-003 item B"
              << std::endl;
    std::exit(1);
  }
}

int main() {
  // 1. Resets at once, with code 7.
  expectImmediateReset(0, 0);

  // 2. No dependence on the reset count. 3 was the old suppression threshold:
  //    from there on the ERROR_STATE route returned to Idle and never reset,
  //    and outOfMemory is never cleared, so the device bounced forever.
  expectImmediateReset(3, 1024);
  expectImmediateReset(4, 1024);
  expectImmediateReset(200, 65535);

  // 3. No out-of-memory event: the block does nothing.
  resetWorld();
  RecoveryState::resetCount = 0;
  outOfMemory = -1;
  oomBlock();
  if (!resets.empty() || !transitions.empty() || !RecoveryState::alerts.empty()) {
    std::cerr << "FAIL: the out-of-memory block acted with outOfMemory < 0"
              << std::endl;
    std::exit(1);
  }

  std::cout << "oom_immediate_reset_test: out-of-memory resets immediately with "
               "RESET_CAUSE_OUT_OF_MEMORY, no ERROR_STATE route, no reset-count "
               "dependence" << std::endl;
  return 0;
}
CPP
  } > "$generated"

  if ! clang++ -std=c++17 -Wall -Wextra -I "$repo_root/src" \
      "$generated" -o "$binary" 2>"$work/${tag}.build.log"; then
    return 1
  fi
  "$binary"
}

# --- Part 1: the shipped source must pass ------------------------------------
if ! build_and_run "$repo_root/src/Generalized-Core-Counter.cpp" "shipped"; then
  echo "FAILED: the shipped out-of-memory block does not reset immediately with RESET_CAUSE_OUT_OF_MEMORY" >&2
  [[ -s "$work/shipped.build.log" ]] && cat "$work/shipped.build.log" >&2
  exit 1
fi

# --- Part 2: mutation - restoring the ERROR_STATE route must fail ------------
mutant="$work/Generalized-Core-Counter.mutant.cpp"
cp "$repo_root/src/Generalized-Core-Counter.cpp" "$mutant"
python3 - "$mutant" <<'PY'
import sys

path = sys.argv[1]
text = open(path).read()
original = """  if (outOfMemory >= 0) {
    Log.info("out of memory occurred size=%d", outOfMemory);
    delay(100);
    System.reset(RESET_CAUSE_OUT_OF_MEMORY);
  }
"""
restored = """  if (outOfMemory >= 0) {
    Log.error("Out-of-memory event detected (param=%d freeHeap=%lu) - resetting",
              outOfMemory,
              (unsigned long)System.freeMemory());
    RecoveryState::raiseAlert(14);
    transitionTo(ERROR_STATE, "out of memory");
  }
"""
if original not in text:
    sys.exit("MUTATION SETUP FAILED: the shipped out-of-memory block does not "
             "match the expected text; update tests/oom_immediate_reset_test.sh")
open(path, "w").write(text.replace(original, restored, 1))
PY

if build_and_run "$mutant" "mutant" >/dev/null 2>&1; then
  echo "MUTATION NOT CAUGHT: restoring the ERROR_STATE route (raiseAlert(14) + transitionTo(ERROR_STATE, \"out of memory\")) still passes the test" >&2
  exit 1
fi
echo "Mutation caught: restoring the ERROR_STATE route fails the test"

# The checked-in source was never written to; only copies under $work were.
rm -rf "$work"
echo "oom_immediate_reset_test: full suite passed"

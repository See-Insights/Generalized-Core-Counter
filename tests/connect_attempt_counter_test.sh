#!/bin/zsh
# WO-2026-10-01-001 item B - the "every 4th attempt gets the 11-minute budget"
# rule must also count attempts that FAILED. Before this fix the counter only
# advanced on success, so a device at or below 50% charge whose connections
# kept failing never reached DEEP_ATTEMPT_COUNTER_THRESHOLD and never got the
# 660 s attempt that leaves room for the Device OS modem power-cycle.
#
# Behavioral test. The REAL `evaluateConnectBudget()` and the REAL failed-
# attempt increment are extracted verbatim from src/state/State_Connect.cpp and
# run against a fake SystemConfig/PowerManager, so the budget actually selected
# after three failures is observed rather than pattern-matched. Thresholds come
# from the real ConnectivityPolicy header.
set -euo pipefail

repo_root="${0:A:h:h}"
generated="${TMPDIR:-/tmp}/connect_attempt_counter_test.cpp"
binary="${TMPDIR:-/tmp}/connect_attempt_counter_test"

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

{
  cat <<'CPP'
#include <cassert>
#include <iostream>

#include "power/ConnectivityPolicy.h"  // real thresholds and budgets

// ----- Fake persistence / power surface --------------------------------------

namespace SystemConfig {
uint16_t connectAttemptBudgetSec = 0;
uint8_t connectionAttemptCounter = 0;

uint16_t get_connectAttemptBudgetSec() { return connectAttemptBudgetSec; }
uint8_t get_connectionAttemptCounter() { return connectionAttemptCounter; }
void set_connectionAttemptCounter(uint8_t value) { connectionAttemptCounter = value; }
} // namespace SystemConfig

namespace PowerManager {
struct Instance {
  float socValue = 0.0f;
  float soc() const { return socValue; }
};
inline Instance &instance() {
  static Instance inst;
  return inst;
}
} // namespace PowerManager

CPP

  echo "// ===== Extracted verbatim from src/state/State_Connect.cpp ====="
  extract_braced_block "$repo_root/src/state/State_Connect.cpp" \
    "struct ConnectBudgetContext {"
  echo
  extract_braced_block "$repo_root/src/state/State_Connect.cpp" \
    "ConnectBudgetContext evaluateConnectBudget() {"
  echo
  echo "// The failed-attempt increment from handleConnectingState()'s"
  echo "// connect-timeout path, lifted verbatim."
  echo "void countFailedConnectAttempt() {"
  extract_braced_block "$repo_root/src/state/State_Connect.cpp" \
    "uint8_t failedAttemptCounter = SystemConfig::get_connectionAttemptCounter();"
  echo "}"

  cat <<'CPP'

// ----- Test driver ------------------------------------------------------------

int main() {
  const float lowCharge = ConnectivityPolicy::DEEP_ATTEMPT_SOC_THRESHOLD - 10.0f;
  PowerManager::instance().socValue = lowCharge;
  SystemConfig::connectAttemptBudgetSec = 0; // unconfigured: use policy defaults

  // --- 1. A failed attempt increments the counter, up to the threshold.
  SystemConfig::connectionAttemptCounter = 0;
  countFailedConnectAttempt();
  assert(SystemConfig::connectionAttemptCounter == 1);
  countFailedConnectAttempt();
  assert(SystemConfig::connectionAttemptCounter == 2);

  // --- 2. Three consecutive failures at 40% charge earn the deep budget on the
  //        next attempt - the case that could never happen before the fix.
  SystemConfig::connectionAttemptCounter = 0;
  for (int attempt = 0; attempt < 3; attempt++) {
    ConnectBudgetContext before = evaluateConnectBudget();
    assert(!before.allowDeepAttempt);
    assert(before.budgetMs == ConnectivityPolicy::CONNECT_BUDGET_DEFAULT_MS);
    countFailedConnectAttempt(); // this attempt timed out
  }
  assert(SystemConfig::connectionAttemptCounter ==
         ConnectivityPolicy::DEEP_ATTEMPT_COUNTER_THRESHOLD);

  ConnectBudgetContext fourth = evaluateConnectBudget();
  assert(fourth.allowDeepAttempt);
  assert(fourth.budgetMs == ConnectivityPolicy::CONNECT_BUDGET_DEEP_MS);

  // --- 3. The counter is still guarded: it never runs past the threshold, so
  //        the reset at the start of a deep attempt remains meaningful.
  countFailedConnectAttempt();
  countFailedConnectAttempt();
  assert(SystemConfig::connectionAttemptCounter ==
         ConnectivityPolicy::DEEP_ATTEMPT_COUNTER_THRESHOLD);

  // --- 4. The "above 50%" rule is untouched: healthy charge still gets the
  //        deep budget with a zero counter.
  SystemConfig::connectionAttemptCounter = 0;
  PowerManager::instance().socValue =
      ConnectivityPolicy::DEEP_ATTEMPT_SOC_THRESHOLD + 10.0f;
  ConnectBudgetContext healthy = evaluateConnectBudget();
  assert(healthy.allowDeepAttempt);
  assert(healthy.budgetMs == ConnectivityPolicy::CONNECT_BUDGET_DEEP_MS);

  std::cout << "Connect attempt counter test passed (failed attempts reach the deep budget)\n";
  return 0;
}
CPP
} > "$generated"

clang++ -std=c++17 -Wall -Wextra -pedantic \
  -I"$repo_root/tests/stubs" -I"$repo_root/src" \
  "$generated" \
  -o "$binary"

"$binary"

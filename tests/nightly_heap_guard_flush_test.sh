#!/bin/zsh
set -euo pipefail

repo_root="${0:A:h:h}"
generated_flag_on="$TMPDIR/nightly_heap_guard_flush_flagon.cpp"
generated_flag_off="$TMPDIR/nightly_heap_guard_flush_flagoff.cpp"
binary_flag_on="$TMPDIR/nightly_heap_guard_flush_flagon"
binary_flag_off="$TMPDIR/nightly_heap_guard_flush_flagoff"

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

generate() {
  local out_file="$1"
  {
    cat <<'CPP'
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

#include "power/ConnectivityPolicy.h"

// Records the exact order System.reset()/System.freeMemory() and
// PowerDiagnostics::flushDiagBatch() were invoked in, so the test can assert
// that a flush happens *before* any reset - not merely that both happened
// somewhere during the call.
std::vector<std::string> callOrder;

struct FakeSystem {
  unsigned long freeMemoryValue = 0;

  unsigned long freeMemory() const { return freeMemoryValue; }

  void reset() { callOrder.push_back("System.reset"); }
};

FakeSystem System;

namespace PowerDiagnostics {
void flushDiagBatch() { callOrder.push_back("PowerDiagnostics::flushDiagBatch"); }
} // namespace PowerDiagnostics

// WO-2026-08-12-001 item 2 added a pure-read confirmation log call to
// Connectivity::isRadioPoweredOn() inside this extracted block. Stub it the
// same way flushDiagBatch() is stubbed above - a fixed value is fine here
// since this test only cares about flush-before-reset ordering, not the
// radio-state value itself (that is covered by
// tests/modem_teardown_confirmation_logging_test.py).
namespace Connectivity {
bool isRadioPoweredOn() { return false; }
} // namespace Connectivity

CPP

    echo "void nightlyHeapGuardCheck(bool overnightFallbackSleep) {"
    extract_braced_block "$repo_root/src/state/State_Sleep.cpp" "Before a long overnight"
    echo "}"

    cat <<'CPP'

int main() {
  // Non-empty-accumulator scenario: heap has drifted below the reset
  // threshold during an overnight fallback-sleep cycle. Fix #1 requires
  // flushDiagBatch() to run before System.reset() so accumulated
  // PowerDiag/ChargeDiag/stale-SOC-resync entries from this cycle are not
  // silently dropped by the reset.
  callOrder.clear();
  System.freeMemoryValue = ConnectivityPolicy::NIGHTLY_HEAP_RESET_THRESHOLD_BYTES - 1;
  nightlyHeapGuardCheck(/*overnightFallbackSleep=*/true);

CPP

    if [[ "$out_file" == "$generated_flag_on" ]]; then
      cat <<'CPP'
  // ENABLE_DIAGNOSTICS_PUBLISH_MODE=1 build: flush must be called, and must
  // precede the reset.
  assert(callOrder.size() == 2);
  assert(callOrder[0] == "PowerDiagnostics::flushDiagBatch");
  assert(callOrder[1] == "System.reset");
  std::cout << "Nightly heap-guard flush-before-reset test passed (flag=1)\n";
CPP
    else
      cat <<'CPP'
  // ENABLE_DIAGNOSTICS_PUBLISH_MODE=0 (default/shipped) build: the flush
  // call site is compiled out entirely (fix #3), so only the reset happens.
  assert(callOrder.size() == 1);
  assert(callOrder[0] == "System.reset");
  std::cout << "Nightly heap-guard reset-only test passed (flag=0)\n";
CPP
    fi

    cat <<'CPP'

  // Heap above both thresholds: neither flush nor reset should fire.
  callOrder.clear();
  System.freeMemoryValue = ConnectivityPolicy::NIGHTLY_HEAP_WARN_THRESHOLD_BYTES + 1;
  nightlyHeapGuardCheck(/*overnightFallbackSleep=*/true);
  assert(callOrder.empty());

  // Not an overnight-fallback cycle at all: guard must not fire regardless
  // of heap level.
  callOrder.clear();
  System.freeMemoryValue = 0;
  nightlyHeapGuardCheck(/*overnightFallbackSleep=*/false);
  assert(callOrder.empty());

  return 0;
}
CPP
  } > "$out_file"
}

generate "$generated_flag_on"
generate "$generated_flag_off"

clang++ -std=c++17 -Wall -Wextra -pedantic \
  -DENABLE_DIAGNOSTICS_PUBLISH_MODE=1 \
  -I"$repo_root/tests/stubs" -I"$repo_root/src" \
  "$generated_flag_on" \
  -o "$binary_flag_on"

clang++ -std=c++17 -Wall -Wextra -pedantic \
  -DENABLE_DIAGNOSTICS_PUBLISH_MODE=0 \
  -I"$repo_root/tests/stubs" -I"$repo_root/src" \
  "$generated_flag_off" \
  -o "$binary_flag_off"

"$binary_flag_on"
"$binary_flag_off"

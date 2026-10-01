#!/bin/zsh
# WO-2026-10-01-001 item D - the signal must read "not available" when Device
# OS has no reading, instead of 0/0.
#
# Before the fix, `Cellular.RSSI()` returning an invalid signal (-1 strength
# and -1 quality during acquisition) was rounded to 0/0 and marked valid, so
# the `sig=na` log branches that already exist in ConnDiag/ConnSummary never
# ran and the logs claimed a real 0/0 reading.
#
# Behavioral test. The REAL `sampleConnectionSignal()` is extracted verbatim
# from src/state/State_Connect.cpp and driven with both invalid and valid fake
# Device OS readings.
set -euo pipefail

repo_root="${0:A:h:h}"
generated="${TMPDIR:-/tmp}/connection_signal_validity_test.cpp"
binary="${TMPDIR:-/tmp}/connection_signal_validity_test"

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

// Compile the cellular branch, which is what the Boron ships.
#define Wiring_Cellular 1
#define Wiring_WiFi 0

// ----- Fake Device OS signal surface -----------------------------------------

struct CellularSignal {
  float strength = -1.0f;
  float quality = -1.0f;
  float getStrength() const { return strength; }
  float getQuality() const { return quality; }
};

struct FakeCellular {
  CellularSignal next;
  CellularSignal RSSI() const { return next; }
};
FakeCellular Cellular;

CPP

  echo "// ===== Extracted verbatim from src/state/State_Connect.cpp ====="
  extract_braced_block "$repo_root/src/state/State_Connect.cpp" \
    "void sampleConnectionSignal(int &strengthPct, int &qualityPct, bool &valid) {"

  cat <<'CPP'

// ----- Test driver ------------------------------------------------------------

int main() {
  int strengthPct = 999;
  int qualityPct = 999;
  bool valid = true;

  // --- 1. Device OS has no reading yet (the acquisition case): -1 percentages.
  //        `valid` must be false and -1 must be kept, so the sig=na branches run.
  Cellular.next = CellularSignal{-1.0f, -1.0f};
  sampleConnectionSignal(strengthPct, qualityPct, valid);
  assert(!valid);
  assert(strengthPct == -1);
  assert(qualityPct == -1);

  // --- 2. A partially invalid reading is still "not available", not 0/0.
  Cellular.next = CellularSignal{-1.0f, 42.0f};
  sampleConnectionSignal(strengthPct, qualityPct, valid);
  assert(!valid);
  assert(strengthPct == -1);
  assert(qualityPct == -1);

  Cellular.next = CellularSignal{42.0f, -1.0f};
  sampleConnectionSignal(strengthPct, qualityPct, valid);
  assert(!valid);
  assert(strengthPct == -1);
  assert(qualityPct == -1);

  // --- 3. A real reading is still reported, rounded as before.
  Cellular.next = CellularSignal{63.4f, 28.6f};
  sampleConnectionSignal(strengthPct, qualityPct, valid);
  assert(valid);
  assert(strengthPct == 63);
  assert(qualityPct == 29);

  // --- 4. A genuine 0/0 from Device OS is still reported as a real reading.
  Cellular.next = CellularSignal{0.0f, 0.0f};
  sampleConnectionSignal(strengthPct, qualityPct, valid);
  assert(valid);
  assert(strengthPct == 0);
  assert(qualityPct == 0);

  std::cout << "Connection signal validity test passed (no 0/0 conversion artefact)\n";
  return 0;
}
CPP
} > "$generated"

clang++ -std=c++17 -Wall -Wextra -pedantic \
  "$generated" \
  -o "$binary"

"$binary"

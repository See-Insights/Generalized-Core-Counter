#!/bin/zsh
set -euo pipefail

# WO-2026-08-24-001: proves the "flags=0x%04x" build-flag witness on the
# Boot: serial line (src/Generalized-Core-Counter.cpp) and the "flags" field
# in the ledger firmware object (src/cloud/DeviceStatusPublisher.cpp) cannot
# silently drift apart from each other or from what was actually compiled --
# the exact failure mode this WO exists to fix (ENABLE_BORON_USB_SOURCE_OVERRIDE
# defaulted to 1 in source but was compiled out of a shipped binary with no
# way to tell from the outside).
#
# Two checks:
#   1. Byte-for-byte fidelity: the compiledBuildFlags bitmask expression is
#      extracted verbatim from BOTH real source files and diffed. If a future
#      edit changes the bit layout in one call site but not the other, this
#      fails immediately.
#   2. Compiled-artifact proof, not source reasoning: the verbatim extracted
#      expression is dropped into a tiny harness, compiled twice against the
#      REAL src/BuildProfile.h under two different build-flag configurations
#      (default, and a deliberately-flipped set), and the two resulting
#      *executed* values are asserted to differ and to match the values
#      independently hand-computed from BuildProfile.h's actual defaults.
#   3. Addendum A: the same harness is compiled twice more, once with
#      PLATFORM_ID/PLATFORM_BORON defined and matching (the real Boron
#      condition that gates PowerManager.cpp's USB source override) and once
#      with PLATFORM_ID defined but NOT matching PLATFORM_BORON (the shape of
#      a non-Boron platform build). The resulting *executed* 0x4000 bit is
#      asserted to be set in the former and clear in the latter -- proving
#      the bit actually flips with the override's real compile-time guard,
#      not merely that source text mentions it.

repo_root="${0:A:h:h}"
app_src="$repo_root/src/Generalized-Core-Counter.cpp"
publisher_src="$repo_root/src/cloud/DeviceStatusPublisher.cpp"

extract_block() {
  local file="$1"
  awk '
    /const uint16_t compiledBuildFlags =/ { flag=1 }
    flag { print }
    flag && /;/ { exit }
  ' "$file" | sed -E 's/^[[:space:]]+//'
}

boot_block=$(extract_block "$app_src")
ledger_block=$(extract_block "$publisher_src")

if [[ -z "$boot_block" ]]; then
  echo "FIDELITY CHECK FAILED: compiledBuildFlags witness not found in $app_src" >&2
  exit 1
fi
if [[ -z "$ledger_block" ]]; then
  echo "FIDELITY CHECK FAILED: compiledBuildFlags witness not found in $publisher_src" >&2
  exit 1
fi

if [[ "$boot_block" != "$ledger_block" ]]; then
  echo "FIDELITY CHECK FAILED: Boot: line and ledger firmware-object flags witnesses have drifted apart (bit assignments differ)" >&2
  echo "--- Boot: witness ($app_src) ---" >&2
  echo "$boot_block" >&2
  echo "--- Ledger witness ($publisher_src) ---" >&2
  echo "$ledger_block" >&2
  exit 1
fi
echo "Boot: and ledger flags-witness bit layouts are byte-for-byte identical"

# The Boot: line's log format string must actually carry the witness field
# (not just have the bitmask computed and silently discarded).
if ! grep -q 'flags=0x%04x' "$app_src"; then
  echo "FIDELITY CHECK FAILED: Boot: line format string missing flags=0x%04x in $app_src" >&2
  exit 1
fi

# The ledger firmware object must actually write the computed bitmask under
# the JSON field name "flags" (not just compute it and drop it).
if ! grep -q 'writerBase.name("flags").value((int)compiledBuildFlags)' "$publisher_src"; then
  echo "FIDELITY CHECK FAILED: ledger firmware object does not write compiledBuildFlags to a \"flags\" field in $publisher_src" >&2
  exit 1
fi
echo "Both call sites actually emit the witness (not merely compute-and-discard it)"

# --- Compiled-artifact proof: build the REAL extracted expression against
# the REAL BuildProfile.h under two different flag configurations and confirm
# the resulting witness value tracks the compiled flags. ---
harness="$TMPDIR/build_flags_witness_harness.cpp"
{
  echo '#include "BuildProfile.h"'
  echo '#include <cstdint>'
  echo '#include <cstdio>'
  echo 'int main() {'
  echo "$boot_block"
  echo '  std::printf("%u", (unsigned)compiledBuildFlags);'
  echo '  return 0;'
  echo '}'
} > "$harness"

binary_default="$TMPDIR/build_flags_witness_default"
binary_flipped="$TMPDIR/build_flags_witness_flipped"

clang++ -std=c++17 -Wall -Wextra -pedantic -I"$repo_root/src" \
  "$harness" -o "$binary_default"

# Flip DEV_BUILD, ALLOW_BLOCKING_SERIAL_WAITS, ENABLE_GATE_TRACE on and
# ENABLE_PMIC_FORENSICS off relative to BuildProfile.h's shipped defaults.
clang++ -std=c++17 -Wall -Wextra -pedantic -I"$repo_root/src" \
  -DDEV_BUILD=1 -DALLOW_BLOCKING_SERIAL_WAITS=1 \
  -DENABLE_PMIC_FORENSICS=0 -DENABLE_GATE_TRACE=1 \
  "$harness" -o "$binary_flipped"

default_value=$("$binary_default")
flipped_value=$("$binary_flipped")

# Hand-computed from BuildProfile.h's actual defaults as of this WO:
#   default: ENABLE_PMIC_FORENSICS(bit3=0x0008) + ENABLE_DIAGNOSTICS_PUBLISH_MODE(bit13=0x2000) = 0x2008 (8200)
#   flipped: DEV_BUILD(bit0=0x0001) + ALLOW_BLOCKING_SERIAL_WAITS(bit1=0x0002)
#            + ENABLE_GATE_TRACE(bit8=0x0100) + ENABLE_DIAGNOSTICS_PUBLISH_MODE(bit13=0x2000) = 0x2103 (8451)
#            (ENABLE_PMIC_FORENSICS forced to 0 here, so its bit drops out)
expected_default=8200
expected_flipped=8451

if [[ "$default_value" != "$expected_default" ]]; then
  echo "FAILED: default-build compiledBuildFlags=$default_value, expected $expected_default (0x2008)" >&2
  exit 1
fi
if [[ "$flipped_value" != "$expected_flipped" ]]; then
  echo "FAILED: flipped-build compiledBuildFlags=$flipped_value, expected $expected_flipped (0x2103)" >&2
  exit 1
fi
if [[ "$default_value" == "$flipped_value" ]]; then
  echo "FAILED: witness value did not change between two different compiled configurations" >&2
  exit 1
fi

echo "Default-build witness=$default_value (0x$(printf '%04x' "$default_value")), flipped-build witness=$flipped_value (0x$(printf '%04x' "$flipped_value"))"

# --- Addendum A (WO-2026-08-24-001): the new 0x4000 bit must actually flip.
# It mirrors the SAME #if guard that gates the USB source override in
# src/power/PowerManager.cpp:
#   #if defined(PLATFORM_ID) && defined(PLATFORM_BORON) && (PLATFORM_ID == PLATFORM_BORON)
# Prove -- by compiling the verbatim-extracted expression, not by reading
# source -- that:
#   1. with PLATFORM_BORON undefined (host build; both binaries above already
#      cover this: neither $default_value nor $flipped_value has 0x4000 set)
#      the bit is clear, and
#   2. with PLATFORM_ID/PLATFORM_BORON defined and matching (the real Boron
#      condition), the bit is set, and
#   3. with PLATFORM_ID defined but NOT matching PLATFORM_BORON (e.g. a P2 or
#      Argon build, where PLATFORM_ID != PLATFORM_BORON), the bit stays clear
#      -- this is the EXPECTED, CORRECT non-Boron case, not a defect.
mask_0x4000=16384

if (( (default_value & mask_0x4000) != 0 )); then
  echo "FAILED: default-build (PLATFORM_BORON undefined) has 0x4000 SET; expected clear" >&2
  exit 1
fi
if (( (flipped_value & mask_0x4000) != 0 )); then
  echo "FAILED: flipped-build (PLATFORM_BORON undefined) has 0x4000 SET; expected clear" >&2
  exit 1
fi
echo "PLATFORM_BORON undefined (host build): 0x4000 bit clear in both configurations, as expected/correct for non-Boron"

binary_boron_match="$TMPDIR/build_flags_witness_boron_match"
binary_boron_mismatch="$TMPDIR/build_flags_witness_boron_mismatch"

# Real Boron condition: PLATFORM_ID and PLATFORM_BORON defined and equal.
clang++ -std=c++17 -Wall -Wextra -pedantic -I"$repo_root/src" \
  -DPLATFORM_ID=1 -DPLATFORM_BORON=1 \
  "$harness" -o "$binary_boron_match"

# Non-Boron platform shape: PLATFORM_ID defined but does not match
# PLATFORM_BORON (e.g. P2/Argon/MSOM at build time).
clang++ -std=c++17 -Wall -Wextra -pedantic -I"$repo_root/src" \
  -DPLATFORM_ID=1 -DPLATFORM_BORON=2 \
  "$harness" -o "$binary_boron_mismatch"

boron_match_value=$("$binary_boron_match")
boron_mismatch_value=$("$binary_boron_mismatch")

if (( (boron_match_value & mask_0x4000) == 0 )); then
  echo "FAILED: PLATFORM_ID==PLATFORM_BORON configuration compiledBuildFlags=$boron_match_value (0x$(printf '%04x' "$boron_match_value")) does NOT have 0x4000 set; the override-compiled witness bit did not flip" >&2
  exit 1
fi
if (( (boron_mismatch_value & mask_0x4000) != 0 )); then
  echo "FAILED: PLATFORM_ID!=PLATFORM_BORON (non-Boron) configuration compiledBuildFlags=$boron_mismatch_value (0x$(printf '%04x' "$boron_mismatch_value")) has 0x4000 SET; expected clear (a clear bit is correct on non-Boron platforms)" >&2
  exit 1
fi
if (( boron_match_value == boron_mismatch_value )); then
  echo "FAILED: 0x4000 witness bit did not change between matching and non-matching PLATFORM_ID/PLATFORM_BORON configurations" >&2
  exit 1
fi

echo "Boron-matched witness=$boron_match_value (0x$(printf '%04x' "$boron_match_value")), non-Boron-shaped witness=$boron_mismatch_value (0x$(printf '%04x' "$boron_mismatch_value")): 0x4000 bit tracks the compiled override guard"
echo "Build-flags witness test passed"

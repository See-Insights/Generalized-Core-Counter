#!/bin/zsh
set -euo pipefail

# WO-2026-10-03-001 (v35-LedgerNoRetry): behavioral test against the REAL
# src/cloud/Cloud.cpp and src/cloud/DeviceStatusPublisher.cpp. Same harness
# shape as tests/clock_status_republish_test.sh; the only new override is
# this WO's Particle.h (see tests/stubs/ledger_no_retry_overrides/), which
# must come FIRST on the include path so it wins over the parent harness's
# copy. Everything else - MyPersistentData.h and CloudLinkStubs.cpp - is
# reused unmodified from tests/stubs/clock_status_republish_overrides/.

repo_root="${0:A:h:h}"
binary="$TMPDIR/ledger_no_retry_test"

clang++ -std=c++17 -Wall -Wextra -pedantic \
  -I"$repo_root/tests/stubs/ledger_no_retry_overrides" \
  -I"$repo_root/tests/stubs/clock_status_republish_overrides" \
  -I"$repo_root/tests/stubs/power_source_override_overrides" \
  -I"$repo_root/src" \
  "$repo_root/tests/ledger_no_retry_test.cpp" \
  "$repo_root/src/power/PowerManager.cpp" \
  "$repo_root/src/cloud/DeviceStatusPublisher.cpp" \
  "$repo_root/src/cloud/Cloud.cpp" \
  "$repo_root/tests/stubs/clock_status_republish_overrides/CloudLinkStubs.cpp" \
  -o "$binary"

"$binary"

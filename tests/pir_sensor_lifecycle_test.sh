#!/bin/zsh
set -euo pipefail

# WO-2026-10-14-002: PIRSensor start/sleep/wake call order and ISR-to-motion-flag path,
# using the real src/sensors/PIRSensor.{h,cpp} and recording fakes. Set PIR_SENSOR_SRC_DIR
# to a copy of src/ (sensors/PIRSensor.{h,cpp}) to test private sources (mutation runs).

tests_dir="${0:A:h}"
repo_root="${tests_dir:h}"
src_dir="${PIR_SENSOR_SRC_DIR:-$repo_root/src}"
work="$(mktemp -d "${TMPDIR:-/tmp}/pir_sensor_lifecycle.XXXXXX")"
trap 'rm -rf "$work"' EXIT

clang++ -std=c++17 -Wall -Wextra -pedantic \
  -I"$tests_dir/stubs/pir_lifecycle" -I"$src_dir" -I"$repo_root/src" \
  "$tests_dir/pir_sensor_lifecycle_test.cpp" "$src_dir/sensors/PIRSensor.cpp" -o "$work/bin"
"$work/bin"

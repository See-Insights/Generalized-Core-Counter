#!/bin/zsh
set -euo pipefail

# WO-2026-09-23-001 (Step 5), revised acceptance criterion 9.
#
# Compiles and links the REAL src/MyPersistentData.cpp - the one translation
# unit that now sees the storage layer - against a host StorageHelperRK stub,
# so the facade functions exercised here are production code, not a mirror.
#
# This is the first host test that builds the persistence implementation
# rather than grepping it, which is what makes shared-state, severity
# arbitration, validate() sizing and deferred-vs-forced flush observable
# rather than asserted by inspection.
#
# MUTATION TESTING (added after Stage 7 review)
# ---------------------------------------------
# Passing assertions only mean something if they can fail. After the clean
# run, this script rebuilds the SAME test against deliberately broken COPIES
# of MyPersistentData.cpp (production sources are never modified) and
# requires the test to fail on each. Both mutations are the exact defects
# Stage 7 showed the earlier version of this test could not detect.

repo_root="${0:A:h:h}"
binary="$TMPDIR/persistence_facade_behavior_test"
work="$TMPDIR/persistence_facade_behavior_mutations_$$"
mkdir -p "$work"
trap 'rm -rf "$work"' EXIT

build() {
  local impl="$1" out="$2"
  clang++ -std=c++17 -Wall -Wextra -pedantic \
    -I"$repo_root/tests/stubs/persistence_backend" -I"$repo_root/src" \
    "$repo_root/tests/persistence_facade_behavior_test.cpp" \
    "$impl" \
    -o "$out"
}

# --- Clean run -------------------------------------------------------------
build "$repo_root/src/MyPersistentData.cpp" "$binary"
"$binary"

# --- Mutation runs ---------------------------------------------------------
# Each mutation must make the test FAIL. If one passes, the test has a blind
# spot and this script says so rather than reporting success.
mutate_and_expect_failure() {
  local name="$1" impl="$2"
  local mutant_bin="$work/mutant"
  if ! build "$impl" "$mutant_bin" 2>"$work/build.log"; then
    echo "FAIL (mutation harness): mutant '$name' did not compile - the mutation is malformed, not the code" >&2
    cat "$work/build.log" >&2
    exit 1
  fi
  if "$mutant_bin" >/dev/null 2>&1; then
    echo "FAIL (mutation): '$name' was NOT detected - the behavior test has a blind spot" >&2
    exit 1
  fi
  echo "OK (mutation): '$name' correctly detected"
}

# Mutation 1 (Stage 7): raiseAlert() only ever writes when no alert is set,
# so every upward escalation is silently dropped.
sed 's/if (getAlertSeverity(value) > getAlertSeverity(existing)) {/if (existing == 0) {/' \
  "$repo_root/src/MyPersistentData.cpp" > "$work/no_escalation.cpp"
if ! grep -q 'if (existing == 0) {' "$work/no_escalation.cpp"; then
  echo "FAIL (mutation harness): could not plant the no-escalation mutation - raiseAlert() has been rewritten, update this script" >&2
  exit 1
fi
mutate_and_expect_failure "raiseAlert() never escalates upward" "$work/no_escalation.cpp"

# Mutation 2 (Stage 7): CurrentReadings::loop() services the WRONG store.
sed 's|^void loop() { current.loop(); }$|void loop() { sensorConfig.loop(); }|' \
  "$repo_root/src/MyPersistentData.cpp" > "$work/wrong_store.cpp"
if ! grep -q 'void loop() { sensorConfig.loop(); }' "$work/wrong_store.cpp"; then
  echo "FAIL (mutation harness): could not plant the wrong-store mutation - the facade loop() forwarders have been rewritten, update this script" >&2
  exit 1
fi
mutate_and_expect_failure "CurrentReadings::loop() services a sibling's store" "$work/wrong_store.cpp"

echo "persistence_facade_behavior_test: clean run passed and all mutations detected"

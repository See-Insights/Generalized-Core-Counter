#!/bin/zsh
set -euo pipefail

repo_root="${0:A:h:h}"
work="$(mktemp -d "${TMPDIR:-/tmp}/watchdog_inferred.XXXXXX")"
trap 'rm -rf "$work"' EXIT

clang++ -std=c++17 -Wall -Wextra -pedantic -I"$repo_root/src" \
  "$repo_root/tests/watchdog_inferred_classification_test.cpp" -o "$work/t"
"$work/t"

# The same header with each rule broken must fail the table.
hdr="$repo_root/src/time/HibernateWakeDiagnostics.h"
mutant() {
  local label="$1" old="$2" new="$3"
  mkdir -p "$work/m_$label/time"
  python3 - "$hdr" "$work/m_$label/time/HibernateWakeDiagnostics.h" "$old" "$new" <<'PY'
import sys
src, dst, old, new = sys.argv[1:5]
t = open(src).read()
if t.count(old) != 1:
    sys.exit(f"MUTATION_NOT_APPLIED: {old!r}")
open(dst, "w").write(t.replace(old, new))
PY
  clang++ -std=c++17 -I"$work/m_$label" "$repo_root/tests/watchdog_inferred_classification_test.cpp" -o "$work/m_$label/t" 2>/dev/null
  if "$work/m_$label/t" >/dev/null 2>&1; then echo "MUTANT_SURVIVED: $label"; exit 1; fi
  echo "caught: $label"
}
mutant always_wake "return !errorPathByte && !failsafeRecent;" "return false;"
mutant never_wake "return !errorPathByte && !failsafeRecent;" "return true;"
mutant no_recency "(timeValid && age >= 0 && age <= kPowerOffEvidenceMaxAgeSec)" "true"
mutant invalid_time_accepted "(timeValid && age >= 0 && age <= kPowerOffEvidenceMaxAgeSec)" "(!timeValid || (age >= 0 && age <= kPowerOffEvidenceMaxAgeSec))"
mutant sentinel_nonzero "evidenceByte == kPowerOffSentinel" "evidenceByte != 0"
mutant sentinel_legacy_two "evidenceByte == kPowerOffSentinel" "evidenceByte >= 2"
mutant no_byte "return !errorPathByte && !failsafeRecent;" "return !failsafeRecent;"
mutant any_stage "failsafeStage == 3 &&" "failsafeStage != 0 &&"
echo "watchdog_inferred_classification_test: classification and mutants ok"

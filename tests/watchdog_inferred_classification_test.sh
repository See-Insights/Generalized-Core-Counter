#!/bin/zsh
set -euo pipefail

repo_root="${0:A:h:h}"
work="$(mktemp -d "${TMPDIR:-/tmp}/watchdog_inferred.XXXXXX")"
trap 'rm -rf "$work"' EXIT

gcc_src="$repo_root/src/Generalized-Core-Counter.cpp"
hdr="$repo_root/src/time/HibernateWakeDiagnostics.h"

# Extract the REAL boot classification block (read, classify, clear, flush) from setup().
extract() {
  python3 - "$1" "$2" <<'PY'
import sys
t = open(sys.argv[1]).read()
a = t.index("  bool inferredWatchdog = false;\n")
end = "SystemConfig::flushNow();"
b = t.index(end, a) + len(end)
open(sys.argv[2], "w").write(t[a:b] + "\n")
PY
}

build_run() { # <include-dir> <block> <out>
  clang++ -std=c++17 -Wall -Wextra -pedantic -I"$1" -DBOOT_BLOCK_H="\"$2\"" \
    "$repo_root/tests/watchdog_inferred_classification_test.cpp" -o "$3" 2>/dev/null
  "$3"
}

extract "$gcc_src" "$work/block.h"
clang++ -std=c++17 -Wall -Wextra -pedantic -I"$repo_root/src" -DBOOT_BLOCK_H="\"$work/block.h\"" \
  "$repo_root/tests/watchdog_inferred_classification_test.cpp" -o "$work/t"
"$work/t"

mutate() { # <src> <dst> <old> <new>
  python3 - "$@" <<'PY'
import sys
src, dst, old, new = sys.argv[1:5]
t = open(src).read()
if t.count(old) != 1:
    sys.exit(f"MUTATION_NOT_APPLIED: {old!r}")
open(dst, "w").write(t.replace(old, new))
PY
}
expect_fail() { # <label> <include-dir> <block>
  if build_run "$2" "$3" "$work/m_$1" >/dev/null 2>&1; then echo "MUTANT_SURVIVED: $1"; exit 1; fi
  echo "caught: $1"
}

hmutant() { # header mutants
  local label="$1" old="$2" new="$3"
  mkdir -p "$work/h_$label/time"
  mutate "$hdr" "$work/h_$label/time/HibernateWakeDiagnostics.h" "$old" "$new"
  expect_fail "$label" "$work/h_$label" "$work/block.h"
}
hmutant always_wake "return !(evidenceByte == kPowerOffSentinel);" "return false;"
hmutant never_wake "return !(evidenceByte == kPowerOffSentinel);" "return true;"
hmutant sentinel_nonzero "evidenceByte == kPowerOffSentinel" "evidenceByte != 0"
hmutant sentinel_legacy_two "evidenceByte == kPowerOffSentinel" "evidenceByte >= 2"

bmutant() { # boot-block mutants
  local label="$1" old="$2" new="$3"
  mutate "$work/block.h" "$work/b_$label.h" "$old" "$new"
  expect_fail "$label" "$repo_root/src" "$work/b_$label.h"
}
bmutant boot_clear_removed "  RecoveryState::set_powerOffRequested(0);" ""
bmutant boot_flush_removed "  SystemConfig::flushNow();" ""
echo "watchdog_inferred_classification_test: classification, two-boot consumption and mutants ok"

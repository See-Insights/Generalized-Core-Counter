#!/bin/zsh
set -euo pipefail

# WO-2026-10-25-001: the `Wake: ... led=` field is the LED time left in seconds. tests/wake_summary_led_test.cpp
# compiles the real logWakeSummary(), extracted by marker text from State_Sleep.cpp, and checks the logged value.
# The compiled copy is read back and compared with a fresh extraction (COPY_MISMATCH on drift), and the check is
# shown to fail on the old `/ 1000UL` code. WAKE_SUMMARY_SRC_ROOT tests another src/ root.

tests_dir="${0:A:h}"
repo_root="${tests_dir:h}"
src_root="${WAKE_SUMMARY_SRC_ROOT:-$repo_root/src}"
work="$(mktemp -d "${TMPDIR:-/tmp}/wake_summary_led.XXXXXX")"
trap 'rm -rf "$work"' EXIT

cat > "$work/extract.py" <<'PY'
import sys

path, out = sys.argv[1:3]
text = open(path).read()
start = "void logWakeSummary(const char *reason) {"
if text.count(start) != 1:
    sys.exit(f"MARKER_NOT_UNIQUE: {start!r} x{text.count(start)} in {path}")
i = text.find(start)
j = text.find("\n}\n", i)
if j < 0:
    sys.exit("MARKER_MISSING: end of logWakeSummary()")
open(out, "w").write(text[i:j + 3])
PY

run_with() {
  # $1 = State_Sleep.cpp to extract from, $2 = tag
  python3 "$work/extract.py" "$1" "$work/$2.inc"
  c++ -std=c++17 -Wall -Wextra -Werror -DWAKE_SUMMARY_INC="\"$work/$2.inc\"" \
    "$tests_dir/wake_summary_led_test.cpp" -o "$work/$2.bin"
  "$work/$2.bin"
}

run_with "$src_root/state/State_Sleep.cpp" cur
python3 "$work/extract.py" "$src_root/state/State_Sleep.cpp" "$work/fresh.inc"
cmp -s "$work/cur.inc" "$work/fresh.inc" || { echo "COPY_MISMATCH: the compiled logWakeSummary() differs from the file"; exit 1; }

# The old code (divided by 1000 again) must fail the check.
old="$work/old_State_Sleep.cpp"
python3 - "$src_root/state/State_Sleep.cpp" "$old" <<'PY'
import sys

src, dst = sys.argv[1:3]
text = open(src).read()
new = "(unsigned long)signalLEDTimeRemaining());"
if text.count(new) != 1:
    sys.exit("MUTATION_NOT_APPLIED")
open(dst, "w").write(text.replace(new, "(unsigned long)(signalLEDTimeRemaining() / 1000UL));"))
PY
if run_with "$old" old > "$work/old.out" 2>&1; then echo "MUTANT_SURVIVED: the old / 1000UL code passed"; exit 1; fi
grep -q "led=0s" "$work/old.out" || { cat "$work/old.out"; echo "MUTANT_FAILED_FOR_THE_WRONG_REASON"; exit 1; }
echo "wake_summary_led_test: the old / 1000UL code is caught"

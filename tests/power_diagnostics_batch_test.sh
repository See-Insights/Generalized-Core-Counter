#!/bin/zsh
set -euo pipefail

repo_root="${0:A:h:h}"
binary="$TMPDIR/power_diagnostics_batch_test"
out_file="$TMPDIR/power_diagnostics_batch_test.out"

clang++ -std=c++17 -Wall -Wextra -pedantic \
  -DENABLE_DIAGNOSTICS_PUBLISH_MODE=1 \
  -I"$repo_root/tests/stubs/diag_overrides" -I"$repo_root/src" \
  "$repo_root/tests/power_diagnostics_batch_test.cpp" \
  "$repo_root/src/power/PowerDiagnostics.cpp" \
  "$repo_root/src/power/PowerManager.cpp" \
  -o "$binary"

# Run first so the C++ test's own exit code (its boundary/overflow/closing
# assertions) is checked directly, rather than being swallowed by a pipe.
"$binary" > "$out_file"

# Each stdout line captured above is one "pdiag" payload PowerDiagnostics
# would have queued on-device. Confirm every single one parses as valid
# JSON (not just the two boundary payloads the C++ test itself asserts on).
line_count=0
while IFS= read -r line; do
  line_count=$((line_count + 1))
  python3 -c "
import json, sys
payload = sys.argv[1]
try:
    json.loads(payload)
except json.JSONDecodeError as e:
    print(f'INVALID JSON on line {sys.argv[2]}: {e}\n  payload={payload!r}', file=sys.stderr)
    sys.exit(1)
" "$line" "$line_count"
done < "$out_file"

echo "Validated $line_count captured pdiag payload(s) as parseable JSON"
echo "Power diagnostics batch JSON-validity test passed"

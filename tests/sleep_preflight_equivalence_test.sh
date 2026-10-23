#!/bin/zsh
set -euo pipefail

# WO-2026-10-21-004 (Sleep S-5): moving the pre-sleep cycle summary and the Wi-Fi radio-off guard out of
# handleSleepingState() into endOfCycleSummary() and wifiOffGuardPass() changes no behaviour.
# tests/sleep_preflight_equivalence_test.cpp runs one scenario table through (a) the handler's summary block,
# Wi-Fi guard block, the four guard statics and the entry resets, extracted by marker text from the base
# State_Sleep.cpp, and (b) the current handler's two call lines and entry resets with endOfCycleSummary() and
# wifiOffGuardPass() extracted by marker text from the current file, against the same fakes
# (tests/stubs/sleep_teardown). The release build runs the single-pass table (the guard compiles out, as on
# Boron); two small extra compiles cover the flags that change the code under test: -DDEV_BUILD=1 (the awake
# ceiling allowance) and -DWiring_WiFi=1 (the guard, multi-pass). The three transcripts, joined, must equal,
# byte for byte, fixtures/sleep_preflight_transcript.txt ('# ' header lines stripped first). The extracted
# copies the harness compiled are read back and compared with a fresh extraction: COPY_MISMATCH on drift.
# Step (a) needs the base commit named in the fixture header. Mutants of a private copy of State_Sleep.cpp
# must each change the transcript.
# SLEEP_PREFLIGHT_SRC_ROOT tests another src/ root; SLEEP_PREFLIGHT_TAMPER_COPY=1 demonstrates COPY_MISMATCH.
# '--regenerate' rewrites the fixture from the base blocks (a).

tests_dir="${0:A:h}"
repo_root="${tests_dir:h}"
src_root="${SLEEP_PREFLIGHT_SRC_ROOT:-$repo_root/src}"
fixture="$tests_dir/fixtures/sleep_preflight_transcript.txt"
stubs="$tests_dir/stubs/sleep_teardown"
work="$(mktemp -d "${TMPDIR:-/tmp}/sleep_preflight_equivalence.XXXXXX")"
trap 'rm -rf "$work"' EXIT

base="$(sed -n 's/^# Base commit: //p' "$fixture" 2>/dev/null || true)"
base="${base:-e9475d7}"
base_src="$work/base_State_Sleep.cpp"
have_base=0
if git -C "$repo_root" show "${base}:src/state/State_Sleep.cpp" > "$base_src" 2>/dev/null; then have_base=1; fi

cat > "$work/extract.py" <<'PY'
import sys

kind, path, outdir = sys.argv[1:4]
text = open(path).read()
HANDLER_END = "  WakeContext wake;"


def span(start, end, keep_start=True, keep_end=False):
    if text.count(start) != 1:
        sys.exit(f"MARKER_NOT_UNIQUE: {start!r} x{text.count(start)} in {path}")
    i = text.find(start)
    j = text.find(end, i + len(start))
    if j < 0:
        sys.exit(f"MARKER_MISSING: {end!r} in {path}")
    return text[(i if keep_start else i + len(start)):(j + len(end) if keep_end else j)]


out = {}
out["reset"] = span("    cloudGate.lastQueueDepth = 0xFFFF;\n", "  }\n\n  SleepTeardown::guardLostTimestamp();", keep_start=False)
if kind == "base":
    out["statics"] = span("#if Wiring_WiFi\n  static bool wifiOffGuardActive", "  static uint8_t wifiOffGuardRetries = 0;\n#endif\n", keep_end=True)
    out["region"] = span("  {\n    measure.batteryState(BatterySampleContext::PreSleep);", HANDLER_END)
else:
    out["funcs"] = span("void endOfCycleSummary(bool useNetworkStandby) {", "struct SleepPlan {")
    out["region"] = span("  endOfCycleSummary(useNetworkStandby);", HANDLER_END)
for k, v in out.items():
    open(f"{outdir}/{kind}_{k}.inc", "w").write(v)
PY

extract() {
  # $1 = base|cur, $2 = source file
  python3 "$work/extract.py" "$1" "$2" "$work"
}

if (( have_base )); then extract base "$base_src"; fi
extract cur "$src_root/state/State_Sleep.cpp"
if [[ "${SLEEP_PREFLIGHT_TAMPER_COPY:-}" == 1 ]]; then echo "// drift" >> "$work/cur_funcs.inc"; fi

build_run() {
  # $1 = extra flags, $2 = mode (a|b), $3 = output, $4 = work dir holding the .inc copies
  local flags="$1" mode="$2" out="$3" inc="$4"
  c++ -std=c++17 -Wall -Wextra -Wno-unused-parameter -Werror -DWiring_Cellular=1 -DHAL_PLATFORM_CELLULAR=1 \
    ${=flags} -I"$stubs" -I"$src_root" \
    -DBASE_STATICS_INC="\"$inc/base_statics.inc\"" -DBASE_RESET_INC="\"$inc/base_reset.inc\"" \
    -DBASE_REGION_INC="\"$inc/base_region.inc\"" -DCUR_FUNCS_INC="\"$inc/cur_funcs.inc\"" \
    -DCUR_RESET_INC="\"$inc/cur_reset.inc\"" -DCUR_REGION_INC="\"$inc/cur_region.inc\"" \
    "$tests_dir/sleep_preflight_equivalence_test.cpp" -o "$out.bin"
  "$out.bin" "$mode" > "$out"
}

run_all() {
  # $1 = mode, $2 = output prefix, $3 = work dir holding the .inc copies
  build_run "" "$1" "$2.release" "$3"
  build_run "-DDEV_BUILD=1" "$1" "$2.devbuild" "$3"
  build_run "-DWiring_WiFi=1" "$1" "$2.wifi" "$3"
  cat "$2.release" "$2.devbuild" "$2.wifi" > "$2"
}

if [[ "${1:-}" == "--regenerate" ]]; then
  (( have_base )) || { echo "NO_BASE_COMMIT: $base"; exit 1; }
  run_all a "$work/regen.txt" "$work"
  python3 - "$base" "$work/regen.txt" "$fixture" <<'PY'
import hashlib
import re
import sys

base, body_path, fixture = sys.argv[1:4]
body = open(body_path).read()
counts = re.findall(r"^SCENARIOS (\d+) PASSES (\d+) WAIT (\d+) CONTINUE (\d+) DISTINCT_EVENTS (\d+)$", body, re.M)
scen = sum(int(c[0]) for c in counts)
passes = sum(int(c[1]) for c in counts)
head = f"""# Frozen output of the pre-change pre-sleep cycle summary and Wi-Fi radio-off guard (handler summary block, guard block, guard statics, entry resets): WO-2026-10-21-004 (Sleep S-5).
# Base commit: {base}
# Scenarios: {scen} (release {counts[0][0]} + DEV_BUILD matrix {counts[1][0]} + Wiring_WiFi matrix {counts[2][0]}); passes: {passes}
# Per-build summary (scenarios passes WAIT CONTINUE distinct-events): release {' '.join(counts[0])}; DEV_BUILD {' '.join(counts[1])}; Wiring_WiFi {' '.join(counts[2])}
# Body sha256: {hashlib.sha256(body.encode()).hexdigest()}
# Generated once by extracting the blocks by marker text from {base}:src/state/State_Sleep.cpp (git show),
# wrapping them in tests/sleep_preflight_equivalence_test.cpp against tests/stubs/sleep_teardown, and running mode a.
"""
open(fixture, "w").write(head + body)
PY
  echo "fixture written: $fixture"
  exit 0
fi

grep -v '^# ' "$fixture" > "$work/expected.txt"
python3 - "$fixture" "$work/expected.txt" <<'PY'
import hashlib
import re
import sys

fixture, body_path = sys.argv[1:3]
head = open(fixture).read()
body = open(body_path).read()
want = re.search(r"^# Body sha256: (\w+)$", head, re.M).group(1)
if hashlib.sha256(body.encode()).hexdigest() != want:
    sys.exit("FIXTURE_HEADER_SHA_MISMATCH")
scen = int(re.search(r"^# Scenarios: (\d+)", head, re.M).group(1))
if sum(int(n) for n in re.findall(r"^SCENARIOS (\d+) ", body, re.M)) != scen:
    sys.exit("FIXTURE_HEADER_SCENARIO_COUNT_MISMATCH")
PY

run_all b "$work/new.txt" "$work"

# What the harness compiled must be what the file says: re-extract and compare every copy.
mkdir -p "$work/fresh"
extract_fresh() { python3 "$work/extract.py" "$1" "$2" "$work/fresh"; }
extract_fresh cur "$src_root/state/State_Sleep.cpp"
(( have_base )) && extract_fresh base "$base_src"
for f in "$work"/fresh/*.inc; do
  cmp -s "$f" "$work/${f:t}" || { echo "COPY_MISMATCH: ${f:t} compiled by the harness differs from the file"; exit 1; }
done

diff -u "$work/expected.txt" "$work/new.txt" > "$work/new.diff" || { head -60 "$work/new.diff"; echo "EQUIVALENCE_FAILED: State_Sleep.cpp differs from the frozen transcript"; exit 1; }

if (( have_base )); then
  run_all a "$work/old.txt" "$work"
  diff -u "$work/expected.txt" "$work/old.txt" > "$work/old.diff" || { head -60 "$work/old.diff"; echo "FIXTURE_NOT_FROM_BASE: the base blocks differ from the fixture ($base)"; exit 1; }
else
  echo "BASE_CHECK_SKIPPED: commit $base is not in this tree; only the fixture comparison ran"
fi

# The transcript is not trivial: every branch of the moved code shows up, in both outcomes.
for line in "OUT WAIT" "OUT CONTINUE" "BUILD release" "BUILD devbuild" "BUILD wifi" "MEASURE batteryState 2" \
            "FINALIZE now=" "soc=0 chg=" "soc=65535" "chg=255" "soc=1000" "soc=124 " "soc=123 " \
            "LOG WARN INV: awake_ms=" "LOG WARN INV: entering sleep with cloud=1 radioOn=1 (standby=0)" \
            "LOG WARN INV: entering sleep with cloud=0 radioOn=1 (standby=0)" "LOG WARN INV: entering sleep with cloud=1 radioOn=0 (standby=0)" \
            "LOG WARN INV: entering sleep with cloud still connected (standby=1)" "LOG WARN INV: connect_ms=" \
            "LOG INFO CYCLE end awake=" "soc=-1.0% chg=0" "soc=87.5% chg=-1" "q=2/3/5" "q=-1/-1/5" "q=2/3/-1" "conn=deep/timeout/25000ms" \
            "conn=normal/aborted/100ms" "lastOk=1699999999" "lastOk=0" \
            "LOG INFO SLEEP: WiFi still on - starting bounded radio-off guard" \
            "LOG INFO SLEEP: WiFi radio powered off before sleep (" \
            "LOG WARN SLEEP: WiFi radio remained on after 2001 ms (" "(255 retries)" "(2 retries)" "(6 retries)" \
            "CALL Connectivity::requestCloudDisconnectOnly" "CALL Connectivity::requestRadioPowerOff" "PROGRESS WIFI_OFF_GUARD" \
            "guard active=1 start=5000 last=5400 retries=2" "guard active=0 start=0 last=0 retries=0" "retries=255"; do
  grep -qF -- "$line" "$work/new.txt" || { echo "LOG_LINE_NEVER_EXERCISED: $line"; exit 1; }
done
mutate_and_expect_diff() {
  # $1 = label, then old/new pairs applied to a private copy of State_Sleep.cpp
  local m="$work/mut_${1// /_}"; shift
  local label="${m:t}"
  mkdir -p "$m"
  cp "$src_root/state/State_Sleep.cpp" "$m/State_Sleep.cpp"
  python3 - "$m/State_Sleep.cpp" "$@" <<'PY'
import sys

path, pairs = sys.argv[1], sys.argv[2:]
text = open(path).read()
for old, new in zip(pairs[0::2], pairs[1::2]):
    if text.count(old) != 1:
        sys.exit(f"MUTATION_NOT_APPLIED: {old!r} x{text.count(old)}")
    text = text.replace(old, new)
open(path, "w").write(text)
PY
  cp "$work"/base_*.inc "$m/" 2>/dev/null || true
  python3 "$work/extract.py" cur "$m/State_Sleep.cpp" "$m"
  run_all b "$m/new.txt" "$m" > "$m/build.log" 2>&1 || { echo "MUTANT_DID_NOT_BUILD: $label"; cat "$m/build.log"; exit 1; }
  if diff -q "$work/expected.txt" "$m/new.txt" > /dev/null; then echo "MUTANT_SURVIVED: $label"; exit 1; fi
  echo "caught: $label"
}

mutate_and_expect_diff "30 s slack dropped" "        30000UL; // slack" "        0UL; // slack"
mutate_and_expect_diff "standby branches of the radio invariant swapped" \
  "    if (!useNetworkStandby) {
      if (Particle.connected() || Connectivity::isRadioPoweredOn()) {" \
  "    if (useNetworkStandby) {
      if (Particle.connected() || Connectivity::isRadioPoweredOn()) {"
mutate_and_expect_diff "guard retry one interval late" "(nowMs - wifiGuard.wifiOffGuardLastRetryMs) >= WIFI_OFF_GUARD_RETRY_MS" "(nowMs - wifiGuard.wifiOffGuardLastRetryMs) > WIFI_OFF_GUARD_RETRY_MS"
mutate_and_expect_diff "guard give-up reset dropped" "               wifiGuard.wifiOffGuardRetries);
      wifiGuard.wifiOffGuardActive = false;" "               wifiGuard.wifiOffGuardRetries);
      wifiGuard.wifiOffGuardActive = true;"
mutate_and_expect_diff "soc rounding dropped" "(uint16_t)(soc * 10.0f + 0.5f)" "(uint16_t)(soc * 10.0f)"
mutate_and_expect_diff "charging mapping swapped" "((battState == 2) ? 1 : 0)" "((battState == 2) ? 0 : 1)"
mutate_and_expect_diff "connect margin dropped" "(Observability::cycleStats().connect_budget_ms + 1000UL)" "Observability::cycleStats().connect_budget_ms"
mutate_and_expect_diff "guard wait limit one short" "constexpr unsigned long WIFI_OFF_GUARD_MAX_MS = 2000UL;" "constexpr unsigned long WIFI_OFF_GUARD_MAX_MS = 1999UL;"
mutate_and_expect_diff "retry counter cap at 254" "if (wifiGuard.wifiOffGuardRetries < 255) {" "if (wifiGuard.wifiOffGuardRetries < 254) {"
mutate_and_expect_diff "guard wait result inverted" "  if (wifiOffGuardPass(useNetworkStandby)) return;" "  if (!wifiOffGuardPass(useNetworkStandby)) return;"
mutate_and_expect_diff "entry reset leaves the retries" "    wifiGuard.wifiOffGuardRetries = 0;
#endif
  }" "#endif
  }"

echo "sleep_preflight_equivalence_test: all three transcripts match the frozen pre-change output, all mutations detected"

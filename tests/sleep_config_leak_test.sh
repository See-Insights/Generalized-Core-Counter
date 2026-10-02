#!/bin/zsh
set -euo pipefail

# WO-2026-10-02-003 (v34-SleepConfigLeak): free memory stays level across wake
# cycles.
#
# Part 1 extracts the wake sources actually configured at each of the four
# sleep sites in the REAL src/state/State_Sleep.cpp (using the same extractor
# as tests/sleep_config_ownership_structural_test.py) and generates a C++
# header of configure functions from them.
#
# Part 2 compiles tests/sleep_config_leak_test.cpp against Device OS 6.4.1's
# REAL system/inc/system_sleep_configuration.h - the header whose move
# assignment (:205-210) memcpys over the old wakeup_sources list without
# freeing it - and runs >= 1,000 simulated sleep cycles per site under a
# counting allocator. The fresh-local pattern must lose zero bytes; the old
# shared-reset pattern (the mutation) must lose bytes on every cycle.
#
# The only host shim is a two-line prelude (stdint + an IRQn_Type typedef);
# the sleep configuration class itself is Device OS's, unmodified.
#
# Set SLEEP_CONFIG_SRC_ROOT to point Part 1 at a copy of src/.

repo_root="${0:A:h:h}"
src_root="${SLEEP_CONFIG_SRC_ROOT:-$repo_root/src}"
device_os_root="${DEVICE_OS_ROOT:-$HOME/.particle/toolchains/deviceOS/6.4.1}"

work_dir=$(mktemp -d)
trap 'rm -rf "$work_dir"' EXIT

fail() {
  echo "FAILED: $1" >&2
  exit 1
}

sleep_src="$src_root/state/State_Sleep.cpp"
[[ -f "$sleep_src" ]] || fail "missing source $sleep_src"

header="$device_os_root/system/inc/system_sleep_configuration.h"
[[ -f "$header" ]] || fail "missing Device OS header $header"

# --- Part 1: the real sites -----------------------------------------------
sites_header="$work_dir/real_sites.h"

python3 -B - "$repo_root" "$sleep_src" "$sites_header" <<'PY'
import os
import sys

sys.dont_write_bytecode = True
repo_root, sleep_src, out_path = sys.argv[1:4]
sys.path.insert(0, os.path.join(repo_root, "tests"))
import sleep_config_ownership_structural_test as structural

with open(sleep_src, "r", errors="replace") as fh:
    sites = structural.extract_sites(fh.read())

order = ["hibernate", "ulp", "stop-gpio", "stop-timer-only"]
emitted = []
body = []
for tag in order:
    site = sites.get(tag)
    if site is None:
        sys.exit("EXTRACT_FAILED: sleep site %r not found in %s" % (tag, sleep_src))
    if site["mode"] is None:
        sys.exit("EXTRACT_FAILED: sleep site %r has no mode()" % tag)
    fn = "cfg_" + tag.replace("-", "_")
    lines = ["  cfg.mode(%s);" % site["mode"]]
    n = 1
    for pin, edge in site["gpio"]:
        lines.append("  cfg.gpio(%s, %s);" % (pin, edge))
        n += 1
    if site["duration"] is not None:
        lines.append("  cfg.duration(%s);" % site["duration"])
        n += 1
    has_net = site["network"] is not None
    if has_net:
        lines.append("  if (standby) { cfg.network(%s, %s); }"
                     % (site["network"][0], site["network"][1]))
    body.append(
        "static void %s(SystemSleepConfiguration &cfg, bool standby) {\n"
        "  (void)standby;\n%s\n}\n" % (fn, "\n".join(lines)))
    emitted.append((tag, fn, n, has_net))

with open(out_path, "w") as fh:
    fh.write("// Generated from %s - do not edit.\n" % sleep_src)
    fh.write("#pragma once\n\n")
    fh.write("struct RealSite {\n"
             "  const char *name;\n"
             "  void (*configure)(SystemSleepConfiguration &, bool);\n"
             "  int expectedWakeSources;\n"
             "  bool hasNetworkStandby;\n"
             "};\n\n")
    fh.write("\n".join(body))
    fh.write("\nstatic const RealSite kRealSites[] = {\n")
    for tag, fn, n, has_net in emitted:
        fh.write('  {"%s", &%s, %d, %s},\n'
                 % (tag, fn, n - 1, "true" if has_net else "false"))
    fh.write("};\n")

for tag, fn, n, has_net in emitted:
    print("  %-16s wake-sources=%d network-standby=%s"
          % (tag, n - 1, "yes" if has_net else "no"))
PY

echo "Wake sources lifted from $sleep_src:"
grep -E "^  (cfg\.|if \(standby)" "$sites_header" | sed 's/^/  /'
echo ""

# --- Part 2: compile against the real Device OS header and run ------------
cat > "$work_dir/prelude.h" <<'EOF'
#pragma once
// Host shim: the only thing Device OS 6.4.1's headers need that a host
// toolchain does not provide. The sleep configuration class is unmodified.
#include <stdint.h>
typedef int IRQn_Type;
EOF

binary="$work_dir/sleep_config_leak_test"

# PLATFORM_ID=3 selects Device OS's own GCC/host HAL. Like the Boron's
# nRF52840 HAL (and unlike RTL872X), it is not HAL_PLATFORM_RTL872X, so the
# wake-source builders use `new`/`delete` - exactly the allocation path that
# runs on Dev-09.
clang++ -std=gnu++17 -O1 -g -Wall -Wno-unused-function \
  -DPLATFORM_ID=3 -DRELEASE_BUILD \
  -DREAL_SITES_HEADER="\"$sites_header\"" \
  -DREAL_SITES_SOURCE="\"${sleep_src#$repo_root/}\"" \
  -DDEVICE_OS_HEADER_DESCRIPTION="\"$header (real, unmodified)\"" \
  -include "$work_dir/prelude.h" \
  "$repo_root/tests/sleep_config_leak_test.cpp" \
  -I"$device_os_root/system/inc" \
  -I"$device_os_root/hal/inc" \
  -I"$device_os_root/hal/shared" \
  -I"$device_os_root/hal/src/gcc" \
  -I"$device_os_root/wiring/inc" \
  -I"$device_os_root/services/inc" \
  -I"$device_os_root/dynalib/inc" \
  -I"$device_os_root/platform/shared/inc" \
  -o "$binary"

"$binary"

echo ""
echo "sleep_config_leak_test (real Device OS 6.4.1 header + counting allocator) passed"

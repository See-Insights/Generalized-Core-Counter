#!/bin/zsh
set -euo pipefail

# WO-2026-10-17-001: retained boot-state magic/version check (real header) and
# the post-OTA boot's boot-storm path (mirror pinned by the structural test).

tests_dir="${0:A:h}"
repo_root="${tests_dir:h}"
work="$(mktemp -d "${TMPDIR:-/tmp}/retained_boot_test.XXXXXX")"
trap 'rm -rf "$work"' EXIT

c++ -std=c++14 -Wall -Wextra -Werror -pedantic -I"$repo_root/src" \
  "$tests_dir/retained_boot_test.cpp" -o "$work/retained_boot_test"
"$work/retained_boot_test"

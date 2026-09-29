## Implementation Report — WO-2026-09-28-001 Stage 6

**Model/reasoning actually used:** `claude-opus-5`, medium.

### Files and lines changed (`src/` = 4 physical lines, budget met)
`git diff --numstat -- src/`: `BuildProfile.h 1/1`, `FirmwareVersion.h 1/1`, `Version.cpp 2/2` → **4 lines**.

- `src/BuildProfile.h:246` — `ENABLE_DIAGNOSTICS_PUBLISH_MODE 1` → `0`, with trailing comment `// bench builds enable with -DENABLE_DIAGNOSTICS_PUBLISH_MODE=1 (WO-2026-09-28-001)`. Guard and `#error` untouched.
- `src/Version.cpp` — `FIRMWARE_VERSION = "v26-NoPdiag"`; notes: *"Diagnostics publish off by default: release builds emit no diagnostics events, so diagnostics cannot fill the publish queue and cost reports"*.
- `src/FirmwareVersion.h:28` — `FIRMWARE_PRODUCT_VERSION` 25 → 26.

### Test change (only one)
`tests/build_flags_witness_test.sh`: `expected_default 8200→8`, `expected_flipped 8451→259`, comment + failure-message hex updated. **Harness-proven**: `Default-build witness=8 (0x0008), flipped-build witness=259 (0x0103)` — matches the WO. `tests/publish_with_ack_structural_test.py` passes unchanged.

### Verification
1. **Suite: 43/44 (sh via zsh, py via python3)** — see deviations.
2. **Release ARM (boron, no EXTRA_CFLAGS)**: text/data/bss **149708 / 1090 / 2196** (v25: 150640/1090/2444). `strings` → `pdiag` count **0**; `v26-NoPdiag` **present**; serial `PowerDiag[%lu]:`/`ChargeDiag:` formats **present** (3).
3. **Bench ARM (`EXTRA_CFLAGS="-DENABLE_DIAGNOSTICS_PUBLISH_MODE=1"`)**: **150816 / 1090 / 2444**; `strings` → `pdiag` **present**. Release build then rebuilt clean; `target/` now holds the release build.

### Deviations
1. **BLOCKER — second test pins the old default.** `tests/power_source_override_test.cpp:294` asserts `capturedFlags == 0x6008` (0x4000|0x2000|0x0008); with the new default it is `0x4008`. `tests/power_source_override_test.sh` fails. Per the dispatch I **did not change it** — stop and report. One-line fix (`0x6008`→`0x4008` plus its comment at lines 263–270) would restore 44/44, but that is outside authorized scope.
2. **Release-notes wording revised after first build.** My initial notes string contained the literal `pdiag`, which put it in the release `.bin` and broke acceptance criterion 1. Reworded to "no diagnostics events"; re-verified count 0. (`v26-NoPdiag` is capital-P, so it does not match.)
3. **Build-procedure note (no code impact).** `EXTRA_CFLAGS` changes alone do not trigger a rebuild; user objects live in the device-OS tree, not `target/`. `make … clean-user` is required between the release and bench builds — the first bench attempts silently reused release objects.

No commits, pushes, branch changes, or flashing. Temp artifacts under `build-tmp/` removed. `docs/` untouched by me (the pre-existing modified/untracked docs were already there).


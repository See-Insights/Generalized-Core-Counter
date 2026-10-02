<!-- Copilot claude-opus-5 (medium) Stage 6 Implementation Report for item A, extracted verbatim from its --share transcript by Claude Code, 2026-10-02. Claude Code checked: archive unchanged (120584); only build-tmp/wo20261002-003-stage6/ created and removed. The untracked particle-bug-report-draft.md is Claude Code's, written on Chip's instruction. -->

## Implementation Report — WO-2026-10-02-003 (v34-SleepConfigLeak)

### Change per site (`src/state/State_Sleep.cpp`)
All four sites now build a fresh block-scope local and pass it to `System.sleep()`; nothing is ever assigned to an existing `SystemSleepConfiguration`.

| Site | Change | Net code lines |
|---|---|---|
| Hibernate (`:1116`) | `config = SystemSleepConfiguration();` → `SystemSleepConfiguration hibernateConfig;`; 3 call lines + `System.sleep(hibernateConfig)` renamed | 0 (+3 comment) |
| Ultra-low-power | reset at `:1202` removed; local `ulpConfig` declared at the configure point; `.mode/.gpio/.network/.duration/System.sleep` renamed | 0 |
| STOP fallback | `SystemSleepConfiguration stopConfig;` replaces the reset; chain + sleep renamed | 0 |
| STOP timer-only | `SystemSleepConfiguration stopTimerConfig;` replaces the reset; chain + sleep renamed | 0 |
| Global `Generalized-Core-Counter.cpp:131` removed | | −1 |
| `extern` `StateMachine.h:30` removed | | −1 |

Wake sources, modes, breadcrumbs (incl. `setAppBreadcrumb(24)` at `:1201`), logging, `AwakeCycles::recordSleepReturn()` and the pre-sleep order are byte-for-byte unchanged apart from the receiver name.

**Budget: about 12 allowed → actual net −2** `src/` code lines (nonblank, non-comment). `git diff --stat`: `src/FirmwareVersion.h 4 ++--`, `src/Generalized-Core-Counter.cpp 1 -`, `src/state/StateMachine.h 4 ++--`, `src/state/State_Sleep.cpp 35 ++++++-------`.

### Tests
**New `tests/sleep_config_ownership_structural_test.py`** (Acceptance 1 + 3). Comment/string-scrubbed source invariants: no file-scope/`static`/`extern` `SystemSleepConfiguration`, no `= SystemSleepConfiguration(`, all four sleep sites pass a block-scope local, no two share an object, and each site's pins/edges/RTC duration/network standby (under `if (useNetworkStandby)` + `#if HAL_PLATFORM_CELLULAR`) match 43b8a69. The transcribed expectations are re-derived from `git show 43b8a69:` by the same extractor and must agree. Mutations, each on a throwaway copy of `src/` — all four **detected**: global restored; assignment-reset reintroduced; a site made `static`; a wake edge flipped `RISING`→`FALLING`.

**New `tests/sleep_config_leak_test.{sh,cpp}`** (Acceptance 2). Wake sources are lifted from the real `State_Sleep.cpp` by the structural extractor and code-generated, then compiled against **Device OS 6.4.1's real `system/inc/system_sleep_configuration.h`, unmodified** — it compiles on the host with `PLATFORM_ID=3` (Device OS's own GCC HAL, which like nRF52840 is not `HAL_PLATFORM_RTL872X`, so the builders use the same `new`/`delete` path as the Boron) plus a two-line shim (`<stdint.h>`, `typedef int IRQn_Type;`). **Approach: counting allocator**, not LeakSanitizer — LSan is unavailable on macOS/arm64, and counting gives exact bytes/cycle. 5,000 cycles per site:

- fresh local: `hibernate 0 B`, `ulp 0 B` (standby off and on), `stop-gpio 0 B`, `stop-timer-only 0 B`, 0 blocks.
- **mutation** (one long-lived object reset by move assignment, destructor never run): `hibernate 80.0`, `ulp 144.0`, `stop-gpio 112.0`, `stop-timer-only 32.0` B/cycle — the zero-leak assertion fails on every site. ULP-with-standby's 144 B/cycle brackets the field evidence of 117–141 B/wake.

**Updated existing test (reported as required): `tests/sleep_breadcrumb_sequence_test.py`** pinned the literal `System.sleep(config)` — the exact text this WO changes. Changed to the shape `System\.sleep\(\s*\w+\s*\)`; all other checks untouched, passes. No other existing test needed changes. `tests/publish_with_ack_structural_test.py` passes unchanged.

### Commands and results
- Host suite (`tests/*.sh` with **zsh**, `tests/*.py` with **python3**): **before 57/57**, **after 59/59** (57 + the 2 new tests).
- Local ARM release build, boron, Device OS 6.4.1, after `make clean-user`, in scratch: **text 150692 / data 1090 / bss 2180** vs v33's 150780 / 1090 / 2204 → −88 text, 0 data, **−24 bss** (the removed global). `strings` → `v34-SleepConfigLeak`.
- Linkage: `arm-none-eabi-nm -C` → **no symbol named `config`** and no `.bss/.data` object of type `SystemSleepConfiguration`; `objdump -dC` → **5 `bl` to `SystemClass::sleep(...)`** (the four sites + the pre-existing `bootStormSleep`) and **6 `bl` to `~SystemSleepConfiguration()`**, i.e. the destructor now runs on the sleep path.
- `./bump_version.sh v34-SleepConfigLeak "..."` → `FIRMWARE_PRODUCT_VERSION 34`, `FIRMWARE_VERSION "v34-SleepConfigLeak"`.

### Cleanup
Deleted **only `build-tmp/wo20261002-003-stage6/`** (and the `tests/__pycache__/` my own test run created; the script now runs `python3 -B`). `build-tmp/`, `build-tmp/connectivity-archive/` and the WO-2026-10-01-001 transcript are intact.

### Deviations
- Leak checker is a counting allocator rather than LeakSanitizer (LSan unsupported on macOS/arm64). Explicitly permitted by the WO.
- One existing test updated (`sleep_breadcrumb_sequence_test.py`), as reported above.
- No dispatch/WO difference found. Nothing committed, pushed, merged or flashed; the working tree is left uncommitted. `docs/` untouched — the `RECOVERY_PLAN_2026-09-26.md` modification and the untracked `docs/work-orders/*` files (including a `-particle-bug-report-draft.md` that appeared during the session) are not mine.

### Model
`claude-opus-5`, reasoning **medium** — as dispatched.

---


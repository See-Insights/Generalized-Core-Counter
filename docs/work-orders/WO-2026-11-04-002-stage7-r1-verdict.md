4e0b8d565f885dec6991ed59196d5c59f507d5f9

VERDICT: **NOT VERIFIED**

### BLOCKING B1 — Failed binding deletion can cause an unbounded boot loop

The application guard is correctly implemented as specified at `src/Generalized-Core-Counter.cpp:303-307`: `bound()` → `disable()` → warning → reset with data 8 only when `r == 0`.

However, Device OS 6.6.0’s `hal_exrtc_unbind()` ignores both `Am18x5::end()` and `SystemCache::del(EXRTC_CONFIG_DATA)` errors, then unconditionally returns zero (`hal/shared/exrtc_hal.cpp:298-305`). Consequently, `r == 0` does **not** establish that the persisted binding was deleted.

The persistence operation itself is synchronous: `TlvFile::del()` rewrites/truncates the file and calls `sync()` (`services/src/tlv_file.cpp:218-310`), which calls `lfs_file_sync()` and returns its result (`services/src/tlv_file.cpp:86-99`). Therefore, there is no apparent reset-versus-asynchronous-flush race after a successful delete.

The severe case is a failed delete:

1. `hal_exrtc_unbind()` still returns zero.
2. The application resets.
3. On the next boot, `hal_exrtc_init()` reloads the surviving cache entry (`hal/shared/exrtc_hal.cpp:253-265`).
4. The guard sees `bound()` again and resets again.

This loop is unbounded. The guard executes before `bootInProgress`, boot-storm handling, and `RecoveryState::resetCount`; reset reason 140 is also excluded from storm counting. The device can therefore remain permanently unable to boot.

The behavioral test models `disable()` as returning its deletion failure (`tests/binding_delete_guard_test.sh:96-101,141-160`), which is not the Device OS 6.6.0 contract. It has no case for “delete failed, API returned zero, binding remains,” so it cannot detect this failure mode.

Disposition required before v49.1: prevent reset unless binding removal is confirmed, or change the Device OS layer to propagate the `end()`/cache-deletion failure. Add a surviving-binding mutation/test.

The hardware cleanup has the same ignored-error weakness. `Am18x5::end()` releases WDI/D5 and the interrupt pin, then issues the CONFIG_KEY software reset and waits 100 ms (`hal/shared/am18x5.cpp:529-569`). On a successful I2C reset, the next boot should start with reset chip configuration. But register-write transmission errors are not checked (`hal/shared/am18x5.cpp:1331-1347`), and `hal_exrtc_unbind()` ignores `end()` failure. If that reset does not reach the chip, a previously armed watchdog/alarm can survive. The vendored `ab1805.setup()` reads the chip afresh, but the application does not reprogram the watchdog until `src/Generalized-Core-Counter.cpp:807`; Device OS source does not prove that an early surviving hardware watchdog reset is bounded. This reinforces B1.

### NOTE N1 — Boot-storm, recovery, and retained state

- Reset reason is 140 with data 8 (`src/Generalized-Core-Counter.cpp:311-312`).
- Reason 140 is excluded from `qualifiesForStormCount` (`src/Generalized-Core-Counter.cpp:344-361`), so a successful one-time deletion does not count toward or trip boot-storm recovery.
- `RecoveryState::set_resetCount(+1)` does count reason 140 (`src/Generalized-Core-Counter.cpp:616-627`). This can affect later `resolveErrorAction()` thresholds, especially alert 16’s `>=1` hard-power-cycle threshold (`src/state/State_Error.cpp:32-55,81-93`). It does not itself trigger recovery and is not an independent failure.
- Excluding `(RESET_REASON_USER, data 8)` from that existing increment is trivial and fits the remaining source budget.
- A reset before `ensureRetainedLoopForensicsInitialized()` can leave an old breadcrumb/snapshot, but retained boot state is validated before the guard and loop-forensics state validates its own magic/version on the next boot (`src/observability/LoopForensics.cpp:154-166`). Reset data 8 identifies the deliberate reset. No unsafe retained-state interpretation was found.

### NOTE N2 — Unbound fleet behavior

When no binding exists, `ExternalTime.status()` performs a SystemCache lookup but does not access I2C or the AB1805. The lazy singleton constructor only initializes a separate `TimeClass(TimeSource::EXTERNAL)` object; that constructor merely assigns fields (`wiring/src/spark_wiring_time.cpp:84-87`). It does not change the system time source.

No other production `src/` or `lib/` path calls `ExternalTime` or constructs `Time(TimeSource)`.

Fresh baseline/candidate builds measured:

- Text: `151732 → 151988` (`+256`)
- Data: unchanged at `1114`
- BSS: `2200 → 2216` (`+16`)

Thus the unbound effect is one cache lookup, singleton construction, and the measured image growth. No I2C traffic or 100 ms chip-reset delay occurs. Hardware boot-time impact remains covered by the required unbound control gate.

### NOTE N3 — Placement, tests, and linkage verified

Nothing before the guard reads time, touches Wire, or initializes the vendored AB1805. Logging is available through the already-constructed global handler. The order test strips comments and compares executable positions (`tests/binding_delete_guard_order_test.py:15-41`).

Host suite on a clean tracked-files export: **114/114 passed** — 65 shell tests under zsh and 49 Python tests under python3. `thermal_coupling_structural_test.py` also passed separately on that export.

The guard test extracts the real block from `setup()` (`tests/binding_delete_guard_test.sh:42-48,114-118`). Seven independently applied scratch mutations were all caught:

- Guard after `ab1805.setup()` — caught by pre-guard `Time.isValid()`.
- Unconditional `disable()` — unbound case acted unexpectedly.
- Guard removed — extraction failed.
- Reset when `r != 0` — success/error behavioral cases failed.
- Different reset cause — expected cause 8 assertion failed.
- Warning removed — warning assertion failed.
- Error path returns — setup-continuation assertion failed.

Fresh Device OS 6.6.0 ELF inspection showed:

- `hal_exrtc_get_status` call at `0x000b5d36`
- bound test and unbound branch
- `hal_exrtc_unbind` at `0x000b5d4e`
- warning call at `0x000b5d5a`
- nonzero-result branch at `0x000b5d5e`
- load immediate 8 and `SystemClass::reset` at `0x000b5d62–0x000b5d64`
- loop-forensics initialization afterward at `0x000b5d68`

### NOTE N4 — Reset bookkeeping, documentation, and scope

Code 8 is unique and documented in `src/ResetCause.h:15-24`. `System.reset(8)` is stored as USER reason plus data, and `resetReasonData` is surfaced by startup status and the device-status ledger (`src/Generalized-Core-Counter.cpp:1287-1292,1349-1383`; `src/cloud/DeviceStatusPublisher.cpp:525-529`). The structural-test changes legitimately check the eighth code, call site, documentation, and payload.

No missing active reset-data enumeration was found in `docs/reference`. One documentation miss remains: `docs/recovery-architecture.md:81-85` says every deliberate reset first performs `flushAllBeforeReset()` but does not list this new pre-initialization reset as an exception. Documentation only; not independently blocking.

Scope is correct:

- `src/Generalized-Core-Counter.cpp`: `+7`
- `src/ResetCause.h`: `+1`
- Total `src/`: `+8`, within the `+10` budget
- No `AB1805_RK`, `lib/`, retained layout, or persisted-layout changes
- Binding-present and rollback benches are moved to the v50 pre-ship checklist; the unbound control remains the v49.1 hardware gate (`docs/work-orders/WO-2026-11-04-002-v49-binding-delete-guard.md:65-68`)

`git diff --check` passed. The only worktree status entry is the pre-existing staged dispatch document; this review made no tracked changes.

Model: **gpt-5.6-sol**  
Reasoning level: **xhigh**  
Interpreter/toolchain: **zsh 5.9; Python 3.14.6; Apple clang 14.0.3; arm-none-eabi-gcc 10.2.1 (2020-q4-major)**
# Field Meanings Reference

This document captures source-traced interpretations of logging fields and patterns observed in field telemetry. These are established through code inspection and live testing, not pattern-matching — intended to prevent repeated re-derivation and misreading across health checks and investigations.

## Sleep and Teardown Logging

### `Sleep: td=Xms cloud=Xms modem=Xms standby=Y/Y mode=Z tier=W occ=V`

**Source:** `State_Sleep.cpp` lines ~1085, disconnect-detection branch  
**Represents:** Teardown-completion measurement, fired **BEFORE** actual sleep-mode decision (not the sleep mode itself)

**Components:**
- `td=` = total teardown elapsed (disconnect request to ready-to-sleep)
- `cloud=` = cloud disconnect elapsed
- `modem=` = modem-off elapsed
- `standby=Y/Y` = network standby requested/effective flags
- `mode=` = **connectivity mode** (INT=INTERMITTENT, IKA=INTERMITTENT_KEEP_ALIVE, DISC=DISCONNECTED, CONN=CONNECTED), **NOT sleep mode**
- `tier=` = battery reporting tier (H/HEALTHY, C/CONSERVING, CR/CRITICAL, S/SURVIVAL)
- `occ=` = occupancy state

**When HIBERNATE actually occurs:** Logged separately as `Sleep: HIBERNATE reason=closed dur=Xs wakePin=X` (source line ~1085, after teardown measurement).

**Important:** A standard `Sleep: td=...` line with `mode=INT` does NOT indicate INTERMITTENT sleep mode; it indicates INTERMITTENT connectivity mode at the time of teardown measurement. The actual sleep decision happens after this line fires. **To distinguish HIBERNATE from INTERMITTENT sleep, check uptime counter behavior at wake, not this teardown log.**

---

### `TimeDiag: tz=... valid=... epoch=... utc=... local=... open=... close=... isOpen=...`

**Represents:** Time diagnostic check at state transition (not sleep-mode related)  
**Components:**
- `isOpen` = whether currently within business hours (open/close window); 0=closed, 1=open
- `open=` and `close=` = configured business hours (6-22 by default)
- `utc=` and `local=` = time readings in both zones

---

### `LoopStage: stage=SLEEP_PREP elapsed=Xms state=Y q=Z connMs=...`

**Represents:** Sleep-preparation-span timing measurement (how long the SLEEP_PREP stage took)  
**Components:**
- `elapsed=` = time since SLEEP_PREP began
- `connMs=` = connection attempt duration in this cycle
- `q=` = queue depth at time of measurement

---

### `HibernateDiag: ...`

**Represents:** HIBERNATE eligibility check (various conditions logged as pass/fail)  
**Patterns:**
- `HibernateDiag: pass` = all checks passed, HIBERNATE will be attempted
- `HibernateDiag: fail=REASON` = HIBERNATE blocked by reason (e.g., `fail=disabled`, `fail=rtc_invalid`, `fail=duration requested=X`)

---

## Serial Communication and Disconnect Patterns

### USB CDC Disconnection After Sleep Modes

**Mechanism:** The `Sleep: td=...` log followed by rapid `SERIAL_DISCONNECTED` event indicates a **CDC peripheral power-down race condition**.

The USB CDC connection drops as part of sleep (HIBERNATE or other low-power modes) power-down sequence — the MCU cuts power to peripherals during entry. Any log line printed immediately before disconnect competes with the USB link shutdown; may or may not reach the forwarder's buffer before the connection is lost.

**This is NOT a cloud transmission issue.** The serial forwarder reads USB CDC locally and independently of cellular/cloud connectivity. Expected behavior: last received log may be incomplete or missing in timeline when device powers down hard.

---

## Reset Codes and Boot Paths

### `Boot: reset=X` Codes

**Source:** Device OS boot diagnostics  
**Meanings:**
- **30 (POWER_MANAGEMENT):** Proper wake from HIBERNATE (expected after HIBERNATE cycle)
- **20 (PIN_RESET):** External pin reset or other wake source (not HIBERNATE)
- **40 (WATCHDOG):** Watchdog timer expired (device stalled/hung)
- **Other codes:** See Device OS documentation for complete reference

**Important:** Uptime counter behavior distinguishes sleep modes reliably:
- **Continuous climb:** Device running without reboot (INTERMITTENT sleep, cloud operations, etc.)
- **Reset to low value:** MCU reboot occurred (HIBERNATE exit, watchdog reset, hard reset)
- **Interpretation:** If uptime **decreases** between two timestamped log entries, an MCU reboot definitely occurred between them. HIBERNATE causes MCU reset on wake; INTERMITTENT sleep never resets uptime.

---

## Power Source and Profile Selection

### PowerDiag Fields: `source=` and `profile=`

**Sources (from Device OS `System.powerSource()`):**
- `USB_HOST` = USB enumerated + VBUS present (code 2)
- `USB_ADAPTER` = USB adapter (code 3)
- `VIN` = VIN line power (code 1)
- `UNKNOWN` = Power source unrecognized (code 0)
- `UNAVAILABLE` = Initial/transient state before first valid reading

**Profiles (PMIC power configuration):**
- `UsbBench` = USB charging profile (3.88V min, 4.112V charge, 896mA limit)
- `Solar` = Solar charging profile (5.08V min, 4.208V charge, 900mA limit)
- `NotApplicable` = No PMIC or uninitialized state

**Selection Logic (PowerManager.cpp):**
- VIN source → always select Solar profile
- USB source → always select UsbBench profile
- BATTERY or UNKNOWN source → use last-applied profile (if valid), else fallback to configured default (Solar if `solarPowerMode` enabled, else UsbBench)

---

### HIBERNATE Wake Power Source Artifact

**Observed pattern:** First `PowerDiag` after HIBERNATE wake may show `source=VIN profile=Solar` even on USB-powered devices.

**Root cause:** USB re-enumeration takes ~1-2 seconds after MCU reset. During this window, Device OS `System.powerSource()` may return VIN (or UNKNOWN/unavailable), before USB VBUS presence is confirmed by Nordic hardware.

**Mitigation:** `ENABLE_BORON_USB_SOURCE_OVERRIDE` (enabled by default, BuildProfile.h line 206) checks Nordic USB registers directly:
- Condition: `source==VIN/UNKNOWN && usbAddr!=0 && VBUS present && usbReg ready`
- If met: override to USB_HOST, apply UsbBench profile
- If not met: wait for next power reading (VBUS typically stable within 1-2 seconds)

**Expected behavior:** Transient artifact lasting ~1 PowerDiag reading, no lasting effect once VBUS stabilizes. **Not a defect** — this is documented Device OS behavior with a purpose-built workaround.

**Why this matters:** First-reading Solar profile does NOT indicate actual solar power operation when USB VBUS is present; it reflects USB enumeration timing, not power source selection.

---

## Ledger Synchronization

### `LedgerCb: kind=DATA|STATUS seq=X globalSeq=Y ...`

**Represents:** Ledger callback after cloud sync attempt  
**Components:**
- `kind=DATA` or `kind=STATUS` = ledger type synced
- `seq=X globalSeq=Y` = sequence numbers
- `found=1` = ledger found in cloud
- `age=Xms` = how long ago the ledger was updated
- `countBefore=X countAfter=Y` = pending items before/after sync
- `pendingData=` and `pendingStatus=` = items still awaiting sync

**Healthy patterns:**
- `pendingData=0 pendingStatus=0` = all synced, clean
- One entry with `pending=1` on the other kind = normal, will sync on next opportunity
- Age values in seconds range = recent, not stale

**Warning patterns:**
- `LedgerDuplicateStillInflight` = same item sync attempted multiple times without clearing; may indicate stuck inflight request
- `pending=1` for both DATA and STATUS across multiple cycles = ledger sync falling behind

---

## Ledger Payload Sizes

### `LedgerPayloadData: bytes=X/512 schema=Y` and `LedgerPayloadStatus: bytes=X/896 schema=Y`

**Represents:** Snapshot of ledger size in current cycle  
**Components:**
- `bytes=X/512` = used/total for DATA ledger
- `bytes=X/896` = used/total for STATUS ledger
- `schema=Y` = ledger schema version

**Typical healthy ranges:**
- DATA: 200-350 bytes used (schema 2)
- STATUS: 450-600 bytes used (schema 2)
- Schema version: should be consistent (typically 2 for current builds)

---

## Timing and Measurement

### `SLEEP_PREP` Elapsed Time

The `LoopStage: stage=SLEEP_PREP elapsed=Xms` measurement tracks time spent in sleep preparation (cloud ops gate, disconnect, radio off). This is not the sleep duration itself, but the pre-sleep work duration.

**Typical ranges:**
- Normal cycle: 5-15 seconds
- Ledger-heavy cycle: 20-60 seconds
- Timeout/gate-blocked cycle: approaches cloud sync budget (60-120 seconds)

**Regression indicator:** Same cycle number appearing multiple times with similar elapsed values = SLEEP_PREP logging is firing more than once per cycle (should be once). Check `WO-2026-08-11-001` regression status if this appears.

---

## Mixed Corrected/Raw Power Source Values Within a Single pdiag Batch

**Observed pattern:** A single published `pdiag` batch can legitimately contain
BOTH a corrected `source=` value (from `PowerDiag` entries and ordinary pdiag
entries, sourced from `powerReport.reading.powerSource` after
`PowerSourceOverride` has run) AND a raw, uncorrected `source=` value (from
`ChargeDiag` entries and reason-10 pdiag entries, sourced directly from
`System.powerSource()` / `PowerPlatform::readPowerSource()` before any
override is applied). See `src/sensors/SensorManager.cpp` (ChargeDiag /
reason-10 sampling sites) and `src/power/PowerManager.cpp`
(`PowerSourceOverride` correction) for the two respective sampling points.

**This is intentional, not an inconsistency to debug.** The two entry types
serve different purposes:
- `PowerDiag` / ordinary pdiag entries report the corrected, override-applied
  source — the value actually used for profile selection and cloud telemetry.
- `ChargeDiag` / reason-10 pdiag entries deliberately report the raw,
  pre-override source — preserved as independent ground-truth evidence. This
  raw value is what made the original VIN/Solar power-source root-cause
  investigation possible, and removing it (by folding it into the corrected
  value) would remove the diagnostic signal a future instance of this bug
  class would need to be investigated the same way.

**What to expect:** Within the same pdiag batch, it is normal and expected for
a `ChargeDiag`/reason-10 entry's `src=` field to differ from a `PowerDiag`/
ordinary entry's `source=` field in the same or an adjacent cycle (for
example, during any window where the raw hardware-reported source and the
override-corrected source temporarily diverge). Do not treat this divergence
alone as a bug signal; confirm which entry type (corrected vs. raw) is being
compared before concluding there is a genuine reporting defect.

---

## References and Related Work Orders

- **WO-2026-08-12-001:** Radio-off confirmation logging at HIBERNATE entry (depends on HIBERNATE actually occurring; validate via uptime reset)
- **WO-2026-08-11-001:** SLEEP_PREP regression check (confirmed holding clean; no flooding)
- **ENABLE_BORON_USB_SOURCE_OVERRIDE:** USB source misreporting workaround (enabled by default; fires on HIBERNATE wake once VBUS stable)


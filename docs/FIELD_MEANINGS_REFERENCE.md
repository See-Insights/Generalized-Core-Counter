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

### VIN/Solar Reading on a USB-Powered Device

A USB-powered device can report `source=VIN profile=Solar`. **Two distinct behaviors share this symptom.** They are not the same thing and must not be read interchangeably: one is benign and self-correcting, the other is an unresolved defect with a measurable charge cost. Determine which case you are looking at before drawing any conclusion.

What is shared is the surface reading: `source=VIN profile=Solar` on a device that is physically on USB. The underlying register state is **not** known to be identical between the two cases, and should not be assumed so — in Case 1 enumeration is still in progress, whereas Case 2 was observed with USB fully enumerated (`usbAddr=0x4`) and the Nordic USB regulator ready (`usbReg=0x3`). The simultaneous `vbus=0` is a separate BQ24195 PMIC `VBUS_STAT` reading; it is not the override's VBUS predicate.

**The distinguishing question:** does the reading correct itself on the next `PowerDiag`, or does it persist?

---

#### Case 1 — Transient, immediately post-reset (benign)

**Pattern:** A single `PowerDiag` reading immediately after a reset (including HIBERNATE wake) shows `source=VIN profile=Solar`, then corrects on the next reading.

**Mechanism:** USB re-enumeration takes ~1-2 seconds after MCU reset. During that window `System.powerSource()` may return VIN (or UNKNOWN/unavailable) before USB VBUS presence is confirmed by Nordic hardware.

**Mitigation:** Beginning with v23-Diag-Soak, the Boron USB source override is compiled unconditionally on Boron and checks Nordic USB registers directly:
- Condition: `source==VIN/UNKNOWN && usbAddr!=0 && VBUS present && usbReg ready`
- If met: override to USB_HOST, apply UsbBench profile
- If not met: wait for next power reading

**Expected behavior:** Lasts ~1 PowerDiag reading, no lasting effect once VBUS stabilizes. **Not a defect** in this form — expected Device OS behavior with a purpose-built workaround.

**Scope of this claim:** "Not a defect" applies *only* to the transient case — a reading that demonstrably self-corrects within a cycle or two of a reset. It does not extend to Case 2.

---

#### Case 2 — Persistent, multi-hour (confirmed defect, corrective failure explained)

**Pattern:** The same `source=VIN profile=Solar` reading persists for hours and does **not** self-correct. Critically, it has been observed beginning *well after* a clean boot rather than immediately following one — the device first reads USB correctly, then degrades to VIN and stays there.

**Confirmed on Boron-Dev-09, 2026-08-13** (v22-Diag-Soak, Device OS 6.4.1), telemetry-verified:
- Session booted 08:45:41 SGT; `ChargeDiag` at 08:52 correctly read `src=USB_HOST prof=USB vcell=4.157`.
- By 09:52 it had reverted to `src=VIN prof=SOLAR` and remained there until 16:50 — approximately **7 hours** in a single session, with no self-correction.
- Across the 24-hour window, 32 of 43 `ChargeDiag` readings reported `src=VIN prof=SOLAR`; ~10h50m total spanning two sessions.
- The override did not engage: `PowerDiag[1271]: source=VIN profile=Solar vbus=0 pg=1 usbAddr=0x4 usbReg=0x3`. This was **not** an unsatisfied-precondition case. `vbus=0` is the BQ24195 PMIC's `VBUS_STAT` field and is not used by the override. The actual Nordic predicate was satisfied: `usbAddr & 0x7f != 0` and `usbReg & 0x03 == 0x03`. Disassembly of the exact deployed v22 artifact (`app_hash 64188ac3...`) confirmed that the override had been compiled out, so that binary never evaluated the satisfied predicate.

**This is not benign — it has a measurable charge cost.** Observed on Dev-09: while on the Solar profile, the PMIC reported fast-charging but the battery net-discharged, producing 7 escalating `PMIC: Stuck in Fast Charging for 6+ hours with no material gain` errors (14:56–16:50 SGT) with `vcell` declining 3.961V → 3.885V. A watchdog reset at 16:56:43 cleared the condition; the device re-detected USB_HOST, `vcell` recovered to 4.101V and SOC rose 70.4% → 81.2%.

The charge failure is *consistent with* the Solar profile's 5.08V minimum input exceeding what a USB supply provides (see the profile table above), but that link has not been independently confirmed on hardware and is stated here as a plausible reading of the numbers, not an established mechanism.

**Root cause of the failed correction: confirmed.** The deployed v22 binary did not contain the override because its build guard compiled it out. The v23-Diag-Soak fix removes that optional guard and compiles the override unconditionally on Boron, so this specific silent compile-out failure mode should not recur. The upstream reason Device OS changed its raw source from a correct USB_HOST reading to VIN remains unknown; that is separate from why the application failed to correct the reading.

**Why this matters:** A Solar profile reading does not indicate actual solar operation when USB is connected. In Case 1 it reflects enumeration timing and is harmless. In Case 2 the upstream source degradation remains unexplained, but the prolonged Solar-profile consequence is explained by the override being absent from that deployed binary. On battery-backed field units, unlike a USB bench unit, that consequence has no benign failure mode.

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

**Typical ranges** — ⚠️ *unverified, treat with caution:*
- Normal cycle: 5-15 seconds
- Ledger-heavy cycle: 20-60 seconds
- Timeout/gate-blocked cycle: approaches cloud sync budget (60-120 seconds)

These ranges were recorded during the same investigation that produced the retracted 24-hour assessment and have **not** been re-confirmed against telemetry. The only `SLEEP_PREP` sample in the verified Boron-Dev-09 window (2026-08-13) was `elapsed=1644182` — about 27 minutes, roughly 13× the stated maximum:

```
17:28:04  LoopStage: stage=SLEEP_PREP elapsed=1644182 state=3 q=7 connMs=0
```

Whether that indicates the ranges are wrong, or that a long dwell is simply expected in `INTERMITTENT_KEEP_ALIVE` where the sleep path is rarely exercised, is **not established** — one sample cannot distinguish them. Do not treat these ranges as a threshold for raising or dismissing a concern until they are re-derived from a window with real sleep cycles.

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
- **WO-2026-08-11-001:** SLEEP_PREP regression check — **status: UNRESOLVED.** A previous entry here recorded this as "confirmed holding clean; no flooding". That claim originated in the retracted 24-hour assessment and is not supported by telemetry. The redone assessment (Boron-Dev-09, 2026-08-13) found exactly **one** `SLEEP_PREP` line in 24 hours, which is too few samples to demonstrate either a regression or its absence. The device spent that window in `INTERMITTENT_KEEP_ALIVE` with multi-hour continuous sessions, so it was rarely exercising the sleep path at all. Re-run over a window containing real sleep cycles before recording any verdict.
- **Boron USB source override:** Beginning with v23-Diag-Soak, the workaround is compiled unconditionally on Boron. The Dev-09 2026-08-13 episode did not demonstrate a failed runtime predicate: `usbAddr=0x4` and `usbReg=0x3` satisfied it, but disassembly confirmed the override was absent from the deployed v22 binary. See "VIN/Solar Reading on a USB-Powered Device", Case 2.
- **OPEN — upstream persistent VIN/Solar misdetection (unnumbered, root cause unknown):** Why Device OS changes the raw source from a correct USB_HOST reading to VIN mid-session remains unresolved. The concurrent PMIC `vbus=0` value is not the Nordic override predicate and must not be used to infer that the predicate failed. Evidence: Boron-Dev-09 24-hour soak assessment, 2026-08-13.

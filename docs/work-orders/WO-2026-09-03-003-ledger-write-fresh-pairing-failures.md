# WO-2026-09-03-003: ledger_write() failing repeatedly before succeeding on fresh device pairing

**Status:** Drafted, not dispatched. Specification only.

**Origin:** Observed live on Boron-Dev-11, 2026-09-02, immediately following
the carrier-board swap - a fresh MCU + carrier pairing with no prior
session/ledger history on this exact combination.

## The observation

Four consecutive `ledger_write()` failures (`seq=1`-`4`, both `status` and
`data` kinds), error code `-1911`, each followed by:

```
[wiring] ERROR: ledger_write() failed: -1911
[wiring] ERROR: Failed to encode ledger data: -1911
[app] WARN: LedgerFail: seq=<n> ... error=-1911
```

`seq=5` (status) and `seq=6` (data) then succeeded normally, and Ledger sync
continued without further failures through the rest of the observed session.

## Ruled out this session

**Carrier-board FRAM.** Confirmed via source that `sysStatusData`'s constructor
uses `StorageHelperRK::PersistentDataFile` - the flash-filesystem backend - so
local persisted app state lives on the MCU module's own flash, not
carrier-board hardware. Particle's own documentation confirms Ledger is also
stored in the device's flash filesystem, not external hardware. **The carrier
swap is very unlikely to be the cause.**

## Leading hypothesis

`SYS_DATA_MAGIC` / `SYS_DATA_VERSION` (or Ledger's equivalent) validation
failing on a genuinely fresh / never-synced state, causing deterministic
failure-then-recovery as local and cloud-side sequence numbers converge. Not
corruption in the destructive sense - more likely a fresh-pairing edge case in
Ledger's own sync/retry logic.

## Acceptance criteria

- Trace how `SYS_DATA_MAGIC` / `SYS_DATA_VERSION` - and Ledger's analogous
  fields, if separate - are validated on load, and confirm whether a
  fresh/absent file produces exactly this failure-then-recovery signature.
- Determine whether this is Device OS's own Ledger sync behaviour (out of
  firmware's control, expected on first-ever sync) or something in this
  project's own wrapper/encoding path.
- **If expected:** document it as known/benign fresh-pairing behaviour. No fix
  needed.
- **If not expected:** characterise whether any data is actually lost during
  the 4 failed attempts, or whether it is fully recovered by the successful
  retry. Current evidence suggests full recovery - **confirm rather than
  assume.**

## Provenance

Surfaced incidentally during the Dev-11 hardware investigation
(`WO-2026-08-31-004`) and is **independent of it**. Filed separately so it is
not conflated with the RTC / backup-rail work.

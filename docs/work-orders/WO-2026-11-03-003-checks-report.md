# WO-2026-11-03-003 checks report (read-only; worktree gcc-wt/rel, main, v48)

Scheme under test: PRODUCT_VERSION = major*100 + iteration (v49.1 -> 4901), up to about 9999. String `v49.<n>-<name>`.
Particle docs fetched 2026-11-03.

## Q1. FIRMWARE_PRODUCT_VERSION and fw_version in the repo

### Observation
- `src/FirmwareVersion.h:18` `#define FIRMWARE_PRODUCT_VERSION 48` (a plain int literal). `:21` `inline const char* FIRMWARE_VERSION = "v48-OccupancyEndsAtClose";` (24 chars).
- Only consumer of the integer: `src/Generalized-Core-Counter.cpp:43` `PRODUCT_VERSION(FIRMWARE_PRODUCT_VERSION);`. No other use in src/, lib/, tests/, tools/, docs/contracts, docs/reference. No C field carries it; no uint8_t anywhere.
- The string FIRMWARE_VERSION is consumed at:
  - `Generalized-Core-Counter.cpp:1007` (Boot log line, `%s`).
  - `:1345` and `:1374` (the `status` event JSON, `"version":"%s"`, into `char status[1024]` at `:1281`).
  - `cloud/DeviceStatusPublisher.cpp:212` (ledger `firmware.version`).
  - `observability/StartupSnapshotRuntime.cpp:13` into `StartupSnapshot::firmware`, `char firmware[32]` (`StartupSnapshot.h:20`), copied with snprintf (`StartupSnapshot.cpp:7-12`, truncating, no overflow).
- No version comparison (`>=`, min-version gate, config-generation tie) on either value anywhere in src/. Config uses `config.generation` (a separate counter), `kLedgerSchemaVersion = 2` (`DeviceStatusPublisher.cpp:53`) is independent. `default-settings-v3.23.json` has no firmware version key.
- `docs/contracts/ledger-contracts.md:79,109,120` show `firmware.version` as a string ("20.0-dev"), `startup.firmware` as a string. No size limit stated there.
- Payload headroom: `docs/RECOVERY_PLAN_2026-09-26.md:116` says STATUS ledger payload was 834 of 896 bytes (62 bytes headroom) when written (September; not re-measured by me).
- `tests/startup_snapshot_test.cpp:19` uses "20.0-dev", so dotted strings are already exercised there.
- Release tooling: `bump_version.sh:29` `PRODUCT_VERSION_INT = sed -E 's/^v//; s/[.-].*$//'` (strips at the first `.` or `-`), validated as digits (`:31`), written to the header by sed (`:76`). `PROJECT_STATUS.md:237,257-264` documents "point releases share the PRODUCT_VERSION".
- Fleet-ops side (docs and code, read only): `fw_version` is a Particle webhook envelope field, typed `fw_version?: string` (`particle-fleet-operations/scripts/src/types.ts:16`); tests use '13','14'; `docs/API.md:176` `firmwareVersions: ["14"]` (string list). Fleet-ops `tools/breadcrumb-decode.js:31` and `docs/STYLE_GUIDE.md:92-97` say `fw_version` is a label, never consulted for decoding. `tools/exerciser_baseline.py:47,162` here groups by `fw_version` as an opaque string (`or "?"`), no numeric parsing.
- Nothing found comparing versions numerically in fleet-ops docs; I did not read all of its code.

### Inference
- 4901..9999 fits everywhere: the only integer use is the PRODUCT_VERSION macro (Particle stores it as uint16, see Q3).
- `v49.<n>-<name>`: +2 chars over today's format (`v49.1-` vs `v49-`), 26 chars max for the current name, under the 32-char snapshot buffer (names over 25 chars would truncate harmlessly in startup.firmware only). Costs about 2 bytes in each status/ledger payload; against a 62-byte headroom (stale figure) that is negligible but worth re-measuring.
- What BREAKS: `bump_version.sh:29` would derive 49 from `v49.1-name`, i.e. every iteration would get PRODUCT_VERSION 49 and Particle would see the same version. The script must be changed to compute major*100+iteration (and map `v49` with no iteration to 4900 or 49, decide). This is the only code-level break found. Also `FirmwareVersion.h:6-9` and `PROJECT_STATUS.md:226-264` comments/policy (point releases share a version) become wrong.
- Mixed semantics risk: 48 (old scheme) vs 4901 (new): monotonic increase holds (4901 > 48), but a future reader of `fw_version` sees "48" then "4901". Any fleet-ops consumer that sorts `fw_version` as strings would order "4901" < "49"? No: "4901" > "48" lexically too, but "5001" vs "4901" fine; "999" vs "4901" would misorder, only historical values 9..99 vs 4-digit are affected. Unconfirmed whether any consumer sorts.

## Q2. Ubidots keyed on fw_version?

### Observation
- Repo search for "ubidots" (excluding docs/html) finds: hook name `Ubidots-Sensor-Hook-v1` (`default-settings-v3.23.json:31`), `tests/report_payload_fields_test.py:9-10` (every new top-level payload key must be an unquoted JSON number because Ubidots auto-creates a variable per key), `RECOVERY_PLAN:321` (the Ubidots template gets `"vc": "{{vc}}"`). The hook template itself is not in the repo.
- The `status` event carries `"version":"..."` (`Generalized-Core-Counter.cpp:1345`), but that goes on the `status` event, not the Ubidots hook (the hook payload fields are the `{{...}}` template keys, not stored here).
- Fleet-ops docs (`operations/wo-2026-09-30-001-...rollback-drill.md:3`) say Ubidots is the system of record; no document describes dashboards keyed on firmware version.

### Inference / cannot tell
- The repo cannot show whether the Ubidots hook template, variables, dashboards or alerts use `fw_version` (Particle's `{{{PARTICLE_...}}}`/`fw_version` fields) or the `version` string. Needs a look at the live Ubidots account and the Particle integration template. If a dashboard filters on a numeric `fw_version`, 4901 is still numeric and valid; a widget with a fixed list/threshold such as "fw >= 45" would be unaffected (4901>45). Unconfirmed.

## Q3. Particle: development devices and version numbers

### Observation (URLs fetched 2026-11-03)
- https://docs.particle.io/getting-started/console/development-devices/ : development devices "are prevented from receiving any automatic product firmware updates" and "will ignore both released product firmware as well as any firmware version it has been locked to run." Freely flashable by IDE/CLI. The page does not say whether a lock can be set on a development device.
- https://docs.particle.io/getting-started/cloud/ota-updates/ : "The firmware version must be an integer that increments each time a new binary is uploaded to the Console." Version typed at upload must match the compiled one. Lock/Flash-now is done in the Console device Edit view; a lock is "respected above all else" (for non-development devices).
- https://docs.particle.io/reference/device-os/api/macros/product_version/ : example "PRODUCT_VERSION(1); // increment each time you upload to the console"; no range or type stated; "Devices marked as development devices do not need a valid PRODUCT_VERSION." (The page does not mention 16-bit, uint16 or 65535.)
- Community post (https://community.particle.io/t/os-device-api-to-get-the-product-id-and-product-version/49685), not official: runtime variable is `uint16_t __system_product_version`, set by the macro.
- Community (https://community.particle.io/t/unlock-device-to-firmware-when-marked-development-avoid-risky-updates/61984): locking while marked development, then un-marking, can trigger an immediate OTA to the targeted version; workaround is to flash a binary of the matching version first. Matches this repo: `docs/bench-validation.md:356` and `docs/releases/v24-thermal-inhibit-validation.md:97` note a device with `desired_firmware_version` set is OTA-reverted.

### Inference
- A development device does not honour a product firmware pin/lock (it ignores it); only direct flashes/uploads reach it. The pin takes effect only after un-marking development, and may then OTA immediately. Whether the Console lets you set a lock on a development device: not confirmed.
- Product version range: official docs give no limit. Only an unofficial source says uint16 (0..65535), so 9999 fits. NOT confirmed officially; the Console upload dialog's own validation (any max) is unconfirmed. Test with a throwaway upload of 4901 before relying on it.
- Monotonic: docs say the integer "increments each time a new binary is uploaded". Whether the Console rejects a lower or duplicate number is not stated in the fetched pages (I recall the Console refusing a version already uploaded to the product; unconfirmed). The scheme is monotonic if iterations ascend and no major is released out of order (4902 before 5001 fine).

## Summary (5 lines)
1. `bump_version.sh:29` must change: it derives 49 from `v49.1-name`; compute major*100+iteration (and decide the `v49` with no iteration case).
2. No C width problem: the only integer use is `PRODUCT_VERSION(FIRMWARE_PRODUCT_VERSION)` (Particle uint16, 9999 fits; uint16 only community-sourced); no version comparisons in src/ or fleet-ops docs.
3. `v49.<n>-<name>` is +2 chars: fits `char firmware[32]` and the 1024-byte status buffer; re-measure the 896-byte status ledger (62 bytes headroom in Sept).
4. Update the comments in `FirmwareVersion.h:6-9` and `PROJECT_STATUS.md:226-264` ("point releases share PRODUCT_VERSION"); Ubidots dependence on `fw_version` cannot be determined from the repo (check the live hook/dashboards).
5. Development devices ignore product pins (flash directly only); un-marking may OTA at once; confirm in Console that 4901 uploads and that lower-than-existing versions are accepted/rejected as expected.

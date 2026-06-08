# v14 Field Rollout Checklist

**Small-team, field-focused checklist for See Insights outdoor Particle devices (Boron / Photon 2).**

Use this before broader v14 deployment.

## Release Identity
- [ ] Confirm Particle integer release version is set to v14 in source.
- [ ] Confirm version strings are consistent across source constants, README.md, CHANGELOG.md, and release notes.
- [ ] Confirm release notes describe cloud recovery improvements, logging cleanup, and any sleep-related changes.

## Build Profile Safety
- [ ] Confirm `DEV_BUILD=0` for this field release (`src/BuildProfile.h`).
- [ ] Confirm `ALLOW_BLOCKING_SERIAL_WAITS=0`.
- [ ] Confirm `CONNECTIVITY_FAILSAFE_TEST_MODE=0`.
- [ ] Confirm `ENABLE_PMIC_FORENSICS` setting is intentional and documented for this release.
- [ ] Confirm production build flags / command are documented in README.md or release notes.

## Persistent Data Safety
- [ ] Confirm any retained/persistent data layout changes are documented.
- [ ] Confirm migration behavior from v13 → v14 is understood and safe.
- [ ] Confirm field devices can upgrade from v13 to v14 without losing critical configuration.
- [ ] Confirm reset/status payload includes enough information to diagnose migration problems.

## Secret / Local Data Hygiene
- [ ] Confirm no Wi-Fi passwords, Particle tokens, local paths, private device IDs, or customer-specific secrets are committed.
- [ ] Confirm `.gitignore` covers local/operational artifacts.
- [ ] Confirm a local secret scan was run before this release (document result in release notes).

## Platform Behavior
- [ ] Document Boron vs Photon 2 sleep behavior:
  - Boron ULP sleep behavior
  - Photon 2 HIBERNATE wake behavior
  - Expected reset reason / status behavior after hibernate
- [ ] Confirm field operators understand that Photon 2 hibernate wake may appear as a reset but is expected.

## Operational Validation
- [ ] Confirm clean logs show one-line REPORT output.
- [ ] Confirm `REPORT!` appears for alerts, resets, or slow connections.
- [ ] Confirm STATUS payloads are preserved.
- [ ] Confirm OTA update sequence is visible in Particle logs.
- [ ] Confirm at least one successful v14 report after OTA per device (during soak).

## Rollback / Recovery
- [ ] Confirm previous stable release is available for rollback.
- [ ] Confirm Particle console can target specific devices for OTA.
- [ ] Confirm local serial logs can be captured if needed for troubleshooting.

**Reviewed and approved for field rollout:** ________________ Date: __________
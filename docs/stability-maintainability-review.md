# Stability & Maintainability Review

**For small-team field-stability focus on See Insights outdoor Particle devices (Boron / Photon 2).**

This document refines Grok’s architectural recommendations into practical, low-overhead actions. Prioritized for firmware stability, resiliency, maintainability, and field operations. No source code changes. Focus on documentation, lightweight checks, and narrow issues.

Current context: Actively testing/releasing v14 (cloud recovery + logging cleanup). Beyond v11 soak.

## P0 — Must Do Before Broader Field Rollout

Items that could prevent field failures or make recovery/debugging materially safer.

### Verify firmware version/release number consistency
- **Why it matters**: Mismatched version strings across build artifacts, README, CHANGELOG, and source can cause confusion during OTA, debugging, and fleet analysis.
- **Risk reduced**: Prevents deploying/debugging the wrong firmware image or misattributing field behavior.
- **Suggested next action**: Add a release checklist (or section in RELEASE_NOTES) confirming PRODUCT_VERSION, FIRMWARE_VERSION, README header, CHANGELOG, and Doxygen are in sync. Especially important around v14 changes.
- **Estimated effort**: Small
- **GitHub issue?**: Yes — “Add release consistency checklist for Particle integer firmware versions”

### Confirm BuildProfile.h defaults are field-safe
- **Why it matters**: DEV_BUILD=1 and CONNECTIVITY_FAILSAFE_TEST_MODE defaults must not leak into production builds.
- **Risk reduced**: Avoids bench-only timings or blocking serial waits in field devices (battery drain, unexpected behavior).
- **Suggested next action**: Document explicit production build command/flags in README and release.sh. Add pre-release validation step to confirm test mode=0.
- **Estimated effort**: Small
- **GitHub issue?**: Yes — “Confirm and document field-safe BuildProfile defaults before v14 rollout”

### Confirm persistent data structure/versioning is documented and migration-safe
- **Why it matters**: MyPersistentData.h/cpp is central; layout changes (e.g., v14 recovery fields) require safe migration for in-field devices.
- **Risk reduced**: Prevents data corruption or lost config on OTA/upgrade.
- **Suggested next action**: Add explicit versioning/migration notes to docs/ (e.g., in architecture-overview.md or new persistent-data.md). Reference StorageHelperRK behavior.
- **Estimated effort**: Small / Medium
- **GitHub issue?**: Yes — “Document persistent data versioning and migration rules”

### Confirm no local paths, credentials, Wi-Fi SSIDs/passwords, Particle tokens, or device-specific secrets are committed
- **Why it matters**: Outdoor fleet; any leak compromises security or enables targeted attacks.
- **Risk reduced**: Eliminates credential exposure risk.
- **Suggested next action**: Run local secret scan (e.g., via GitHub tool or `git grep` / trufflehog) before each release. Document result in release notes. .gitignore already present; verify effectiveness.
- **Estimated effort**: Small
- **GitHub issue?**: Yes — “Run and document local secret scan before field rollout”

### Confirm v14 recovery/logging changes are documented in CHANGELOG and release notes
- **Why it matters**: Cloud recovery and logging cleanup are critical for field resiliency.
- **Risk reduced**: Operators and future maintainers understand new behavior and can debug effectively.
- **Suggested next action**: Ensure CHANGELOG.md and RELEASE_NOTES_*.md are updated with details, known limitations, and validation steps before broader rollout.
- **Estimated effort**: Small
- **GitHub issue?**: Yes — “Document v14 cloud recovery and logging changes”

## P1 — High Value, Low Overhead

Practical for small team.

### Add a lightweight build verification command to README
- **Why it matters**: Reproducible builds reduce "works on my machine" issues.
- **Risk reduced**: Ensures consistent production images across team members.
- **Suggested next action**: Add simple `make` or PlatformIO / particle CLI equivalent command (generalized, no local paths).
- **Estimated effort**: Small
- **GitHub issue?**: Yes — “Add lightweight build verification instructions to README”

### Add a simple GitHub Actions compile check (if practical)
- **Why it matters**: Catches compile breaks early without full CI.
- **Risk reduced**: Early detection of build issues before field testing.
- **Suggested next action**: Minimal workflow for Boron/P2 compile only, if dependencies allow without heavy setup.
- **Estimated effort**: Medium
- **GitHub issue?**: Defer if setup overhead high; consider later.

### Add a persistent-data migration note
- **Why it matters**: Supports safe evolution.
- **Risk reduced**: Field device upgrades.
- **Suggested next action**: Brief section in docs/.
- **Estimated effort**: Small
- **GitHub issue?**: Combined with P0 item.

### Add a troubleshooting section for reset reasons, hibernate wake behavior, cloud recovery, and Particle OTA events
- **Why it matters**: Field devices are remote; quick debug info saves time.
- **Risk reduced**: Faster recovery from field issues.
- **Suggested next action**: New subsection in README or recovery-architecture.md covering breadcrumbs, reset reasons, failsafe logs, etc. Document Boron vs Photon 2 differences.
- **Estimated effort**: Small / Medium
- **GitHub issue?**: Yes — “Document Boron vs Photon 2 sleep and reset behavior + troubleshooting”

### Document Boron vs Photon 2 sleep behavior differences
- **Why it matters**: Key to power/resiliency understanding.
- **Risk reduced**: Better expectations for field performance.
- **Estimated effort**: Small
- **GitHub issue?**: Included above.

## P2 — Nice Later

Valuable but not urgent.

- Refactor oversized files (e.g., Generalized-Core-Counter.cpp) incrementally.
- Add state-machine unit tests or host-side tests if feasible (low priority for small team).
- Add long-term log analytics ideas (post-v14).
- Deeper platform abstraction cleanup.
- Dashboards (external, not firmware).

## Defer / Do Not Do Now

- Full CI/CD deployment pipeline.
- Prometheus/Influx export or advanced analytics.
- Large-scale test harness.
- Major refactor of main state machine.
- Enterprise-style security hardening beyond basic secret hygiene.
- Automated OTA rollout system.

These would add overhead without immediate field risk reduction for the current small-team context.

## Recommended Next 5 GitHub Issues

1. “Document persistent data versioning and migration rules”
2. “Add release consistency checklist for Particle integer firmware versions”
3. “Document Boron vs Photon 2 sleep and reset behavior + troubleshooting section”
4. “Add lightweight build verification instructions to README”
5. “Run and document local secret scan before v14 field rollout”

**Review complete.** This keeps focus on stability without over-engineering. Update this doc as v14 progresses.
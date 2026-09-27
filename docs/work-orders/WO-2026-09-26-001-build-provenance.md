# WO-2026-09-26-001: build provenance

**Goal, in plain language:** the code the fleet runs is the code in the repo.

**Status:** Opened 2026-09-26. Not started. Recovery plan **Phase 4 backlog** (moved from Phase 1 item 1b the same day; `docs/RECOVERY_PLAN_2026-09-26.md`).

**Workflow:** Per `AI_DEVELOPMENT_WORKFLOW.md` (see §12, guardrail 5).

## Problem

`project.properties` lists four libraries that are also vendored in `lib/`:

```
dependencies.StorageHelperRK=0.0.5
dependencies.AB1805_RK=0.0.4
dependencies.LocalTimeRK=0.1.3
dependencies.PublishQueuePosixRK=0.0.7
```

A Particle cloud build installs the registry copies of these over `lib/` (and, as PublishQueuePosixRK dependencies, registry BackgroundPublishRK 0.0.2 and SequentialFileRK 0.0.2). A local toolchain build compiles the vendored copies. So local and cloud builds of the same commit can contain different library code.

Confirmed 2026-09-26 with a cloud build of `599038e` in a scratch copy: a `#warning` marker in each of the four vendored headers never fired, and `AB1805::REG_OSC_STATUS_OMODE` compiled as `0x01` (registry 0.0.4). The vendored value, with PR #41, is `0x10`. **PR #41's AB1805 fix is therefore absent from every cloud-built binary.** Evidence: `WO-2026-09-25-001-phase1-provenance-compile.log` (on the WO-2026-09-25-001 branch).

## Scope

1. **Compare each vendored library against its registry version, and list every difference.** The known differences are PR #41's AB1805 constants (commit `ab2e0fe`, `lib/AB1805_RK/src/AB1805_RK.h`). The other libraries are expected to be identical to their registry versions, but that needs confirming, file by file.
2. **Remove the four `dependencies.*` lines** from `project.properties`, so cloud builds compile the vendored copies.
3. **Verify in a cloud-built binary** that the vendored code is in it: the `OMODE` check (compiles as `0x10`), plus one marker per library, each shown to fire.

## Sequencing

Separate from WO-2026-09-25-001, so the bench tests one change at a time. Originally planned to land right after WO-2026-09-25-001; moved to the Phase 4 backlog on 2026-09-26.

## Bench check

Small: the AB1805 fields in the `status` event read correctly (`ab1805WakeReason`, `watchdogSource`), on a cloud-built binary that passed the check in scope item 3. The device-status ledger's clock block (`oscStatus`, `fos`, `aos`) also comes from the AB1805 and can be checked at the same time.

## Approval record

- [ ] Investigation: library comparison (Stage 4)
- [ ] Chip approval (Stage 5)
- [ ] Implementation (Stage 6)
- [ ] Codex verification, including the cloud-binary check (Stage 7)
- [ ] Chip final gate / commit (Stage 8)

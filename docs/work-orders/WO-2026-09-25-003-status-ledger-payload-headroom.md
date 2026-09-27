# WO-2026-09-25-003: status ledger payload headroom

**Status:** Stub. Stage 4 (investigation) pending.

**Workflow:** Per `AI_DEVELOPMENT_WORKFLOW.md`.

## Problem

The device-status ledger payload is built into a fixed buffer of `DEVICE_STATUS_PAYLOAD_CAPACITY = 896` bytes (`src/cloud/Cloud.h:397`), a limit the firmware sets for itself, not a Particle Ledger limit. Dev-14's payload was observed at 831–856 bytes on 2026-09-25 (most often 853), and Copilot's WO-2026-09-25-001 analysis puts the existing fields' deployed worst case at 903 bytes, already over the cap. Before WO-2026-09-25-001, an oversized payload wrote past the end of the stack buffer; WO-2026-09-25-001 adds a guard that refuses the publish and logs an error instead.

## Scope (to be refined at Stage 3)

1. Measure the payload: observed range across the fleet, and the structural worst case of every field.
2. Then either raise the cap after checking the stack (two buffers of that size, `bufferBase` and `bufferPublish`, plus the saved last-published copy), or slim the payload. Any change to the ledger format affects its consumers.
3. Alert whenever a publish is refused for size.

## Related

- WO-2026-09-25-001 (decision 5 moved the delivery counters to the `status` event because of this headroom).

## Approval record

- [ ] Investigation (Stage 4)
- [ ] Chip approval (Stage 5)
- [ ] Implementation (Stage 6)
- [ ] Codex verification (Stage 7)
- [ ] Chip final gate / commit (Stage 8)

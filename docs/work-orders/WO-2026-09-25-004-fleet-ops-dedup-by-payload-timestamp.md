# WO-2026-09-25-004: fleet-ops — key records by device ID + event name + payload timestamp

**Status:** Stub. Stage 4 (investigation) pending. Repository: `particle-fleet-operations` (not this repository). Deployment is Chip's.

**Workflow:** Per `AI_DEVELOPMENT_WORKFLOW.md`.

## Problem

WO-2026-09-25-001 moves device publishes to at-least-once delivery (explicit `WITH_ACK`, retry until acknowledged). When an acknowledgment is lost, the firmware resends the same event with the same payload, including the same payload `timestamp`. Codex's WO-2026-09-25-001 Stage 7 review found that fleet-ops keys its records on the cloud's top-level `published_at` rather than on the timestamp inside the payload, so each resend gets a new `published_at` and creates a second record. (Copilot's Stage 6 analysis had concluded the opposite; Codex corrected it: `WO-2026-09-25-001-stage7-codex/REVIEW.md`.)

## Proposed direction (to be confirmed at Stage 3)

Key records by device ID + event name + payload timestamp, so a resend of the same event overwrites or is rejected rather than duplicated. Scope includes DynamoDB, event history, and the S3 archive keys, plus any consumer that counts records.

## Related

- WO-2026-09-25-001 decision 8: at-least-once delivery with consumers that tolerate duplicates; this WO tracks the fleet-ops side and does not block WO-2026-09-25-001.
- Ubidots: WO-2026-09-25-001 records whether any widget aggregates dailyoccupancy with SUM.

## Approval record

- [ ] Investigation (Stage 4)
- [ ] Chip approval (Stage 5)
- [ ] Implementation (Stage 6)
- [ ] Codex verification (Stage 7)
- [ ] Chip final gate / deploy (Stage 8)

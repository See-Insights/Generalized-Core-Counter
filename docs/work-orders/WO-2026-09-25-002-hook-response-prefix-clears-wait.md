# WO-2026-09-25-002: hook-response/ prefix clears the webhook-response wait before checking status

**Status:** Stub. Stage 4 (investigation) pending. Out of scope for WO-2026-09-25-001.

**Workflow:** Per `AI_DEVELOPMENT_WORKFLOW.md`.

## Problem

Found by Codex during the WO-2026-09-25-001 Stage 4 investigation (`WO-2026-09-25-001-stage4-codex/REPORT-webhook-loss-investigation.md`, section 3). After a report is queued, the firmware arms a webhook-response wait that should keep the connection up until the report's webhook responds or times out. The app subscribes both to its device-ID response topic and to the broad `hook-response/` prefix (`src/Generalized-Core-Counter.cpp:955–964`), and `UbidotsHandler()` clears the single wait flag on **any nonempty response, before checking its status** (`src/Generalized-Core-Counter.cpp:2394–2411`). An unrelated or failed response can therefore release the wait and let the device sleep early. At 15:00 SGT on 2026-09-25, Dev-14 tore down the connection before even the minimum 5-second response timeout, with no timeout log; this gap is one possible explanation. Codex rated it plausible but unproven: there is no response-topic trace, and when verbose mode is off the release is silent.

## Approval record

- [ ] Investigation (Stage 4)
- [ ] Chip approval (Stage 5)
- [ ] Implementation (Stage 6)
- [ ] Codex verification (Stage 7)
- [ ] Chip final gate / commit (Stage 8)

## Update 2026-09-29 (WO-2026-09-29-001, item B)

- **The broad subscription is removed in v27** (WO-2026-09-29-001 item B). History: it was added in `eda6b7e` (v3.24), with the reason "works with whatever webhook name is configured in the ledger". Every product integration (product 42131) replies on `{{PARTICLE_DEVICE_ID}}/hook-response/{{PARTICLE_EVENT_NAME}}`, which the device-ID subscription already matches. The only reply the broad `hook-response/` prefix ever caught was `pdiag`'s, whose integration has no response topic. That reply releasing the report wait is this WO's problem, and v26 stopped release builds publishing `pdiag`.
- **Still open here (Chip, 2026-09-29):** the device-ID subscription also receives the replies for `status`, `watchdog`, `hibernate_wake`, and the AWS-ingest copy of `Ubidots-Sensor-Hook-v1` (the same event name as the Ubidots integration). `UbidotsHandler()` clears `session.awaitingWebhookResponse` on any nonempty response, so any of those can still end the report wait before Ubidots itself replies.

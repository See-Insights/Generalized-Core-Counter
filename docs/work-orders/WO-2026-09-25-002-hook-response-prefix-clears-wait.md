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

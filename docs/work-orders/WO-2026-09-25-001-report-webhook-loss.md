# WO-2026-09-25-001: Report webhook loss (publish acknowledgment and timing after connecting)

**Status:** Stage 7 round 2 **NOT VERIFIED** (2026-09-25): round-1 findings resolved, three narrower defects remain. Awaiting Chip's decisions before a Stage 6 round 4. Branch `wo/2026-09-25-001-publish-with-ack` (stacked on WO-2026-09-24-001 at `599038e`). Do not flash the Stage 7 candidate binaries. Priority: highest; other work waits on this.

**Workflow:** Per `AI_DEVELOPMENT_WORKFLOW.md`.

## Problem

Report webhooks (`Ubidots-Sensor-Hook-v1`) are silently lost. The device logs them as accepted into the publish queue and later logs "queue drained", but they never appear in Particle's event stream, the integration history, AWS, or Ubidots. Other events are sometimes lost the same way.

Working hypothesis (not yet proven): events sent in the first seconds after the cloud connection comes up are lost at the device or session level, and `PublishQueuePosix` removes them because it counts them as sent without a confirmed acknowledgment.

Found during the WO-2026-09-24-001 bench on Dev-14, where it hid that WO's end-of-day snapshot webhook (15:00 SGT, 2026-09-25).

## Evidence (Dev-14, 2026-09-25, SGT)

| Wake | Report ran | Connected | First event received | Lost |
|---|---|---|---|---|
| 06:13 (flash) | offline, then trusted | — | — | the post-reset 0 report (reset at 06:15:32 cleared the RAM queue; separate cause) |
| 14:41 | occupancy end | — | pdiag 14:41:19 | the report webhook (DATA ledger updated, no webhook) |
| 15:00:03 | offline, before connecting | 15:00:06 | pdiag 15:00:11 | both report webhooks (`ConnSummary q=3`, then queue drained) |
| 15:22 (reset) | none | ~15:22:23 | pdiag 15:22:27 | 1 of 4 queued events (`q=4`; pdiag, hibernate_wake, status arrived) |
| 15:49 (reset) | online (already connected) | 15:49:22 | pdiag 15:49:31 | report delivered (15:49:39, 200 at ingest); status possibly lost (`q=3` at connect) |

Supporting facts established before dispatch:

- Every event type goes through the same queue: `PublishQueuePosix::instance().publish(…, PRIVATE)` (report, pdiag, status, hibernate_wake, watchdog). No direct `Particle.publish()` calls.
- No firmware gate between `publishData()` and the queue: no open-hours check, no dedupe, no minimum interval. The webhook-health logic only observes `hook-response`.
- `q=` in `Report:` lines is `publish()`'s return value; in `LoopStage`/`ConnSummary`/`Connect: ok` it is queue depth.
- At 15:00 the AWS ingest Lambda received exactly one Particle-sourced request in 07:00:00–07:00:40Z (pdiag, 07:00:11.574Z), and none for the two report webhooks.
- The Particle console showed no rate limiting (`publish.rate_limited: 0` at 14:18). Integration history and AWS ingest agree event for event.

The full evidence, including the 15:00 and 15:22 USB serial captures, is in the Stage 4 dispatch.

## Stage 4: Codex investigation

- Dispatch: `WO-2026-09-25-001-stage4-codex-investigation-dispatch.md` (verbatim as sent).
- Agent: Codex CLI 0.154.0, `codex exec`, model `gpt-6-astra`, reasoning `ultra` (top model and highest level listed by the CLI at dispatch time).
- Sandbox: workspace-write limited to a scratch directory, `/tmp`, and `~/.aws`; repository read-only; network enabled for read-only AWS queries. Note: the AWS SSO session resolves to an AdministratorAccess role, so AWS read-only is enforced by instruction, not by credentials.

### Results

The full report and its evidence are in `WO-2026-09-25-001-stage4-codex/`:

- `REPORT-webhook-loss-investigation.md`: the report, verbatim
- `QUEUE-findings.md`, `DOS-findings.md`, `DEV14-evidence.md`, `FLEET-findings.md`: line-numbered source and data evidence
- `BENCH-A-publish-diagnostics.diff`, `BENCH-B-explicit-ack.diff`: the two draft bench builds, not applied, not built, not flashed
- `fleet-data/`: scope CSVs and the report-to-delivery match audit trail
- `repo-final-validation.json`: Codex's integrity check (all 1,178 hashed files unchanged)

Raw inputs (per-device timelines up to 50 MB, raw S3 objects, Lambda windows) were not copied into the repository.

**Root cause: a proven gap; the specific cause of the observed losses is unproven.** Codex's summary: "PRIVATE-only Future success permits queue removal without waiting for a cloud ACK. The specific incident cause remains unproven."

| Conclusion | Confidence | Basis |
|---|---|---|
| A PRIVATE-only publish's Future succeeds after the local send, not on a cloud ACK | High, proven in source | Device OS 6.4.1 `communication/src/publisher.cpp:88–98`: the completion handler waits for an ACK only when `WITH_ACK` is set; otherwise `handler.setResult()` runs right after `channel.send()` |
| The queue deletes an event on that Future success, so it can remove events the cloud never confirmed | High, proven in source | `BackgroundPublishRK.cpp:83–94` passes `ok.isSucceeded()` to the queue's completion callback |
| A non-graceful disconnect discards outstanding CoAP retransmissions | High for the mechanism | App teardown calls `Particle.disconnect()` without graceful options (`src/power/Connectivity.h:38–47`); Device OS defaults to TERMINATE, which resets the CoAP retransmit store (`dtls_protocol.h:103–105`, `coap_channel.h:584–588`). Default retries fall roughly 4–6, 8–12, then 16–24 s after send. |
| That mechanism caused the 15:00 losses | Moderate, **not proven** | At 15:00 the queue drained 4.83 s after `ConnSummary`, and teardown began about 4.85 s after it, inside the first retry window. No per-attempt send/ACK trace exists. |
| "Everything sent in the first N seconds after connecting is lost" | **Contradicted** as a fixed rule | The first event (pdiag) at 15:00 was delivered while the later reports were lost; many early reports elsewhere were delivered |
| A session-resume bug | Unproven | Device OS gates "connected" on handshake completion; the captures contain no session trace |

Other findings:

- **Report flags:** every queue publish is `PRIVATE` only (0x01). `WITH_ACK` is 0x08. The library already accepts it through its existing second flags argument; no new API is needed. The queue already waits 2000 ms after it sees `Particle.connected()`, then 1000 ms between publishes (`PublishQueuePosixRK.h:414–416`).
- **Library logs:** the queue logs through `Logger _log("app.pubq")` at TRACE, and the app's log handler is INFO with no `app.pubq` override, so the absence of `[app.pubq]` lines is expected and says nothing about delivery.
- **`setPausePublishing()` is unsafe on its own here:** while paused, the queue reports itself sleep-safe even with events pending, so the device could sleep with the report still queued.
- **A second, unproven gap:** the report arms a webhook-response wait that should hold the connection, but at 15:00 teardown came before even the minimum 5 s timeout, with no timeout log. `UbidotsHandler()` clears the wait on *any* nonempty response on the broad `hook-response/` prefix, before checking its status (`src/Generalized-Core-Counter.cpp:2394–2411`), so an unrelated response could release it.
- **`q=` depth is not an inventory:** `getNumEvents()` can omit files and overcount after a failed attempt, so queue-depth counts at reset wakes (for example 15:22's `q=4`) can't identify a missing event.
- **Clock correction for the evidence table:** the "06:13 / 06:15:32 / 06:18:24" rows are UTC (14:13–14:18 SGT); 14:41, 15:00, 15:22 and 15:49 are SGT.

**Fleet scope (14 days, 2026-09-11 16:00 to 2026-09-25 16:00 SGT):**

- 5,992 report deliveries in S3 across 12 devices. Only 115 `Report:` lines survived in the serial forwarder, all on Dev-09, -11 and -14; 105 matched a delivery and 10 did not (8.7% of a sparse, biased sample).
- **The true loss rate can't be measured from the archive**, for any device. No record of generated reports exists apart from serial capture, and the nine production devices have no serial capture at all.
- 7 of the 10 unmatched reports were logged 2.2–4.1 s before `ConnSummary: ok` on a connection with queue depth 2, and each of those connections delivered its pdiag. So the pattern extends to Dev-09 and Dev-11. But 32 matched reports had the same pre-connect, depth-2 profile and were delivered.

| Device | Firmware (event) | Delivered | Report lines | Matched | Unmatched |
|---|---|---:|---:|---:|---:|
| Boron-Dev-09 | 24 | 387 | 47 | 40 | 7 |
| Boron-Dev-11 | 24 | 277 | 27 | 24 | 3 |
| Boron-Dev-14 | 24 | 574 | 41 | 41 | 0 |
| SAMIT-TRAIL02 | 24 | 263 | 0 | — | — |
| 8 production devices (ToM-MCP-*, Morrisville-Tennis-*) | 21 | 4,491 total | 0 | — | — |

The per-day and full device × firmware × day tables are in `fleet-data/`. Dev-14's 0 unmatched does not mean no loss: its 15:00 and 14:41 losses were outside forwarder coverage.

**Draft bench builds (not applied):**

| Draft | Version string | Change |
|---|---|---|
| A: diagnostics | `v24-Pubq-Diag-A` | TRACE for `app.pubq`/`app.seqfile`, CoAP/session trace, and one `PubqAttempt` line per actual publish attempt (event, flags, result, ms since connect). No delivery-policy change. |
| B: candidate fix | `v24-Pubq-Ack-B` | Same diagnostics, plus every queued send normalized to explicit `WITH_ACK` at dispatch: `(curEvent->flags & ~NO_ACK) \| WITH_ACK`. This also covers events already persisted in the queue. The existing 2 s / 1 s pacing is kept. |

Both are standalone diffs against the current working tree and pass `git apply --check`. Neither has been compiled.

**Codex's recommendation:** run A to capture the mechanism, then B as the first candidate fix. Explicit ACK closes the proven remove-before-ACK gap without inventing a safe delay. A delay (a 10 s window, with the sleep gate also held) is only a later experiment if A shows a residual startup problem after ACK supervision. Risks of B: longer awake time, slower queue drain, and possible duplicates when an ACK is lost (at-least-once, not exactly-once).

**Bench protocol:** section 8 of the report. In summary: reproduce the 15:00 close flow offline so the queue holds `[pdiag, report(total>0), report(total=0)]` before connecting; capture full USB serial from boot; run A, then B, with about 20 cycles per condition. Conditions: fresh and resumed sessions, a 2.7-minute and a 5-minute offline delay, and a report generated after connecting as a control. **A run passes only if every queued event has exactly one matching integration-history delivery and one matching AWS record.** An ACK or a "queue drained" line on its own never counts as a pass.

Model used: `gpt-6-astra`, reasoning `ultra` (its delegated sub-investigators inherited the same settings).

## Stage 5 decisions (Chip, 2026-09-25)

1. **Go directly to B** (skip bench A). B includes A's tracing.
2. **Add delivery counters to the status payload:** publishes attempted, acknowledged, failed, and retried, plus the count of events still queued at sleep. This is how the fleet-wide loss rate gets measured from now on.
3. **B's costs become acceptance criteria:**
   - (a) measure the increase in awake time per cycle;
   - (b) confirm that duplicate deliveries (the same payload timestamp resent after a lost acknowledgment) create no duplicate records in fleet-ops ingestion or duplicate dots in Ubidots. Check the ingest Lambda's deduplication, and how Ubidots handles the same timestamp. If either can't tolerate duplicates, that's in scope here.
4. **The `hook-response/` gap is out of scope.** It is WO-2026-09-25-002.
5. **Decision 5 (2026-09-25):** the delivery counters go in the status event, not the device-status ledger. The ledger format is unchanged. Keep the overflow guard, which fixes an existing stack overwrite. Deviations accepted: the project.properties dependency removal, the overflow guard, and the application-thread hooks. The duplicate question is settled on the bench (criterion 7), including a check of whether any Ubidots widget aggregates dailyoccupancy with SUM.

6. **Decision 6 (P1), 2026-09-25:** sleep is allowed when queue empty OR (delivery budget expired AND no publish in flight). The budget is 90 s by default and configurable. Before sleeping with events queued, move RAM-queue events to flash. Add a sleptWithQueued counter, and log each budget expiry.
7. **Decision 7 (P2/P3), 2026-09-25:** log the q snapshot and CycleDelivery before System.sleep(). Add an abandoned counter computed at boot from attempts without an outcome, with the invariant a = k + f + abandoned. Add a test that catches mutation (iv). Correct FIELD_MEANINGS_REFERENCE.md (GateFail q).
8. **Decision 8 (criterion 7), 2026-09-25:** at-least-once delivery with consumers that tolerate duplicates. Deduplication in fleet-ops moves to WO-2026-09-25-004 (user deploys it) and doesn't block this WO. Ubidots: record whether any widget aggregates dailyoccupancy with SUM. Criterion 7 changes to: duplicates are possible and counted; the consumers' tolerance is tracked in WO-2026-09-25-004.

   Context for decision 5: Dev-14's status-ledger payload was observed at 831–856 of the 896-byte cap (most often 853), so the 56-byte counter block would have pushed ordinary cycles past the cap, and the new guard would then have skipped the status publish. The headroom question itself is WO-2026-09-25-003.

**Branching:** `wo/2026-09-25-001-publish-with-ack`, created from the committed WO-2026-09-24-001 branch (stacked). Merge order: WO-2026-09-24-001 first, then this WO. This WO's documents move onto that branch.

**Stage 6:** Copilot, top model, reasoning high (a multi-file, safety-relevant change). Implement B starting from Codex's draft (`WO-2026-09-25-001-stage4-codex/BENCH-B-explicit-ack.diff`), rebased onto WO-2026-09-24-001 decision 8's code, plus the counters. Version string `v24-Pubq-Ack-B`.

**Stage 7:** Codex, top model, highest reasoning, via `codex exec`: a full review. Mutations, each of which must be caught: (i) drop `WITH_ACK`; (ii) delete the queue entry before the acknowledgment; (iii) allow sleep with unacknowledged events queued.

## Acceptance criteria

1. Every queued publish is sent with explicit `WITH_ACK`, including events already persisted in the queue from an older build.
2. A queue entry is removed only after its publish's Future succeeds with an acknowledgment. A failed or unacknowledged publish leaves the event queued for retry.
3. The device does not sleep, and does not tear down the cloud connection, while an unacknowledged event is still queued (outside the existing sleep-gate timeout, which must be logged if it fires).
4. The `status` event carries the delivery counters: attempted, acknowledged, failed, retried, and queued-at-sleep (decision 5). The device-status ledger payload is unchanged.
5. B's tracing (the `PubqAttempt` line per attempt, `app.pubq`/`app.seqfile` TRACE, CoAP/session trace) is present.
6. The increase in awake time per cycle is measured and recorded (decision 3a).
7. (Revised by decision 8.) Duplicates are possible and counted; the consumers' tolerance is tracked in WO-2026-09-25-004. Record whether any Ubidots widget aggregates dailyoccupancy with SUM.
8. The version string is `v24-Pubq-Ack-B`.
9. Binary verification: confirm in the exact binary to be flashed (strings or disassembly) that the WITH_ACK path and the new hooks are present.
10. The retained counters' placement is confirmed from the linker map.
11. (Decision 6.) With the queue non-empty, the device sleeps once the delivery budget (default 90 s, configurable) has expired and no publish is in flight. RAM-queue events are moved to flash first, `sleptWithQueued` is incremented, and each budget expiry is logged.
12. (Decision 7.) The `q` snapshot and `CycleDelivery` are logged before `System.sleep()`, so hibernate cycles emit them. An `abandoned` counter, computed at boot from attempts without an outcome, keeps the invariant `a = k + f + abandoned`. A test catches mutation (iv). `FIELD_MEANINGS_REFERENCE.md` describes `GateFail q` correctly.

## Bench protocol (for after the flash; drafted 2026-09-25, not yet run)

**Device and capture:** Dev-14 with the `v24-Pubq-Ack-B` build, USB serial captured continuously from boot (`particle serial monitor` or equivalent, saved to a file), before the first connection. Record the build hash and version in the capture.

**Record before changing anything:** Dev-14's current `timing.reportingIntervalSec`, `timing.openHour`, and `timing.closeHour` from its device-settings ledger. (As of 2026-09-25: interval 1800, open 6, close 15. `closeHour` 15 was itself a bench setting; its pre-bench value must be taken from the ledger history or the WO-2026-09-24-001 records.)

**Shorten the interval:** set `timing.reportingIntervalSec` to **600** in Dev-14's device-settings ledger. The firmware accepts 300–86400 s (`src/cloud/ConfigApply.cpp:265`). 600 s gives 20 cycles in about 3 h 20 min and leaves room in each cycle for B's acknowledgment waits (20 s timeout) and the queue's 30 s retry backoff. Each cycle must be "report generated while offline, then connect": confirm from the capture that the report's `Report:` line precedes that wake's `ConnSummary`.

**Cycles to include (all during open hours unless noted):**
- At least 20 "report offline, then connect" cycles.
- At least one close: set `closeHour` to the next hour so a report runs at the boundary. This exercises WO-2026-09-24-001's closing record (stamped at boundary − 1).
- At least one occupancy start and one occupancy end (walk past the sensor, then leave it clear past the debounce).

**Restore afterwards:** set `timing.reportingIntervalSec` back to its recorded value (1800), and `closeHour` (and `openHour`, if changed) back to the recorded pre-bench values, in the device-settings ledger. Confirm the restore from the next device-settings ledger sync and the next status payload.

**Pass criteria (all required):**
- Every queued event is delivered, matched one for one against the Particle integration history (and the AWS raw record), by device, event name, and full payload. An ACK or a "queue drained" line alone never counts.
- The status-payload counters agree with the capture and the integration history: attempted = acknowledged + failed, retries accounted for, and queued-at-sleep matches the events left for the next connection.
- No duplicate records in fleet-ops ingestion or Ubidots.
- The awake-time increase per cycle, compared with the pre-B build, is recorded.

## Approval record

- [x] Codex investigation (Stage 4) — 2026-09-25, `gpt-6-astra` at reasoning `ultra`. Mechanism proven (queue removes PRIVATE-only publishes without a cloud ACK); incident cause unproven. Report and evidence in `WO-2026-09-25-001-stage4-codex/`. Repository verified unchanged by both Codex and Claude Code.
- [x] Chip approval (Stage 5) — 2026-09-25. Decisions 1–4 above; decision 5 added the same day after Stage 6 round 1, with criteria 9 and 10; decisions 6–8 added after Stage 7 round 1, with criteria 11 and 12 and a revised criterion 7.
- [ ] Implementation (Stage 6) — round 1 done 2026-09-25 (`claude-opus-5`, high; report in `WO-2026-09-25-001-stage6-copilot-report.md`); round 2 done the same day (decision 5: counters moved to the `status` event; `claude-opus-5`, high; report in `WO-2026-09-25-001-stage6-round2-copilot-report.md`). Round 3 needed for the Stage 7 findings.
- [ ] Codex verification (Stage 7) — round 1, 2026-09-25, `gpt-6-astra` at reasoning `ultra`: **NOT VERIFIED**. The ACK fix is present in source and in both candidate binaries (criteria 1, 5, 8, 9, 10 verified). Blockers: P1, the bounded wait is unreachable while connected with a stuck queue (a host reproduction stayed connected for a simulated hour); P2, a successful HIBERNATE skips the `q` snapshot and `CycleDelivery`; P2, a reset during an attempt permanently unbalances the counters; P2, mutation (iv) (dequeue on ACK failure) survives all 48 tests; P3, the `GateFail` q documentation is reversed. Criterion 7: fleet-ops keys use the cloud `published_at`, so a resend after a lost ACK creates a second record (Copilot's round-1 premise was wrong); bench still pending. Review, verdict, binary verification, and mutation evidence in `WO-2026-09-25-001-stage7-codex/`. Suite 48/48 (sh via zsh, py via python3). Candidate binaries (not for flashing): local SHA-256 `eb956a11…16a6`, cloud `8ad80f19…3899`.
      Round 2 (review of Stage 6 round 3), 2026-09-25, `gpt-6-astra` at reasoning `ultra`: **NOT VERIFIED**. All five round-1 findings resolved; mutations (i)–(iv) all caught; the never-recovering-ACK reproduction sleeps at 90.001 s with the event retained in flash; the reset-mid-publish invariant holds (`a=2 k=1 f=0 b=1 r=1`); hibernate emits `CycleDelivery`; binary and map checks pass (retained block 24 bytes at `0x2003f450`, version 2). Remaining: P1, after the 25 s in-flight hold, teardown proceeds with a publish still in flight (`State_Sleep.cpp:698`), and a delayed-completion reproduction reached it with the current RAM event not on disk; P2, `sleptWithQueued` undercounts repeated SLEEPING→SLEEPING cycles (`State_Sleep.cpp:531`); P2, an expired delivery budget can carry into the next connection (`State_Sleep.cpp:803`). Criteria 6 and 7 are bench items (awake-time measurement; the Ubidots SUM-widget record). Test hygiene: three new test scripts leave an empty `build-tmp/`. Suite 49/49 (sh via zsh, py via python3). Evidence in `WO-2026-09-25-001-stage7-round2-codex/`. Candidate binaries (not for flashing): local `93a3c764…127a`, cloud `b2160449…fc4a`.
- [ ] Chip final gate / commit (Stage 8)

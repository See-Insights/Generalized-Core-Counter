# Recovery Plan: 2026-09-26

**Goal:** every report the device makes should reach Ubidots, using the design already built: send with `WITH_ACK`, wait for Ubidots' reply on a specific response topic, and escalate through the error state if the reply doesn't come. Restore that design; don't invent new mechanisms.

## What we know

- **Why reports go missing:** `WITH_ACK` was accidentally dropped in `eda6b7e` (v3.24, 2026-02-09) from the report, status, and diagnostic publishes. The whole fleet has run without it since then. Not caused by Step 5 or any WO this month.
- **The original design was sound.** The problems come from drift away from it:
  - `WITH_ACK` was lost (the missing reports);
  - an extra subscription to the whole `hook-response/` prefix was added. The original response topic (the device ID) is still subscribed; the broad prefix lets any reply release the wait (WO-2026-09-25-002).
- **The alert ladder is already fixed.** The old overwrite bugs in `alertResolution()` (cases 12, 13, 40) are gone: today's `resolveErrorAction()` returns alert 40's action directly. The webhook supervision block in `State_Report.cpp` is a *delayed* escalation to the error state, not a workaround for a broken case.
- **Cloud builds do not use the vendored libraries.** Because `project.properties` lists them, a cloud build installs the registry copies of AB1805_RK 0.0.4, StorageHelperRK 0.0.5, LocalTimeRK 0.1.3, PublishQueuePosixRK 0.0.7 (and its dependencies BackgroundPublishRK 0.0.2 and SequentialFileRK 0.0.2) over `lib/`. Confirmed with a cloud build of `599038e`: `REG_OSC_STATUS_OMODE` compiled as `0x01` (registry), so **PR #41's AB1805 fix is absent from every cloud-built binary**.
- **The fix is mostly restoration.**

## Worth keeping from the abandoned branch (`archive/wo-2026-09-25-001-round3`)

- The overflow guard, which fixes a stack overwrite (WO-2026-09-25-003).
- Verifying the actual binary (with strings, disassembly, or a compile-time probe), not only the source.
- The loss mechanism: without `WITH_ACK`, the queue deletes an event once it leaves the device.
- Duplicates: fleet-ops keys records by `published_at`, so resends create second records (WO-2026-09-25-004).
- The breadcrumb-18 connect stall (WO-2026-09-25-005), and the status ledger headroom (WO-2026-09-25-003).
- Diagnostic D: 7 lines that log each report's exact payload, for bench use.
- The method of matching reports to webhooks, for measuring delivery.

## Phase 1 investigation (answered 2026-09-26)

Codex (`gpt-6-astra`, reasoning high, read-only, against `599038e`), plus one scratch cloud build by Claude Code.

1. **Can Idle keep the device awake once `WITH_ACK` is back? Yes.** If Particle never acknowledges while connected, Idle refuses to sleep and its safety ceiling is disabled (`State_Idle.cpp:233`, `:280`, `:303`). v3.23 could also hang. A one-line handoff (`State_Idle.cpp:238` → `if (!updatesPending)`) passes the wait to Sleep's existing bounded gate (30–120 s, then alert 43 and disconnect).
2. **Alerts: already fixed** (see "What we know").
3. **Response topic:** the device ID, still subscribed; the extra `hook-response/` subscription is the problem.
4. **Library provenance: cloud builds use the registry copies**; PR #41 is absent from them (see "What we know").
5. **Queue publishes:** all six (report, `status`, `watchdog`, `hibernate_wake`, diagnostic helper, `pdiag`) are `PRIVATE` only.

## Way forward

**Phase 1: restore delivery** (first, because production has lost data since February).
- **1. WO-2026-09-25-001:** `PRIVATE | WITH_ACK` on all six queue publishes; the one-line Idle handoff (`if (!updatesPending)`) with the unused gate lines removed; a structural test that every queue publish carries `WITH_ACK`. About 15 lines of `src/`.
- WO-2026-09-25-002: remove the extra `hook-response/` subscription, keeping the device-ID response topic.
- Bench on Dev-14 (stacked with WO-2026-09-24-001, plus diagnostic D, with USB serial): 20 cycles of "report offline, then connect", one close, and one occupancy start and end. Pass: every report is delivered and the device sleeps every cycle.
- Then the fleet. Merge order: WO-2026-09-24-001, then 25-001, then 25-002.

**Phase 2: the alert ladder.** Only the `deepPowerDown()` bench check (the AB1805 configuration-key issue). The ladder itself needs no code change.

**Phase 3: clarity (Step 5.5).** Pilot: the reporting state in four plain steps (always report → daily cleanup if closing → already connected? → connect now or later), about 20 lines, with the details in named owners (`DailyBoundary`, `Connectivity::shouldConnectNow`) and the response wait restored explicitly. Then `setup()` as named phases, then the other states.

**Phase 4: backlog**, in order: build provenance (vendored versus registry libraries; WO-2026-09-26-001), fleet-ops duplicates, ledger headroom, the connect stall, the Dev-11 oscillator test, the PWGT fix.

## Guardrails (`AI_DEVELOPMENT_WORKFLOW.md` §12)

1. **History first:** check `git log -S` before designing anything new. If the behavior existed before, restoring it is the default fix. For a restoration, Stage 7 verifies against "at least as good as the version restored", not an expanded fault model.
2. **A plain-language goal** at the top of every WO.
3. **A size budget in every dispatch.** Going over it means stopping and reporting.
4. **The two-round rule:** two rounds that add new mechanisms, or two rounds without VERIFIED, means stop and restate the goal with the user.
5. **Verify the binary, not the source,** when vendored libraries are involved. Local and cloud builds may use different library copies. Bench-test the build type the fleet will receive.

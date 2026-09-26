AGENT: Codex · MODEL: gpt-6-astra · REASONING: ultra
AUTHORIZATION SCOPE: read-only code and data investigation, plus drafting (not applying) bench-build diffs / Not authorized: edits to the working tree, commits, flashing, settings changes, or anything that writes to infrastructure.
Dispatched via codex exec by Claude Code, per AI_DEVELOPMENT_WORKFLOW.md (repo root; §4 Dispatch format is on the pending docs/2026-09-25-workflow-consolidation branch). Priority: highest. Everything else waits on this.

## Environment (read carefully)

- Repository (READ ONLY): /Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter — branch wo/2026-09-24-001-daily-cleanup-boundary-fix. Do not modify anything in it (the sandbox also prevents this).
- Your working directory is a scratch folder. Write ALL output there: the report, the two draft diffs (as .diff/.patch files against the repo's current working tree), and any intermediate data. Draft diffs must never be applied to the repo.
- Particle library sources: lib/PublishQueuePosixRK/ in the repo. Device OS 6.4.1 sources: ~/.particle/toolchains/deviceOS/6.4.1/ (read only).
- Fleet data (read only): AWS CLI with AWS_PROFILE=particle-admin, region us-east-1. Raw events bucket: s3://infrastack-rawparticlelogsbucket4adba400-7yquehl7iulz/particle-events/<YYYY-MM-DD>/<eventName>/<deviceId>/<publishedAt>.json. Ingest Lambda log group: /aws/lambda/InfraStack-ParticleLogIngestionFunctionD5193211-ckpMn4aFdjbe. Fleet-ops CLI (read only): /Users/chipmc/Documents/Maker/AWS/particle-fleet-operations/tools/telemetry (devices, fleet, device, timeline, serial; all support --json; see its docs/tools.md). Only read/list/describe/filter operations. Never write to AWS.
- Shell tests (if you run any) are zsh: run via shebang or `zsh <script>`, never bash. State the interpreter in results.

## Problem

Report webhooks (Ubidots-Sensor-Hook-v1) are silently lost. The device logs them as accepted into the queue and later logs queue drained, but they never appear in Particle's event stream, the integration history, AWS, or Ubidots. Other events are sometimes lost the same way. The user considers a loss inside Particle's cloud unlikely. Working hypothesis: events sent in the first seconds after the connection comes up are lost at the device or session level, and the queue removes them because it counts them as sent without a confirmed acknowledgment.

## Evidence (Dev-14 = Boron-Dev-14, e00fce688e592afaf23ac4fb, 2026-09-25; all times SGT = UTC+8)

| Wake | Report ran | Connected | First event received | Lost |
|---|---|---|---|---|
| 06:13 (flash) | offline, then trusted | — | — | the post-reset 0 report (reset at 06:15:32 cleared the RAM queue; separate cause) |
| 14:41 | occupancy end | — | pdiag 14:41:19 | the report webhook (DATA ledger updated, no webhook) |
| 15:00:03 | offline, before connecting | 15:00:06 | pdiag 15:00:11 | both report webhooks (ConnSummary q=3, then queue drained) |
| 15:22 (reset) | none | ~15:22:23 | pdiag 15:22:27 | 1 of 4 queued events (q=4; pdiag, hibernate_wake, status arrived) |
| 15:49 (reset) | online (already connected) | 15:49:22 | pdiag 15:49:31 | report delivered (15:49:39, 200 at ingest). status possibly lost (q=3 at connect) |

Other facts:
- All publishes go through PublishQueuePosix::instance().publish(…, PRIVATE), the same queue.
- Report: q= is publish()'s return value; ConnSummary/Connect q= is queue depth.
- No [app.pubq] lines appear in serial.
- The Particle console shows no rate limiting; diagnostics showed publish.rate_limited: 0 at 14:18.
- The Particle integration history and AWS ingest agree event for event, and Ubidots plots the payload timestamp.
- Claude Code's prior finding (for context, verify independently): at 15:00 the ingest Lambda received exactly one Particle-sourced request 07:00:00–07:00:40Z (pdiag at 07:00:11.574Z); none for the two report webhooks. At 15:22 AWS stored pdiag 07:22:27.643Z, hibernate_wake 07:22:28.821Z, status 07:22:31.216Z against q=4.

### USB serial capture, 15:00 (verbatim from the user)

```
[Connected]
0002667653 [app] INFO: PowerDiag[24]: post-wake source=USB_HOST profile=UsbBench vbus=1 pg=1 soc=86.2% usbAddr=0x2 usbReg=0x3
0002667656 [app] INFO: Wake: reason=TIMER open=0 ready=1 occ=0 led=0s
0002667657 [app] INFO: LoopStage: stage=SLEEP_PREP elapsed=1124053 state=3 q=1 connMs=1127883
0002667659 [app] INFO: StateReq: Sleep->Report reason=sleep-timer-report
0002667757 [app] INFO: State: Sleep->Report
0002667758 [app] INFO: Daily boundary reached (boundary=1790319600 last=1790316808 now=1790319603 close=15) - running dailyCleanup
0002667892 [app] INFO: LedgerPayloadData: bytes=219/512 schema=2
0002668168 [app] INFO: Report: occ=0 totalMin=9 alert=19 q=1 ledger=req
0002668169 [app] INFO: Running Daily Cleanup
0002668178 [app] INFO: PowerDiag[25]: post-refreshInputProfile source=USB_HOST profile=UsbBench vbus=1 pg=1 soc=86.2% usbAddr=0x2 usbReg=0x3
0002668208 [app] INFO: ChargeDiag: chg=DONE(3) fault=0x00 vbus=USB pg=1 th=OK vsys=0 vcell=4.021 soc=86.2 src=USB_HOST prof=USB
0002668211 [app] INFO: Charge: soc=86.2 d15=-0.14 v=4.021 dv=+0.000 chg=DONE src=USB_HOST prof=USB a=1s c=0s t=0s
0002668345 [app] INFO: LedgerPayloadData: bytes=217/512 schema=2
0002668349 [app] INFO: Report: occ=0 totalMin=0 alert=19 q=1 ledger=req
0002668363 [app] INFO: StateReq: Report->Connect reason=keep alive mode
0002668365 [app] INFO: State: Report->Connect
0002668383 [app] INFO: Connect: start budget=660s heap=76656
0002671631 [app] INFO: ConnSummary: ok elapsed=2924 last=CONNECTED cellMs=0 netMs=0 cloudMs=2924 cloudRecoverStage=0 cloudRecoverCount=0 sig=77/12 heap=75712 q=3
0002671661 [app] INFO: ChargeDiag: chg=DONE(3) fault=0x00 vbus=USB pg=1 th=OK vsys=0 vcell=4.021 soc=86.2 src=USB_HOST prof=USB
0002671663 [app] INFO: PowerDiag[26]: connect-success source=USB_HOST profile=UsbBench vbus=1 pg=1 soc=86.2% usbAddr=0x2 usbReg=0x3
0002671680 [app] INFO: LedgerPayloadStatus: bytes=839/896 schema=2
0002672109 [app] INFO: LedgerPayloadData: bytes=217/512 schema=2
0002672132 [app] INFO: Connect: ok elapsed=3268ms sig=77/19 q=3 heap=75536
0002672133 [app] INFO: StateReq: Connect->Idle reason=connect-complete
0002672234 [app] INFO: State: Connect->Idle
0002672888 [app] INFO: LedgerCb: kind=DATA seq=11 globalSeq=12 found=1 age=4991 ms=2672888 upd=3613208497 sync=3613213303 countBefore=2 countAfter=1 pendingData=0 pendingStatus=1
0002673839 [app] INFO: LedgerCb: kind=STATUS seq=12 globalSeq=12 found=1 age=2142 ms=2673839 upd=3613212300 sync=3613214262 countBefore=1 countAfter=0 pendingData=0 pendingStatus=0
0002674588 [app] INFO: LedgerCallback: kind=input ledger=device-settings synced=3613214906
0002676461 [app] INFO: Low-power idle: queue drained and no updates pending - entering SLEEPING_STATE
0002676463 [app] INFO: StateReq: Idle->Sleep reason=low power idle
0002676467 [app] INFO: State: Idle->Sleep
0002676479 [net.pppncp] ERROR: PPP error event data=5
0002685372 [app] INFO: Sleep: td=8902ms cloud=24ms modem=8902ms standby=0/0 mode=IKA tier=H occ=0
0002685374 [app] INFO: TimeDiag: tz=SGT-8 valid=1 epoch=1790319621 utc=2026-09-25 07:00:21 local=2026-09-25 15:00:03 open=6 close=15 isOpen=0 trusted=1 openness=1 syncAgeMs=1461028 ~
0002685376 [app] INFO: Clamping night sleep duration to max supported 32760 seconds (requested=53997)
0002685378 [app] INFO: HibernateDiag: check enabled=1 requested=32760
0002685381 [app] INFO: HibernateDiag: pass epoch=1790319620
0002685392 [app] INFO: ModemTeardown: radioOn=0 point=hibernate
0002685393 [app] INFO: Sleep: HIBERNATE reason=closed dur=32760s wakePin=8
0002685394 [app] INFO: PowerDiag[27]: pre-hibernate source=USB_HOST profile=UsbBench vbus=1 pg=1 soc=86.2% usbAddr=0x2 usbReg=0x3
[Disconnected]
```

### USB serial capture, 15:22 (verbatim from the user)

```
[Connected]
0000003966 [ncp.client] INFO: Using internal SIM card
0000017380 [app] INFO: ConnSummary: ok elapsed=16302 last=CONNECTED cellMs=14769 netMs=0 cloudMs=1533 cloudRecoverStage=0 cloudRecoverCount=0 sig=78/19 heap=76392 q=4
0000017414 [app] INFO: PowerDiag[5]: post-refreshInputProfile source=USB_HOST profile=UsbBench vbus=1 pg=1 soc=86.2% usbAddr=0x2 usbReg=0x3
0000017438 [app] INFO: ChargeDiag: chg=DONE(3) fault=0x00 vbus=USB pg=1 th=OK vsys=0 vcell=4.021 soc=86.1 src=USB_HOST prof=USB
0000017441 [app] INFO: PowerDiag[6]: connect-success source=USB_HOST profile=UsbBench vbus=1 pg=1 soc=86.1% usbAddr=0x2 usbReg=0x3
0000017447 [app] INFO: LedgerPayloadStatus: bytes=831/896 schema=2
0000017933 [app] INFO: LedgerPayloadData: bytes=217/512 schema=2
0000018264 [app] INFO: Connect: ok elapsed=16599ms sig=78/19 q=4 heap=76488
0000018264 [app] INFO: StateReq: Connect->Idle reason=connect-complete
0000018280 [app] INFO: ClockResync: requesting sync (no confirmed sync yet this boot)
0000018379 [app] INFO: State: Connect->Idle
0000018933 [app] INFO: ClockResync: sync advanced, rtcUpdated=1 epoch=1790320945 correctionSec=0
0000018970 [app] INFO: LedgerPayloadStatus: bytes=837/896 schema=2
0000019130 [app] INFO: LedgerCb: kind=DATA seq=2 globalSeq=2 found=1 age=1192 ms=19130 upd=3614548403 sync=3614549967 countBefore=2 countAfter=1 pendingData=0 pendingStatus=1
0000024022 [app] INFO: Low-power idle: queue drained and no updates pending - entering SLEEPING_STATE
0000024022 [app] INFO: StateReq: Idle->Sleep reason=low power idle
0000024084 [app] INFO: State: Idle->Sleep
0000028975 [app] INFO: LedgerPayloadStatus: bytes=838/896 schema=2
0000037798 [app] INFO: LedgerCb: kind=STATUS seq=1 globalSeq=3 found=1 age=20336 ms=37798 upd=3614568519 sync=3614568308 countBefore=2 countAfter=1 pendingData=0 pendingStatus=0
0000039022 [app] INFO: LedgerCb: kind=STATUS seq=3 globalSeq=3 found=1 age=1544 ms=39022 upd=3614568519 sync=3614569869 countBefore=1 countAfter=0 pendingData=0 pendingStatus=0
0000039029 [app] INFO: GateRelease: wait=14943 reason=ledger
0000039030 [app] INFO: Gate: ok wait=14943
0000039042 [net.pppncp] ERROR: PPP error event data=5
0000060886 [app] INFO: Sleep: td=21854ms cloud=16ms modem=21854ms standby=0/0 mode=IKA tier=H occ=0
0000060888 [app] WARN: MODEM_HEALTH: unstable reason=slow_teardown elapsed=21854
0000060889 [app] WARN: MODEM_POLICY: standby temporarily disabled reason=unstable_modem
0000060893 [app] INFO: TimeDiag: tz=SGT-8 valid=1 epoch=1790320986 utc=2026-09-25 07:23:06 local=2026-09-25 15:23:00 open=6 close=15 isOpen=0 trusted=1 openness=1 syncAgeMs=41973 la~
0000060898 [app] INFO: Clamping night sleep duration to max supported 32760 seconds (requested=52620)
0000060902 [app] INFO: HibernateDiag: check enabled=1 requested=32760
0000060909 [app] INFO: HibernateDiag: pass epoch=1790320986
0000060941 [app] INFO: ModemTeardown: radioOn=0 point=hibernate
0000060942 [app] INFO: Sleep: HIBERNATE reason=closed dur=32760s wakePin=8
0000060943 [app] INFO: PowerDiag[7]: pre-hibernate source=USB_HOST profile=UsbBench vbus=1 pg=1 soc=86.1% usbAddr=0x2 usbReg=0x3
[Disconnected]
```

## Investigate

1. Acknowledgment semantics. What flags do report publishes actually carry (PRIVATE only?), and what's the effective default: WITH_ACK or NO_ACK, in Device OS 6.4.1 and in PublishQueuePosix? Does the library expose a WITH_ACK option? How does it decide an event is sent: the publish call returning, the future resolving, or a cloud ACK? Quote the exact lines.
2. Removal paths. Quote every path in PublishQueuePosix that removes an event (success, failure, retry limit, corrupt file, the 800-event cap). Can an event be removed after a publish that failed or was never acknowledged?
3. Timing after connecting. When does the queue start sending after Particle.connected() goes true? Is there any delay or readiness check? What does Device OS 6.4.1 do with publishes issued immediately after a session resume (compare the logs' "Failed to load session data" and resumed-session cases)? Note that setPausePublishing() exists and is unused.
4. Pattern check. For the five wakes above, reconstruct the send order and timing of each queued event from the code plus logs, and test whether "sent within N seconds of connecting = lost" explains every case, including the counterexamples (14:36, a flash-queued event delivered 5 minutes late; and 06:18:24, queued during 2.7 min of DNS failures and delivered).
5. Fleet scope (data, read-only). For every device over the last 14 days, match reports (Report: lines in the serial forwarder, a lower bound since it drops lines, plus the ledger and status timestamps where useful) against Ubidots-Sensor-Hook-v1 deliveries in S3. Give the loss rate per device, per firmware version, and per day, and flag whether losses cluster at the start of a connection. Include production devices.
6. Library logging. Why don't the library's log lines appear, and what change enables them?

## Deliverables (write to REPORT-webhook-loss-investigation.md in your working directory)

- Root cause, with file and line evidence, and a confidence level. Say plainly if it's unproven.
- Scope table from step 5.
- Two bench-build diffs, drafted and not applied, each with a distinct version string (FIRMWARE_VERSION in src/Version.cpp), saved as separate files in your working directory:
  (A) diagnostics only: library logging at trace level, plus one log line per send attempt (event name, flags, attempt result, ACK result, milliseconds since connect).
  (B) candidate fix: explicit WITH_ACK on all queue publishes, and/or setPausePublishing() held until N seconds after Particle.connected(). State which one you recommend and why.
- A bench protocol that reproduces the 15:00 case (report while offline, then a connection) and passes only if every queued event is delivered, matched one for one against the integration history.
- The model and reasoning level actually used.

Do not commit, push, stash, reset, or check out anything. Do not write to the repository or to AWS.

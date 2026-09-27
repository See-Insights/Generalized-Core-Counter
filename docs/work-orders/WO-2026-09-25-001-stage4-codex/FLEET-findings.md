# Fleet scope findings

Read-only extraction window: **2026-09-11 08:00:00 UTC ≤ event time < 2026-09-25 08:00:00 UTC**, exactly 14 days (2026-09-11 16:00 to 2026-09-25 16:00 SGT). Snapshot inventory was fetched at approximately 08:01 UTC Sep 25. Product 42131 inventory and project current state both contain the same 12 devices, including all nine production-named devices. Daily rows are UTC; first and last rows are partial days.

**Observed result:** 5,992 device Ubidots deliveries, 115 captured `Report:` lines, 105 matched captured reports, and 10 unmatched captured reports. This is an **8.7% unmatched fraction in a sparse, biased serial cohort**, not a measured fleet loss rate. True per-device/per-firmware/per-day loss rates cannot be determined from this archive because generated-but-undelivered reports without serial capture leave no complete historical denominator. All nine production devices have zero serial records. The 41 captured Dev-14 reports all match deliveries, despite independently established Sep 25 Dev-14 losses outside forwarder coverage. Thus even 0% here emphatically does not mean loss-free.

## Per device

| Device | Event firmware | S3 delivered | Serial rows | Report lines | Matched | Unmatched | Cohort unmatched |
|---|---:|---:|---:|---:|---:|---:|---:|
| Boron-Dev-11 | 24 | 277 | 14467 | 27 | 24 | 3 | 11.1% |
| Boron-Dev-14 | 24 | 574 | 6563 | 41 | 41 | 0 | 0.0% |
| Boron-Dev-09 | 24 | 387 | 11839 | 47 | 40 | 7 | 14.9% |
| ToM-MCP-PCKL1-JAN14-2025 | 21 | 475 | 0 | 0 | 0 | 0 | N/A |
| ToM-MCP-PCKL3 | 21 | 667 | 0 | 0 | 0 | 0 | N/A |
| Morrisville-Tennis-MAFC-1-SWAPPED | 21 | 500 | 0 | 0 | 0 | 0 | N/A |
| Morrisville-Tennis-MAFC-2 | 21 | 466 | 0 | 0 | 0 | 0 | N/A |
| ToM-MCP-Court2 | 21 | 721 | 0 | 0 | 0 | 0 | N/A |
| ToM-MCP-Court1 | 21 | 630 | 0 | 0 | 0 | 0 | N/A |
| ToM-MCP-Court3-DEC28-2024 | 21 | 439 | 0 | 0 | 0 | 0 | N/A |
| SAMIT-TRAIL02 | 24 | 263 | 0 | 0 | 0 | 0 | N/A |
| ToM-MCP-PCKL2 | 21 | 593 | 0 | 0 | 0 | 0 | N/A |

Production-named devices use current build `21.0_Test` except SAMIT-TRAIL02, which uses `v24-Thermal-Inhibit`. Dev09/11/14 currently report `v24-Thermal-Inhibit`. Event `fw_version` is numeric 21/24; semantic version strings from the latest ledger do not establish a historical build identity for every event (several changes can reuse version 24). All currently report Device OS 6.4.1. No firmware/version serial lines survived this window.

## Per firmware

| Event fw_version | S3 delivered | Report lines | Matched | Unmatched | Cohort unmatched | Actual loss rate |
|---|---:|---:|---:|---:|---:|---|
| 21 | 4491 | 0 | 0 | 0 | N/A | Not identifiable |
| 24 | 1501 | 115 | 105 | 10 | 8.7% | Not identifiable |

## Per UTC day

| UTC day | S3 delivered | Report lines | Matched | Unmatched | Cohort unmatched |
|---|---:|---:|---:|---:|---:|
| 2026-09-11 (partial) | 288 | 2 | 2 | 0 | 0.0% |
| 2026-09-12 | 402 | 3 | 3 | 0 | 0.0% |
| 2026-09-13 | 505 | 5 | 5 | 0 | 0.0% |
| 2026-09-14 | 435 | 15 | 15 | 0 | 0.0% |
| 2026-09-15 | 413 | 3 | 3 | 0 | 0.0% |
| 2026-09-16 | 427 | 8 | 8 | 0 | 0.0% |
| 2026-09-17 | 426 | 8 | 8 | 0 | 0.0% |
| 2026-09-18 | 448 | 15 | 13 | 2 | 13.3% |
| 2026-09-19 | 504 | 10 | 10 | 0 | 0.0% |
| 2026-09-20 | 441 | 6 | 3 | 3 | 50.0% |
| 2026-09-21 | 438 | 10 | 9 | 1 | 10.0% |
| 2026-09-22 | 346 | 8 | 7 | 1 | 12.5% |
| 2026-09-23 | 354 | 6 | 4 | 2 | 33.3% |
| 2026-09-24 | 457 | 10 | 9 | 1 | 10.0% |
| 2026-09-25 (partial) | 108 | 6 | 6 | 0 | 0.0% |

The delivered column is grouped by Particle published time, and serial cohort counts by forwarder capture time. These columns are not a subtraction-based loss calculation: a report can be delivered on a later day. Full device × firmware × day counts are in `fleet-data/scope-by-device-firmware-day.csv`.

## Connection association and unmatched reports

Seven of the ten unmatched reports were logged **2.173–4.057 seconds before `ConnSummary: ok` by device uptime**, with queue depth **2** at that connection. Each of those seven connections has a received `pdiag` but no matching report. This supports a connection/sleep-boundary problem extending to Dev09 and Dev11. It does not identify the actual transmit time or prove the first N seconds are universally unsafe: **32 matching reports were also generated ≤10 seconds before a q=2 connection**, and 60 of 67 observed reports generated ≤10 seconds before any recorded connection delivered. In the broader comparison: preconnect ≤10 s has 7/67 unmatched; other nearby-connect cases have 0/39; cases without a connect line within 15 min have 3/9. Serial gaps and delayed logs limit these comparisons.

| Device | Forwarder report time UTC | Unmatched evidence / connection |
|---|---|---|
| Boron-Dev-11 | 2026-09-18T05:21:59.041952+00:00 | q=2; report 2.432 s before connect; pdiag received |
| Boron-Dev-11 | 2026-09-18T10:00:20.106194+00:00 | q=2; report 2.173 s before connect; pdiag received |
| Boron-Dev-09 | 2026-09-20T07:35:14.568083+00:00 | q=2; report 2.424 s before connect; pdiag received |
| Boron-Dev-11 | 2026-09-20T08:17:39.306281+00:00 | q=67 before report; prolonged connection outage, next recorded successful connect next day after a boot/uptime reset |
| Boron-Dev-09 | 2026-09-20T11:00:05.589006+00:00 | ConnSummary omitted by forwarder; Report→Connect transition, pdiag received about 4 s after report capture |
| Boron-Dev-09 | 2026-09-21T05:57:04.055544+00:00 | q=2; report 2.608 s before connect; pdiag received |
| Boron-Dev-09 | 2026-09-22T12:00:06.473657+00:00 | q=2; report 2.768 s before connect; pdiag received |
| Boron-Dev-09 | 2026-09-23T02:35:36.048024+00:00 | q=2; report 3.005 s before connect; pdiag received |
| Boron-Dev-09 | 2026-09-23T02:55:55.387112+00:00 | q=2; report 4.057 s before connect; pdiag received |
| Boron-Dev-09 | 2026-09-24T08:17:43.117784+00:00 | q=494 before report; DNS failures; q=497 on first recorded connect at 09:02; later queue drains |

The two long-backlog cases are not evidence of loss during the first few seconds of a fresh connection. A reset is a separate plausible cause in the Dev11 outage case. The Dev09 backlog had another captured report eventually delivered roughly six hours after generation; delayed delivery was therefore searched by payload time, not a short arrival window. Exact serial/S3 references and neighboring events are retained in `report-matches.json` and `unmatched-report-context.txt`.

## Matching and coverage

1. Read CloudFormation outputs, current-state inventory, and Product inventory through the telemetry CLI. Direct AWS CLI DynamoDB Query auto-pagination retrieved all **47,106** records over the exact interval for all 12 devices (no `--limit`/`--max-items`). Of those, **32,869** were forwarder records (this includes collector/path rows labeled serial); only **115** contain `Report: occ=...`. All 115 have `q=1`.
2. Listed all 15 intersecting UTC S3 `Ubidots-Sensor-Hook-v1` prefixes with automatic pagination. There are **6,002** objects inside the interval, including **10** under `deviceId=api`; those are excluded as manual/API publishes. The remaining **5,992** object keys exactly equal the 5,992 device-report S3 keys in DynamoDB: no missing keys on either side. All 6,002 bodies were fetched successfully, including the 10 excluded objects. No added device IDs appeared in S3.
3. Matched captured report to a unique device delivery using numeric occupancy, dailyoccupancy/totalMin, alerts, and embedded payload timestamp. Payload time is matched against capture time within 60 seconds; one-to-one nearest matching avoids reusing an event. All 105 selected matches have payload times **2.029–36.803 seconds before** capture. One initial symmetric-window candidate ambiguity was a later scheduled report with identical values 35 seconds after capture; the selected preceding event is unambiguous when requiring payload ≤ capture + 2 seconds. There are no duplicate selected event IDs/files. This is a correlation, not an application-provided unique report ID.
4. Searched deliveries throughout the complete 14-day period, so queuing delays do not cause false gaps simply because arrival is late. Five matches arrived >10 min after the log, including about 61 min and six hours. Future deliveries after the cutoff, old untrusted device time, same-valued hidden reports, reset loss, and archival coverage still prevent treating every unmatched observation as a proven device/session loss.
5. The current device-status ledger provides only a latest `reporting.lastReportEpoch`, not a historical count. Eleven of the twelve latest ledger report times have a delivery with payload time within 2 seconds; Dev14 has `lastReportEpoch=1790319603` (07:00:03 UTC) with none, corroborating the supplied 15:00 capture. Court1 differs by one second and is matched, not counted missing. The fleet reports `deviceData` history/projection coverage zero. `lastApplicationReportAt` is not used as an independent report denominator because the projection can advance on other event types.
6. Semantics and source boundaries: the figures count receipt at the AWS Particle webhook ingest/S3 archive, not separate confirmations from Ubidots or Particle integration history for all 14 days. Fleet integration-history matching is unavailable here. Missing S3 events alone do not identify where transport failed.

## Artifacts, read-only discipline, and execution

`scope-by-device.csv`, `scope-by-firmware.csv`, `scope-by-day.csv`, and `scope-by-device-firmware-day.csv` are final machine-readable scope tables. `report-matches.json/.csv` preserve report text, S3 source key, match file, and connection timing. `timeline-all-normalized.json` and each `timeline-<id>.json` preserve the full query data; `s3-ubidots-<date>.json` preserve complete object inventories; `raw-ubidots/` contains all downloaded object bodies; `ledger-spotchecks.json` records the latest-ledger comparisons.

All shell commands used **zsh**, with Python 3 and Node.js for local parsing/read-only clients. No shell tests, mutations, configuration changes, infrastructure writes, deploys, or repository edits were performed. Node AWS SDK direct SSO loading saw an expired old token; existing working AWS CLI credentials were read into process memory via `aws configure export-credentials` without logging credentials or changing settings; all subsequent downloads succeeded. Current-state snapshots have credential-like and needless identity fields removed. No tokens/secrets remain in the retained inventory artifacts. Model/reasoning inherited from parent: GPT-6 Astra, ultra.

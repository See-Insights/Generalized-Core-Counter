AGENT: Codex · MODEL: gpt-6-astra · REASONING: high
AUTHORIZATION SCOPE: read-only code, git history, the local copy of the AWS event archive, forwarded serial logs in that copy, the local Device OS 6.4.1 source, and the saved Particle documentation pages / Not authorized: edits to tracked files, commits, builds, AWS or any network access, device actions. Write only your report (the `-o` file) and, if you need them, scratch files under `build-tmp/`.

# Connectivity investigation (Codex, read-only), 2026-10-01

Per AI_DEVELOPMENT_WORKFLOW.md: Roles table (Codex: investigation and verification), §5 model routing, and §12 guardrails 1 (check the history first: `git log -S` before proposing anything new) and 3 (each proposal carries a size estimate).

**Goal, in plain language:** stop spending battery and radio time on connections that can't succeed or don't finish their job, and recover quickly when the modem is stuck, without working against Device OS's own connection handling.

**Repository:** `Generalized-Core-Counter`, branch `wo/2026-09-29-002-config-cleanup` at `9e10778` (v30-LedOffAtNight). Fleet devices run v28 (`6e7a3e1`, main) or v30. Treat code claims as true for both unless the diff between them says otherwise.

## Where the data is (your sandbox has no network)

- **Event archive, 14 Sep – 1 Oct 2026 (UTC days):** `build-tmp/connectivity-archive/<UTC day>/<event>/<device id>/<published_at>.json`. Events: `Ubidots-Sensor-Hook-v1` (reports), `status`, `watchdog`, `hibernate_wake`, `pdiag`, `serialLog`. In each file, `particle.data` is HTML-escaped JSON. A report's `timestamp` is epoch ms of the report's stamp; `connecttime` is seconds; `resets` is the reset count.
- **Serial logs** (`serialLog`, forwarded over USB from a bench Pi): only Dev-09, Dev-14 and Dev-11. Each file is one line: `particle.timestamp` is UTC receive time, `particle.logLine` is the device line (its first field is device millis), `particle.eventType` is `LOG` or a forwarder state (`SERIAL_CONNECTED`, `SERIAL_MISSING`, ...). The port drops while the device sleeps, so serial covers awake time only and often starts a few seconds after the wake. US devices have no serial: use events only.
- **Device OS 6.4.1 source:** `~/.particle/toolchains/deviceOS/6.4.1/`.
- **Particle documentation**, saved 2026-10-01 as text with the source URL on the first line: `build-tmp/connectivity-archive/particle-docs/`. These are the cellular connect guidance, `System.updatesPending()`, the system events reference, the hardware watchdog, and sleep. Cite the URL in the file's first line. If you rely on any other Particle page from memory, list it under "unverified citations" so it can be checked.

### Devices

| Name | Device ID | Region / zone | Notes |
|---|---|---|---|
| Dev-09 | e00fce68399ee6244a963935 | SG, SGT | bench, serial |
| Dev-14 (boron-soak-1) | e00fce688e592afaf23ac4fb | SG, SGT | bench, serial |
| Dev-11 | e00fce683f6063bf254283dd | SG, SGT | retired 29 Sep (RTC drift, hardware). Don't use for conclusions. |
| ToM-MCP-Court1 | e00fce68c9f3e64f79ccc884 | US, EST5EDT | |
| ToM-MCP-Court2 | e00fce686c3f94ad871d6479 | US, EST5EDT | |
| ToM-MCP-Court3 | e00fce686e1a157c27984295 | US, EST5EDT | rapid PIR wakes (WO-2026-09-21-003) |
| ToM-MCP-PCKL1 | e00fce6865fe7770b80a014e | US, EST5EDT | |
| ToM-MCP-PCKL2 | e00fce68b015d86f790c924e | US, EST5EDT | |
| ToM-MCP-PCKL3 | e00fce6853367e34da7e905c | US, EST5EDT | |
| Morrisville MAFC-1 | e00fce686548d46c4b45e380 | US, EST5EDT | |
| Morrisville MAFC-2 | e00fce6841443bcc0f3178e4 | US, EST5EDT | |
| SAMIT-TRAIL02 | e00fce687bbfcdc64e7b5f50 | US, EST5EDT | closes at 23:00 |

Other device IDs in the archive are fleet devices outside the soak. Use them for US statistics if their events are informative, and say so.

## Data rules, applying to every item

- **Region.** Dev-09 and Dev-14 are BRN404X (North American LTE-M) Borons on Singtel. A Muon M524 at the same location connects in seconds. Report Singapore and US evidence separately. Fleet conclusions rest on US devices; mark any finding supported only by Singapore data.
- **Breadcrumb 18 has two different causes. Always filter on stage:**
  - `stage=connectivity` (with bc=18) = the connection stall, WO-2026-09-25-005. In scope for item 4.
  - `stage=diag` (with bc=18) = the rapid-wake backlog problem, WO-2026-09-21-003. For example, Court3's three watchdog resets on 30 Sep with 152–421 events queued. Out of scope. Count them separately and don't merge them into any connection-stall figure.
- **Citations.** Cite file:line for every code claim, and the device, UTC time and archive file for every data claim.
- **Premises.** Anything in this dispatch marked as a question is open. Don't treat a suggested candidate as established; rule it in or out with evidence.

## 1. Attempts with no signal (sig=0/0)

Example: Dev-14 on 29 Sep, `ConnSummary: fail elapsed=660002 last=CELLULAR_ACQUIRE … sig=0/0`.

**History first:** this is already filed as `docs/work-orders/WO-2026-09-15-002-cellular-acquire-registration-stall-no-escalation.md` (drafted 15 Sep, never dispatched). Read it first and build on it: the staged cloud recovery never runs during `CELLULAR_ACQUIRE`, and its Dev-09 log shows `WARN: Resetting the modem due to the network registration timeout` about 10 minutes into a 660 s attempt.

(a) Which signal, registration and modem-state values does Device OS 6.4.1 expose (signal strength and quality, registration status, `Cellular.ready()`, modem power state)? How soon after the modem powers on does each become meaningful?

(b) From the logs, for each failed attempt: when did signal first read 0/0, how long did it stay there, and did any attempt that read 0/0 for N minutes still succeed later? Find the N past which success never happened, separately for the US and Singapore.

(c) Options and their trade-offs: give up early and sleep; reset the modem once, then give up; or leave things as they are. Particle's connect guidance (saved page) says Device OS fully power-cycles the modem after 10 minutes of failing, and recommends at least 11 minutes, or a variable backoff that still does the full 11 minutes periodically. Confirm the 10-minute reset in the Device OS source and its exact timing. Say whether any option duplicates or pre-empts what Device OS already does. Don't recommend anything that contradicts the guidance.

## 2. Connection budgets

`ConnectivityPolicy.h:62–63` defines two connection budgets:

- **660 s, `CONNECT_BUDGET_DEEP_MS`:** used when the device has made at least 3 connection attempts, or its charge is above 50% (`State_Connect.cpp:118–123`; thresholds at `ConnectivityPolicy.h:80–81`). This is the `Connect: start budget=660s` in the Dev-09 and Dev-14 logs, including Dev-14's sig=0/0 failure on 29 Sep.
- **300 s, `CONNECT_BUDGET_DEFAULT_MS`:** used when charge is ≤ 50% and there have been fewer than 3 attempts. No `budget=300s` line has been seen in Dev-14's archived serial logs for 22–30 Sep. Find out whether any device has actually used it.

900 s is only the configuration ceiling: `CONNECT_BUDGET_CONFIG_MAX_SEC = 900` (`ConnectivityPolicy.h:65`), checked in `ConfigApply.cpp:305` and `State_Idle.cpp:307`.

(a) Confirm the selection logic above, and list any other place a connection budget is chosen (mode, recovery stage, configuration override). Keep two things separate:
- **A different 300:** `State_Idle.cpp:307–308` falls back to 300 when the configured value is out of range, but that limits how long the device stays idle while connected. It isn't a connection budget. Don't mix the two.
- **The minimum is inconsistent:** `CONNECT_BUDGET_CONFIG_MIN_SEC = 120` (`ConnectivityPolicy.h:64`, commented "Particle docs minimum"), but `ConfigApply.cpp:305` and `State_Idle.cpp` accept anything from 30. Say which minimum is actually enforced, and where.

(b) With `git log -S`: when were 660, 300, 900, 120 and 30 introduced, and why?

(c) Does any configuration or device produce a connection budget above 660 in practice? Do any devices have a configured override?

(d) Compare with Particle's documented guidance on connection timeouts and minimum registration time (saved page), and recommend one budget policy, including whether the "below 50% charge" rule is worth keeping.

## 3. Slow modem teardown

Teardowns of 7–22 s, `[net.pppncp] ERROR: PPP error event data=5`, then `MODEM_HEALTH: unstable reason=slow_teardown` → `MODEM_POLICY: standby temporarily disabled reason=unstable_modem`.

(a) How often, by device, region, and the state the device was going into (ULP sleep or hibernate).
(b) Does it ever cause harm (lost events, a failed sleep, a reset, a missed wake), or does it only cost time? Quantify the time.
(c) What triggers it: the sequence of calls in teardown, whether a cloud session or PDP context is still active, or a network-initiated detach. Use the code and Device OS behavior.
(d) The "standby disabled" response: what does it change (time to the next connection, power drawn during sleep), and is it worth that cost?
(e) Conclusion: address it or leave it, and why.

## 4. The connection stall (WO-2026-09-25-005), and how each stall ends

(a) List every occurrence of breadcrumb 18 in the connectivity stage (device, UTC time, firmware, events queued, connection age). For each, did it follow a phase from items 1–3: no signal, a slow teardown, or a particular budget or recovery stage? Correlate only; don't try to solve WO-2026-09-25-005 here.

(b) For every stall, identify the recovery mechanism that ended it and explain why it took as long as it did. Include stalls that ended in an application reset (`resetReason=140`) with no watchdog event; those have no stage field, so classify them from the surrounding evidence.

Case to include: **PCKL1 (e00fce6865fe7770b80a014e), 1 Oct 02:00–03:02Z, v28 (US).**
- The closing report for 22:00 EDT was made: stamped 21:59:59 EDT with a daily count of 394, delivered at 03:01:57Z.
- The device was silent for about an hour. Particle's last-heard time stayed at 01:53:55Z.
- The status event at 03:01:59Z shows `resetReason=140`, `appBreadcrumb=18` at uptime 57,581,700 ms, `failsafeStage=0`, `alert=0`. No watchdog event was published.
- Its next report was stamped 23:01:46 EDT with a count of 0 and `rs=1`.

Settle these:
- **Which code issued the reason-140 reset?** On Boron `Wiring_Watchdog=1` (`HAL_PLATFORM_HW_WATCHDOG`, Device OS 6.4.1). So the `ApplicationWatchdog` handler at `Generalized-Core-Counter.cpp:1813` is not compiled in, and a hardware-watchdog reset would report 60.
  - Rule each candidate in or out with evidence: connectivity failsafe stage 2 (`:2632`, which sets `BREADCRUMB_CONNECTIVITY_FAILSAFE_HARD` first), ThrashGuard tier 3 (`ThrashGuard.cpp:151`, which raises alert 18), any `System.reset()` in a library or in Device OS, and a reset requested from the cloud.
  - Check whether `failsafeStage` and `alert` are still meaningful in the first status after a reset, or whether they're cleared at boot.
- **Is breadcrumb 18 a stall location or a loop that was still running?** Breadcrumb 18 is set at `:1650` on every loop pass, so it can't tell those apart.
- **Why did recovery take about an hour?** The AB1805 is set to `WATCHDOG_MAX_SECONDS` (124 s, `:1266`) and petted by `ab1805.loop()` (`:1629`) on every pass. Was the loop still running? If so, what state was the device in? Was it repeatedly retrying a connection under the 660 s budget?
- **What bounds a stall like this today, and what should?** An hour stuck connecting is exactly the wasted battery this investigation is about.

Also check whether MAFC-1 (e00fce686548d46c4b45e380, silent since 23:01Z 30 Sep) and Court3 (e00fce686e1a157c27984295, silent since 21:59Z 30 Sep) are cases of the same stall. If their events after the silence aren't in the archive copy, say so and list what to look for once they report.

## 5. Downloads cut short by sleep

On 30 Sep from 21:43 SGT, Dev-09 went to sleep partway through its v30 download about five times. Each time it logged `Low-power idle: no updates pending …` just after `comm.ota … Starting firmware update`, waited about 70 s for its queue, then slept and resumed at the next wake. Meanwhile alert 40 fired, unsent reports grew from 1 to 5, and its closing report was delivered 8 hours late (at 06:01).

(a) The message `Low-power idle: no updates pending …` is logged at `State_Idle.cpp:250`. The pre-sleep checks are `System.updatesPending()` at `State_Sleep.cpp:490` and `Generalized-Core-Counter.cpp:2523`. Settle the Device OS question: does `System.updatesPending()` return true while a download is already in progress? Cite Device OS documentation (saved pages) or source. If it doesn't, name the API or system event that does show a download in progress (for example the `firmware_update` system events), and how the existing checks could use it.

**History first: the state already exists.** `FIRMWARE_UPDATE_STATE` (`StateMachine.h:21`, handler `State_Connect.cpp:722–777`, introduced around v3.07–v3.11 in Dec 2025 / Jan 2026: use `git log -S FIRMWARE_UPDATE_STATE`). It is entered only when `System.updatesPending()` is true (`State_Connect.cpp:651–652`). It leaves when `updatesPending()` turns false (`:752`; the comment at `:751` says "and no OTA in progress", but nothing checks for that), on the user button, or after `FIRMWARE_UPDATE_MAX_MS` = 5 min (`ConnectivityPolicy.h:94`). No code has ever subscribed to the `firmware_update` system event (`git log -S firmware_update` finds nothing). The firmware never calls `System.disableUpdates()`. Dev-09's serial nevertheless logs `Entering FIRMWARE_UPDATE_STATE` on 30 Sep (for example `build-tmp/connectivity-archive/2026-09-30/serialLog/e00fce68399ee6244a963935/2026-09-30T12-35-47-482768+00-00.json`).

Device OS 6.4.1 `system/inc/system_event.h:52, 77–80`: event `firmware_update` (`1<<8`) with param `firmware_update_begin` = 0, `firmware_update_progress` = 2, `firmware_update_complete` = 1, `firmware_update_failed` = −1. Docs: https://docs.particle.io/reference/device-os/api/system-events/system-events-overview/ (Chip's pointer; the reference table is saved locally).

(d) Should a download in progress drive the existing `FIRMWARE_UPDATE_STATE`? Answer:
- What actually triggered the `Entering FIRMWARE_UPDATE_STATE` lines on Dev-09 on 30 Sep, given that `updatesPending()` is documented for updates held while disabled.
- Whether entering on `firmware_update_begin` (and staying through `progress`, leaving on `complete` / `failed`) is enough to stop the device sleeping mid-download, using the state that exists rather than a new one.
- What the 5-minute cap should become for a download, using the observed transfer times (Dev-14 completed 194 chunks in one session on 30 Sep 13:50–13:51Z; Dev-09 needed about five sessions).
- What happens to queued reports and the webhook ack (alert 40) while the transfer runs.
- What happens if a transfer stalls: the state needs an exit, but it shouldn't end a healthy download.
- The size in lines. This is expected to be small; say if it isn't.

(b) Does this happen on US devices during OTA rollouts? Check the v25 → v28 → v30 rollouts (27 Sep – 1 Oct): look for updates spread across several connections, for the device sleeping mid-download, and for resets during an update. Include MAFC-1's v25 watchdog reset with 200 events queued during its v28 update (about 06:01 EDT on 30 Sep: which stage was it?), and Court3's v25 → v29 → v30 updates on 30 Sep (apply the stage filter: its stage=diag resets belong to WO-2026-09-21-003).

(c) The cost: for each affected device, the number of download attempts, the extra connection time, and how long reports were held back.

## 6. Battery cost summary

Per device per day, the time spent awake in connections that failed (items 1–2), took longer than needed (item 3), were stuck (item 4), or were cut short (item 5). US and Singapore separately. Say how each figure was derived, and where it's a lower bound because serial isn't available.

## Output

- **Findings:** one page, one short paragraph per item, each with its evidence.
- **Ranked proposals,** each with: its plain goal, the change, its estimated size in lines (`src/` + `lib/`, and tests), its risk, whether it follows or departs from Particle's guidance (cited), and the data it rests on. The OTA-in-progress fix (item 5) is expected to be small; say if it isn't. Prefer restoring or extending existing mechanisms over new ones (guardrail 1).
- **What's still unknown,** with the specific capture or experiment that would settle it.
- **Unverified citations,** if any.
- **The model and reasoning level actually used.** No code.

DISPATCH: Codex, Stage 4 independent investigation (AI_DEVELOPMENT_WORKFLOW.md §3 Stage 4). Model gpt-5.6-sol, reasoning high, 60-minute box.
AUTHORIZATION SCOPE: READ-ONLY. No code changes, no commits, no network. Sandbox read-only; do not edit any file in this worktree.
UNDER REVIEW: integration/v50-ab1805 @ 1d1d5af7a22dc47094a4c23b515b348bd8ae153d
(This worktree is checked out at that SHA. Quote the SHA in the first line of your report; otherwise the report is invalid.)

# Chip-watchdog resets about 124 s after an ULP wake on v50.1 (5001)

Requested by the architect (16 Nov 2026). Chip confirmed he did not touch Dev-09 at the times below.

## What was observed (Dev-09, product 5001, 15 Nov; evidence read from the S3 archive and the serial log)

All three resets are reset reason 20 / data 0, alert 19, `watchdogSource: inferred` (the firmware cannot tell an AB1805 watchdog reset from any other PIN_RESET without power-off evidence: `Generalized-Core-Counter.cpp:760-766`).

| Reset | Boot (Z) | Prior session uptime | Wake to reset | Watchdog event of the prior session (stage / state / bc / connAge) |
|---|---|---|---|---|
| #52 | 11:02:11 | 1785.7 s | about 128 s after an ULP wake (a 691 s sleep) | connectivity / 4 CONNECTING / 18 / 827508 ms; serial shows CloudRecover stage 2 `network-reset` at 120 s |
| #53 | 12:04:27 | 3735.4 s | about 125 s after an ULP wake (3600 s sleep) | diag / 4 CONNECTING / 18 / 0 (cloud not yet connected) |
| #54 | 13:06:32 | 3726.8 s | about 120 s after an ULP wake (3600 s sleep) | sleep / 3 SLEEPING / 18 / 15705 ms (cloud had connected 15.7 s earlier; queue drained; reset at sleep entry) |

Breadcrumb 18 is `BREADCRUMB_PUBLISH_QUEUE_EXIT` (`src/observability/LoopForensics.h:29`): the loop was still passing the end of `loop()` right before each reset. The estimated wake time has about +/-2 s of error (serial `TimeDiag` truncates to seconds). After the hibernate wake at 22:00:48Z all awake windows were under 124 s (cellular connect about 15 s) and there was no reset for over 2 h of uptime.
Boot-time arithmetic: boot = `TimeDiag utc` minus uptime. ULP wake = boot + sleep start + duration.

Raw evidence (read it; do not rely only on this table):
- Dev-09 serial snapshot: `/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/build-tmp/bench/Dev-09_night1_snapshot.log` (548 lines; five segments separated by reboots).
- Dev-09 archive events (status, watchdog, hibernate_wake) for 15 Nov: `/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/1098a306-e8d7-47cd-8131-01208474cc08/scratchpad/dev09/` (`status/`, `watchdog/`, `hibernate_wake/`; the `data` field is HTML-escaped JSON).

## Relevant code (all at the SHA above; re-cite every line yourself)

- Timeout 124 s and init/start at boot: `Generalized-Core-Counter.cpp:299-300`. Only refresh: `:1133` (loop()).
- Stop before ULP: `src/state/State_Sleep.cpp:773`; start after wake: `:914`. Hibernate stop `:706`. Boot-storm holdoff stop/start `Generalized-Core-Counter.cpp:397-399`.
- `src/power/ConnectivityFailsafe.cpp:135-144`: comment at `:139`, "Device OS's modem shutdown (up to 120 s) runs before the HAL disables the chip watchdog".
- Device OS 6.6.0 source (read-only, no network): `/Users/chipmc/.particle/toolchains/deviceOS/6.6.0` (`hal/shared/watchdog_hal.cpp`, `hal/shared/am18x5.cpp`, `hal/shared/exrtc_hal*`, `wiring/src/spark_wiring_watchdog.cpp`, `hal/inc/watchdog_*.h`). The AM18x5 watchdog is `ExternalWatchdog` (a second watchdog instance; Device OS does not refresh it, per the comment at `Generalized-Core-Counter.cpp:1133`).
- The AWAKE (Device OS) watchdog is a different instance: 60 s (`Generalized-Core-Counter.cpp:159`); it would give reset reason 60, not 20.

## Questions (answer each; mark every statement as VERIFIED (code or data read) or INFERRED)

1. **Does `ExternalWatchdog.refresh()` actually reset the AM18x5's watchdog counter?** Trace it to the register or pin write (Device OS 6.6.0 source). Is it rate-limited, gated on a flag or state (started/stopped, instance flags), or a no-op on some path (for example after `stop()` then `start()`, or after a sleep)? Is it called on **every** loop pass, including during CONNECTING, CloudRecover and the publish-queue drain, or can `loop()` skip `:1133` (early return, a long blocking call inside a state handler, a `delay`, a lock)? Compare how the 124 s timeout is configured at `:299` and re-applied at `State_Sleep.cpp:914` (does `start()` reload the configured timeout? does it need `init()` again after `stop()`?).
2. **Explain the timing of #52-#54.** Each reset came 120-128 s after the watchdog was restarted at wake, with the loop running (bc 18). Does that prove refresh is ineffective, or is there another explanation (for example the chip counting from a different event, a stop that does not stop, the 124 s window starting at the sleep-entry stop rather than the wake start, a second source such as a Device OS modem shutdown before the HAL disables the chip watchdog)? Treat the `ConnectivityFailsafe.cpp:139` note explicitly: up to 120 s of modem shutdown inside `System.sleep()` before the chip watchdog is disabled. That is a candidate for #54, which reset at sleep entry. Say which of #52, #53 and #54 each explanation can and cannot cover. Rank the explanations and say what evidence would separate them.
3. **The smallest fix**, with an estimate of `src/` lines, and a **host test that would have caught it** (for example: a refresh during a wake longer than 124 s keeps the chip fed; or a modem-shutdown-before-disable ordering test). Check the existing host tests under `tests/` for the pattern to follow and for what already covers `ExternalWatchdog`; say whether the real function can be exercised on the host, or only a mirror.
4. **A one-step bench test** that proves the fix: keep 5001 awake for more than 124 s after a wake, with serial attached, and log each refresh (what to log and where; the smallest instrumentation).

Also report anything in the Dev-09 evidence that contradicts a reading above. Do not propose changes beyond these questions.

## Boundaries
- Read-only. Do not edit, build, flash, commit or push. No network (the sandbox has none).
- The 60-minute box starts when you begin. If the box runs out, return what you have and say what is unanswered.

## Report format
- First line: `UNDER REVIEW: integration/v50-ab1805 @ 1d1d5af7a22dc47094a4c23b515b348bd8ae153d`.
- At most 150 lines. Per question: the answer first (one sentence), then the evidence as `file:line` (Device OS paths relative to the 6.6.0 root), each tagged VERIFIED or INFERRED. End with a one-line recommendation (proceed, revise, or investigate further) and a list of what you could not verify.
- Return the complete report in your final message.

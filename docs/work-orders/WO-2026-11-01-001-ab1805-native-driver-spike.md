# WO-2026-11-01-001: AB1805 native-driver spike on Dev-14 (unmerged bench build)

**Author:** Claude Code, 1 Nov 2026, from the architect's consolidated rulings (Step 0 report `WO-2026-10-31-001-step0-report.md`, "Rulings"); the architect may amend · **Status:** APPROVED with amendments (architect, 1 Nov; see "Approved amendments")
**Branch:** `spike/ab1805-native-driver`, from the then-current `main`. **Never merged, never released, never uploaded to the product.** Flashed to Dev-14 over laptop USB only.
**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §3 Stage 4 (investigation), §13 (c), (j)

## Plain goal

Prove on real hardware that Device OS's built-in AM18x5 driver can do every job `AB1805_RK` does for GCC, using the fleet-wide configuration the rulings fixed, and gather the facts the switch WO and recovery item 21 need. The spike answers questions; it ships nothing.

## Approved amendments (architect, 1 Nov)

1. **Dev-14 is unlocked and marked as a development device for the spike** so Particle does not push v48 over the spike build (P3 needs the cloud). **Re-lock it to v48 after the restore.**
2. **Run P7 first** (v48's vendored `deepPowerDown()` on a serial command): it answers whether today's fleet stage 3 powers down.
3. **No meter:** the P6/P7 pass is the modem LED dark, the device offline for the timed period, and a timer boot with a power-on reset reason. A USB meter's current reading is preferred if Chip has one.
4. **P8 is skipped;** the spike report records it as skipped.
5. **Hands:** Chip, in Singapore, with Dev-14. Start from Tue 3 Nov after the v48 verdict. (This closes the location question below.)

## Start condition

- **After the v48 soak ends, from Tue 3 Nov** (the v48 verdict on Dev-09 and Dev-14 is in; Dev-14 is released from the lock).
- Chip at the bench with Dev-14, laptop USB, a way to see the power rail and the modem (below), and the SIM/antenna as they are.
- Location and hands: Chip, in Singapore (amendment 5).

## The configuration under test (ruled)

`particle::Am18x5Configuration` bound once: `Wire`/0x69, `.interruptPin(WKP)`, `.watchdogPin(D5)`, RC fallback on, always-on RC off (`wiring/inc/spark_wiring_exrtc.h:330-350`; `hal/inc/exrtc_hal_am18x5.h:47,56-57`). **The setters for `rc_fallback` and `rc_on_battery` are found in this spike** (Step 0 marked them INF). Reset reason plus time window classifies wakes; no raw WDT/ALM/TIM/SLST flags.

## What the spike builds

- A copy of the then-current `main` with `AB1805_RK` removed from the wake, time, watchdog and power-off paths and the native driver in its place, only as far as each proof needs. Diagnostics may be `Log` lines; the payload, `retained` layout and persisted layout stay untouched.
- A build flag or serial command per proof so Chip can run them one at a time. The exact shape is Claude Code's, listed in the spike report.
- **Time API:** implement both `TimeClass(TimeSource::EXTERNAL)` and `hal_rtc_*` with the external source for get/set, run proof 2 on each, and **recommend one** for the switch WO.
- Budget: no `src/` line limit (unmerged), but the spike report states the net lines a switch would need.

## Proofs and bench steps (each: expected result, pass, what to record)

**P1. Bind, then status ready.**
- Step: boot the spike on USB; the build binds once; reboot twice.
- Pass: `ExternalTime`/status reports ready after the bind and after each reboot; the clock source is the external RTC; no GCC library call touches the chip.
- Record: the status fields at boot, the binding storage behavior, the working setter for `rc_fallback` and `rc_on_battery`, and the **oscillator-event and clock-source status** on a healthy crystal (input for item 21).

**P2. External time get/set across a reset and a power-cycle.**
- Step: set a known time through the chosen API with the cloud off (modem off); soft reset; then a full power-cycle (USB and battery out for 30 s).
- Pass: the time after each is within 2 s of the expected value; run for both time APIs.
- Record: the offsets, and the API recommendation.

**P3. Does Device OS write cloud-synced time into the AM18x5 by itself?**
- Step: with the chip holding a time deliberately 60 s wrong, connect to the cloud, let it sync, then read the chip's time and compare with `src/time/Clock.cpp:392-405` (GCC's own write-back via `setRtcFromSystem`).
- Pass/record: yes or no, and after how long. This decides whether GCC's write-back survives the switch.

**P4. External watchdog refresh and a forced watchdog reset.**
- Step: `ExternalWatchdog` init and start with a short timeout; refresh normally for 10 minutes (the D5 toggle is the refresh); then stop refreshing.
- Pass: no reset while refreshing; a hardware watchdog reset within the timeout plus 1 s after refreshing stops, and the spike's boot log classifies it by reset reason plus time window.
- Record: the timeout set, the elapsed time to reset, the reset reason value, and `stop`/`start` behavior around a sleep (the ULP path GCC uses).

**P5. Two consecutive 8-hour native hibernates.**
- Step: hibernate for 28,800 s on the native path, wake on the alarm, run 10 minutes awake (cloud on), hibernate for 28,800 s again; repeat once more if time allows. Laptop USB logs both wakes.
- Pass: **each wake lands within 5 s of the request** (Dev-14's GCC wakes show err 1 s, `hibernate_wake` `err`), and the second is not an immediate stale-alarm wake. If the second wake fires early or never: stop, record it, and the architect decides (a narrow raw-status operation or a Device OS fix).
- Record: both `actual - req` values, the wake reason each time, whether the alarm flag needed clearing.

**Order: P7 first, then P1-P6.** (Amendment 2.)

**P6. A timed `POWER_OFF`, on the power rail and the modem.**
- Step: sleep with `POWER_OFF` and a 10-minute timer, powered from a bench supply or a USB power meter with a battery in; watch the rail and the modem (a current reading or its status LED) from the request to the wake.
- Pass: the rail current drops to the chip-only level (not the MCU's sleep level) during the off period, the modem is off, and the device boots at the timer (within 10 s of 10 minutes). **This is the observation the log line cannot give.**
- Record: the off current, the off time, the boot reason. Repeat once with USB removed.

**P7. The old failsafe stage-3 question (item 18), for contrast.**
- Step: on a v48 build (the vendored `deepPowerDown()`), call it from a serial command on the same rig and watch the same rail and modem.
- Pass/record: does the device actually power down, or fall through to `System.reset()`? This settles the PWGT-key suspicion (INF in Step 0) on real hardware.

**P8. RC fallback. SKIPPED (amendment 4); the report records it as skipped.**
- Step: with the chip bound, stress the crystal (a brief cold-spray or the crystal damped by a probe, Chip's judgment and risk) and boot.
- Record: the clock-source and oscillator-event status on RC fallback. **Optional; skipped if it risks the board.** A skipped P8 is stated plainly.

## Restore (mandatory, last step)

- **Delete the stored binding before flashing v48 back:** call the unbind (`hal_exrtc_unbind`, `hal/shared/exrtc_hal.cpp:303`; the wiring equivalent is confirmed in the spike) and reboot to confirm no binding remains (the external RTC reports not bound).
- Then flash v48 over USB and check the first boot: reset 70 is not expected (USB flash), `version` `v48-OccupancyEndsAtClose`, one clean connect, and the AB1805 library works (time read, watchdog start).
- Record the restore in the spike report. Dev-14 goes back to v48 and the soak's anchors.

## Agents and models

- Spike build and report: Claude Code (the main session; the architect names the model if it matters). Analysis of any unexpected chip behavior: Codex `gpt-5.6-sol`, **high**, read-only, stdin closed, `--skip-git-repo-check`.
- Controller check of every citation and number (§13 (j)).
- Bench steps and the hands: Chip.

## Output

`docs/work-orders/WO-2026-11-01-001-spike-report.md`, at most 150 lines: each proof with observed result (OBS) or inference (INF), the time-API recommendation, the setter names, the net lines a switch needs, the items for the switch WO, and a STOP/PROCEED for the switch. Committed on the spike's docs branch, then stop for the architect. **Do not merge anything.** The switch WO waits for v49 (which deletes any stored `EXRTC_CONFIG_DATA` at boot) to be the default.

## Standing rules

- No change to any other device. Dev-09, the pilot devices and the fleet stay on their firmware.
- `retained` RAM and the persisted layout are untouched.
- The spike build never goes to a product, a release, or the Particle cloud as a firmware upload.

## Amendment 5 (3 Nov 2026, architect ruling; amends ruling 3): P5 is two 4-hour hibernates
**Reason:** Chip leaves for the airport before 13:30 on 4 Nov, so two 8-hour hibernates (about 17 h) cannot finish with him present. **The 8-hour night proof moves to the v50 bench soak,** which covers two real nights.
- **P5 step/pass:** the same, with `P5_HIBERNATE_SEC` = 14,400 s (4 h) per hibernate instead of 28,800 s; 10 minutes awake with cloud on between them. Pass is unchanged: each wake within 5 s of the request, and the second is not an immediate or early stale-alarm wake.
- **Build:** `spike/src/spike.cpp:21` `P5_HIBERNATE_SEC` 28800 → 14400, no other change (the spike's `p5` takes no duration argument). Local toolchain build (Device OS 6.6.0, fresh scratch dir): 26,746 bytes; `arm-none-eabi-objdump` of `startP5Round` shows `mov.w r3, #14400` at `0xb4496`; `nm` has `startP5Round(unsigned char)` and `saveRec()`. Binary: `~/Downloads/spike-2026-11-03/ab1805-spike-p5-4h.bin` (SHA-256 `134331b49f5fee26e47ec549d9c7d8316fd03dadd03349cfacd6d3d89a4bd1d3`); the original 8 h `ab1805-spike.bin` is kept.
- **Run plan:** Chip runs it from about 21:00 SGT (13:00Z): wake 1 about 17:00Z, second hibernate about 17:10Z, wake 2 about 21:10Z. Restore (`unbind`, `reboot`, `status` bound=0, flash `v48-restore.bin`, re-lock Dev-14 to v48, unmark development) happens before Chip leaves.
- **Coverage lost:** an 8 h alarm is not exercised by this spike; the v50 bench soak covers it.

## Result record (4 Nov 2026; Dev-14 e00fce688e592afaf23ac4fb; source: `~/screenlog.0`, S3, controller reads)
**P7 (vendored `deepPowerDown(120)`, chip unbound): PASS, it powers down** (architect verdict). Call at sys 1793708957; boot about 122 s later with `resetReason=20` (PIN_RESET), data 0, `bootSTATUS=0xff`, RTC at boot 0 (system time 946684809, invalid): a full power loss, and the stored session was lost. Record for wake classification: expect reason 20 after a deepPowerDown wake on this hardware (the AB1805 asserting nRST at power-up is a guess, not confirmed). LED observation (Chip, 4 Nov): the modem LED went dark and the serial port disconnected for 120 s, then the device came back up and connected to the cloud. No meter reading. An earlier call at sys 1793708749 is in the log; only the second is treated as the P7 run. `~/spike-p7.log` is a 138-byte stub; `screenlog.0` holds the data.

**P5 (as amended, Amendment 5: two 4 h native hibernates; `ab1805-spike-p5-4h.bin`):** both rounds ran; `phase=3 round=2 wakes=8` (DONE). Round 1 requested at ext RTC 1793710324 (12:52:04Z), due 1793724724. Both wakes: `reason=30` (`RESET_REASON_POWER_MANAGEMENT`, Device OS 6.6.0 header), `bootSTATUS=0x04` (AM18x5 alarm flag set).
| Round | actual-req | uptime at RTC read |
|---|---|---|
| 1 | +27 s | 15,006 ms |
| 2 | +31 s | 15,006 ms |
- **No early or stale-alarm wake:** both errors are positive; wake 2 was not early or immediate.
- **The +27/+31 s do not test the 5 s pass directly; the spike's measurement has two biases (inference):** (1) `setup()` waits up to 15 s for a USB serial connection (`waitFor(Serial.isConnected, 15000)`, `spike.cpp:233`) before reading the RTC, and nobody was connected, so both reads are at 15,006 ms; (2) the due time is `reqEpoch + reqSec` with `reqEpoch` taken before the cloud/modem teardown, and `System.sleep` starts later (round 1's log: request at uptime 274.66 s, muxer stopped at 285.98 s, about 11.3 s). Removing both gives about +0.7 s for round 1 (before Device OS time to `setup()`); round 2's teardown time was not logged (the same 11.3 s would give about +4.7 s), so round 2 within 5 s is unproven.
- **Verdict for the architect:** no stale-alarm failure seen; timing within about 5 s likely for round 1, unproven for round 2. The 8 h proof moves to the v50 bench soak (Amendment 5), which should log wake error with GCC's own method.
- The `spike` cloud events are not in the S3 archive (the event type is probably not forwarded, or was not published); nothing from Dev-14 reached the archive between 12:01Z and 23:37Z.

**Boot-vs-first-sync RTC (P5 start boot):** external RTC valid at boot before the cloud (1793710057 at uptime 7.4 s; cloud connected 1.3 s later); the next `status` at millis 18,888 read 1793710068, i.e. 11.0 s of clock for 11.044 s of millis: no visible jump at first sync. The `FIRST SYNC:` line is not in the log.

**Restore:** `unbind` returned 0 and the reboot showed `bound=0 present=0 ready=0`. `v48-restore.bin` flashed; S3 shows Dev-14's first status at 23:37:08Z with `fw_version` 48, `v48-OccupancyEndsAtClose`, resetReason 20, watchdogResetCount 127, alert 0, failsafeCount 0. The first hook is the boundary catch-up close (timestamp 1793714399000 = 13:59:59Z = 21:59:59 SGT, dailyoccupancy 405, cyc 1), as on ChurchSt-2. Console re-lock to v48 and the development-device unmark: Chip.

**Not covered by this record:** P1-P4, P6 (and P8, skipped) have no evidence in the log reviewed here; none is claimed.

## Ruling (architect, 4 Nov 2026): the provisional ruling is final
**The AB1805 native-driver switch stays in v50.** The proof is **two real nights in the v50 bench soak, with serial attached** (so the wake lines, the boot clock-vs-first-sync reading and the pre-sleep teardown time are captured; the spike's P5 lacked them, see the result record above). **v50 soak checklist item:** attach serial for both nights; per wake record `actual - req` by GCC's own method, reset reason, AM18x5 STATUS at boot, and the teardown time before the sleep call. Fallback as planned if it fails. There is no v50 soak checklist file yet; this WO and the v50 release WO carry the item until one exists.

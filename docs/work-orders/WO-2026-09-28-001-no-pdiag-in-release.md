# WO-2026-09-28-001: diagnostics must never cost a report (v26)

**Goal, in plain language:** release builds don't publish `pdiag`, so diagnostics can't fill the publish queue and cause reports to be deleted.

**Status:** Opened 2026-09-28 (Chip). Stage 5 scope set by Chip in the opening instruction (below). Stage 7 VERIFIED WITH NOTES (2026-09-28); awaiting Chip's Stage 8: commit, a local release build of v26, flash ToM-MCP-Court3 first, then Dev-14, a day's check with (a)/(b)/(c), then the fleet.

**Workflow:** Per `AI_DEVELOPMENT_WORKFLOW.md`, including §12 (guardrails 1 and 3: an existing switch is set, not a new mechanism; size budget about 3 lines of `src/`).

**Branch:** `wo/2026-09-28-001-no-pdiag-release`, from `origin/main` at `9d1a285` (v25-WithAck).

## Evidence: ToM-MCP-Court3 on v25-WithAck (2026-09-28)

Read-only, from the AWS raw archive (`particle-events/<UTC day>/<event>/e00fce686e1a157c27984295/`), Claude Code, 2026-09-28 22:58Z.

- On v25 from 09:33 EDT (`status` `v25-WithAck`, reset reason 70).
- **Only 2 reports on v25 in about 9 hours:** payload stamps 09:33:15 EDT (`occupancy=1`, `dailyoccupancy=36`) and 14:15:01 EDT (`occupancy=0`, `dailyoccupancy=36`, `resets=2`, delivered at 16:46:31 EDT). The open hours 10–13 and 15–17 EDT have no report at all. On v21 the same morning, it reported hourly.
- **612 `pdiag` batches in the same window.** Each batch is one wake. The last batches show wakes every 6–12 s (a PIR wake, then back to sleep, with no report).
- **Watchdog at 16:46 EDT:** `bc=18`, `stage=diag`, **`queue=800`**, `state=1`.
- **Missing `status` events:** the 14:15 report carries `resets=2`, and the `pdiag` uptime counters show three boots on v25, but the archive has no `status` event for the resets before 16:46 EDT.
- **An occupied session that added 0 minutes:** 09:33:15 EDT `occupancy=1` → 14:15:01 EDT `occupancy=0`, with `dailyoccupancy` unchanged at 36. Filed against WO-2026-09-21-003 (below); not in this WO's scope.

## Cause

One `pdiag` event is queued per wake (`PowerDiagnostics::flushDiagBatch()`, `src/power/PowerDiagnostics.cpp:439`, `PublishQueuePosix::instance().publish("pdiag", payload, PRIVATE | WITH_ACK)`). Under rapid PIR wakes (WO-2026-09-21-003), that is several events a minute. The queue is capped at 800 files (`withFileQueueSize(800)`, `src/Generalized-Core-Counter.cpp:1133`). Over the cap, `PublishQueuePosix::checkQueueLimits()` (`lib/PublishQueuePosixRK/src/PublishQueuePosixRK.cpp:233–238`) **discards the oldest event, whatever it is**: reports and `status` events included. The `queue=800` in the watchdog event is that cap.

Why this is new on these devices: production v21 had no `pdiag`. `ENABLE_DIAGNOSTICS_PUBLISH_MODE` was promoted from bench-only to default-on in v22-Diag-Soak (`src/BuildProfile.h:232–247`), so every v25 device inherited it. With `WITH_ACK` restored in v25, each `pdiag` also stays queued until the cloud acknowledges it.

## Fix

**A switch already exists; set it, don't add a mechanism.** `ENABLE_DIAGNOSTICS_PUBLISH_MODE` (`src/BuildProfile.h:246`) gates the whole `pdiag` path: the in-RAM batch, the `ChargeDiag` capture into it, the flush calls in `State_Sleep.cpp`, and the publish. The serial `PowerDiag`/`ChargeDiag` log lines are outside the gate and stay.

| Change | File |
|---|---|
| `ENABLE_DIAGNOSTICS_PUBLISH_MODE` default `1` → `0` | `src/BuildProfile.h:246` |
| `FIRMWARE_VERSION` → `"v26-NoPdiag"`, with a matching `FIRMWARE_RELEASE_NOTES` line | `src/Version.cpp` |
| `FIRMWARE_PRODUCT_VERSION` 25 → 26 | `src/FirmwareVersion.h:28` |

**Bench builds** turn it on with `EXTRA_CFLAGS="-DENABLE_DIAGNOSTICS_PUBLISH_MODE=1"`, as `BuildProfile.h:30` documents.

**Size budget:** about 3 lines of `src/` (the four physical lines above). Going over means stop and report.

**Tests:**
- `tests/publish_with_ack_structural_test.py` counts publish call sites in the **source text** (`MIN_PUBLISH_SITES = 6`). The `pdiag` publish stays in the source, compiled out, so the count is unchanged: **no update**.
- `tests/build_flags_witness_test.sh` pins the default build-flags word to `0x2008`, which includes this flag's bit (`0x2000`). With the new default it becomes `0x0008` (default) and `0x0103` (flipped). This is the one test that must change, because it pins the default this WO changes.
- **Correction (Claude Code, 2026-09-28, after Stage 6):** a second test also pins the default. `tests/power_source_override_test.cpp:294` asserts the `status` event's build-flags word is `0x6008` (`0x4000` Boron + `0x2000` this flag + `0x0008`); with the new default it is `0x4008`. The WO's statement above that the witness test is the only one to change was wrong. Copilot stopped and reported it rather than changing it.

**Fleet check:** the `status` event's build-flags word carries bit `0x2000` for this flag (`src/cloud/DeviceStatusPublisher.cpp:233`), so a v26 device can be confirmed to have it off.

## Acceptance criteria

1. A release build (no `EXTRA_CFLAGS`) contains no `pdiag` publish: the string `pdiag` is absent from the binary.
2. A bench build with `-DENABLE_DIAGNOSTICS_PUBLISH_MODE=1` still contains it.
3. Serial `PowerDiag`/`ChargeDiag` logging is unchanged in both.
4. Suite passes (sh via zsh, py via python3).
5. Version `v26-NoPdiag`, product version 26.

## Filed elsewhere

- **Court3's 0-minute session** (09:33–14:15 EDT, `dailyoccupancy` unchanged at 36): filed against WO-2026-09-21-003 (rapid wake cycle). Whether the session was ended by one of the unrecorded resets, or never credited, is for that investigation.

## Approval record

- [x] Scope (Stage 5): Chip, 2026-09-28, in the opening instruction: use the existing switch, off by default, on only in bench builds; about 3 lines of `src/`; one Copilot round (`claude-opus-5`, medium); a narrow Stage 7 (Codex `gpt-6-astra`, high). Not authorized: commits, flashing.
- [x] Implementation (Stage 6) — round 1, 2026-09-28, Copilot `claude-opus-5`, reasoning medium: the four `src/` lines as specified (numstat 4/4, within budget) and the witness-test values (default 8 = `0x0008`, flipped 259 = `0x0103`, harness-proven). Release ARM build 149708 / 1090 / 2196: `strings` finds no `pdiag`, finds `v26-NoPdiag`, and the `PowerDiag`/`ChargeDiag` serial formats. Bench build (`-DENABLE_DIAGNOSTICS_PUBLISH_MODE=1`, after `make clean-user`) 150816 / 1090 / 2444: `pdiag` present. **Stopped: suite 43/44**, because `tests/power_source_override_test.cpp:294` pins `0x6008` (see the correction under Tests). Deviations reported: the first release-notes wording contained `pdiag` and put it in the binary, so it was reworded; `EXTRA_CFLAGS` changes need `make clean-user` to take effect. Report: `WO-2026-09-28-001-stage6-copilot-report.md`. Chip authorized the test fix as a narrow edit (no second Copilot round).
- [x] Narrow test edit (Claude Code, authorized by Chip, 2026-09-28): `tests/power_source_override_test.cpp:294` `0x6008` → `0x4008`, and its comment (lines 263–270) updated to the new default. Suite 44/44 (sh via zsh, py via python3). Final diff: `src/` 4 lines (`BuildProfile.h` 1/1, `FirmwareVersion.h` 1/1, `Version.cpp` 2/2); tests `build_flags_witness_test.sh` 8/6, `power_source_override_test.cpp` 6/5. The `make clean-user` trap Copilot hit is now recorded in `AI_DEVELOPMENT_WORKFLOW.md` §2, "Verifying compile-time flags".
- [x] Codex verification, narrow (Stage 7) — 2026-09-28, `gpt-6-astra`, reasoning high: **VERIFIED WITH NOTES.** Release build after `make clean-user`: `nm` finds no `flushDiagBatch` or batch storage in the ELF or the object; no `pdiag` in the `.bin`; `v26-NoPdiag` present. Bench build (`-DENABLE_DIAGNOSTICS_PUBLISH_MODE=1`, after `make clean-user`): `flushDiagBatch` and `diagBatch` present in both; `pdiag` in the `.bin`. Sizes: release 149708 / 1090 / 2196 (`.bin` 150802 bytes; a second clean release build was byte-identical), bench 150816 / 1090 / 2444. Serial `PowerDiag[%lu]:` and `ChargeDiag:` formats identical in both. Suite 44/44 (sh via zsh, py via python3); `publish_with_ack_structural_test.py` unchanged and passing. Witness harness 8 / 259; mutating the default back to 1 fails it. The one note: the scope check flagged the diff's documentation changes (`AI_DEVELOPMENT_WORKFLOW.md` build note, the WO-2026-09-21-003 filing); Chip authorized both, and the `src/` and test diffs are exactly as authorized. Working tree byte-identical before and after. Verdict: `WO-2026-09-28-001-stage7-verdict.md`.
- [ ] Chip final gate / commit (Stage 8)

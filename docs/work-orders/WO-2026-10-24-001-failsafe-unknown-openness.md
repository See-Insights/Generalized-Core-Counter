# WO-2026-10-24-001: the connectivity failsafe must act when openness is Unknown (recovery item 18)

**Author:** architect, 2026-10-24 · **Status:** ready (Step 0 first) · **Priority:** HIGH, ahead of other recovery items
**Branch:** `wo/2026-10-24-001-failsafe-unknown-openness`, from main. It ships in **v47** with P7-A (#142); the PR is marked "do not merge until v46 is cut".
**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §3 Stages 6–8; §12.1–12.4; §13 (b), (c), (g), (j)
**Evidence:** recovery-plan item 18 (#150), Dev-09, 23–24 Oct:
- **The stage-2 reset:** at 23 Oct 10:40Z (status `resetReason 140`, `resetReasonData 2`, `failsafeStage 2`).
- **The trap:** after the reset there was no sync, so the clock was untrusted and openness Unknown. The failsafe never escalated, and the device stayed dark for 14 h until a manual reset.
- **Precedent:** Dev-14 on 15 Oct.

## Plain goal

The failsafe ladder (stage 2, a system reset; stage 3, the AB1805 deep power-down) must keep climbing after its own stage-2 reset, even though that reset leaves the clock untrusted. **Today `connectivityFailsafeSupervisor()` returns unless openness is exactly Open** (`src/power/ConnectivityFailsafe.cpp:228`), so Unknown silently disables recovery. That is the circular trap the comment at `:190–205` warns against.

## The change (re-check at Step 0)

- **At `:228`:** `if (Clock::openness() != Clock::Openness::Open || now <= ageBase) return;` becomes `if (Clock::openness() == Clock::Openness::Closed || now <= ageBase) return;`.
- **Unknown is then treated as Open,** the same rule as the other CONNECTED-mode sites (`State_Sleep.cpp`, the CONNECTED+OPEN abort: "Unknown is treated as Open").
- **Nothing else changes:** no change to the stage-3 timing, cooldown, jitter, the low-battery suppression or the age base.

## Step 0 (Claude Code, read-only, §13 (c))

1. Re-check `:228` and the age-base lines (`:221–229`). Confirm `openedAt = DailyBoundary::todayAt(openTime)` is computed from `Time.now()`, so it still works on an RTC-seeded, valid but untrusted clock.
2. **The night case:** a genuinely closed site whose clock is untrusted would now count as Unknown. Can the failsafe then act at night?
   - The age base is the later of the last connection and today's opening, and the threshold is 3 h. So from 22:00 the age crosses 3 h at about 01:00 local, and stage 2 could fire at night.
   - Say whether this is acceptable. A device with an untrusted clock and no connection is broken whatever the hour, and a night reset costs one boot.
   - **STOP** only if the night case can loop: repeated stage-2 resets without escalating to stage 3.
3. **History (§12.1):** `git log -S "openness() != Clock::Openness::Open"` on that file. Was the strict Open check deliberate (WO-2026-10-02-001 item A), and did it consider Unknown?
4. **Size:** +0 to +1 `src/` lines. **PROCEED** at ≤ +20.

## Tests

A host test of `connectivityFailsafeSupervisor()` (there is an existing failsafe test harness; reuse it). Each case sets openness, a last connection older than `CONNECTIVITY_FAILSAFE_STALE_SEC` and the stage, then asserts:
1. **Unknown, stage 0:** stage 2 acts.
2. **Unknown, stage 2, past the cooldown and jitter:** stage 3 acts. **This is the trap case.**
3. **Closed:** no action.
4. **Open:** unchanged from today.

Mutation: restore `!= Open`. Case 2 must fail.

## Budget

- **`src/`:** +0 to +1 net, hard limit +20.
- **Tests:** guideline +100. Record ruling 9's numbers.

## Bench (after v46; can be remote)

There is no need to wedge a modem. A `CONNECTIVITY_FAILSAFE_TEST_MODE` build on Dev-14 can show stage 2 followed by stage 3 with the clock untrusted after the reset. Claude Code proposes the steps at Stage 8; Chip decides whether to run it remotely or after 31 Oct.

## Agents and models

- **Step 0:** Claude Code, read-only.
- **Stage 6:** Copilot, Sonnet tier, **medium**: a one-line change plus four test cases.
- **Stage 7:** Codex `gpt-5.6-sol`, **high**, stdin closed. This is recovery logic that decides whether a field device can save itself. It checks:
  - the night case;
  - stage 3 being reachable after stage 2 with an untrusted clock;
  - no change to the other gates.
- **Stage 8:** pre-approved on VERIFIED, or on VERIFIED WITH CONCERNS when every concern is Low. Controller test-only fixes follow §13 (d).
- **PR:** marked "do not merge until v46 is cut"; ships in v47 with #142.

## Closing record

- Budget versus actual.
- The test cases.
- The night-case answer.
- History.
- Ruling 9's numbers.

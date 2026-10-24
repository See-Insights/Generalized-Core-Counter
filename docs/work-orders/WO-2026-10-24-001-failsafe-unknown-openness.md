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

## Step 0, Stage 6 and controller reproduction (24 Oct)

**Step 0** (`WO-2026-10-24-001-step0-report.md`): PROCEED, net `src/` 0. Two sites, not one: `ConnectivityFailsafe.cpp:228` and the TEST_MODE prediction `ConnectivityFailsafeTest.cpp:159` (the open-hours test requires them to agree). The night case: an Unknown clock can act only between the close hour and midnight (22:00–24:00 local at the default; 00:00–06:00 `todayAt(openTime)` is in the future and returns); no loop (the stage is persisted before the reset and the 6 h cooldown plus jitter blocks a repeat). History: the strict check was deliberate (WO-2026-10-02-001 item A required that "an untrusted or Unknown clock never acts"); this WO reverses it for the failsafe.
**Stage 6** (Copilot, Sonnet 5.5, medium; one round, implementation commit `b466176` on `wip/failsafe-unknown-openness`): the two lines (`== Closed` and `!= Closed`) plus two comment lines at `:228`; `connectivity_failsafe_open_hours_test.{sh,cpp}` re-anchored (three-valued openness, `REAL_OPEN_GATE` parsed from the supervisor body, the prediction needle); a new harness `connectivity_failsafe_unknown_openness_test.{cpp,sh}` on the real extracted supervisor body (`COPY_MISMATCH`), 13 cases. `src/` tree hash `87715dba…` → `0d5c6028f3c6a915fd4c3eac81706bf27a1c1625928c48c4976eb26f75445dc8`.
**Controller reproduction** (Claude Code): suite **105/105** (baseline 104); git-backed mutations: `!= Open` at `:228` fails the new harness and the re-anchored test; `== Open` in the prediction fails the re-anchored test; "Unknown skips the cooldown" fails the new harness; the tamper flag prints `COPY_MISMATCH`; the control passes.

## Stage 7 result (24 Oct)

**Stage 7** (Codex `gpt-5.6-sol`, high; `WO-2026-10-24-001-stage7-verdict.md`): **VERIFIED WITH CONCERNS (Low only)**, retained outcome A, one round. Checks 1 (exact change, other gates and sites unchanged), 2 (night windows 22:00–24:00 default, 23:00–24:00 Trail02, 03:00–24:00 for `openTime` 0; **no path to repeated stage-2 resets**), 3 (stage 3 first possible at `max(T + 6 h + jitter, max(todayAt(openTime), lastConnection) + 3 h)`; Dev-09's would have been 09:00 SGT, about 14 h 20 min after the stage-2 reset; `Time.now()` is defined on an RTC-seeded clock), 6 (release `151340/1114/2212` unchanged, supervisor `0x298→0x294`; TEST_MODE `152868→152860`; `.backup` identical, SHA `adfe530f…`) PASS. Concerns, all Low, **left for the architect:**
- **C1:** the persist-before-reset ordering is correct in the source but not pinned: Codex's mutant moving `persistConnectivityFailsafeState` after `System.reset()` survives the new harness (the reset fake returns). A real reset would stop later code, so that mutant could permit repeated stage-2 resets. Candidate: make the fake's `System.reset` record the persisted stage at call time (a test-only fix).
- **C2:** the supervisor's prose at `ConnectivityFailsafe.cpp:167` and `:220` still says it acts "during open hours only", and its `@see docs/architecture/connectivity-failsafe.md` target does not exist; a comment change for the next time the file is touched.
- **C3:** tests +303 lines against the +100 guideline (Step 0 had expected +150 to +220).

## Closing record

| Item | Budget | Actual |
|---|---|---|
| `src/` net | ≤ +20, hard (expected +0 to +1) | **+2** (+4/−2): `ConnectivityFailsafe.cpp` +3/−1 (one gate line and two comment lines), `ConnectivityFailsafeTest.cpp` +1/−1; only the two authorized files |
| Tests | guideline +100 | **+303** net (+326/−23): the re-anchor +18 net, the new harness about 285 |
| Suite | baseline 104 | **105/105**; golden payload unchanged |

- **The test cases (new harness, 13):** Unknown at stage 0 acts (stage 2, `System.reset`); **Unknown at stage 2 past the cooldown and jitter acts at stage 3 (`deepPowerDown`), the trap case**; Unknown inside the cooldown, inside the jitter window, at stage 3, and with `now <= ageBase` do nothing; Closed does nothing; Open unchanged. The re-anchored open-hours test adds Unknown to its mirror.
- **The night-case answer:** acceptable and loop-free (above).
- **History:** WO-2026-10-02-001 item A (`52ab58d`, v32) required that an untrusted or Unknown clock never acts; reversed here for the failsafe (recovery item 18, PR #150).
- **Test complexity (ruling 9), against main `0c565fa`:** `src/` 22,306 → 22,392 (**+86**); test lines excluding fixtures 28,893 → 34,459 (**+5,566**); fixture lines 834 → 20,435 (**+19,601**, unchanged by this WO); suite 91 → 105 (**+14**). This WO alone: +2 / +303 / 0 / +1.
- **ELF and retained:** outcome A (above). **Argon:** not built.
- **What it buys, honestly:** with Unknown treated as Open, Dev-09 would have had its stage-3 deep power-down at about 09:00 SGT on 24 Oct (about 14 h 20 min after the stage-2 reset) instead of never; that matches the WO-2026-10-02-001 consequence ("a late stage 2 pushes stage 3 into the next open period"). Chip's reset at 00:53Z came 7 min earlier. Shortening the delay (the age base or cooldown) is a separate decision.
- **Bench (proposal; Chip decides whether to run it remotely or after 31 Oct):** a `CONNECTIVITY_FAILSAFE_TEST_MODE` build (stale threshold 5 min) on Dev-14, with the modem unable to register (needs a physical step: antenna or SIM removed, or a Particle device-level block), shows stage 2 then, after the 6 h cooldown plus jitter, stage 3 with the clock untrusted after the reset. Remote steps: OTA the test build to Dev-14 only; the wedge needs hands or a carrier block; so it waits for 31 Oct unless a remote registration block is arranged. Claude Code will prepare the build and the expected log lines (`Failsafe: stage=2 action=system-reset`, `Failsafe: stage=3 action=deep-powerdown`, `ConnectivityFailsafeTest` prediction lines) at v47.
- **Deviations:** the second source site (`ConnectivityFailsafeTest.cpp:159`) was not in the WO; two allowed comment lines made the net +2 instead of +0. **Rounds:** Stage 6 one; Stage 7 one (VERIFIED WITH CONCERNS, all Low). Ships in v47 with #142. **The PR is marked "do not merge until v46 is cut".**

## For the architect

- C1 (pin the persist-before-reset ordering) is a small test-only follow-up; C2 is a comment fix; both left open.
- The delay between a stage-2 reset and stage 3 on an Unknown clock (the next opening plus 3 h) is unchanged by this WO.

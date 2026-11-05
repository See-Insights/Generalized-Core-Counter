# WO-2026-11-05-006: clock trust should survive a restart and be withdrawn on evidence (recovery item 21): Step 0 (read-only)

**Author:** Claude Code, 5 Nov 2026 · **Status:** dispatched on the architect's instruction of 5 Nov ("so I21 isn't the long pole for v50.2"); read-only, touches no held branch
**Branch:** `wo/2026-11-05-006-item21-step0`, from `main` (`41d9911`, v49.1 plus #190, #191)
**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §3 Stage 4 (independent investigation); §5; §12.1, §12.5; §13 (c), (j)
**Agents:** Part A (code): Codex `gpt-5.6-sol`, **high**, read-only (`-s read-only`), stdin from the dispatch file, new output file, report ≤ **150 lines** (do not flag overages). Part B (fleet data): a Sonnet subagent, read-only (S3 raw archive, AWS profile `particle-admin`), ≤ 150 lines.

## Plain goal
Today the clock is trusted only if a cloud sync completed **this boot**, less than 24 h ago (`ClockTrust.h`, `kMaxSyncAgeMs`; `Clock.cpp:477-480`). Any reset or power cycle makes it untrusted until the next sync, although the AB1805 keeps time across a reset and is lost only when its battery is physically removed. The rule cost items 18 and 19 and, on Dev-09, a missed close and a lost night's sleep (23-24 Oct). Item 21 asks for **trust that survives a restart and is withdrawn on evidence** (a large `correctionSec` at sync, an invalid RTC, an oscillator-fault event), not by the reset itself. This Step 0 finds out what the rule should be and what it costs, **with no code change**, so the I21 piece (v50.2) can be a small WO as soon as S1-S3 land.

## Context (read first)
- Recovery plan item 21 (`docs/RECOVERY_PLAN_2026-09-26.md`, the "21." entry) and its Step 0 list; items 10, 15, 18, 19; #151 (Unknown openness counts as open).
- `WO-2026-11-03-001-step0-report.md` §6-§13 (item 19: "item 21 must not land first … it cannot define behavior when trust is genuinely withdrawn").
- The v50 plan: `WO-2026-11-05-002-v50-ab1805-switch.md` (ruling 3: oscillator-fault evidence is **event-based**, "fault seen since bind", via `ExternalTime.onEvent`; ruling 5: the trust rule is unchanged in S1-S3; I21 depends on S1-S3, the event telemetry and C1/C2), and S1 `WO-2026-11-05-003-v50-s1-boot-config-time.md` (after S1 the chip is read through `hal_rtc_*` and `TimeClass(EXTERNAL).isValid()`).
- **Held branches, read-only context (do not check out or modify):** C1 `origin/wo/2026-11-03-004-item19-c1-millis-expiry` (#185) and C2 `origin/wo/2026-11-03-006-item19-c2-untrusted-split` (#184), which change how an untrusted clock credits sessions (`pieceUntrusted`, `occupancyStartMs`, RAM only). Read them with `git show <ref>:<path>`; **cite which tree each file:line is from.**

## Part A: questions (Codex; observation and inference marked separately; cite file:line)
1. **Every decision gated on trust.** List every use of `isClockTrusted()`, `Clock::openness()` and the trust helpers in `ClockTrust.h` (bare-name search, §13 (c)), with the decision each one gates (report timing, session close and credit, night sleep, close stamps, failsafe "Open" tests, hibernate eligibility, the daily rollover) and the trust-machinery tests that pin it. **What changes for each if trust survives a restart?** Mark any that would act on a wrong clock if trust were wrongly kept.
2. **The failures the rule caused or worsened** (items 18 and 19; Dev-09, 23-24 Oct; Dev-11), with the mechanism in the code, and how many are fixed on `main` already by #151, item 18, item 10 (v48) and by C1/C2: **what does item 21 still buy** after C1/C2 (e.g. fewer lost partial sessions per reboot, earlier close stamps, the night sleep)?
3. **The rule.** Design the evidence-based rule: trust carried across a restart from a **persisted last-sync epoch** (and what else), lost on (a) a `correctionSec` over a threshold at a sync, (b) `TimeClass(EXTERNAL).isValid()` false or an implausible epoch, (c) an oscillator-fault **event** since bind (and what to do on a boot where the event was missed: the chip's sticky OF flag is no longer readable through the public API), (d) a long unsynced span (is 24 h still the right bound across restarts? who wins if the RTC drifts?). State each condition's inputs, where they are read, and the failure when each is wrong (trusted a bad clock; distrusted a good one).
4. **Where the last-sync epoch lives.** Options: `retained` RAM (lost at a power-down, kept across a reset), the persisted device data (`MyPersistentData`/FRAM or the file store), the AB1805 itself (user RAM). For each: survives which events (reset, watchdog, hibernate wake, deep power-down, battery removal), the persisted-layout impact (**Step 7 rules: any layout change needs the architect's ruling**; list which option needs none), flash/FRAM write rate, and the first-boot-after-OTA behaviour (no value stored: untrusted, as today).
5. **Interaction with S1-S3, C1/C2, item 10/15, #151.** What I21 needs from S1 (the `hal_rtc` helpers, `TimeClass(EXTERNAL).isValid()`) and from the event telemetry (is `ExternalTime.onEvent` available in 6.6.0, and what events carry); what changes in C2's `pieceUntrusted` and the "untrusted for the whole boot" assumption; whether I21 can be built and tested on `main` first and rebased, or must wait for S1-S3; whether item 15 (occupancy lost across a reset) changes the value of I21.
6. **Candidate designs** (at least: (a) persist only the last-sync epoch, trust carries if it is < N h old and the RTC is valid; (b) (a) plus withdrawal on a large `correctionSec`; (c) (b) plus the oscillator-fault event), each with: behaviour on the Dev-09 23-24 Oct timeline and on a clean day (unchanged), the worst case if trust is wrongly kept, estimated `src/` lines, tests that exist and must change, and the layout impact.
7. **Bench plan.** What reproduces it (a reset mid-session with a valid RTC and no cloud yet; an RTC set wrong by a known amount; the skew hook `skew N` in `time/RtcSkewTest.h`; an oscillator-fault event if it can be induced) and the pass numbers.
8. **Stop decision:** STOP if no rule can distinguish a good restart from a bad clock without a layout change the architect has not ruled, or if the evidence inputs do not exist on 6.6.0; say why. Otherwise PROCEED, ranking the designs and naming the smallest I21 WO shape (**≤ +20 `src/` per WO**, removals count; propose the split if it does not fit).

## Part B: fleet data (Sonnet subagent; read-only; separate report `WO-2026-11-05-006-step0-fleet-data.md`)
Size the threshold from facts: the `correctionSec` values at every `ClockResync` (or the equivalent field) over the last 14 days on the fleet (S3 raw archive `s3://infrastack-rawparticlelogsbucket4adba400-7yquehl7iulz/particle-events/<UTC day>/<event>/<id>/`, profile `particle-admin`; the event/field names are found in `src/` and `docs/FIELD_MEANINGS_REFERENCE.md`): per device the count, median, 95th percentile, maximum and each value over 5 s, 60 s and 600 s with its date and the boot it was on; separate first-sync-after-reset from later syncs. Report the numbers and what they imply for a threshold; do not choose it. Dev-11's failure (the correction the plan says would have shown it) is looked for by name.

## Constraints
Read-only: no code, no device, no commit (the controller commits the reports). Part A has no network. Reports state the model and reasoning level actually used and the interpreter line if any tests are run (`sh via zsh`, `py via python3`). Observation (source read with file:line) and inference are kept apart; any number about the fleet or the Dev-09 timeline is recomputed from the plan's figures, not assumed.

## Out of scope
Any change to `src/`, the persisted layout or `retained`; the failsafe ladder; the C1/C2 credit logic; the report format; the S1-S3 pieces.

## Approval record
Architect: instructed 5 Nov to draft and, being read-only and touching no held branch, to dispatch now (Codex `gpt-5.6-sol` high, §3 Stage 4). Chip authorizes dispatch: per the relay of 5 Nov.

## Part A result and architect's rulings (5 Nov 2026)
Part A report: `WO-2026-11-05-006-step0-report-partA.md` (Codex `gpt-5.6-sol` high, main `41d9911`): **PROCEED**, candidate C, 17-20 `src/` lines if the S-set supplies the latch. **Rulings:**
1. **Candidate C approved:** persisted last-sync epoch (`lastTimeSync`), **no layout change**; zero is the "no carry" value; withdrawal on a `correctionSec` over T, an invalid read or write, or the fault latch.
2. **The "fault seen since bind" latch is owned by the S-set** (S1 if it fits, else an S4 of ≤ +10; S1 Amendment 2). I21 reads **only the boolean**. At callback registration an **RC-fallback `clockSource()` counts as "fault seen"**. **No persisted latch.**
3. **First boot after the upgrade carries trust** from a recent sync (no migration marker).
4. **Required test:** C2 regression (a piece opened while trusted, then trust withdrawn).
5. **T waits for Part B** (the fleet `correctionSec` data); the I21 WO is drafted after it.

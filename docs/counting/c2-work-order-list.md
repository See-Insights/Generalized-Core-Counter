# C2: GCC counting parity, work order list (draft)

DRAFT 24 Nov 2026 (Claude Code), for the architect. Source: the 15 items in `docs/counting/gcc-vs-v50-gap-analysis.md` section (c) ("(c) item N" below), with Chip's decisions of 24 Nov recorded in the deviation register of `docs/counting/v50-parity-spec.md`. Rule: GCC counting behaves like v50/v54 unless a change is a recorded, conscious decision. Counting work goes to `main` only (never the release branch).

**Line budgets are estimates**, taken from the sizes of the code the gap analysis cites. They are changed or added lines, source plus tests, and are not measured. Each WO needs a Step 0 before its budget is firm.

## Ready (smallest first)

| # | WO | Scope | Budget (src / tests / docs) | Changes counts? |
|---|---|---|---|---|
| C2-1 | Evidence: sleep-wake double count, and Ubidots duplicate timestamps ((c) item 2; UNKNOWN 1 and 2) | Bench: send one PIR pulse into a sleeping COUNTING device on `main` and read `hourly` (1 or 2). Ubidots: post two `hourly` dots with one timestamp and read the result. No firmware change. | 0 / 0 / 40 | No (evidence). Gates C2-6 and the timestamp risk in the register. |
| C2-2 | Counting product ledger default and webhook template check ((c) item 3) | Product default for counting units: `sensorMode` 0, 3600 s reporting, and the new webhook name **`Ubidots-GCC-Counter-Hook-v1`** (proposed; checked against the 14 names in the 12 Nov prefix check, in both directions and ignoring case: none equal, begin with, or begun by it; it differs from `Ubidots-Counter-Hook-v1` at the 8th character). It replaces the code fallback `counting-webhook-v1` as the configured name. Chip builds the integration for it in the counting product with the legacy template minus `maxmin`: `hourly`, `daily`, `battery` (value plus `context.key1`), `temp`, `alerts`, `resets`, `connecttime`. Check the template against that field list. Today a counting unit left on `sensorMode` 1 or on an unmatched name reports no counts. | 0 to 20 / 20 / 40 | No device counts; decides whether Ubidots receives them. Needs the webhook definition from Chip (not in either repo). |
| C2-3 | Payload length and `snprintf` check ((c) item 4) | Measure the worst case against the 256-byte buffer (estimate 235), check the `snprintf` return, test the maximum case. Do this before C2-5 changes the format. | 15 / 40 / 10 | No |
| C2-4 | Alert mapping and events ((c) item 11; Chip decision 9) | Record the legacy-to-GCC mapping (done in the register); list which GCC codes need their own Ubidots events (16, 19, 41 to 44 are undecided); Chip creates the events. The clear-after-report rule for held codes is a separate question for the architect. | 0 / 0 / 80 | No |
| C2-5a | Debounce to legacy, about 200 ms merge ((c) item 5; Chip decision 3) | Replace the 500 ms discard (`src/sensors/PIRSensor.h:79`) with the legacy lockout behaviour (`Visitation-Counters.cpp:788`, `:803`). Host test with edge trains at 100, 200, 300 and 500 ms. | 25 / 70 / 10 | **Yes**: recorded in the register as matching v50. |
| C2-6 | One event per sleep wake ((c) item 6) | Make the wake count (`State_Modes.cpp:180-183`) and the ISR flag a single event with one shared debounce. **Depends on C2-1:** if the bench shows one count per pulse, this is a no-op and is closed. | 30 / 60 / 10 | Yes, if C2-1 shows a double count. |
| C2-7 | Count-file flush before an OOM reset, and the 250 ms save delay ((c) item 13) | Flush before `System.reset()` on out-of-memory (`MyPersistentData.cpp:613`, `:634`). Review whether 250 ms loses the last count on an unplanned reset. | 40 / 50 / 10 | Only after an unplanned reset (loss, not drift). |
| C2-8 | CONNECTED-mode report on the clock-hour change ((c) item 8) | Today 3600 s after `lastReport` (`State_Idle.cpp:197-209`); legacy reports on the hour boundary. **Decision still needed** (not in Chip's 24 Nov list). | 30 / 60 / 10 | Hourly attribution, not device totals. |
| C2-9 | Payload parity ((c) item 10; Chip decisions 7 and `maxmin` drop) | Counting-mode payload to the legacy template: integer `battery` with `key1` as its context, integer °F `temp`, `key1` strings identical to legacy (spelling included), `resets` and `alerts` order as legacy, and `maxmin` stays absent (dropped). Move `vc`, `fh`, `lfb`, `cyc` and `slp` to the status ledger. **Watch:** the fleet health checks read `fh`, `cyc` and `slp` from the hourly hook today (occupancy hook); confirm the occupancy payload is not touched, and check whether the status ledger needs a schema change. | 60 / 120 / 30 | No counts; changes the Ubidots variables. Largest ready item; has a Step 0 for the ledger. |

## Held

| # | WO | Why held |
|---|---|---|
| C2-H1 | Daily rollover (Chip decision 4; the rollover part of (c) item 1; the boot-time new-day reset of the ranked list) | **Open, pending fleet data on overnight counts** (Chip, 24 Nov). Nothing changes until he decides. |
| C2-H2 | Turn the sensor off at close, before the close report and connection ((c) item 9) | Moves where evening stragglers land relative to the daily rollover, so it waits with C2-H1. |
| C2-H3 | Untrusted clock and invalid config: count in closed hours? when to reset the day? ((c) item 12) | The "when to reset the day" half is the rollover decision. Waits with C2-H1. The fleet question (UNKNOWN 8, how often a fleet unit has an untrusted clock) can be read now, read-only. |

## Closed by decision

- **`maxmin` ((c) item 10, part B; Chip, 24 Nov):** dropped, confirming the 20 Oct decision. No WO. The Ubidots `maxmin` variable goes stale by design; Chip may retire it in Ubidots.
- **Timestamp ((c) item 7; Chip decision 5):** GCC xx:59:59 kept, recorded as a deviation. The only open question is the duplicate-timestamp behaviour in Ubidots, covered by C2-1.
- **Missing legacy paths (Chip decision 8):** not ported, each recorded as a deliberate drop. This also closes the remote-operations item (14, `setDailyCount`, `resetCounts`, `SendNow`): configuration is by ledger.
- **Pressure (car) path ((c) item 15):** out of scope until a car counter migrates.

## Suggested order

C2-1, C2-2, C2-3 and C2-4 (no code or tiny code, in parallel); then C2-5a; then C2-6 (needs C2-1); C2-7; C2-9; C2-8 when decided. Held items (C2-H1 to C2-H3) wait for Chip. A side-by-side day of legacy and GCC on the same PIR signal (gap analysis UNKNOWN 7) should follow C2-5a and C2-6, to measure the net difference.

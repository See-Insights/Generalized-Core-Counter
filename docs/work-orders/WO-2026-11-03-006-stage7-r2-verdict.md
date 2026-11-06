bb6e75d82b8f838438ded819d3afac4527013c38

**NOT VERIFIED — one blocking acceptance-spec finding; the production implementation itself is verified.**

Model/reasoning actually used: Codex · `gpt-5.6-sol` · `xhigh`  
Interpreter line: **109/109 (sh via zsh, py via python3)**

## Findings

1. **Blocking — Amendment 2’s arithmetic is nominally correct, but it is not yet a deterministic Dev-14 pass rule.**

   The nominal timeline is correct: 5,700/5,431 seconds before the boundary and 6,270/6,001 seconds after the eleventh close. However, round 2 starts a restored piece at setup’s `millis()` at [Generalized-Core-Counter.cpp:503](/private/tmp/gcc-step0/c2/src/Generalized-Core-Counter.cpp:503). With no post-boot PIR, the sentinel records the first predicate time at [State_Common.h:335](/private/tmp/gcc-step0/c2/src/state/State_Common.h:335), and the close calculation at [State_Common.h:401](/private/tmp/gcc-step0/c2/src/state/State_Common.h:401) credits:

   `floor((firstEvaluationMs − setupStartMs + 300000) / 1000)`

   Thus 301 seconds occurs only when that gap is 1,000–1,999 ms. The gap matters one-for-one. After the start assignment, setup permits up to 30.5 seconds of serial waiting at [Generalized-Core-Counter.cpp:556](/private/tmp/gcc-step0/c2/src/Generalized-Core-Counter.cpp:556) and up to 1.4 seconds of battery stabilization at [SensorManager.cpp:567](/private/tmp/gcc-step0/c2/src/sensors/SensorManager.cpp:567), for 31.9 seconds of explicit delay. Other synchronous setup work has no aggregate source-level timeout. The 60-second watchdog bounds an unserviced stall, not the total gap because setup loops can feed it. Therefore “about 301 seconds” is not a hard bench expectation.

   Wake overhead must also be applied at both checkpoints, not only the close-report figures. Previous bench sessions ranged from 569–581 seconds at [v46 release plan:37](/private/tmp/gcc-step0/c2/docs/work-orders/2026-10-25-v46-release-and-soak.md:37). Finally, `dailyoccupancy` is `floor(totalSeconds/60)` at [PIRSensor.cpp:25](/private/tmp/gcc-step0/c2/src/sensors/PIRSensor.cpp:25), so the nominal payloads 95/90/104/100 may move when measured overhead crosses a minute boundary.

   The architect should approve a formula-based pass rule:

   - pre-boundary control: sum the first ten measured clean pieces;
   - pre-boundary fault: measured straddler plus nine clean pieces;
   - close report: add the measured eleventh piece;
   - payload: truncate the resulting aggregate `/60`.

   The published numbers remain the correct nominal examples for ten/eleven exact 570-second pieces and a one-second setup-to-evaluation gap.

2. **Note — stale piece fields are fixed on every production open/close path.**

   Common close clears both fields at [State_Common.h:453](/private/tmp/gcc-step0/c2/src/state/State_Common.h:453). Both awake and PIR-wake opens initialize them at [State_Modes.cpp:70](/private/tmp/gcc-step0/c2/src/state/State_Modes.cpp:70) and [State_Modes.cpp:221](/private/tmp/gcc-step0/c2/src/state/State_Modes.cpp:221); boundary reopen does so at [State_Report.cpp:80](/private/tmp/gcc-step0/c2/src/state/State_Report.cpp:80); boot restoration does so at [Generalized-Core-Counter.cpp:503](/private/tmp/gcc-step0/c2/src/Generalized-Core-Counter.cpp:503). No other production path sets `occupied=true`.

3. **Note — wall checks are correctly confined to trusted pieces.**

   The zero-start and future-start checks are guarded by `!piece` at [State_Common.h:398](/private/tmp/gcc-step0/c2/src/state/State_Common.h:398) and [State_Common.h:416](/private/tmp/gcc-step0/c2/src/state/State_Common.h:416). The one-day and daily-total validation remains shared at [State_Common.h:420](/private/tmp/gcc-step0/c2/src/state/State_Common.h:420).

   My focused real-helper scratch run produced:

   - ten-hour forward correction: 360 seconds;
   - ten-hour backward correction: 360 seconds;
   - zero wall start: 360 seconds.

   For `pieceUntrusted=false`, the conditions and wall subtraction reduce exactly to C1 behavior. The trusted zero/future-start anomaly branches, B2 retry, and B3 cap tests all passed.

   Unsigned subtraction remains valid across ordinary rollover only while elapsed time is under one full `uint32_t` period. At approximately 49.7 days it aliases; an aliased duration can evade the one-day check. That is the stated monotonic-clock limit.

4. **Note — regression sweep passed.**

   The LED-independent untrusted close is at [State_Modes.cpp:158](/private/tmp/gcc-step0/c2/src/state/State_Modes.cpp:158), while trusted LED-low behavior remains unchanged. B2 re-arms only a trusted piece that loses trust at [State_Common.h:370](/private/tmp/gcc-step0/c2/src/state/State_Common.h:370); B3 remains on the trusted boot path.

   `src/` is exactly `+26/−10 = +16 net`. Persisted and retained owners are unchanged. S-4/S-6 are byte-identical to round 1, with Git blob hashes `f3ffa61e…` and `914e77f7…`.

   The additions to [standby_latch_daily_clear_test.cpp:17](/private/tmp/gcc-step0/c2/tests/standby_latch_daily_clear_test.cpp:17) are compilation stubs only; they do not change its modem-latch inputs or assertions.

5. **Note — tests and independent mutations passed.**

   The clean-clone host suite passed **109/109**, including a separate clean-tree thermal scan. The C2 harness extracts the real predicate, close helper, and update function at [untrusted_piece_c2_test.sh:142](/private/tmp/gcc-step0/c2/tests/untrusted_piece_c2_test.sh:142); boot and boundary sites are explicitly structural checks.

   Six independently applied scratch mutants were caught:

   - stale start after close;
   - boundary reopen without piece fields;
   - restored zero-start check for pieces;
   - restored future-start check for pieces;
   - boot restore without the monotonic start;
   - one-debounce boot fallback.

6. **Note — fresh Device OS 6.6.0 linkage verified.**

   Fresh Boron build: **text/data/bss 152036/1114/2208**. `nm -C` resolves `closeOccupancySessionSafely` at `0x000c1d40`. Objdump shows exactly four direct production calls:

   - idle: `0xc2076`, source [State_Idle.cpp:52](/private/tmp/gcc-step0/c2/src/state/State_Idle.cpp:52);
   - awake update: `0xc25d2`, source [State_Modes.cpp:137](/private/tmp/gcc-step0/c2/src/state/State_Modes.cpp:137);
   - sleep wake: `0xc27a4`, source [State_Modes.cpp:161](/private/tmp/gcc-step0/c2/src/state/State_Modes.cpp:161);
   - boundary/reporting: `0xc2ac6`, source [State_Report.cpp:60](/private/tmp/gcc-step0/c2/src/state/State_Report.cpp:60).

7. **Note — Dev-14 remains pending.**

   Nothing in the tracked diff claims the bench passed; Stage 6 round 2 explicitly leaves it to Chip at [stage6-r2 report:64](/private/tmp/gcc-step0/c2/docs/work-orders/WO-2026-11-03-006-stage6-r2-copilot-report.md:64). Network access was prohibited, so the remote PR body was not inspected. Before merge, it must state the bench is pending and use the architect-approved formula-based checkpoint criteria above.
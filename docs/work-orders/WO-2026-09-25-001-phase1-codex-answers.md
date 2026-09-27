_Codex answers, verbatim. Codex CLI 0.154.0, `gpt-6-astra`, reasoning high (confirmed in the run header). Question 4 was completed afterwards by Claude Code's scratch cloud build; see `WO-2026-09-25-001-phase1-provenance-compile.log`._

Investigated **`599038e`**. The actual checkout was clean at `edd7a32` on `archive/wo-2026-09-25-001-round3`; it remains unchanged. References below use `599038e` unless marked otherwise.

1. **Yes, if Particle ACKs never succeed while connected:** Idle blocks sleep, and queue failure disables its ceiling (`src/state/State_Idle.cpp:233`, `:280`, `:303`). Missing Ubidots replies alone are a separate condition.  
   v3.23 also could hang: its Idle timeout reached Sleep, but Sleep waited indefinitely *before* its bounded gate (`eda6b7e^:src/state/State_Idle.cpp:125`; `src/state/State_Sleep.cpp:60`).  
   Smallest functional change: **one line**, change Idle’s line 238 to `if (!updatesPending)`; removing the four unused gate lines makes **five existing lines affected**.  
   This hands queue waiting to Sleep’s existing 30–120-second gate, which raises alert 43 and proceeds with disconnect (`src/state/State_Sleep.cpp:147`, `:570`, `:609`, `:626`).

2. **No overwrite bugs remain:** the equivalent is `resolveErrorAction()`; cases 12/13 are absent, and case 40 directly returns its action (`src/state/State_Error.cpp:31`, `:59`, `:110`).  
   Raising alert 40 does **not itself enter Error**; the short timeout only records it (`src/Generalized-Core-Counter.cpp:1707`).  
   Report explicitly avoids immediate Error, but conditionally enters it after >6 hours, with trusted open hours, a previous reply, recent connection and cooldown (`src/state/State_Report.cpp:130`, `:158`, `:302`).  
   Thus supervision supplies delayed routing; it is **not compensating for an overwritten case-40 result**. Once reached, Error can select a soft reset (`src/state/State_Error.cpp:74`).

3. Today subscribes to **`System.deviceID()` and `"hook-response/"`** (`src/Generalized-Core-Counter.cpp:955`, `:964`).  
   The original `responseTopic`, introduced in `8aa2643`, was exactly **the device ID string**, without prefix or suffix (`8aa2643:src/Generalized-Core-Counter.cpp:203`).  
   That original subscription already exists; this history provides no different topic string to restore.

4. Cloud selects registry **StorageHelperRK 0.0.5, AB1805_RK 0.0.4, LocalTimeRK 0.1.3, PublishQueuePosixRK 0.0.7** (`project.properties:3–6`; [Particle’s precedence documentation](https://docs.particle.io/firmware/best-practices/libraries/)).  
   Downloaded registry AB1805 constants are `0x0c/0x04/0x01` (`AB1805_RK.h:875–877`), versus vendored `0xc0/0x20/0x10` (`lib/AB1805_RK/src/AB1805_RK.h:892–894`); source evidence therefore predicts **PR #41 is omitted**.  
   **Application-binary verification remains incomplete:** my command ran from the wrong scratch directory and compiled an AB1805 example. I cannot present it as application evidence.  
   Replacement authorization is pending because the requested limit was one cloud build. Retained `codex-phase1-ab1805-example-NOT-APPLICATION.bin` and its `.sha256`; removed my other temporary artifacts.

5. **Six queue publish sites:** report—`PRIVATE` (`src/Generalized-Core-Counter.cpp:2061`); status—`PRIVATE` (`:2283`).  
   Watchdog—`PRIVATE` (`:2323`); hibernate_wake—`PRIVATE` (`:2358`); diagnostic helper—caller-supplied `flags`, default/callers `PRIVATE` (`:2471`, `:128`).  
   pdiag—`PRIVATE` (`src/power/PowerDiagnostics.cpp:439`). **No `Ubidots_Alert_Hook` queue call exists**; the reply diagnostic is `"Ubidots Hook"` through the helper (`src/Generalized-Core-Counter.cpp:2436`).  
   History verified: report/status/helper ACK usage at `4196c17`/`8aa2643`/`15db55c`; all three removed by `eda6b7e`, without a stated ACK-removal rationale.

Model: **gpt-6-astra** · Reasoning: **high**
UNDER REVIEW: wo/2026-11-06-001-v50-s4-oscillator-fault-latch @ 6106f6ca7f59d758aeb71347be6516089166c3de

**PASS for S4 as scoped.**

| # | Verdict | Evidence |
|---|---|---|
| 1. F1 latch | **PASS** | [`ExternalRtc.h:35–37`](/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s4-r2/src/time/ExternalRtc.h:35) uses a false-initialized function-local `std::atomic<bool>` and a relaxed callback store. [`Generalized-Core-Counter.cpp:663`](/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s4-r2/src/Generalized-Core-Counter.cpp:663) uses `= true`, correctly producing a stronger seq_cst store. No production clear, persistence, or unsafe read site exists. `nm` shows one-byte BSS with no guard variable/dynamic initialization. |
| 2. F3 tests | **PASS** | [`external_rtc_structural_test.py:57–72`](/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s4-r2/tests/external_rtc_structural_test.py:57) requires atomic storage, relaxed callback store, and bind → read → initial set → registration order. [`external_rtc_test.cpp:122–137`](/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s4-r2/tests/external_rtc_test.cpp:122) executes the combined mask. Stub operators at [`Particle.h:92–99`](/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s4-r2/tests/stubs/external_rtc/Particle.h:92) match Device OS `EnumFlags` semantics and are host-test-only. All four new mutations fail. |
| 3. Residual | **PASS** | Exactly one `Am18x5.status().clockSource()` read remains at [`Generalized-Core-Counter.cpp:658`](/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s4-r2/src/Generalized-Core-Counter.cpp:658). The accepted window and rejection of a second read are recorded at [`WO:53–61`](/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s4-r2/docs/work-orders/WO-2026-11-06-001-v50-s4-oscillator-fault-latch.md:53). |
| 4. Regression since round 1 | **PASS** | Base diff changes only [`Generalized-Core-Counter.cpp:663–664`](/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s4-r2/src/Generalized-Core-Counter.cpp:663) and [`ExternalRtc.h:2,34–38`](/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s4-r2/src/time/ExternalRtc.h:34). S1–S3, payload, retained/persisted layouts, Clock, and write-back are untouched. |
| 5. Budget/build/suite/mutations | **PASS** | Net `src/`: **+8**. Exact CI discovery: **121/121**; helpers exit zero bare. Clean-export thermal test passed. Fresh Device OS 6.6.0 Boron build passed. Independent mutation audit caught **6/6**: all four Copilot mutants plus new thread-local-latch and disconnected-callback-atomic mutants. |
| 6. Boot/S1/I21 | **PASS** | S1 bind at [`Generalized-Core-Counter.cpp:295`](/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s4-r2/src/Generalized-Core-Counter.cpp:295) precedes the source snapshot and registration. The constant-initialized atomic is safe even if the worker callback runs immediately after registration. I21 can consume the accessor without a new dependency; no trust logic or contract field was added. |

### Findings

None. No corrective change or `src/` budget impact is required.

### Verification record

- Fresh build: **text 151092 / data 1114 / bss 2256**, matching Copilot round 2.
- `nm`/`objdump`: callback `0x000b53cc`; one-byte latch BSS `0x2003d728`; setup calls `hal_exrtc_event_handler_add`.
- Callback relaxed store compiles to a conditional byte store without barriers. Registration-time seq_cst assignment has `dmb` barriers.
- `git diff --check` passed; tracked worktree remained unchanged.
- A diagnostic forced-6.6.0 run produced 120/121 only because `sleep_config_leak_test.sh` deliberately targets the older Device OS 6.4.1 move-assignment leak. Its documented CI/default fixture passed, and exact CI discovery was 121/121.

Model/reasoning: **gpt-5.6-sol, xhigh**.  
Interpreter: **Python 3.14.6; zsh 5.9 (arm64-apple-darwin26.0); GNU Arm Embedded 10.2.1**.  
Build: **Device OS 6.6.0, Boron, buildscripts 1.17.2; fresh `BUILD_PATH_BASE` passed as a make argument**.  
**Round 2 of 2.**

No hardware was available or exercised. Healthy-crystal/RC-fallback boot behavior and physical oscillator-fault delivery remain unverified on hardware.
UNDER REVIEW: wo/2026-11-06-001-v50-s4-oscillator-fault-latch @ 6a6a57dce125871ede2357e48d42b8c9559a5e0a

**NOT VERIFIED for S4 as scoped.**

| # | Verdict | Evidence |
|---|---|---|
| 1. Latch | **FAIL** | RAM-only, correctly named, never cleared/persisted, but [`ExternalRtc.h:34–36`](/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s4-r1/src/time/ExternalRtc.h:34) uses a plain `bool` across the RTC worker and future I21 application-thread reads. |
| 2. Registration and order | **FAIL** | The required boot-source read and initial set precede registration at [`Generalized-Core-Counter.cpp:658–664`](/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s4-r1/src/Generalized-Core-Counter.cpp:658). However, a fault between lines 658 and 664 is lost. |
| 3. Callback | **PASS** | It sets only for `CLOCK_SOURCE_EXTERNAL_FAILURE`, using `RtcEvents::isSet`, and performs no logging, flush, persistence, or trust logic. Device OS `EnumFlags::isSet` correctly recognizes bit `0x02` in a combined mask. |
| 4. Scope/linkage | **PASS** | Only the two intended `src/` files changed. No payload, retained/persisted layout, Clock/write-back, or S1/S1b/S2/S3 behavior changed. `nm`/objdump confirm linkage and registration. |
| 5. Tests | **FAIL** | All tests pass and all supplied mutants are caught, but the suite permits both production defects above and does not execute a combined event mask. |
| 6. Budget/build | **PASS** | `src/` is +7/−0 net. Fresh Device OS 6.6.0 Boron build passed at **151084 / 1114 / 2256** text/data/bss. |
| 7. Boot/I21 hand-off | **PASS with blockers above** | S1 bind at `Generalized-Core-Counter.cpp:295–296` precedes the snapshot and registration. No additional scoped behavior change found. I21 must consume the latch in its trust decisions and own revocation, persistence/continuity policy, flushing, and any contract change. |

### Findings

1. **High — cross-thread data race**  
   `src/time/ExternalRtc.h:34–36`; Device OS `am18x5.cpp:1407–1472`.

   The worker thread writes the flag while I21 will read it on the application thread. Cortex-M4 byte loads/stores do not tear, and one-loop staleness would be acceptable, but a plain `bool` still constitutes a C++ data race and undefined behavior. `volatile` does not provide C++ inter-thread synchronization and is not an adequate fix.

   Smallest standards-correct fix: include `<atomic>` and change the function-local static/reference to `std::atomic_bool`. The existing assignment syntax remains valid; the host test’s `bool&` should become `auto&`. This adds one net `src/` line, taking the change to **+8**.

2. **Medium — read→registration fault window**  
   `src/Generalized-Core-Counter.cpp:658–664`; Device OS `am18x5.cpp:620–626,630–650`.

   Device OS registration reaches `onOscillatorEvent()` and clears `OF|ACF` at line 646 before enabling the handler. A fault already present at boot is retained correctly: line 658 reads `OMODE`, so RC fallback sets the latch before registration clears OF.

   But if the crystal fails after line 658 reads EXTERNAL and before line 664 registers:

   - FOS switches the chip to RC and sets OF.
   - Registration clears OF without delivering an event.
   - The earlier source snapshot was healthy.
   - The process-lifetime latch remains false.

   Smallest fix: preserve the required pre-registration read, register, then immediately re-read `clockSource()` and set the latch if it is INTERNAL. Registration clears OF/ACF, not the read-only OMODE state, so the second read catches this window. This adds one more line. Together with the atomic fix, the result is **+9 net**, within +10. It requires adjusting Amendment 1’s “rather than adding a second read” wording while retaining ruling 4’s required first-read-before-registration order.

3. **Medium — tests do not protect the missing guarantees**  
   [`external_rtc_test.cpp:121–131`](/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s4-r1/tests/external_rtc_test.cpp:121), [`external_rtc_structural_test.py:56–70`](/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/7fe8279d-ef98-4f2e-a0e5-e3b17bab0abd/scratchpad/wt-s4-r1/tests/external_rtc_structural_test.py:56).

   Add checks requiring atomic storage and the order `first source read → initial set → registration → second source read`. Add a combined `CALIBRATION_FAILURE | CLOCK_SOURCE_EXTERNAL_FAILURE` runtime vector; the current stub lacks bitwise combination support. These are test-only changes and do not affect the `src/` budget.

### Verification record

- SHA checked first and matched exactly.
- CI discovery: **121/121** — 67 `.sh` via zsh, 54 `.py` via python3.
- Clean-export thermal structural test: passed.
- Normal structural test caught all nine supplied S4 mutations.
- Independent named audit: **7/7 caught**—five supplied mutations plus two new mutations:
  - latch initialized true;
  - calibration failure also sets the latch.
- Scoped `git diff --check e21a4a4 HEAD -- src tests`: passed.
- Tracked worktree remained unchanged.
- Linkage:
  - callback: weak symbol at `0x000b5358`;
  - RAM latch: one-byte BSS symbol at `0x2003d728`;
  - setup passes the callback to `hal_exrtc_event_handler_add`.
- Scratch-only repair probe—atomic plus post-registration recheck—passed the helper/structural test and Device OS build at **151132 / 1114 / 2256**, confirming the **+9** repair compiles.

Model/reasoning: **gpt-5.6-sol, xhigh**.  
Interpreter: **Python 3.14.6; zsh 5.9 (arm64-apple-darwin26.0); GNU Arm Embedded 10.2.1**.  
Build: **Device OS 6.6.0, Boron, buildscripts 1.17.2, fresh `BUILD_PATH_BASE` passed as a make argument**.

No hardware was available or exercised. Physical healthy-crystal/RC-fallback boot behavior and induced oscillator-fault delivery remain **NOT VERIFIED on hardware**.
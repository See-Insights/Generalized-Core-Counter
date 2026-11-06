acbace551f3d2c15c134c7da55a4fb80979da591

**NOT VERIFIED — three blocking findings.**

Model/reasoning actually used: Codex · `gpt-5.6-sol` · `xhigh`  
Interpreter line: **109/109 (sh via zsh, py via python3)**

## Findings

1. **Blocking — the `close==24` boundary reopen leaves a stale `occupancyStartMs` on a new session.** The boundary first closes the old session at [State_Report.cpp:54](/private/tmp/gcc-step0/c2/src/state/State_Report.cpp:54). The close clears `pieceUntrusted` but not `occupancyStartMs` at [State_Common.h:453](/private/tmp/gcc-step0/c2/src/state/State_Common.h:453). The always-open reopen then sets only `occupied` and the wall start at [State_Report.cpp:77](/private/tmp/gcc-step0/c2/src/state/State_Report.cpp:77). Therefore the new session inherits the previous piece’s monotonic start. It is currently inert because the cleared flag is false, but the dispatch explicitly makes any stale piece field on a new/reopened session blocking. `State_Error` itself has no open path ([State_Error.cpp:115](/private/tmp/gcc-step0/c2/src/state/State_Error.cpp:115)); boot restoration correctly starts with zero-initialized RAM and assigns the flag at [Generalized-Core-Counter.cpp:503](/private/tmp/gcc-step0/c2/src/Generalized-Core-Counter.cpp:503).

2. **Blocking — an untrusted millis piece still depends on wall-start validation after trust returns.** The correct millis duration is calculated at [State_Common.h:400](/private/tmp/gcc-step0/c2/src/state/State_Common.h:400), but:

   - `start == 0` still makes the piece invalid at [State_Common.h:398](/private/tmp/gcc-step0/c2/src/state/State_Common.h:398).
   - If synchronization moves the wall clock backward, `start > now + 5` invalidates the piece at [State_Common.h:416](/private/tmp/gcc-step0/c2/src/state/State_Common.h:416).

   A focused test using the real extracted helper opened untrusted, measured 360 seconds on millis, restored trust with a ten-hour backward correction, and expected 360 seconds ([scratch test:241](/private/tmp/gcc-step0/c2/build-tmp/wo-2026-11-03-006-stage7-r1/review-tree/tests/untrusted_piece_c2_backward_test.sh:241)). It failed with total `0`; the session was cleared as an anomaly. The committed test only exercises a forward ten-hour step at [untrusted_piece_c2_test.sh:227](/private/tmp/gcc-step0/c2/tests/untrusted_piece_c2_test.sh:227), so it misses this defect.

   These wall checks are appropriate for the trusted wall-subtraction path, but not for a piece the WO says must stay wholly on millis even after trust returns ([WO:19](/private/tmp/gcc-step0/c2/docs/work-orders/WO-2026-11-03-006-item19-c2-untrusted-split.md:19)). The existing one-day session/daily-total validation at [State_Common.h:420](/private/tmp/gcc-step0/c2/src/state/State_Common.h:420) remains appropriate.

3. **Blocking — the binding bench acceptance numbers do not match the specified timeline or the close report.** Dev-14 inherits product `setting1=300000 ms`; its device settings have no sensor override ([migration report:54](/private/tmp/gcc-step0/c2/docs/work-orders/WO-2026-10-29-001-step0-report.md:54), [config check:163](/private/tmp/gcc-step0/c2/docs/work-orders/WO-2026-10-08-002-step0-report.md:163)).

   From 10:35:21 to 14:00, sessions begin every 1,200 seconds. Ten full 570-second pieces close by 13:44:51; the eleventh opens at 13:55:21 and would close at 14:04:51.

   - Before processing the boundary: control is `10×570 = 5700 s = 95.0 min`; reset fault is `301 + 9×570 = 5431 s = 90.516 min`. The payload truncates `/60`, so these display as `95` and `90` ([PIRSensor.cpp:25](/private/tmp/gcc-step0/c2/src/sensors/PIRSensor.cpp:25)).
   - The requested close report occurs after trust is restored past the boundary. Whether the eleventh piece closes naturally or via the boundary call, it receives the full 570 seconds because the piece formula ignores `closeAt` ([State_Common.h:400](/private/tmp/gcc-step0/c2/src/state/State_Common.h:400)). Thus nominal close-report totals are control `6270 s = 104.5 min` → payload `104`, and fault `6001 s = 100.017 min` → payload `100`, plus possible wake overhead.
   - `97.21 min` is only `47.5%` of the complete 12,279-second window. The exact phase contains 5,979 seconds—99.65 minutes—of physical session time through 14:00 because the eleventh piece is already 279 seconds old.

   The WO’s `96–98` control/fault band at [WO:28](/private/tmp/gcc-step0/c2/docs/work-orders/WO-2026-11-03-006-item19-c2-untrusted-split.md:28) and [WO:36](/private/tmp/gcc-step0/c2/docs/work-orders/WO-2026-11-03-006-item19-c2-untrusted-split.md:36) therefore cannot be the acceptance criterion. Chip should use the checkpoint-specific figures above.

4. **Note — piece fields and ordinary open paths are otherwise correct and RAM-only.** `occupancyStartMs` and `pieceUntrusted` are default-initialized `SessionState` members at [StateMachine.h:70](/private/tmp/gcc-step0/c2/src/state/StateMachine.h:70), and the global is ordinary BSS RAM at [Generalized-Core-Counter.cpp:149](/private/tmp/gcc-step0/c2/src/Generalized-Core-Counter.cpp:149). Both awake and PIR-wake starts assign them from `millis()` and current trust at [State_Modes.cpp:68](/private/tmp/gcc-step0/c2/src/state/State_Modes.cpp:68) and [State_Modes.cpp:220](/private/tmp/gcc-step0/c2/src/state/State_Modes.cpp:220). No persisted/retained owner file differs from `d48dc27`; `persisted_layout_preservation_test.py` and `retained_boot_structural_test.py` passed. Source delta is +15 net, within the +16 budget.

5. **Note — close arithmetic/report behavior is otherwise as specified.** B2 re-arms only a trusted-open piece that later loses trust at [State_Common.h:367](/private/tmp/gcc-step0/c2/src/state/State_Common.h:367). A successful piece close clears the flag at [State_Common.h:453](/private/tmp/gcc-step0/c2/src/state/State_Common.h:453). Idle/update/wake callers use the same result, log, `occupancyChangeTriggered`, and reporting transition as a trusted close at [State_Idle.cpp:48](/private/tmp/gcc-step0/c2/src/state/State_Idle.cpp:48), [State_Modes.cpp:135](/private/tmp/gcc-step0/c2/src/state/State_Modes.cpp:135), and [State_Modes.cpp:158](/private/tmp/gcc-step0/c2/src/state/State_Modes.cpp:158). Quiet modes continue not to force an immediate report, exactly as for trusted closes.

   Unsigned subtraction handles a normal rollover and `lastOccupancyEvent < occupancyStartMs` correctly when the piece is shorter than one full `uint32_t` millis period. A piece open for at least 2³² ms—about 49.7 days—aliases modulo 2³²; for example, 49.7 days plus 570 seconds is indistinguishable from 570 seconds and can evade the one-day validation. This is a finite monotonic-clock horizon, not protection for arbitrarily long continuous pieces.

6. **Note — boot trust and straddler behavior.** The trust read at setup is reliable under current semantics: `cachedTimeSyncedLastMs` starts at zero ([Clock.cpp:131](/private/tmp/gcc-step0/c2/src/time/Clock.cpp:131)), and trust requires both valid time and a confirmed current-boot sync ([Clock.cpp:454](/private/tmp/gcc-step0/c2/src/time/Clock.cpp:454)). RTC-seeded `Time.isValid()` alone never counts as trusted. Consequently every persisted open session currently boots as an untrusted piece; item 21 would be needed to make a trusted boot reachable.

   With no post-boot PIR, the sentinel sets `lastOccupancyEvent` to the first predicate time at [State_Common.h:335](/private/tmp/gcc-step0/c2/src/state/State_Common.h:335). Straddler credit is therefore:

   `first-predicate millis + debounce`

   Nominal first evaluation at 1 second with Dev-14’s 300-second setting gives about **301 seconds**, not 236. There is no C2-specific hard bound of exactly 301 because setup/first-predicate latency is included. A PIR before expiry instead makes the result `last PIR since boot + debounce`. This is entirely post-boot time—not an added pre-boot fallback—but it does credit one full post-boot debounce even when no PIR occurs.

   The committed 236-second scenario uses a 60-second debounce plus PIRs continuing to 176 seconds after boot ([untrusted_piece_c2_test.sh:200](/private/tmp/gcc-step0/c2/tests/untrusted_piece_c2_test.sh:200)); that is not Dev-14’s bench configuration. Item 15 does not alter the result when the persisted open record survives. If it loses `occupied`, no straddler exists and credit is zero, as documented at [Step 0:103](/private/tmp/gcc-step0/c2/docs/work-orders/WO-2026-11-03-001-step0-report.md:103).

7. **Note — LED gate.** The new gate at [State_Modes.cpp:155](/private/tmp/gcc-step0/c2/src/state/State_Modes.cpp:155) closes an occupied untrusted piece on the millis predicate even when the LED is low. The tracked mode harness verifies both that close and the unchanged trusted LED-low non-close at [occupancy_report_by_mode_test.cpp:456](/private/tmp/gcc-step0/c2/tests/occupancy_report_by_mode_test.cpp:456). The trusted-piece LED-low gap remains exactly as in v48/C1; it does not block C2 because the C2 untrusted flag bypasses the LED. The S-6 fixture is byte-identical to v48 and C1, hash `914e77f7653dbe87ee57ed2a425ef250faf433de`.

8. **Note — boundary overshoot and B3.** If trust returns mid-piece and a due boundary closes it before debounce expiry, the formula credits through `last PIR + debounce`, so overshoot is bounded by one debounce. The credit is added to the aggregate current at close and published before daily reset at [State_Report.cpp:54](/private/tmp/gcc-step0/c2/src/state/State_Report.cpp:54). This is the Amendment 1 assignment rule.

   An untrusted boot deliberately gets no B3 anchor at [Generalized-Core-Counter.cpp:503](/private/tmp/gcc-step0/c2/src/Generalized-Core-Counter.cpp:503). The cap remains at [State_Common.h:383](/private/tmp/gcc-step0/c2/src/state/State_Common.h:383) for a future trusted boot and survives B2 retries. Under today’s current-boot trust definition, that trusted-boot case is not reachable.

9. **Note — tests and mutations.** The C2 harness extracts the real predicate, close helper, and `updateOccupancyState()` at [untrusted_piece_c2_test.sh:138](/private/tmp/gcc-step0/c2/tests/untrusted_piece_c2_test.sh:138); open and boot sites are structural exact-text checks rather than runtime execution at [untrusted_piece_c2_test.sh:31](/private/tmp/gcc-step0/c2/tests/untrusted_piece_c2_test.sh:31).

   No existing B2/B3 assertion changed. Tests 7, 8, and 11 retain the same before/after expectations; their setup now explicitly says the piece opened trusted by setting `pieceUntrusted=false` ([occupancy_session_restart_test.sh:287](/private/tmp/gcc-step0/c2/tests/occupancy_session_restart_test.sh:287)). `occupancy_report_by_mode_test.cpp` likewise changes no existing assertion; it only adds the LED-low scenario.

   Seven independently applied scratch mutants were rejected:

   - keep B2 re-arm → failed expected 5,700-second replay;
   - wall-span credit → failed fault replay;
   - persisted-wall-start credit → failed control replay;
   - boot debounce fallback → failed expected 5,431 seconds;
   - all trusted pieces on millis → failed trusted 460-second expectation;
   - drop reporting transition → failed expected 10 reports;
   - omit fresh awake start → failed the two-open-site structural check.

10. **Note — build and production linkage verified.** Fresh Boron Device OS 6.6.0 build passed with text/data/bss **152036 / 1114 / 2208**. ELF `nm -C` gives:

   - `closeOccupancySessionSafely` `W 0x000c1d38`
   - `debounceRemainingMs` `W 0x000c2520`
   - `updateOccupancyState` `T 0x000c25bc`
   - `processSleepWakeEvent` `T 0x000c2780`
   - `handleReportingState` `T 0x000c2a60`

   Objdump shows direct calls to the close helper from Idle `0xc2086`, `updateOccupancyState` `0xc25e2`, sleep-wake `0xc27b4`, and boundary/reporting `0xc2ad6`. `State_Modes.o` has `U session`; `Generalized-Core-Counter.o` defines `B session`. Both C2 log strings are present in the ELF.

11. **Note — bench remains pending.** The committed diff does not claim Dev-14 passed: the Stage 6 report says “bench not done and not claimed” at [stage6 report:3](/private/tmp/gcc-step0/c2/docs/work-orders/WO-2026-11-03-006-stage6-copilot-report.md:3) and repeats that it is Chip’s step at [stage6 report:44](/private/tmp/gcc-step0/c2/docs/work-orders/WO-2026-11-03-006-stage6-copilot-report.md:44). Network access was prohibited, so the remote PR body itself was not inspected. Before bench/merge it must state “pending” and use the corrected checkpoint/close-report figures from finding 3.
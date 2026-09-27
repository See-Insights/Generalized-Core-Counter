_Codex Stage 7 verdict for decision 10, verbatim. Codex CLI 0.154.0, `gpt-6-astra`, reasoning ultra (confirmed in the run header). Dispatch: `WO-2026-09-25-001-stage7-decision10-codex-dispatch.md`. The candidate binary is retained outside the repository (SHA-256 below); it is not for flashing._

**NOT VERIFIED.** The ACK restoration passes, but reset durability fails within the specified fault model.

- **(a) PASS:** Exactly six queue publishes, all with `WITH_ACK`: [Generalized-Core-Counter.cpp](</Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/Generalized-Core-Counter.cpp:2061>) lines 2061, 2283, 2323, 2358, 2471; [PowerDiagnostics.cpp:439](</Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/power/PowerDiagnostics.cpp:439>). No additional sites found.
- **(b) FAIL:** A connected event can remain RAM-only. Sending removes it from `ramQueue` into `curEvent`; the reset handler flushes only `ramQueue` ([queue implementation:313,402](</Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/lib/PublishQueuePosixRK/src/PublishQueuePosixRK.cpp:313>)). The reproduction invoked the actual reset callback while awaiting ACK: **zero files persisted; fresh boot found zero events; next connection made zero retries**. The application cannot reconstruct the original report after advancing `lastReport` and clearing its hourly count ([State_Report.cpp:109](</Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/state/State_Report.cpp:109>)).
- **(c) PASS:** All **8/8 requested mutations** detected: six individual ACK removals, restored `canSleepGate`, and a seventh publish without ACK. Baseline passed before and after ([structural test:102](</Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/tests/publish_with_ack_structural_test.py:102>)).
- **(d) PASS:** **44/44 (sh via zsh, py via python3)** — 22 shell scripts and 22 Python scripts; no skips.
- **(e) PASS:** One cloud build from the repository-root copy using `particle compile boron . --target 6.4.1 --saveTo …`. Actual binary disassembly confirms `0x08 | 0x01 = 0x09` reaches all six publish sites and survives forwarding through the compiled queue libraries. Call addresses: `0xb58f0`, `0xb5c6e`, `0xb5d68`, `0xb5e1c`, `0xb5e96`, `0xbaef0`.

Host reproductions used real application/queue source with mocked hardware and cloud interfaces. Both intermittent modes immediately reached Sleep; one never-ACK event raised alert 43 at **30.000 s**, followed by sleep at **32.500 s** with modeled 2.5 s teardown. At queue depth 113, alert 43 occurred at **120.000 s**, but sleep at **122.500 s**: the [120 s cap](</Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/state/State_Sleep.cpp:156>) bounds the gate, not subsequent teardown.

Disk-backed events survived failed connection, reset, and simulated hibernate/reboot, retried identical payloads with `0x09`, and were deleted only after success. RAM-only reset loss also reproduced before sending and after failure notification but before the next queue loop.

[Retained binary](</private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/96cc6b3e-850e-40a5-bd3f-f119971b47ff/scratchpad/codex-s7-d10/decision10-boron-6.4.1.bin>) SHA-256:
```text
32d250330cca0c47760dc8cb17c51887370cb2ef912306832e5bdfd0442a3985
```

No repository edits performed; source/test/library hashes remained unchanged. Temporary verification artifacts removed. **Model: gpt-6-astra; reasoning: ultra.**
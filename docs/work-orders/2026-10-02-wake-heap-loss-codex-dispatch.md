AGENT: Codex · MODEL: gpt-6-astra · REASONING: high · via codex exec
Per docs/AI_DEVELOPMENT_WORKFLOW.md: Roles table (Codex: investigation), guardrail 1 (history first), and the guardrail on checking facts against their source.
AUTHORIZATION SCOPE: read-only code and Device OS 6.4.1 source (`~/.particle/toolchains/deviceOS/6.4.1/`); draft (not apply) a bench-only diagnostic / Not authorized: edits, commits, builds, device actions, and deleting anything.

**Repository:** `Generalized-Core-Counter`, `main` at `43b8a69`, which contains v33-HourlyWhileOccupied (`691b6c7`, product 33): the firmware Dev-09 was running when the evidence below was recorded. The working tree has one unrelated uncommitted docs edit (`docs/RECOVERY_PLAN_2026-09-26.md`); ignore it.

Plain goal: find what is allocated on every wake from sleep and not freed.

Evidence (Dev-09, v33, 2 Oct, from the report payloads): fh fell from 73,848 to 51,952 bytes over 3 hours while occupied. Per stretch: +70 wake cycles → −8,824 B (−126 B/cycle); +42 → −5,904 B (−141); +50 → −7,048 B (−141); +1 → +616 B. The loss tracks cyc (awake periods ending in sleep), not the number of reports. lfb follows fh closely, so it's loss, not fragmentation.

Investigate:

1. **The wake path, step by step:** everything that runs from the return of `System.sleep()` (ULP) through the post-wake handling (PowerDiag … post-wake, post-refreshInputProfile, ChargeDiag, occupancy handling, LoopStage, the Sleep->Sleep / Sleep->Report decision), up to the next sleep call. List every allocation (new, malloc, String, containers growing, Log with heap formatting, Particle APIs that allocate, the SystemSleepConfiguration objects, wake-reason structures) and whether each is freed. Cite file:line.
2. **Diagnostics that grow without being published:** for example, a power-diagnostics batch or ring buffer that keeps filling now that pdiag isn't published in release builds. The PowerDiag[N] index keeps rising on every wake. Check what that index feeds.
3. **Device OS 6.4.1:** does the ULP `System.sleep()` path, or re-arming the wake sources, allocate on each call? Check the source, and any known Particle issue.
4. **History:** when did each suspect enter the code (`git log -S`), and does the per-wake loss line up with any of those changes?
5. **A bench-only diagnostic, drafted but not applied (≤ 5 lines):** log `System.freeMemory()` at post-wake and at pre-sleep on every cycle, plus a way to tell a sensor wake that goes straight back to sleep apart from a timer wake that reports. That lets the bench pin down which part of the cycle loses the bytes.

**Output:**
- ranked suspects: file:line, the evidence for each, an estimate of bytes per wake, and how confident you are;
- the bench diagnostic;
- a 5-minute bench protocol (repeated sensor wakes, reading the per-cycle change).

No fix yet. Report the model and reasoning level used.

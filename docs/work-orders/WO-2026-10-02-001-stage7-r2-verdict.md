**C: VERIFIED. Overall v32: VERIFIED WITH NOTES**, carrying A, B, D and E from round 1. A’s accepted equal-hours limitation remains: equal opening/closing hours of 21, 22 or 23 disable this failsafe.

| Check | Result and evidence |
|---|---|
| **1. Round-1 reproduction** | **PASS.** Independent compiled extraction of the real code: off at 0 s → searching at 50 s → cloud acquisition at 70 s fires stage 1 at **70,000 ms**, matching v31. Both accumulate **70,000 cellular ms**. Extended sequence fires stage 2 at **190,000 ms** in both. |
| **2. v31 equivalence** | **PASS.** Compared current code directly with `71f955e`: **59,140 sequences / 1,134,670 ticks**, including every off/search ordering of lengths 1–12, three timing cadences, network transitions and cloud re-entry. Recovery firing times, all three accounting counters, timers and summary values match. |
| **3. Labels and assignment ordering** | **PASS.** Actual failure-summary and diagnostic formats produce `last=MODEM_OFF` and `phase=MODEM_OFF`. `ConnPhase:` retains identical event timing and numeric fields; only labels can differ. It reads the previous phase before assignment and emits no off/search-only transition. `ConnDiag` remains trace-gated. |
| **4. Mutation** | **PASS.** Restoring the raw comparison fails my independent harness. The repository mutation test also catches it: reproduction stage 1 does not fire at 70 s. |
| **5. Suite** | **PASS — 56/56:** 28 shell tests through **zsh**, 28 Python tests through **python3**. Frozen-item tests pass. `publish_with_ack_structural_test.py` is byte-identical to base and green. |
| **6. Build** | **PASS.** Local Boron / Device OS 6.4.1 release build after successful `make clean-user`. `strings` finds `MODEM_OFF`. |
| **7. Size** | **PASS.** Round-1 counting rule: C **+7 net source-code lines ≤8**; A–E total **+18**, within about 45. |

Every phase reader was retraced: labels, accounting, boundary detection, `phaseStartMs`, `cloudAcquireElapsedMs`, recovery thresholds, and success/timeout/exhausted summaries. Normalization makes arbitrary off/search substitutions equivalent for timing and accounting; the unconditional assignment preserves the raw diagnostic label. Success summaries retain `CONNECTED`.

| Release artifact | Bytes |
|---|---:|
| Text | **150628** |
| Data | **1090** |
| BSS | **2204** |
| `.bin` | **151722** |

**Preservation confirmed:** final inventory/hash comparison across **125,052 entries** found **0 added, 0 removed, 0 changed**, including `.git`, existing build outputs and all **120,584 archive files**. The working tree is byte-identical. Only `build-tmp/wo20261002-stage7-r2/` was created and removed; nothing outside it was created or deleted. No network access or lasting edits.

**Model:** gpt-6-astra. **Reasoning:** high.
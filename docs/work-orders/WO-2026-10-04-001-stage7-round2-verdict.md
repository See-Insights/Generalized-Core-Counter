**B overall: VERIFIED WITH NOTES.** No defect found in B’s restart-credit implementation. Checks 1–8, 10 and 11 pass. Check 9’s exception list is incomplete; this is the requested report-only note about the accepted limitation.

| Check | Verdict | Evidence |
|---|---|---|
| **1. Round-1 reproduction** | **PASS — VERIFIED** | Extracted production close credits **4,300 s**, both without a post-boot report and with the untrusted report at **41,050**. Over-credit: **300 s**. |
| **2. Boot-time ceiling and anchor** | **PASS — VERIFIED** | Start 1,000, boot anchor 5,000: boots at 5,100 / 5,300 / 5,500 credit **4,100 / 4,300 / 4,300 s**. Covers both sides and equality at the cap. |
| **3. Untrusted clock and callers** | **PASS — VERIFIED** | Extracted production blocks from Idle, Modes and Sleep each survived **12 untrusted close attempts**: occupied/start/total preserved, debounce re-armed, no reporting transition, unoccupied event or `OccAnom`. Subsequent trusted close credits 4,300 s and reports once. |
| **4. Previous boot’s `millis()`** | **PASS — VERIFIED** | Extracted setup statements clear an old persisted event value; Idle/Modes re-arm from this boot’s `millis()`. |
| **5. Long power-off bound** | **PASS — VERIFIED** | **10-hour and 10-day** outages, each with and without post-boot reports, credit 4,300 s: **300 s excess**. |
| **6. RAM-only field and zero sentinel** | **PASS — VERIFIED** | Persistence files unchanged; persisted-layout test passes. ELF places `session` in **`.bss`**, size 56 bytes; DWARF places the anchor at offset 48. Start-zero probe remains invalid. |
| **7. Daily-boundary close** | **PASS — VERIFIED** | Production close preserves `min(boundary, bootEpoch, anchor + debounce)`. Boundaries 4,900 / 5,200 / 5,400 / 42,000 credit **3,900 / 4,200 / 4,300 / 4,300 s**. `DailyBoundary::check()` remains trusted-clock gated. |
| **8. Regression-test mutations** | **PASS — VERIFIED** | Running **the new case alone** passes unchanged. Reading `lastReport` at close time and removing the cap each fail its `sessionSeconds == 4300` assertion. Both scratch mutations restored byte-identically in place. |
| **9. Known-limitation fact check** | **FAIL — NOT VERIFIED as written; report-only** | Trusted-clock requirement is correct. The exceptions are incomplete, and a reset before trust expires is conditional. Details below. |
| **10. Suite and build** | **PASS — VERIFIED** | **65/65**: 35 shell tests through **zsh**, 30 Python tests through **python3**. Clean local Boron 6.4.1 release build succeeds; ELF evidence below. |
| **11. Budget** | **PASS — VERIFIED** | **+15 net `src/` lines / ≤15**. Excludes blanks/comments; includes declarations, directives and braces. Nothing compressed. |

The [failsafe supervisor](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/Generalized-Core-Counter.cpp:2562) additionally defers for disconnected mode, firmware updates, missing connection history, unknown openness, cooldown/jitter, and an active connection attempt. Its age threshold starts at the later of `lastConnection` and today’s opening—not the last clock sync. [Clock openness](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/time/Clock.cpp:567) requires trust and valid configuration. Alert 40 also requires trust, as documented.

Smallest proposed documentation correction to the [fact check](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/docs/work-orders/WO-2026-10-04-001-pre-step6-fixes.md:77), **not applied; zero additional `src/` lines**:

```diff
- Trust lasts 24 h after the last sync, so the reset fires at about 3 h, well before trust lapses. That's why the bound holds.
+ Trust lasts 24 h after the last sync. The failsafe's 3-hour age starts at the later of lastConnection and today's opening; a reset before trust expires is conditional on all recovery gates.
@@
-  - closed hours.
+  - closed or unknown openness (untrusted clock or invalid configuration);
+  - DISCONNECTED mode, firmware-update state, or updates pending;
+  - invalid time, no lastConnection, or the 3-hour age threshold not reached;
+  - recovery cooldown/jitter or an active connection attempt within budget.
```

| Local release build | text | data | bss |
|---|---:|---:|---:|
| Round 1 | 150788 | 1090 | 2188 |
| **Round 2** | **150844** | **1090** | **2196** |
| Change | +56 | 0 | +8 |

`strings` finds **`v37-PreStep6Fixes`**. `nm` confirms **`setup` at `0x000b6a78`**, **`closeOccupancySessionSafely` at `0x000c1758`**, and its callers. The supplied cloud-build result—flash **152070**, RAM **3290**—was not rerun.

B’s budget breakdown: setup **+3**, SessionState **+1**, common helper **+17**, Idle **−3**, Modes **−3**, Sleep **0** = **+15**.

**Preservation confirmed:** SHA-256 comparison of **124,128 files** found zero changes, additions or removals outside scratch. After cleanup, file/directory inventory and Git status match the baseline. Only `build-tmp/wo20261004-001-stage7r2/` was removed; `build-tmp/` and `build-tmp/connectivity-archive/` remain untouched. Nothing outside the authorized scratch directory was created or deleted.

**Model used:** `gpt-6-astra`, **high** reasoning.
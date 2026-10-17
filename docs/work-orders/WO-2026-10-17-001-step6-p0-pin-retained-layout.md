# WO-2026-10-17-001: Step 6 P-0 — pin the retained layout before the code moves

**Author:** architect, 2026-10-17 · **Status:** ready (citation re-check done 2026-10-17, below)
**Branch:** `wo/2026-10-17-001-step6-p0-retained-layout`, from main
**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §3 Stages 6–8; §12.2–12.4; §13 (b), (c), (g), (j)
**Why:** P-1 Stage 6 stopped on its retained-address check (17 Oct). Moving code out of main reordered four retained variables (`bootStormCount`, `bootStormWindowStart`, `appBreadcrumb`, `appBreadcrumbMs`), because GCC lays them out by first use. Device OS keeps retained RAM across an application OTA and checks only one signature word (`wiring/src/user.cpp:202-204`, unchanged in 6.6.0), so the shifted values would be read as valid on the first boot after the update. Six more moves would keep doing this.

## Plain goal

Fix the layout of the unvalidated retained variables in main **once**, so that no later code move can shift them, and so that a layout change, if one ever happens, resets them cleanly instead of reading wrong bytes. No other behavior change.

## Design (do not widen)

- One `retained` struct in `Generalized-Core-Counter.cpp` holding today's unvalidated retained variables there (`:491-499`): `bootStormCount`, `bootInProgress`, `bootStormWindowStart`, `bootStormLastResetReason`, `bootStormLastResetReasonData`, `bootStormTripCount`, `bootStormAlertPending`, `appBreadcrumb`, `appBreadcrumbMs`. Member order then fixes the layout, whatever uses the members.
- A `magic` (32-bit) and a `version` (8-bit) as the first members. If either is wrong at the first read, set every member to today's initial value (`0`, `false`, `BREADCRUMB_NONE`), then write the magic and the version.
- **The check runs before the first read of any member.** Find that first read (setup's boot-storm block around `:855–880`, the startup snapshot, and any `STARTUP()` or constructor-time user) and put the check ahead of all of them.
- **Keep every existing name, so the 40 uses don't change.** For example, `uint8_t &bootStormCount = retainedBoot.bootStormCount;`. References are an acceptable way to do this; a different technique is fine if it is smaller.
- **Out of scope:** `retainedLoopForensics` (it already has a magic and version), the hibernate fields (guarded), `lastTrend*` (sentinel), and the report-only counters in other files.

## Expected one-time effect

The first boot after this OTA finds no magic and resets these values once: the breadcrumb pair reads `BREADCRUMB_NONE`/0 on that boot, and the boot-storm counters start at zero. That is the same as a fresh device. **It must not raise a boot-storm alert or the boot-storm sleep.**

## Budget

**`src/` ≤ +20 net, hard** (estimate about +18). One commit. Tests: guideline +100.

## Tests and checks

- Host test: with the magic wrong, the first read leaves every member at its initial value and writes the magic and version; with it right, the values are kept. A structural test that the check precedes the first member read in `setup()`.
- `nm`: the whole struct is one `.backup` symbol; record its address. From now on, P-moves must leave it, and any other unvalidated retained symbol, at the same address.
- Golden payload test unchanged; suite `N/N`; ELF text change explained by symbol.

## Agents and models

- **Stage 6:** Copilot, Sonnet tier, **high**. It's the boot path and retained memory.
- **Controller (before Stage 7, §13 (g)):** reproduce the build, the `nm` check and the suite.
- **Stage 7:** Codex `gpt-5.6-sol`, **high**. Checks: the member list is complete for main's unvalidated retained variables; the check really precedes every read; the reset values equal today's initializers; no boot-storm path can trip on the post-OTA boot; no other retained symbol moved.

## Bench

Ships in v45 with P-1…P-6. The v45 bench soak adds one check: on the first boot after the OTA, the status shows the breadcrumb reset (`appBreadcrumb` 0), there is no boot-storm alert, and the second boot keeps the values.

## Closing record

Budget versus actual; the struct's address; ELF delta by symbol.

## Claude Code's citation re-check (2026-10-17, main `5a28885`; the `src/` tree equals `71e346f`'s)

**The citations hold.**
- The nine variables are at `Generalized-Core-Counter.cpp:491–499` (`bootStormCount` `:491`, `bootInProgress` `:492`, `bootStormWindowStart` `:493`, `bootStormLastResetReason` `:494`, `…ReasonData` `:495`, `bootStormTripCount` `:496`, `bootStormAlertPending` `:497`, `appBreadcrumb` `:498`, `appBreadcrumbMs` `:499`); `retainedLoopForensics` is `:500`.
- The boot-storm block is `:815–882` (`bootInProgress` read `:818`, count `:849`, window `:856–869`, trip condition `:873`, holdoff sleep `:882`).
- `wiring/src/user.cpp:202–204` checks only `__backup_sram_signature`, and is identical in 6.4.1 and 6.6.0.
- **Uses:** 42 lines in main, of which 9 are the definitions (the WO's "40" is right to within the declarations). **No other source file uses any of the nine**; the only other hit is a comment string in `tests/clock_resync_wiring_test.py:679`.

**The first reads come before the boot-storm block** (the WO says "find them"):
- `setup()` starts at `:785`. Its first call, `waitForDebugSerialIfConnected()` (`:767`), touches none of the nine.
- **`ensureRetainedLoopForensicsInitialized()` (`:788`) can read `appBreadcrumb`**: its reset path reads it at `:616`, and the refresh at `:634`. Then `:792–793` read the breadcrumb pair, `:818` reads `bootInProgress`.
- **So the magic check must be the first statement of `setup()`**, or at least ahead of `:788`. `setAppBreadcrumb` (`:735`) writes the pair and calls the refresh, but nothing calls it before `setup()` (no `STARTUP()` and no static constructor uses any of the nine).

**Observations for Stage 6 (INF, to be confirmed by the `nm` check):**
- Today the nine variables span `0x2003f41c–0x2003f43b` (32 B, between `retainedLoopForensics` at `0x2003f400` and `thrashTripCount` at `0x2003f43c`). A struct with a 4-byte magic and a 1-byte version has about 28 B of members but 8-byte alignment (`time_t`), so it may start at `0x2003f420` and **shift every later retained symbol by a few bytes**, including `__backup_sram_signature` (`0x2003f494` today). That is acceptable for the later symbols (report-only counters, guarded or sentinel-checked values), but report it, with a classification of each moved symbol. If the signature word moves, Device OS itself re-initializes all retained RAM on the first boot after this OTA, which is a clean one-time reset. The magic is still needed so that later moves, which don't change the total size, can't shift the nine unseen.
- Stage 7's check "no other retained symbol moved" cannot hold if the struct's size changes the span; if it does, the report must say which symbols moved and why that is safe. A member order that keeps the span at 32 B from `0x2003f41c` is worth trying, but it needs 8-byte alignment for `time_t`, so don't force it.


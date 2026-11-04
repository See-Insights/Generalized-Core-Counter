# WO-2026-11-04-002: v49 binding-delete guard — delete any stored Device OS RTC binding at boot

**Author:** Claude Code, 4 Nov 2026, DRAFT for the architect's review · **Status:** DRAFT; not dispatched
**Branch:** `wo/2026-11-04-002-binding-delete-guard`, from `main`
**Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §3 Stages 5-8; §4 (SHA rule); §5 (boot-time hardware ownership and time: Codex xhigh); §12; §13 (c), (j)
**Target:** **v49**; the last item in v49's scope (RC1: #172 + item 25 + item 26 durability + this). **It gates the v49.1 tag** (architect, 4 Nov): the rest of v49 is merged on `main` (#176, #181; the release pipeline #180 and #183).
**Budget:** `src/` at most **+10 net** (removals count). Stop and report rather than trim style.
**Agents:** Stage 6 Copilot standard, medium. Stage 7 Codex `gpt-5.6-sol`, **xhigh**, `UNDER REVIEW` header, `-stage7-r<N>` files, at most two rounds.
**Origin:** AB1805 native-driver Step 0 ruling 5 (`WO-2026-10-31-001-step0-report.md`): "v49 deletes any stored `EXRTC_CONFIG_DATA` at boot, so every rollback target is safe; the switch WO comes only after v49 is the default."

## Plain goal
Device OS 6.6.0 owns and talks to the carrier-board AM18x5 (the AB1805 on the Boron's carrier) at boot **only if a valid binding is stored** under `SystemCacheKey::EXRTC_CONFIG_DATA` (Step 0 report §1; `hal_exrtc_init()` loads it first and binds only on success). The v50 switch will create that stored binding (`Am18x5.enable()` stores it, and it persists across reboots; seen on the Dev-14 spike). A device that then runs any GCC build still using the vendored `AB1805_RK` (a rollback, an old image, a recovery flash) would have **two owners of the chip**: Device OS at boot, `AB1805_RK` in `setup()`. This WO makes v49 delete a stored binding at every boot, so **v49 is the oldest safe rollback target once v50 is out** (v48 and earlier have no guard: the fleet must be on v49 before v50 ships, which the sequencing already requires).
It changes nothing on a device that has no stored binding (the whole fleet today; Dev-14 was unbound at the end of the spike).

## Evidence in hand (spike, 3-4 Nov; `WO-2026-11-01-001-ab1805-native-driver-spike.md` result record)
- `Am18x5.enable()` returned 0 and the binding persisted: after a reboot `status` showed `bound=1 present=1 ready=1 clkSrc=2`, and the external RTC was valid at boot before the cloud was up (Device OS had bound it).
- `ExternalTime.disable()` returned 0 and, after a reboot, `status` showed `bound=0 present=0 ready=0`; the vendored `deepPowerDown()` then ran normally (P7).
- Not observed: whether `disable()` takes effect **in the same boot** so that the vendored `ab1805.setup()` works right after it (the open question below).

## The change (re-check every cited line at Step 0; numbers from main `3b6005f`)
1. **One guard in `setup()`, as early as the existing code allows, before the future-date clamp (`Generalized-Core-Counter.cpp:~370-400`, which reads `Time.now()`) and before `ab1805.withFOUT(WKP).setup()` (`:658`):**
   ```cpp
   if (ExternalTime.status().bound()) {                 // Device OS bound the chip from a stored EXRTC_CONFIG_DATA
     const int r = ExternalTime.disable();              // unbind and delete the stored binding
     Log.warn("AB1805: stored Device OS binding deleted (r=%d)", r);
   }
   ```
   About 4-5 lines. The guard acts only when `bound()`; an unbound device does nothing.
2. **If `disable()` fails** (`r != 0`) the boot continues and logs the error; the guard never blocks or resets the boot. (A repeat on the next boot is its own retry.)
3. No new state, no persisted or retained field, no status/ledger field (a log line is the evidence; see question 2).

## Protected / not in scope
`AB1805_RK` usage and its configuration (`withFOUT(WKP)`), the watchdog, the wake-reason classification, hibernate, clock trust, item 15/21, **the switch itself** (v50: its WO removes this guard or replaces it with `enable()`), and `lib/`.

## Step 0 questions (Copilot answers before implementing; stop and report if any answer says the same-boot delete is unsafe)
1. **Same-boot effect.** After `ExternalTime.disable()` in `setup()`, is the chip free for `ab1805.setup()` in the same boot (read `hal_exrtc_unbind`, `hal/shared/exrtc_hal.cpp:~303`, and `spark_wiring_exrtc.h`: does it release the I2C address, the interrupt pin (WKP/D8) and the watchdog pin (D5), and clear any Device OS-side RTC state such as the external time source)? If it does not, the alternative is "delete, then one `System.reset()` at the very top of `setup()`" (nothing to flush there; reset reason 140, to be classified): state the line cost and what the recovery logic does with a reset at that point.
2. **Placement.** The first line of `setup()` after the debug-serial settle (`:300`) vs just before the clamp: which is the earliest point where `Log` works; confirm nothing earlier reads or sets the time or touches `Wire`.
3. **Cost when unbound.** `ExternalTime.status()` on an unbound device: is it a cached read (no I2C traffic, no delay)? Measure or read; the guard runs on every boot, including every nightly hibernate wake.
4. **Compile surface.** `ExternalTime` is declared in Device OS 6.6.0 for the Boron (the spike compiled it); confirm every platform this repo builds for has it, or guard with `#if`.
5. **Side effects on time.** When bound, Device OS may have set `Time` from the chip before `setup()`; after the delete is `Time.isValid()`/`Time.now()` unchanged for the rest of `setup()`?

## Tests (host; the real code, not a copy)
- A fake `ExternalTime` (bound true / false, `disable()` returning 0 / an error) driving the real guard block (extracted from `setup()`): bound → `disable()` called once and one warning logged; unbound → no call; error → logged, boot continues.
- **Order:** a structural test on the real `Generalized-Core-Counter.cpp`: the guard precedes the clamp and `ab1805.withFOUT(WKP).setup()`.
- **Mutations (each must fail):** guard after `ab1805.setup()`; `disable()` called unconditionally; guard removed; an error path that returns from `setup()`; the warning removed.
- `nm` on the local-toolchain ELF: `ExternalTime`/`hal_exrtc_unbind` referenced from `setup()`.

## Acceptance and bench (Chip, Dev-14; nothing here is claimed by Stage 6/7)
1. **Control (no binding):** flash the v49-guard build; boot log has no deletion line; boot-to-connect time and the RTC-vs-cloud time unchanged against a v48 boot.
2. **Binding present:** flash `ab1805-spike.bin` (`~/Downloads/spike-2026-11-03/`), `bind`, `reboot`, `status` (`bound=1`); flash the guard build. First boot: the deletion line once, `r=0`, vendored AB1805 setup succeeds in the same boot (RTC valid, watchdog armed, the usual startup lines), cloud connects, no reset loop. Second boot: **no** deletion line.
3. **Rollback:** flash `v48-restore.bin` (or the previous default) over the guard build after step 2: v48 runs with a single owner (no stored binding), same boot behavior as today.
4. Pass: deletion once and only once per binding; no extra reset (resetReason 140/data) beyond the flash; time within 2 s of the cloud at first sync.

## Rollback
Revert the PR; v48 behavior returns; a stored binding (none today) would stay until v50.

## Open questions for the architect
1. If Step 0 shows the same-boot delete is not safe, is "delete, then one reset at the top of `setup()`" acceptable inside v49 (the bench must then show the reset does not loop and is classified), or should the guard wait for item 26's reset owner?
2. Evidence in the field: log line only (recommendation), or also a status field/ledger flag ("binding deleted") so fleet-ops can see it? A field costs lines and a contract change; nothing in the fleet has a binding today.
3. Sequence: the v50 switch WO must remove or replace this guard (otherwise v50 would delete its own binding every boot) and must state **v49 as the rollback floor**. Agree to record both in the v50 WO now.

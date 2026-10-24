# VERIFIED WITH CONCERNS

**Reviewer:** Codex · **Model:** `gpt-5.6-sol` · **Reasoning:** high

**Test interpreters:** **105/105** — 60 `tests/*.sh` suites via `zsh`, then 45 bare `tests/*.py` suites via `python3`, serial and one suite at a time. ARM builds used Device OS 6.4.1 Boron, GCC ARM 10.2.1, `-frandom-seed=p6`, and a distinct fresh `BUILD_PATH_BASE` for every build. Argon was not built.

Review target: `a4b8a8e551a6e7793a68be511816b5079726db42..b466176104efb469e867cd1c51126d444085af9c`; current review-tree tip `00438f5` adds only the Stage 7 dispatch.

| Check | Result | Evidence |
|---|---|---|
| 1. Exact change and unchanged gates | **PASS** | The only changed `src/` files are `src/power/ConnectivityFailsafe.cpp` (`+3/-1`: one gate replacement and two comment lines) and `src/diagnostics/ConnectivityFailsafeTest.cpp` (`+1/-1`: one prediction replacement). The production gate is `openness() == Closed`; the prediction is `openness() != Closed`. No timing, cooldown, jitter, low-battery, age-base, `Time.isValid()`, `lastConnection`, cadence, active-connect, stage-dispatch, or persistence line changed. `State_Idle.cpp`, `State_Report.cpp`, and `State_Sleep.cpp` are byte-unchanged across the range. |
| 2. Night case and loop analysis | **PASS** | For an old enough `lastConnection`, open hour 06:00, and a cadence below 3 h, Unknown can be eligible from 09:00–24:00; the newly exposed closed-hours interval is 22:00–24:00 at the default close and 23:00–24:00 for Trail02. From 00:00–06:00 `todayAt(06:00)` is future, and 06:00–09:00 has not reached 3 h. A later connection or cadence ≥3 h moves the start later. With `openTime=0`, the earliest interval is 03:00–24:00; an always-open 0/24 site has the same age reset and no newly exposed closed interval. Literal `openTime=24` is invalid (`Config.h` permits 0–12); if corruptly present, `todayAt(24)` is next midnight and blocks the supervisor all day. There is no stage-2 loop in the reviewed source: stage 2 is flushed before reset (`ConnectivityFailsafe.cpp:302-307`), stage 2 advances only to 3, stage ≥3 returns at `:243`, and stage 3 is flushed before power-down at `:311-315`. A future `lastAction` is cleared and advances stage 2 to 3; an older stage 1 advances once to 2. `cloud-ok` refreshes `lastConnection` before clearing the ladder (`State_Connect.cpp:491-493`), so a later stage 2 is a new stale episode, not a reset loop. |
| 3. Stage 3 after stage 2 | **PASS** | Let `T` be the persisted stage-2 time, `J3` the device's stage-3 jitter (0–1799 s), and `D(c)` be 3 h when cadence `c < 3 h`, otherwise `3 h + c`. Subject to the unchanged gates, stage 3 first becomes possible at `max(T + 6 h + J3, max(todayAt(openTime), lastConnection) + D(c))`; a local-date rollover replaces `todayAt` with the next opening. For Dev-09's approximately 18:40 SGT reset, cooldown expires around 00:40–01:09:59, but the new 06:00 boundary delays the age gate until 09:00 SGT (3 h after opening, about 14 h 20 min after `T`) for cadence below 3 h. `ab1805.setup()` seeds `Time.now()` from the RTC, so the arithmetic remains defined when trust is Unknown. A plausible-but-wrong RTC can mis-time the action; `ConnectivityFailsafe.cpp:190-204` explicitly accepts that tradeoff. A backward-clock glitch cannot select stage 2 again. |
| 4. Prediction and other rule sites | **CONCERN** | `ConnectivityFailsafeTest.cpp:156-166` matches the production openness and age rule, and restoring `== Open` makes the re-anchored test fail. A source-wide grep found no other file encoding the old failsafe rule; the remaining exact-Open and not-Closed sites in Idle, Report, and Sleep retain their distinct policies. The named `docs/architecture/connectivity-failsafe.md` does not exist in either endpoint, so it cannot be updated. The supervisor's own prose at `ConnectivityFailsafe.cpp:167` and `:220` still says it acts “during open hours only,” which is no longer literally true for Unknown; see C2. |
| 5. Tests and mutations | **CONCERN** | The new harness extracts and compiles the real supervisor body and compares the compiled copy with a fresh extraction; forced tampering exits with `COPY_MISMATCH`. Its 13 cases cover Unknown/Open/Closed, stage 0/2/3, stale and age-base boundaries, and cooldown plus a 1000 s in-range fake jitter. `activeConnectAttemptWithinBudget=false`, healthy tier, external power true, and no downgrade faithfully select the action path; pre-existing structural suites separately pin the active-connect and low-battery gates. The re-anchored test retains every prior Open/Closed assertion and adds three-valued openness. Restoring `!= Open` fails both failsafe tests; restoring prediction `== Open` fails the re-anchor; making Unknown bypass cooldown fails the real-body harness. My terminal-stage mutant (`>=3`→`>=4`) also fails the real-body harness. My persist-after-reset mutant survives the new harness, the re-anchor, and the cadence test; see C1. |
| 6. ELF and retained | **PASS** | Fresh release builds are `151340/1114/2212 → 151340/1114/2212`; `connectivityFailsafeSupervisor()` is `0x298→0x294`, with no added/removed symbol or other symbol-size change (layout padding absorbs 4 B). Fresh test-mode builds are `152868/1114/2212 → 152860/1114/2212`; the supervisor is `0x224→0x218`, with no other symbol-size change (net text −8 after alignment). The complete `.backup` section is 0x98 bytes at `0x2003f400` in all four ELFs and byte-identical before/after, SHA-256 `adfe530fcec43aa43fd659f9c9e2e83760bd9a1d217dfb77e715f7ed22d3b0e7`. Every retained-range symbol and address is unchanged, and no added `src/` line contains `retained`. |
| 7. Budget and suite | **CONCERN** | Hard `src/` budget passes: `+4/-2 = +2` net (both added lines are comments), within +20. Tests are `+326/-23 = +303` net, 3.03× the +100 guideline and above Step 0's +150–220 expectation; flagged, not failed. Ruling 9 against `0c565fa`: tests excluding fixtures 28,893→34,459 (**+5,566**), fixtures 834→20,435 (**+19,601**), suites 91→105 (**+14**), `src/` 22,306→22,392 (**+86**). This WO alone is +303 / +0 / +1 / +2. Full suite passed 105/105; the golden payload fixture is unchanged (SHA-256 `07de1e1980e4a44e03420540e6c721a93a61b5d6392539219928420c048f0637`); `git diff --check a4b8a8e..b466176` is clean. |

RETAINED OUTCOME: A

## Findings

### C1 — Low — persist-before-reset ordering is verified by inspection but not pinned

- **Observation:** `ConnectivityFailsafe.cpp:302-307` correctly flushes stage 2 before `System.reset()`. In `tests/connectivity_failsafe_unknown_openness_test.cpp:128-150`, the reset fake returns and the assertion inspects only the final persisted stage. Moving persistence after `System.reset()` therefore passed the new harness, the open-hours re-anchor, and the cadence test.
- **Inference:** Real reset can prevent later code from running, so that mutant could leave stage 0/1 persisted and permit repeated stage-2 resets. The reviewed production source is correctly ordered and has no such path; this is a test-maintenance gap in a safety-critical invariant, not a present firmware defect.

### C2 — Low — documentation no longer states the Unknown exception consistently

- **Observation:** `ConnectivityFailsafe.cpp:167` and `:220` say the supervisor acts during open hours only, while `:228-230` deliberately lets Unknown act after close. The `@see` target at `:173`, `docs/architecture/connectivity-failsafe.md`, is absent. `docs/architecture-overview.md` does not encode the old strict-Open failsafe rule.
- **Inference:** This does not affect firmware behavior, but future readers can miss the deliberate 22:00–24:00 or 23:00–24:00 exposure. The source prose should say “skip only known Closed; Unknown uses the daily opening age base,” and a real architecture target should document the exception.

### C3 — Low — test implementation exceeds the guideline

- **Observation:** `tests/connectivity_failsafe_open_hours_test.{cpp,sh}` and `tests/connectivity_failsafe_unknown_openness_test.{cpp,sh}` add a net 303 non-fixture test lines (`+326/-23`), 3.03× the +100 guideline and 83 lines above Step 0's estimate ceiling.
- **Inference:** The real-body extraction and focused matrix are proportionate to the recovery risk, but they add above-guideline maintenance cost. This is test-only and does not weaken the implementation evidence.

## Budget versus actual

| Item | Budget/guideline | Actual |
|---|---:|---:|
| `src/` net | ≤ +20 hard | **+2** (`+4/-2`) |
| Tests excluding fixtures | +100 guideline | **+303** (`+326/-23`) |
| Fixtures | no addition expected | **0** |
| Suites | baseline 104 | **105** (+1) |

## Confirmations

- The review tree was clean before review. The authorized verdict is its only final change.
- Because the review tree's `.git/worktrees` was sandbox-read-only, detached worktrees were created from a local bare clone under `build-tmp/wo20261024-001-stage7/`; every suite, build, and mutation ran there. The non-mutated detached worktrees remained clean.
- The first environment-only build-path attempt encountered a stale dependency path and was discarded; all reported builds used command-line `BUILD_PATH_BASE` overrides and fresh directories.
- Only review-created scratch was removed. No commit, push, stash, reset, checkout/restore in the review tree, device, network, or AWS access occurred.

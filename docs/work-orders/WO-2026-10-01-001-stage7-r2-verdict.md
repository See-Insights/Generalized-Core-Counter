**Overall: VERIFIED WITH NOTES — item A, round 2.** The three round-1 defects are resolved. All required checks passed; the timeout qualifications below remain.

Model/reasoning actually used: **gpt-6-astra, high**.

| Check | Verdict | Evidence |
|---|---|---|
| **1. Three round-1 reproductions** | **PASS — VERIFIED** | Compiled production-code extraction catches `begin` between gate passes before teardown, including normal/standby and cloud online/offline cases. Complete and failed cause no spurious re-entry. An earlier terminal event does not shorten a subsequent download after its new begin. |
| **2. Reference pattern** | **PASS — VERIFIED WITH NOTES** | Callback sets/clears the flag, timestamps begin/progress, and makes no transitions. Sleep checks before teardown on every eligible pass. Flag-clear exits to Sleep. Reference differences are described below. |
| **3. Existing A checks** | **PASS — VERIFIED WITH NOTES** | Sleep entry and unchanged `updatesPending()` entry verified. A **20-minute progressing download** stays in the update state with **zero ThrashGuard trips or teardown requests**. Inactivity exits at **300001 ms**; button exit works. Exactly three local exits remain: flag-clear, button, inactivity. ThrashGuard is **330 s**. The webhook hold is byte-identical to round 1: **3 code lines**. |
| **4. Mid-gate return** | **PASS — VERIFIED** | Clearing `cloudSyncStartMs` produces a fresh gate after the update. The integration test replaces the previous **107-second** queue budget with a fresh **30-second** budget. Remaining gate counters, timestamps and logging state reset on entry. Standby outcome flags retain their existing intended behavior. |
| **5. Mutations** | **PASS — VERIFIED** | **5/5 caught**, individually, with scratch sources restored byte-identically afterward. Details below. |
| **6. Host suite** | **PASS — VERIFIED** | **51/51:** all **25 shell tests via zsh**, all **26 Python tests via python3**. `publish_with_ack_structural_test.py` is unchanged from v30 and green. |
| **7. Linkage/build** | **PASS — VERIFIED** | Local **Boron release, Device OS 6.4.1**, built successfully after successful `make clean-user`. Callback registration and sleep guard confirmed in ELF disassembly. Both required strings found. |
| **8. Size/frozen scope** | **PASS — VERIFIED** | A: **38 added − 11 removed = +27** code lines against `9e10778`, within about 30. B/C/D remain **4 / 14 / 4**. Their edited hunks match the archived round-1 diff, corroborating the supplied byte-identity check. HEAD’s intervening changes are documentation only. |

The legacy transition-count change is necessary: reverting **only** `EXPECTED_TRANSITION_CALLS` to 15 fails because the sleep handler now contains 16 calls. Item A adds exactly one. The test retains its existing transition-ownership, diagnostics and reset-site checks.

| Mutation | Test that failed |
|---|---|
| Move sleep check after first teardown request | Structural |
| Remove sleep check | Behavioral and structural |
| Restore fixed five-minute timer | Behavioral and structural |
| Remove `markProgress` | Behavioral and structural |
| Lower update-state ThrashGuard timeout to 180 s | Structural |

The existing behavioral test extracts the guard separately, so the structural test is essential for detecting incorrect placement.

The reference comparison has these qualifications:

- Besides the three deliberate changes, the implementation retains the approved button override, configuration load, webhook hold, pending-update entry and ThrashGuard integration. Its guard follows the existing CONNECTED/open-hours abort and is conditioned on no disconnect having been requested. Numeric event codes match Device OS.
- The timer uses **`>`**, whereas the reference uses **`>=`**: exactly 300000 ms remains in the state; 300001 ms exits.
- An inactivity exit leaves the in-progress flag set. Consequently, the next [Sleep pass](/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/state/State_Sleep.cpp:484) re-enters the update state. **The supplied reference has this same behavior.** The timeout therefore proves a transition to Sleep, not eventual physical sleep.
- After that timeout transition, the loop invokes ThrashGuard using **Sleep’s 60-second timeout**, before Sleep entry refreshes progress. The combined production-code harness recorded **one trip** in this scenario. This does not pre-empt the update handler’s inactivity exit; sustained progress produced zero trips. The approved global out-of-memory and user-switch exits remain unchanged.

ELF evidence:

- `firmwareUpdateHandler`: **0xb53bc**.
- `setup()` registers pointer **0xb53bd**, event mask **256**, through `SystemClass::on`.
- Sleep’s flag load/check begins at **0xc26f4**, reads **0x2003d8c0**, clears the gate timer and branches to the update transition.
- `handleFirmwareUpdateState`: **0xc09d0**.
- `strings` finds **`v31-ConnectivityFixes`** and **`firmware update in progress`**.

| Build | text | data | bss |
|---|---:|---:|---:|
| Supplied v30 baseline | 150164 | 1090 | 2196 |
| Independently built round 2 | **150420** | **1090** | **2204** |
| Difference | **+256** | **0** | **+8** |

An initial clean attempt targeted installed-toolchain outputs because the environment override was ignored; filesystem protection blocked it. Passing the scratch output path as a make command-line argument corrected this, and clean/build succeeded.

**Preservation confirmed:** all **125,003 pre-existing entries** matched, with file bytes and Git status unchanged and no added or deleted paths outside the authorized scratch directory. Only `build-tmp/wo20261001-stage7-r2/` was removed. `build-tmp/` and `build-tmp/connectivity-archive/` remain intact. No lasting edits, commits, device operations or network access.
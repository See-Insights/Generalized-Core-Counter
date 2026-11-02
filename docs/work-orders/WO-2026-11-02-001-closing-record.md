# WO-2026-11-02-001: closing record

Work Order and Amendment 1: `WO-2026-11-02-001-usb-charge-latch-recovery.md` (on PR #168 with the Step 0; Amendment 1 is in Chip's checkout and goes in with #168 or this PR's merge order, Chip's call).

## Closing record (2 Nov 2026)

**Result:** Stage 6 and Stage 7 complete; the architect accepted the one open Stage 7 item as a documented limitation. Awaiting Chip's Stage 8 gate. Not merged.

**Rounds**
| Round | Agent | Result |
|---|---|---|
| Stage 6 r1 | Copilot `claude-sonnet-5.5`, medium | `src/` +20 net; suite 108/108; ARM build OK |
| Stage 7 r1 | Codex `gpt-5.6-sol`, xhigh | NOT VERIFIED: alert-21 clear changed; raw-VBUS USB gating; `vcell` default; direct remediation fields (cooldown bypass); dense style; alert-codes.md missing |
| Amendment 1 | architect | alert 22; alert-21 clear restored; shared corrected USB test AND VBUS; level-1 via existing path with 1 h cooldown; no default, no 2.5 V; normal style; budget +30 |
| Stage 6 r2 | Copilot `claude-opus-5.5`, high | `src/` +30 net; suite 108/108; deviations ruled: P:342 counter line and four statics (both accepted) |
| Stage 7 r2 | Codex xhigh | NOT VERIFIED: P:342 counter carried when a cycle was running (premature escalation, early clear) |
| Stage 6 r3 | Copilot `claude-opus-5.5`, high | `!remediationInProgress` added on P:342; 0 net lines; scenario 10; 20 mutations caught; suite 108/108 |
| Stage 7 r3 | Codex xhigh | NOT VERIFIED on one item only (below); counter fix, 20-23 clear on Pre/Fast/Done, scenario 7 and 10, size, docs spot-checks all verified |

**Open item, accepted by the architect (2 Nov):** a real charge fault during a latch episode lets the fault path's "charging healthy" clear (P:363-379) clear alert 22 while the charger still reads Not Charging; the latch re-raises after a new 2 h window (24 h attempt limit applies). While raised, 22 can mask alert 20 (equal severity). Documented in `alert-codes.md` row 22. No code change, no round 4. The toggle is the fix; alert 22 is a heads-up.

**Budget:** `src/` +30 net (+46 / -16) against +30 (raised from +20 in Amendment 1): `PmicFaultMonitor.cpp` +29, `MyPersistentData.cpp` +1 (severity entry for 22), `PmicFaultMonitor.h` 0, `SensorManager.cpp` 0. Tests: suite 107 on main, 108 after; scenarios 6-10 and `tests/pmic_latch_call_site_structural_test.py` added. Local ARM release build: text 152,084, data 1,114, bss 2,216, `.bin` 153,202 bytes; `nm` shows `pollAndRemediate(..., float)` and the latch statics.

**Controller checks:** the pmic host test passed 3 of 3 runs and the structural test passed on the round-1 tree, and the pmic test passed twice more on the round-3 tree; Copilot's full suite passed 108/108 each round (Codex's sandbox cannot compile, so it judged by inspection). Row 22 wording checked against the code by the controller (2 h window and 24 h gate P:397-409, the clear P:363-379, strict-greater arbitration `MyPersistentData.cpp:817`).

**Not done / follow-ups:** the gauge quick-start (item 23), solar units, and the release vehicle are out of scope; the release vehicle (candidate v49) is Chip's decision. A bench test on a USB unit (Chip's hands) is still to be run.

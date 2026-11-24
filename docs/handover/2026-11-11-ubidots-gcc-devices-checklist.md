# Ubidots "GCC Devices" group: setup checklist (Chip, in the Ubidots UI)

**Author:** architect, 11 Nov 2026 · **Implements:** alert-reporting handover §8 rulings 1–2 (P2, P3) · **Who:** Chip only (Ubidots is cloud-side; agents have read-only Ubidots access) · **Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` roles (Chip does all console work).
**Verified against Ubidots (read-only MCP), 11 Nov 2026 ~05:45Z:** every event ID, trigger and group below.

## How groups work (the answer to "both groups?")

- **A device can be in any number of groups.** Your groups already overlap: "Active Solar Devices" (259) shares devices with the park and site groups (Visitation Counters 216, Morrisville-Tennis 11, …). Groups only decide which events watch the device.
- **Each Ubidots event watches one group's variable.** All 19 active fleet-alert events watch **"Active Solar Devices"** (`solar-sensors`, id `5bdb2bb21d8472404ce1ce98`, 259 devices). Seven of them don't depend on the code (offline and battery). Twelve are `E30-xx` and fire when `alerts == xx`, using **legacy** wording.
- **So a GCC device must NOT stay in "Active Solar Devices".** If it does, every legacy E30 event keeps firing on GCC codes with the wrong words. That is already happening. On 10 Nov 20:59:51Z, ChurchSt-1's GCC code 21 (charging) posted *"Firmware Update Timed-Out. Msg E30-5"*. On 10 Nov 14:00:51Z its GCC code 40 posted E30-40, which the rulings make digest-only.
- **The rule:** a GCC device is in **"GCC Devices" and its site group, and not in "Active Solar Devices"**. The seven code-independent events are extended to watch "GCC Devices" as well (ruling 1: "apply to both groups unchanged"). Three new events give GCC its own meanings.
- **Finding:** of the 8 devices on GCC now, **only ChurchSt-1 is in "Active Solar Devices"**. WAKEFOREST-PC01 and Vaughan-001…006 are in **no** fleet-alert group. They get no offline (E01–E03) or battery (E11/E12) alerts and no Freshdesk tickets today. This checklist fixes that.

## Phase A — create the group (2 min)
1. Devices → Groups → **+** → Name **`GCC Devices`**, label **`gcc-devices`**. Add no devices yet.

## Phase B — create the three GCC events (ruling 2: Slack only for what needs a visit or an action)
Copy any existing E30 event (⋮ → Copy) so the Slack webhook and channel (`#iot-alerts`) carry over. Then change only the name, trigger and message. **Every trigger: device group = `GCC Devices`, variable `alerts`.** Don't use Active Solar Devices.

| # | Event name | Trigger (`alerts`) | Slack message | Emoji |
|---|---|---|---|---|
| B1 | `E30-21 (GCC) - Charging problem` | **two conditions, AND:** value `>= 21` **and** value `<= 23` (one event for 21/22/23, replacing legacy 11) | `{{device.name}} ({{device.label}}): charging problem, GCC code {{last_value}} (21 timeout/stuck, 22 USB charge latch, 23 battery fault). Msg E30-21 (GCC)` | `:battery:` |
| B2 | `E30-17 (GCC) - Boot storm` | value `== 17` | `{{device.name}} ({{device.label}}): boot storm (repeated restarts), GCC code 17. Msg E30-17 (GCC)` | `:warning:` |
| B3 | `E30-18 (GCC) - State-machine thrash` | value `== 18` | `{{device.name}} ({{device.label}}): state-machine thrash, GCC code 18. Msg E30-18 (GCC)` | `:warning:` |

- **B1 AND condition:** in the trigger, add a second condition row joined with **AND**. Your account already uses this pattern ("KI_041 Office PWRe Alert": `>= 3` AND `< 5`).
- **No events for 15, 19, 20, 31, 40.** They're digest-only (ruling 2), and the digest arrives with the P work. **No events for 16, 41–44** either: log only, as today. Don't copy the legacy E30-11…40 events.
- Leave back-to-normal off, like the legacy E30 events.
- The meanings come from `docs/reference/alert-codes.md` at `f00aa04`: 21 timeout/stuck, 22 USB charge latch, 23 battery fault, 17 boot storm, 18 thrash.

## Phase C — extend the seven code-independent events to "GCC Devices"
For each event, open it → Trigger → device group → **add `GCC Devices` alongside `Active Solar Devices`**. Change nothing else: thresholds, messages and the Freshdesk UbiFunction stay as they are.
**If the picker accepts only one group:** cancel, copy the event (⋮ → Copy), name the copy "… (GCC)", and set only its group to `GCC Devices`. Note in the tick box below which method you used.

| ☐ | Event | Id | Watches |
|---|---|---|---|
| ☐ C1 | E01: … Off-Line for 3 Hours (Validation Event) | `61aea0f29f4b59863ed27bc3` | `battery` inactive 183 min → UbiFunction `3h-event-validation` |
| ☐ C2 | E01: Active Park Sensor - Off-Line for 3 Hours | `6032b05d1d8472615c5397b9` | `3h-flag == 1` |
| ☐ C3 | E01: … Off-Line for 3 Hours (Back to Normal) | `61aea65e13ce5f83f3e78839` | `3h-btn-flag == 1` |
| ☐ C4 | E02: Active Park Sensor Off-Line for +1 Day | `6032b18f1d847263721b2f5c` | `battery` inactive 1 day (+ Freshdesk) |
| ☐ C5 | E03: Active Park Sensor Off-Line for over a week | `604796fc1d84726a367efb73` | `battery` inactive 7 days (+ Freshdesk) |
| ☐ C6 | E11: Battery Charge below 65% for three days | `6032b1ca1d847263b59ee299` | `battery <= 65`, delay 4320 min (+ Freshdesk) |
| ☐ C7 | E12: Battery Charge below 40% | `60479d561d84720fa9b0988a` | `battery <= 40` (+ Freshdesk) |

**All three E01 parts (C1–C3) must cover the group**, or the 3-hour alert never completes.


## Phase D — test with a throwaway device (no field device touched)
Use your own Ubidots token in your terminal. Never paste it into a chat or the repo.
1. Create the test device: `curl -s -X POST https://industrial.api.ubidots.com/api/v1.6/devices/gcc-alert-test -H "X-Auth-Token: $UBIDOTS_TOKEN" -H "Content-Type: application/json" -d '{"alerts": 0}'`
2. Add **`gcc-alert-test`** to **GCC Devices only**.
3. Post `{"alerts": 22}`. **Expect** exactly one post in `#iot-alerts`: "charging problem, GCC code 22 … Msg E30-21 (GCC)", and **no** legacy "Firmware Update Failed" post.
4. Post `0`, then `17` (expect B2); post `0`, then `18` (expect B3); post `0`, then `40` (expect **nothing**).
5. Optional E12 check: post `{"battery": 35}`. Expect one E12 Slack post **and a Freshdesk ticket** (close it).
6. Delete `gcc-alert-test`.

## Phase E — move the GCC devices (only those running GCC now)
| ☐ | Device (Ubidots name) | Add to GCC Devices | Remove from Active Solar Devices |
|---|---|---|---|
| ☐ | Morrisville-Tennis-ChurchSt-1 | yes | **yes** (it's the only one in it) |
| ☐ | WAKEFOREST-PC01 | yes | not in it |
| ☐ | Vaughan-001-Tennis-1 … Vaughan-006-Pickleball-1 (6) | yes | not in them |

- For each device, **add first, then remove**, so it's never unwatched. A minute in both groups risks one duplicate post at most.
- **Keep the site groups** (Morrisville-Tennis, etc.) unchanged.
- **Hold Vaughan-007 and -008** until they're back online (site-visit item: **on return, join `GCC Devices` and set `email1`**, see Phase G). They've been silent since 16 Sep and 4 Oct, so adding them now fires E02/E03 at once and opens two Freshdesk tickets for a visit that's already planned.
- **Expect** WAKEFOREST-PC01 and Vaughan-001…006 to start getting offline and battery alerts for the first time.

## Phase F — from now on, the migration step
Add one line to the migration checklist: **"Ubidots: add the device to `GCC Devices`; remove it from `Active Solar Devices` if it's there; keep its site group."**

## Phase G: the migration checklist step, as written (12 Nov)
For every device that moves to GCC, in the Ubidots UI: **add the device to `GCC Devices`; remove it from `Active Solar Devices`; keep its site group; and set `email1`.** For the Vaughan devices `email1` is `rajeev@seeinsights.com` for now (Vaughan-001 to -006 done; **Vaughan-007 and -008: on return, join `GCC Devices` and set `email1`**; they are on the site-visit list).

**Maintenance rule:** the code-independent events (E01, E02, E03, E11, E12) now exist as two copies: the original (watching `Active Solar Devices`) and the `(GCC)` copy (watching `GCC Devices`). **A change to E01, E02, E03, E11 or E12 goes into both copies.**

## Close-out (Chip, 12 Nov 2026): setup complete and verified
The Ubidots `GCC Devices` setup is complete. The verify script reported **14 of 15 done**; **A03 shows TODO only because the test device was deleted, as planned.** Ids (from the verify output):

| Item | Ubidots id |
|---|---|
| Group `gcc-devices` | `6af411698260386d6d0e3d86` |
| `E30-21 (GCC)` (Phase B1) | `6af412018993ef0c722a508b` |
| `E30-17 (GCC)` (Phase B2) | `6af412195452ba861762c046` |
| `E30-18 (GCC)` (Phase B3) | `6af4122e7e4a7149e17bc0bb` |
| `E12 (GCC)` copy | `6af412f11d79014c3a4b3ba2` |
| `E11 (GCC)` copy | `6af4130bbf5e1c7917e387ea` |
| `E02 (GCC)` copy | `6af41321ea81f7410c1cca5c` |
| `E03 (GCC)` copy | `6af41337745afb2d7b65565e` |
| `E01 (GCC)` validation copy | `6af4134ebf5e1c7917e38a8c` |
| `E01 (GCC)` post copy | `6af4146e7cad033f481a19b6` |
| `E01 (GCC)` reconnected copy | `6af414825452ba861762cdc8` |

The seven code-independent events were therefore handled by **copying** each to a `(GCC)` copy (the picker method in Phase C's note), not by adding the group to the originals. Phase C's ids above are the originals. Vaughan `email1` = `rajeev@seeinsights.com`.

## Not covered here (each goes to its own thread)
- Dashboards or reports that filter on "Active Solar Devices" lose ChurchSt-1 once it moves. Check Data → Dashboards before step E. The MCP can't list dashboards.
- Whether the fleet-ops tool reads Ubidots groups → **fleet-ops thread**.
- **Security (fleet-ops thread):** several old Ubidots events (TriangleLand, SantaCruz, Falls Lake, Dev01) carry **plaintext Particle access tokens** in their webhook bodies, and every Slack event carries the webhook URL. Rotate any token that's still live and retire the dead events. This is a least-privilege item, unrelated to this checklist.

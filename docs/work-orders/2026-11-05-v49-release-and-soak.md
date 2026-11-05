# v49 release and bench soak (v49.1-DurableResets)

**Author:** Claude Code, 5 Nov 2026, from `2026-10-31-v48-release-and-soak.md`; the architect may amend · **Workflow:** `AI_DEVELOPMENT_WORKFLOW.md` §2, §3 ("Release gate: the CI build"), §13 (g), (h), (i), (j); Stage 9 (release pipeline)
**Agents:** soak and baseline runs, a **Sonnet subagent, medium**, read-only. Report the model used.
**Budget:** 0 `src/` lines beyond the version string.

## Plain goal
Ship v49.1 (product version 4901): the Eastern default timezone (item 25, #176), durable deliberate resets (item 26, #181), USB charge-latch recovery (item 22, #172) and the RTC binding-delete guard (WO-2026-11-04-002, #187). Release vehicle: the version-bump PR (`bump_version.sh v49.1-DurableResets`), then the annotated tag `v49.1` on its merge commit; CI builds, publishes the GitHub Release and (after Chip's Environment approval) uploads to product 42131. Tag notes: `docs/releases/v49.1-notes.md` (immutable once pushed: the tag ruleset blocks updates; the GitHub Release body may be edited later with `gh release edit`).

## Release steps (Chip approves the upload; pins and defaults are Chip's)
1. Merge the bump PR (ruleset: PR plus CI). 2. Claude Code creates the annotated tag `v49.1` on the merge commit (the notes content is approved as final) as in Stage 9 item 8: extract only the message (`sed -n '/^=== TAG MESSAGE STARTS/,$p' docs/releases/v49.1-notes.md | tail -n +2 > <scratch>/v49.1-msg.md`); `git tag -a v49.1 --cleanup=verbatim -F <scratch>/v49.1-msg.md <merge-SHA>` (the default cleanup deletes the `##` headings as comments); check with `git cat-file -p v49.1` that the first message line is `## What changed since v48…`, all `##` headings and the exact `---` line are present and no header text is: only then push. 3. The release run: host tests, firmware, `publish` (Release with the `.bin` and `.sha256`), `upload` (waits for Chip's approval in `particle-42131`). 4. Console check: product 42131 → Firmware shows version 4901, title "Development Release - v49.1", the description above `---` plus the Release link. 5. Chip pins Dev-09 and Dev-14 (never rely on the pin for a Development-marked device; flash or un-mark first). 6. `particle binary inspect` of the `.bin` shows "product id 13 at version 4901" before pinning.

## Soak devices and anchors
Dev-09 (e00fce68399ee6244a963935) and Dev-14 (e00fce688e592afaf23ac4fb), pinned by Chip; anchor every check on each device's first v49.1 boot (status reset 70, `v49.1-DurableResets`). Not set as the product default during the soak.

## Checks, per device (as the v48 soak §3, plus)
1. The standard checks: resets (only the reset 70 at the OTA), `watchdogResetCount`, `failsafeCount`, alerts, heap; every cycle sleeps (cyc − slp = 1); closes correct (stamped close − 1 s, 0 next morning); no report lost.
2. **Control case for the binding guard (gates the default, not the tag; architect, 4 Nov):** these devices have no stored RTC binding, so on v49.1 there is **no** "AB1805: stored Device OS binding deleted" serial line, no reset with `resetReasonData` 8, and the boot-to-connect time (`ConnSummary` elapsed, about 19 s on this bench) and the RTC-versus-cloud time at first sync are unchanged against a v48 boot.
3. **Publish-lag trend** (new v49 check): publish time minus the report's data timestamp per device, first versus last third of the day; flag a rising trend (ChurchSt-2 grew from about 5 s to 50 s on 3 Nov; cause not investigated).
4. Timezone: a fresh or reflashed device runs Eastern at first boot; a device that stored `UTC0` keeps it until a ledger zone applies (a ledger zone change takes effect at the next reboot).
5. Durability bench (Chip, Dev-14, pending, not claimed): flush durations per site against the ceilings (250 ms per store, 2 s queue), reboot checks per reachable site, then two nightly hibernates and an OTA in progress (`WO-2026-11-03-005`).

## Not in this soak
Binding present and rollback (the v50 pre-ship checklist, `WO-2026-10-31-001-ab1805-native-driver-step0.md` "v50 record"); item 19 C1/C2 (#185, #184: held until v49 is tagged; v50); the AB1805 switch (v50).

AGENT: Codex · MODEL: gpt-6-astra · REASONING: high · via codex exec
Per docs/AI_DEVELOPMENT_WORKFLOW.md: Roles table (Codex: investigation), guardrail 1 (history first: `git log -S` before proposing anything new), and the guardrail on checking facts against their source (verify every file:line and figure below before relying on it, and correct any that have moved).
AUTHORIZATION SCOPE: read-only code, Device OS 6.4.1 source (`~/.particle/toolchains/deviceOS/6.4.1/`), git history, the archive copies listed below, and forwarded serial logs. Not authorized: edits, commits, builds, device actions, settings changes, network or AWS calls, and deleting anything.
**Output budget:** for each of A–D, one short page at most: findings with evidence, then one proposed change with its plain goal and a size estimate in `src/` lines. No narrative beyond that.

**Purpose:** close out the known issues before Step 6, so Step 6 starts from evidence rather than guesses.

## Repository and data

- **Code:** `Generalized-Core-Counter`, branch `wo/2026-09-24-004-hour-rules` at `ba97977`. That is v36-HourRules: `main` (`6d8aaf9`, v35) plus the hour-rules change. PR #60 is open. The US fleet runs **v35** (the product default, since 10:01–10:04Z on 3 Oct). Dev-09 and Dev-14 run **v36** (since 10:30Z and 10:23Z on 3 Oct). The two releases share all code relevant to A–D. Ignore the uncommitted docs edit in the working tree.
- **Archive copies (read-only), in Particle's S3 layout `<UTC day>/<event>/<deviceId>/<published_at>.json`:**
  - `build-tmp/connectivity-archive/`: 2026-09-14 to 2026-10-01 (reports, status, watchdog, serial), plus Particle docs extracts in `particle-docs/`. **Don't modify it.**
  - `/private/tmp/claude-501/-Users-chipmc-Documents-Maker-Particle-Projects-Generalized-Core-Counter/61f517c9-3c76-4b18-afd3-fa4f31a7be63/scratchpad/codex-data/`:
    - `hibernate_wake` for 2026-09-26 to 2026-10-04;
    - all events (reports, status, watchdog, `serialLog`) for 2026-10-01 to 2026-10-04 (to about 14:10Z);
    - `devices.txt` (device ID, name, country, close hour);
    - `ledgers/`: each device's current `device-status` and `device-settings` ledger, captured from the Particle API at 14:13Z on 4 Oct.
  - Serial is forwarded only for Dev-09 and Dev-14 (SG). The US devices have no serial.

## A. Watchdog resets (hardware watchdog, reset reason 60)

Memory is now level on every device (v34), so these are the main remaining unexplained resets. The AB1805 watchdog is set to WATCHDOG_MAX_SECONDS (124 s, Generalized-Core-Counter.cpp:1266) and is serviced by ab1805.loop() on every main-loop pass (:1629). So a reason-60 reset means the main loop stopped for more than about 124 s.

**Connection stage, breadcrumb 18 (WO-2026-09-25-005).**
- **Recent cases:** PCKL1 12:34Z on 2 Oct, Trail02 20:54Z on 2 Oct, Court1 11:01Z on 3 Oct, PCKL1 12:45Z on 3 Oct.
- **Earlier:** 30 cases (22 US, 8 SG) in the connectivity investigation.
- **Questions:**
  - Which call blocks the loop? Map breadcrumb 18's position. It's set on every loop pass at :1650, so it may be the last checkpoint passed rather than where the loop stopped.
  - List every call after it that can block on the modem or the cloud: Particle.connect, Cellular.*, Particle.publish, ledger calls, the publish queue.

**Sleep stage, breadcrumb 28 (WO-2026-09-03-004).**
- **Cases:** Dev-14 several a night (2–3 Oct); PCKL1 22:02Z on 3 Oct.
- **Question:** the same, for the sleep path. Does System.sleep(), or the modem teardown before it, block? Note the slow teardowns already seen (7–22 s, PPP error event data=5).

**For each stage:** what the evidence shows, and the smallest change that keeps the loop responsive or bounds the wait. Say whether it agrees with Particle's guidance, and cite it.

## B. The restart limitation (design only)

**Plain goal:** a reset during a session doesn't lose the session's minutes.

**Examples:**
- PCKL1 lost about 1 minute at 08:33 EDT on 2 Oct (watchdog);
- MAFC-1's 18:59 EDT session on 30 Sep was never counted;
- Dev-09 lost about 11 minutes during its v33 update on 2 Oct (12:39–12:50 SGT).

**Questions:**
1. What's persisted about an open session today (file:line)?
2. **Proposal to evaluate:** persist the session's start when it begins. On boot, if a session was open and the clock is trusted, credit it up to the moment of the reset and close it.
3. **The key question:** how to know when the reset happened without crediting a long power-off. The options:
   - a "last alive" time persisted at each report;
   - the AB1805's RAM;
   - a cap of one debounce period (300 s).

   Recommend one, with flash wear and size in mind.

## C. Hibernate

1. **What does `result=fail DEEP_POWER_DOWN` mean in the code** when the wake was on time (Dev-09 and Dev-14 recently)? Is it a real failure or a classification bug? Cite file:line.
   - Note that deepPowerDown() did work when the failsafe called it (stage 3, status at 12:43:05Z on 1 Oct).
   - Dev-09 earlier failed 7 of 7 hibernate wakes on older firmware, some of them hours late.
2. **Trail02's trial** (hibernate enabled about 1 Oct). For each night:
   - did it hibernate (a hibernate_wake event)?
   - when did it actually wake?
   - when was its first morning report, compared with its opening?
   - any heap-guard reset?
3. **How hibernate is enabled** (enableHibernateSleep, default false at MyPersistentData.cpp:157 and default-settings-v3.23.json:27), and what else blocks it (the 15-minute to 10-hour limit, State_Sleep.cpp:282–287).
4. **Verdict:** safe to enable for US production devices, or not yet, and why.

## D. Battery trust and tier

**The problem:** the battery tier is often wrong.
- Court3 (gauge 82.1%, vcell 3.90 V) and PCKL1 (77.2%, 3.88 V) were marked untrusted and dropped to CRITICAL. MAFC-2 (86.1%) is CONSERVING with "suspect" trust.
- **The mechanism:** the trust check compares the gauge with restingSocFromVcell(vcell), a resting-voltage table (kOcvKnots, BatteryHealth.cpp:19–31: 3.80 V = 40%, 3.90 V = 50%, 4.05 V = 80%). It marks the gauge untrusted when they differ by 20 points or more, or 28 with the radio on (BatteryHealth.cpp:80–106). radioActive comes from Connectivity::isRadioPoweredOn() (SensorManager.cpp:826–831).

**Questions:**
1. **Across all devices in the archive,** how often do the gauge and the voltage estimate disagree by 20 points or more? Does the gap track any of these, at the time vcell was read:
   - whether the radio was on;
   - the time since waking;
   - charging versus discharging;
   - the power source (solar, USB)?
2. **Where in the wake cycle is vcell read** (file:line)? Is there a point where the cell is genuinely at rest, for example just after waking, before the modem powers up?
3. **Which is more likely right on these devices,** the gauge or the voltage table? If the data can't tell, say so, and name the one bench measurement that would settle it (for example a multimeter reading of a cell at rest beside the gauge's value).

**For context, not to solve here:** the tier also drives two Step 6 items:
- the connection-mode tug-of-war (ConfigApply.cpp:445–455 against BatteryAuthorityCommand.cpp:80–84);
- the failsafe, whose only allowance for deliberate spacing is the low-battery block at Generalized-Core-Counter.cpp:2638–2641.

## Report

The findings and one proposal for each of A–D, each within the output budget, and the model and reasoning level used.

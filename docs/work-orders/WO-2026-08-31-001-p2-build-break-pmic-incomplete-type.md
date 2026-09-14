# WO-2026-08-31-001: P2 build is broken - `PMIC` incomplete type in PmicFaultMonitor.cpp

**Type:** Defect report. Pre-existing; not introduced by WO-2026-08-29-001
or -002.

**Raised because:** Codex Stage 7 finding 7 against WO-2026-08-29-002.
That WO had moved `project.properties` from `platform=p2` to
`platform=boron` so the Boron-gated code under review would actually
compile. Codex correctly objected that this *masks* the P2 break rather
than resolving it, and that changing the default compile/flash target can
make an ordinary developer build produce firmware for the wrong hardware.
The platform change has been reverted; this WO carries the underlying
defect instead.

## Symptom

`particle compile p2 .` fails:

```
src/power/PmicFaultMonitor.cpp: In function 'PmicFaultMonitor::Registers
PmicFaultMonitor::pollAndRemediate(PMIC&, bool, uint8_t, int, bool)':
src/power/PmicFaultMonitor.cpp:143:7: error: invalid use of incomplete type 'class PMIC'
  143 |       pmic.disableCharging();
src/power/PmicFaultMonitor.cpp:145:7: error: invalid use of incomplete type 'class PMIC'
  145 |       pmic.enableCharging();
In file included from src/power/PmicFaultMonitor.cpp:2:
src/power/PmicFaultMonitor.h:5:7: note: forward declaration of 'class PMIC'
    5 | class PMIC;
make[2]: *** [../build/target/user/platform-32-msrc/power/PmicFaultMonitor.o] Error 1
```

`particle compile boron .` succeeds.

**Pre-existing status verified, not assumed.** Reproduced 2026-08-31 twice:
once on the current working tree, and once on an unmodified `git archive
HEAD` export of the repository with neither WO-2026-08-29-001 nor -002
applied. Both produce the identical error at `PmicFaultMonitor.cpp:143`
and `:145`. The defect therefore predates both work orders.

## Cause

`PmicFaultMonitor.h:5` forward-declares `class PMIC;` rather than
including a definition. `PmicFaultMonitor.cpp:1` includes `Particle.h`,
which **defines** `class PMIC` on Boron (the BQ24195 is Boron/cellular
hardware) but **not** on P2/RTL872x, which has no BQ24195. On P2 the
forward declaration therefore stays incomplete and every member call on
`pmic` fails to compile.

The file's own header comment already states the scope - "PMIC fault
monitoring/remediation/telemetry for the BQ24195 (Boron/cellular only)" -
so the code is correctly understood as Boron-only; it simply is not
*guarded* as Boron-only for the P2 build.

## Impact

- The repository does not build for P2 from a clean tree.
- Product 41915 (`NextGen-Occupancy-P2`) has one device, `P2-Dev-01`,
  which has been silent since 2026-08-14 and is running firmware 20. No
  P2 firmware can currently be produced for it.
- Any verification performed with `particle compile p2` is compiling a
  tree that cannot link the PMIC path, so P2 build results in prior work
  orders should not be treated as full-coverage evidence.

## Fix direction (not prescribed - needs its own design pass)

Guard the Boron-only PMIC code for non-PMIC platforms, consistent with
how the rest of the codebase handles platform-conditional hardware (e.g.
the `#if HAL_PLATFORM_CELLULAR && (PLATFORM_ID != PLATFORM_MSOM)` pattern
in `power/ChargeInhibit.cpp`). Options include compiling
`PmicFaultMonitor.cpp` out entirely on platforms without a BQ24195, or
providing a no-op implementation. Whichever is chosen, the P2 build must
go green and the Boron behaviour must be provably unchanged.

## Out of scope

- Any change to PMIC fault-handling *behaviour* on Boron. This is a build
  fix only.
- WO-2026-08-29-001 and -002. Both are Boron-targeted and unaffected.

## Verification

- `particle compile p2 .` succeeds.
- `particle compile boron .` still succeeds, with Boron flash/RAM figures
  unchanged from before the fix.
- Codex Stage 7 at the standard tier.

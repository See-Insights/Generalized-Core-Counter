# Dev-09: 24 h soak of the CI-built v48 binary (gate for replacing the cloud-compile gate)

For Chip, when free. This is the exact file CI built from `main` `0c8b5a2` (the #175 merge); it is the same source as the v48 build Dev-09 runs now.

**File:** `~/Downloads/dev09-ci-soak-2026-11-03/dev09-ci-built-v48-0c8b5a2.bin` (152,890 bytes)
**SHA-256:** `83218db3b47b4d5f9030284105f66bd04b86e0da6eb45c7e65e4cda78c3667b3` (CI run 41580249001, artifact `firmware-boron`; product id 13 at version 48, string `v48-OccupancyEndsAtClose`)

1. Check the hash: `shasum -a 256 <file>` equals the value above.
2. Console: product 42131, Boron-Dev-09 (`e00fce68399ee6244a963935`) → mark as development device and unlock from v48 (as for the Dev-14 spike), so Particle does not push another build over it.
3. Flash over USB (DFU): `particle flash --usb ~/Downloads/dev09-ci-soak-2026-11-03/dev09-ci-built-v48-0c8b5a2.bin`. (Or Console → device → flash, if you prefer OTA to a development device.)
4. Send me: the time of the flash and the first status after boot (expect reset 70, `v48-OccupancyEndsAtClose`).
5. Leave it running 24 h with the exerciser as it is. At +24 h I grade it as the v48 gates were graded (resets, watchdog count, report cadence, session check) against the v48 baseline on Dev-09 and Dev-14.
6. After the grade: re-lock Dev-09 to the product default and unmark it, as you did for Dev-14.

Clean = same result as the v48 gradings. A clean run, with the size difference accepted, is the last condition to replace the cloud-compile gate (#156); the rule from then on is that the binary soaked is the binary uploaded.

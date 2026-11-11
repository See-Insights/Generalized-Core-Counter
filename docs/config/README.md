# Default ledger files

One Particle product per profile (function plus sensor type). Each product has its own complete default file here: `default-settings-<productId>.json`. Today: `default-settings-42131.json` (occupancy, PIR).

- **Same structure everywhere.** The set of keys is fixed by what the firmware reads in `src/cloud/ConfigApply.cpp` (`getMerged*Value` calls). No missing keys, no dead keys.
- **Values differ per product.** Only the structure is shared.
- **Rule:** any WO that adds, removes or renames a ledger field updates every default file in the same PR. `tests/default_settings_coverage_test.py` checks every `default-settings-*.json` and fails if there are none.

## `sensor.setting1` to `setting4` by `sensor.type`

| `sensor.type` | Sensor | `setting1` | `setting2` | `setting3` | `setting4` |
|---|---|---|---|---|---|
| 1 | PIR (occupancy) | Occupancy debounce, ms (production 300000; 60000 for testing only) | unused (0) | unused (0) | unused (0) |

Add a row when a new sensor type is added.

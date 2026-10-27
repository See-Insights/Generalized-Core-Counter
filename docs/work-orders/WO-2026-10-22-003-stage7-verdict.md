# Stage 7 verdict: WO-2026-10-22-003 P7-A

## NOT VERIFIED

**Reviewer:** Codex (GPT-5), high reasoning  
**Review target:** `git diff df6d99d cc84832` (implementation commit `cc84832`)  
**Guard interpreter:** `#!/usr/bin/env python3`

The implementation preserves the deployed records and passes every host/build check, but the new freeze guard does not meet its binding contract: two valid source mutations that it must reject pass unchanged.

| Check | Verdict | Independent evidence |
|---|---|---|
| 1. No live reader | **PASS** | Bare-name and `get_`/`set_` searches over `src/`, `lib/`, and `tests/`, plus macro/`offsetof` inspection, found no remaining reader or writer for the 11 retired slots. Remaining `sensorType` uses are the distinct `SensorData::type`/`sensorConfig` path, sensor payload text, locals, comments, or isolated test stubs. `solarPowerMode` remains read by `PowerManager.cpp`; `lastCountTime` retains its accessors and writers. |
| 2. Layout unchanged | **PASS** | An all-member ARM probe compiled both `df6d99d` and `cc84832`: all 161 emitted words were equal. Counts are SysData header + 51, SensorData header + 5, CurrentData header + 17; sizes are **272/36/88**. Source diff confirms only the 11 member names changed; types, widths, and order did not. Magics remain `0x20a15e75/0x20a47e74/0x20a99e74`; versions remain **4/1/1**. |
| 3. Behavior and downgrade | **PASS** | `PersistentDataBase::initialize()` zeroes the complete record before subclass defaults, so the four no-longer-written first-boot slots are now zero (`structuresVersion` was 2, SysData `sensorType` was 1, `lowPowerMode` was false, `verboseModeStartTime` was 0). v45 has no caller reading any retired slot. StorageHelperRK validates magic/version/size/hash and hashes bytes but does not interpret these fields. A valid existing record is loaded at the same size and its reserved bytes are preserved; only the header hash/size are recomputed. Thus v47-to-v45 downgrade behavior is unchanged. |
| 4. Freeze guard | **FAIL** | Clean guard passes. Ordinary appends to each record, a reserved-slot width change, an unlike-width move, and a version bump fail; a reserved-slot rename passes. However, appending `bool stage7AppendedTailPadding = false;` to `CurrentData` passes while remaining hidden in its existing tail padding, and swapping adjacent live `uint8_t openTime`/`closeTime` also passes. These directly violate the required appended-member and moved-member coverage. The replacement is not weaker than the old selected-offset/size probe in an old check; it is broader, but still does not satisfy the new freeze contract. |
| 5. Retired facade test | **PASS** | The removed section 6 contained only three assertions proving independence of the retired SysData `sensorType` facade from the still-live SensorData facade, plus its call/status line. All other facade behavior sections remain and pass. |
| 6. ELF and retained | **PASS** | Fresh Boron/Device OS 6.4.1 builds, separate `BUILD_PATH_BASE`, `-frandom-seed=p6`: **151724/1094/2228 → 151636/1094/2228** (`text/data/bss`, **−88/0/0**). Normalized `nm -C -S` accounts for it: `sysStatusData::initialize()` shrinks `0x1b0→0x188` (−40); linked setters for `verboseModeStartTime` (`0x0a`), `lowPowerMode` (`0x0c`), SysData `sensorType` (`0x0c`), and `structuresVersion` (`0x0c`) disappear (−46), with two bytes of link alignment. Static-initializer names/sizes are identical. No added line contains `retained`. |
| 7. Budget/suite/golden | **PASS** | Stage 6 `src/`: **+11/−154, net −143** (limit +20). Tests: **+105/−95, net +10** (guideline +100). Full suite **91/91**, serial: 53 shell tests via `zsh`, 38 Python tests via `python3`, stdin closed. `report_payload_golden_test.sh` passes and fixtures are unchanged. Stage-6-only `git diff --check` is clean; the full review range reports only carried whitespace inside the earlier prototype-diff document, covered by the spec's waiver. |

**Retained outcome: A — the whole `.backup` section is byte-identical.** Both are 152 bytes at `0x2003f400`, with identical symbols/addresses/sizes and SHA-256 `adfe530fcec43aa43fd659f9c9e2e83760bd9a1d217dfb77e715f7ed22d3b0e7`.

## Findings

1. **High — observation:** [`tests/persisted_layout_preservation_test.py:70`](../../tests/persisted_layout_preservation_test.py#L70) recognizes only one narrow declaration grammar, and lines 98–99 silently omit anything unmatched. An initialized appended member is therefore absent from the member count/probe; when it occupies tail padding, lines 150–169 see no size or positional change and the guard incorrectly passes. The reproduced `CurrentData` mutation above exited 0.
2. **High — observation:** [`tests/persisted_layout_preservation_test.py:163`](../../tests/persisted_layout_preservation_test.py#L163) freezes only `(offset, size)` pairs, without a stable identity for live members. Swapping `openTime` and `closeTime` leaves the positional sequence identical and exited 0, although persisted values would be interpreted through the opposite accessors. **Inference:** keep rename tolerance only for explicitly reserved slots, or freeze live-member identity/type as well as its offset/size; parsing should fail closed on any unrecognized data declaration.
3. **Low — observation:** [`src/MyPersistentData.h:119`](../../src/MyPersistentData.h#L119) still says fields may be appended at the end, and line 176 still calls the layout append-only. That contradicts this WO's frozen/no-future-append rule and should be updated with the guard fix.

## Budget and ruling 9

| Measure | Baseline | Candidate | Delta |
|---|---:|---:|---:|
| Stage 6 `src/` lines | — | +11/−154 | **−143** |
| Stage 6 test lines | — | +105/−95 | **+10** |
| Tracked test lines excluding fixtures (`0c565fa`) | 28,893 | 28,890 | −3 |
| Fixture lines | 834 | 834 | 0 |
| Root suite cases | 91 | 91 | 0 |
| Tracked `src/` lines (`0c565fa`) | 22,306 | 22,151 | −155 |

## Confirmations

- Review mutations and ARM builds were confined to detached worktrees made from a local bare clone under `build-tmp/wo20261022-003-stage7/`; the review tree was not mutated.
- No commit, push, stash, reset, checkout/restore, device, network, or AWS action was performed.
- The authorized scratch directory was removed after evidence collection. The only review-tree addition is this verdict file.

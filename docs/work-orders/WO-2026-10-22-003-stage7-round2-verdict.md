# Stage 7 round 2 verdict: WO-2026-10-22-003 P7-A

## VERIFIED WITH CONCERNS

**Reviewer:** Codex (`gpt-5.6-sol`), high reasoning  
**Review target:** `git diff cc84832 8a24443 -- src tests/persisted_layout_preservation_test.py`  
**Guard interpreter:** `#!/usr/bin/env python3`

No real freeze gap was found using the ordinary one-member-per-line declaration syntax used by the three persisted records.

| Check | Verdict | Independent evidence |
|---|---|---|
| `src/` code unchanged | **PASS** | The round-2 `src/` delta is **+3/−4, net −1**, entirely comments in `src/MyPersistentData.h`; no code line changed. The changed comments describe the persisted records as frozen. |
| Clean guard and frozen baseline | **PASS** | The clean guard reports **51/5/17 data members**, sizes **272/36/88**, and unchanged magic/version pairs **`0x20a15e75`/4, `0x20a47e74`/1, `0x20a99e74`/1** on ARM Cortex-M4. |
| Real freeze-gap search | **PASS** | Initialized and uninitialized tail-padding appends, a size-increasing end append, an insert, removal, live-member resize, live-member reorder and live-member rename all fail. A `reserved*` rename and the clean tree pass. |
| Not weaker than round 1 | **PASS** | The guard retains the ARM offset/size, record-size, magic and version checks, and adds exact live-member name/type checks, member-count checks, initialized-member parsing and fail-closed handling for otherwise unrecognized class-body lines. The two round-1 escapes—`bool stage7AppendedTailPadding = false;` and swapping `openTime`/`closeTime`—now fail. A version bump also fails. |
| Suite and diff hygiene | **PASS** | Full host suite **91/91**, serial: **53** shell tests via `zsh`, **38** Python tests via `python3`, stdin closed. Round-2 test delta is **+81/−36, net +45**; `git diff --check` is clean. |

## Finding

1. **Low — parser edge case (observation):** the method-declaration allowlist at [`tests/persisted_layout_preservation_test.py:97`](../../tests/persisted_layout_preservation_test.py#L97) also accepts a function-pointer data member. Appending `void (*stage7ParserEdge)();` to `CurrentData` is omitted from the extracted member list and fits its four bytes of ARM tail padding, so the guard exits 0. This declaration form is absent from the real records and is outside this round's verdict-failing ordinary-syntax scope.

## Confirmations

- No ordinary-syntax append, insert, remove, resize, reorder or live-member rename escaped the guard.
- The round-2 guard is not weaker than round 1 in any checked behavior.
- All mutations were confined to detached scratch worktrees and restored byte-identically. No commit, push, device, network or AWS action was performed.
- The authorized scratch directory was removed; the only review-tree addition is this verdict file.

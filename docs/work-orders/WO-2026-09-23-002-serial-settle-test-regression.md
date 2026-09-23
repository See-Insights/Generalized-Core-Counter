# WO-2026-09-23-002: serial_settle_test.py regression on main

**Status:** DRAFT — Stage 3 (Preliminary architecture). Not yet sent to Codex
or approved. No implementation has begun.

**Workflow role:** Produced by Claude (Architect) per `AI_DEVELOPMENT_WORKFLOW.md`.

**Relationship to WO-2026-09-23-001:** Discovered during Step 5's Stage 4
investigation, but independent of it — confirmed failing against `main` at
`efe0e4c` with zero Step 5 source changes present. Deliberately routed as its
own small WO rather than folded into Step 5, per Chip's routing decision:
Step 5 is already the largest, highest-risk step in the roadmap and just grew
(23→27 files, a third persisted store); bundling an unrelated pre-existing
regression into it would make Step 5's diff harder to review and would make
it ambiguous, if something goes wrong during Step 5, whether the serial-test
breakage was old or new.

## 1. Problem statement

`tests/serial_settle_test.py` fails on `main`. It is a structural test (no
`.sh` wrapper — a gap in this session's own verification coverage, since
"full host suite" sweeps only ever iterated `tests/*.sh`) that scans
`src/Generalized-Core-Counter.cpp` for `if (Serial.isConnected())` appearing
as the literal first statement inside `void setup() {`, strictly before
`ensureRetainedLoopForensicsInitialized()`.

The merged Item 2 fix from the 2026-09-22 cleanup batch (PR #37,
"Extract serialWait delay from setup() into waitForDebugSerialIfConnected()")
moved that check into a named function, `waitForDebugSerialIfConnected()`,
called from `setup()` as a single statement:

```cpp
static void waitForDebugSerialIfConnected() {
  if (Serial.isConnected()) {
    delay(ConnectivityPolicy::DEBUG_SERIAL_POST_CONNECT_DELAY_MS);
  }
}

void setup() {
  waitForDebugSerialIfConnected();
  ensureRetainedLoopForensicsInitialized();
  ...
```

This was a correct, intentional, reviewed change (pure move, zero behavior
change, confirmed via `git diff` at the time). The test's assumption —
that the check is textually inline in `setup()` — was invalidated by a
legitimate refactor the test did not anticipate. The underlying behavior the
test was written to protect (the settle delay fires, unconditionally,
before any other setup work) is still true; the test's *mechanism* for
checking that is now stale.

## 2. Operational impact

None on device behavior — this is a test-only defect. Impact is on the
project's verification discipline: this regression has been sitting
undetected on `main` since PR #37 merged (2026-09-22), because no `.sh`
wrapper exists for this test and no aggregate host-suite runner covers bare
`.py` files in `tests/`. It was found only because Step 5's Stage 4
investigation happened to run the full bare-`.py` set.

## 3. Scope

- Repository: `Generalized-Core-Counter`, branch off current `main`.
- File: `tests/serial_settle_test.py` only. No production source change
  required — the production code (`waitForDebugSerialIfConnected()`) is
  correct as merged.
- Non-goal: do not re-inline the check back into `setup()` to satisfy the
  test as originally written. The extraction was the correct direction; fix
  the test to verify the new (correct) structure.

## 4. Evidence

Confirmed independently, 2026-09-23, against `main` at `efe0e4c` (working
tree clean of any other modification):

```
$ git log --oneline -1
72b15fb Add per-entry capture-time timestamp to pdiag batch entries
$ git diff --stat
(empty)
$ python3 tests/serial_settle_test.py
FAIL: Serial.isConnected() check not found before
ensureRetainedLoopForensicsInitialized() in setup()
```

All 16 other bare-`.py` tests under `tests/` pass — this is isolated, not
systemic.

## 5. Proposed architecture

Update `tests/serial_settle_test.py` to verify the new structure while
preserving the original test's actual intent (per its own docstring: confirm
the settle delay is unconditional, fires only when a debug terminal is
already attached, and precedes any other setup work):

1. Confirm `waitForDebugSerialIfConnected();` is the first statement in
   `setup()` (replacing the current inline-`if` search), strictly before
   `ensureRetainedLoopForensicsInitialized()` and any application log line —
   same positional guarantee the original test checked, adapted to the new
   call-site form.
2. Separately confirm `waitForDebugSerialIfConnected()`'s own body contains
   `if (Serial.isConnected())` guarding
   `delay(ConnectivityPolicy::DEBUG_SERIAL_POST_CONNECT_DELAY_MS)` — i.e. the
   delay only ever executes inside that guard, never unconditionally. This
   preserves invariant 3 from the original test (the near-zero-field-cost
   property) against the function body instead of against `setup()`'s
   inline code.
3. Confirm no new `Serial.begin()` call was introduced (invariant 4,
   unchanged from the original test).

This is a pure test update — no production code changes, no new test
infrastructure, no change to `ALLOW_BLOCKING_SERIAL_WAITS` logic (confirmed
unrelated and untouched).

## 6. Risks / tradeoffs

Minimal. The only risk is writing an updated test that's just as brittle to
the *next* refactor (e.g. if `waitForDebugSerialIfConnected()` itself later
gets called from two places, or setup() gains an earlier statement for an
unrelated reason) — worth a brief consideration of whether a slightly looser
invariant (e.g. "called before X, guarded internally by
`Serial.isConnected()`, regardless of exact call-site position relative to
other early setup statements") is more durable than reproducing the original
test's exact positional strictness. Not a blocker — flagging for whoever
implements this to make a judgment call, not something to resolve here.

## 7. Acceptance criteria

1. `python3 tests/serial_settle_test.py` passes against current `main` plus
   this fix.
2. Full host suite green, including this test and the other 16 bare-`.py`
   tests (confirm the runner/process gap — no aggregate host-suite command
   currently covers bare `.py` files — is itself either fixed or explicitly
   named as a follow-up, so this class of miss doesn't recur silently).
3. No production source file changes.
4. Local ARM-toolchain build clean, cloud compile clean (expected no-op
   change to the binary, since this is test-only — confirm size-identical).

## 8. Non-goals

- Reverting or altering the `waitForDebugSerialIfConnected()` extraction.
- Adding a `.sh` wrapper for every bare-`.py` test as part of this WO (worth
  raising as a separate process question, not solved here).
- Any Step 5 work.

## 9. Rollback

Revert the test-file commit; zero production impact either way.

## Approval record

- [x] Chip approval (Stage 5) — dispatched directly, small/uncontroversial scope.
- [x] Copilot implementation (Stage 6) — `gh copilot`/`copilot` CLI, model
      `gpt-5.5`, three rounds (see Stage 6/7 log below).
- [x] Codex verification (Stage 7) — `codex review --uncommitted`, model
      `gpt-6-astra` (config default), three rounds, final verdict Verified.
- [ ] Chip final gate / commit (Stage 8)

## Stage 6/7 log

**Round 1 (Copilot implements):** Updated the test to check for
`waitForDebugSerialIfConnected();` at the call site instead of the old
inline `if`. Full host suite (21 `.sh` + 17 bare `.py`) passed. Local ARM
build (boron) and cloud compile both confirmed size-identical to the
pre-change baseline (149768/1090/2444 local; 150938/3530 Flash/RAM cloud).

**Round 1 review (Codex, Verified with concerns):** Mutation-tested the new
test itself and found the call-site check accepted a commented-out call
(`// waitForDebugSerialIfConnected();`) and a bench-gated call
(`if (ALLOW_BLOCKING_SERIAL_WAITS) waitForDebugSerialIfConnected();`) —
both should have been rejected. Root cause: the line matching the
helper-call substring was excluded from the "only comments/blank lines
before this point" check.

**Round 2 (Copilot fixes):** Tightened the check to require the exact
standalone statement as the first executable line. Claude independently
re-mutated the real source tree (not just Codex's in-memory reproduction)
with both mutations — both now correctly rejected; real source still
passes; restored byte-identical.

**Round 2 review (Codex, Verified with concerns):** Found two more issues:
(a) the fix used `int | None` type-hint syntax requiring Python 3.10+,
which crashes under this machine's `/usr/bin/python3` (3.9.6) — an
undocumented interpreter-version floor introduced by a test-only change;
(b) mutation-testing showed the "no additional `Serial.begin()`" check only
inspected a window around the helper function's *definition*, not its
*call site* in `setup()` — inserting `Serial.begin(9600);` right after the
call site passed undetected.

**Round 3 (Copilot fixes):** Replaced the annotation with `Optional[int]`;
added a separate executable-line check for `Serial.begin(` in the call-site
window. Claude independently confirmed both fixes: ran the test under both
`python3` (3.10.7) and `/usr/bin/python3` (3.9.6) — both pass; mutated the
real source tree with a `Serial.begin(9600);` inserted after the call site
— correctly rejected (`exit=1`); re-confirmed round-1's two mutations still
correctly rejected; source restored byte-identical.

**Round 3 review (Codex, Verified):** Ran 11 mutations independently
(missing call, log/forensics before call, bench-gated call, bench-gated
delay, unguarded delay, delay outside guard, guard inverted, delay removed,
`Serial.begin()` in helper, `Serial.begin()` at call site) — all 11
correctly rejected. All 17 Python tests pass. No actionable issues.

**Final independent verification (Claude):** Full host suite green under
both Python interpreters (21 `.sh` + 17×2 `.py`). Local ARM build
(boron): 149768/1090/2444 — identical to pre-change baseline across all
three rounds. Diff scope confirmed narrow throughout: only
`tests/serial_settle_test.py` changed in the final diff (150 lines
changed), zero production files touched, zero commits made.

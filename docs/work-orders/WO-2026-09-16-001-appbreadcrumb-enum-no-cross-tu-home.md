# WO-2026-09-16-001: `AppBreadcrumb` enum has no cross-translation-unit home

**Type:** Structural fix (Q6 from the ownership map - not yet closed).

**Status:** Drafted, not dispatched. Documentation only - **no fix
authorized.**

**Origin:** Discovered during Step 2 (`HibernateCycle`, this session
2026-09-16) while attempting to fold in the Q6 breadcrumb-literal cleanup
"since `State_Sleep.cpp` was open anyway." The attempt failed to *compile*
(not a test failure - a real `arm-none-eabi-gcc` build error), which is why
this is being filed as its own WO rather than finished inline: the fix
turned out to be structural, not textual, and touching more than the one
file that dispatch authorized.

## The observation

`enum AppBreadcrumb : uint8_t { BREADCRUMB_NONE = 0, ... }` is declared at
`Generalized-Core-Counter.cpp:192`, **inside that file's own anonymous
namespace** (`namespace { ... }`, opened at line 160, closed at line 447).
There is no comment anywhere explaining this as a deliberate choice - it
reads as incidental, not intentional: the enum sits in the same anonymous
namespace as several other genuinely file-local helpers (e.g.
`AwakeWatchdogSleepStrategy`, declared a few lines above it), and appears to
have been swept into that namespace along with them rather than placed
there on purpose.

The consuming function, `void setAppBreadcrumb(uint8_t code)`, **is**
properly shared - declared in `state/State_Common.h:133`, defined in
`Generalized-Core-Counter.cpp:715` (outside the anonymous namespace, with
external linkage) - so any file can already call it. What no other file can
do is call it **with a name** instead of a bare integer, because the names
themselves are invisible outside `Generalized-Core-Counter.cpp`.

This is not a linkage/ODR problem (unlike `ab1805WakeReasonName()`, fixed
during Step 2 with a `...Local()`-plus-wrapper split) - there is no runtime
symbol to link against. It is purely a missing include path: nothing
outside `Generalized-Core-Counter.cpp` can spell `BREADCRUMB_SLEEP_ENTRY`,
because nothing outside that file has ever been able to see it declared.

### Every raw-numeric call site, repo-wide (confirmed by direct search, not assumed)

```
src/state/State_Connect.cpp:383   setAppBreadcrumb(6);
src/state/State_Connect.cpp:531   setAppBreadcrumb(7);
src/state/State_Sleep.cpp:347     setAppBreadcrumb(3);
src/state/State_Sleep.cpp:863     setAppBreadcrumb(22);  // + 5 more collision-history comments
src/state/State_Sleep.cpp:951     setAppBreadcrumb(23);
src/state/State_Sleep.cpp:1067    setAppBreadcrumb(24);
src/state/State_Sleep.cpp:1091    setAppBreadcrumb(25);
src/state/State_Sleep.cpp:1093    setAppBreadcrumb(26);
src/state/State_Sleep.cpp:1098    setAppBreadcrumb(27);
src/state/State_Sleep.cpp:1100    setAppBreadcrumb(28);
src/state/State_Sleep.cpp:1152    setAppBreadcrumb(24);
src/state/State_Sleep.cpp:1349    setAppBreadcrumb(25);
src/state/State_Sleep.cpp:1351    setAppBreadcrumb(26);
src/state/State_Sleep.cpp:1352    setAppBreadcrumb(28);
src/state/State_Sleep.cpp:1392    setAppBreadcrumb(24);
src/state/State_Sleep.cpp:1400    setAppBreadcrumb(25);
src/state/State_Sleep.cpp:1402    setAppBreadcrumb(26);
src/state/State_Sleep.cpp:1403    setAppBreadcrumb(28);
src/state/State_Sleep.cpp:1408    setAppBreadcrumb(24);
src/state/State_Sleep.cpp:1414    setAppBreadcrumb(25);
src/state/State_Sleep.cpp:1416    setAppBreadcrumb(26);
src/state/State_Sleep.cpp:1417    setAppBreadcrumb(28);
src/state/State_Sleep.cpp:1457    setAppBreadcrumb(4);
src/state/State_Sleep.cpp:1649    setAppBreadcrumb(5);
```

24 call sites total: 22 in `State_Sleep.cpp`, 2 in `State_Connect.cpp`
(values 6 and 7 - `BREADCRUMB_CONNECT_REQUESTED` /
`BREADCRUMB_CLOUD_CONNECTED`). No other file was found to call
`setAppBreadcrumb()` with a raw numeric literal - `Generalized-Core-Counter.cpp`
itself already uses named constants exclusively at every one of its own call
sites, which is presumably why this gap wasn't noticed sooner: the file that
can see the names uses them; the two files that can't, don't.

## This is the same shape as the incident this quarter already paid for

This is Q6 from the Structural Ownership Map roadmap, and the underlying
shape is the v21/v24 breadcrumb literal-collision incident from
`WO-2026-09-03-004` (the MAFC-1 investigation): a raw numeric breadcrumb
value meant two different things across firmware generations because
nothing enforced a single named source of truth. That incident was
**contained on the read side** by `particle-fleet-operations`'
`tools/breadcrumb-decode.js`, which decodes a raw `bc` value against the
tree it was compiled from rather than a single global enum. This WO is
about the **write side**: as long as `AppBreadcrumb`'s names are reachable
from only one of the files that write breadcrumbs, a future collision
inside `State_Sleep.cpp` or `State_Connect.cpp` remains just as possible as
the original one was - nothing here has actually been fixed yet, only
worked around after the fact.

## Severity

Low / no live functional risk. This is a maintainability and
drift-prevention fix, not a fix for anything currently broken - the
existing collisions were already resolved by the read-side decoder, and no
new collision is currently known to exist. The risk this WO addresses is
that the *next* new breadcrumb added to `State_Sleep.cpp` or
`State_Connect.cpp` has no way to be checked against the existing set by
name, only by manually reading the enum in a different file and copying a
number by hand - which is exactly how the original incident happened.

## Minimal correct fix shape (assessed, not implemented)

Extract `AppBreadcrumb` (the enum only - not `setAppBreadcrumb()`, which is
already correctly shared via `state/State_Common.h`, and not the retained
`appBreadcrumb`/`appBreadcrumbMs` storage, which can stay put) into its own
header - `state/AppBreadcrumb.h` or similar, matching this project's
existing per-concern header convention (e.g. `state/SleepPrepSpanTiming.h`
sitting alongside `state/State_Sleep.cpp` for a single narrow concern).
`Generalized-Core-Counter.cpp`, `State_Sleep.cpp`, and `State_Connect.cpp`
would all include it. `appBreadcrumbName()` (the switch-to-string function,
currently also anonymous-namespace-local in `Generalized-Core-Counter.cpp`)
would need the same treatment if callers outside that file ever need to
stringify a code - not currently the case, so left out of this fix's
minimal shape unless scoping turns up a need.

**Step 5 overlap - checked, not found, but flagging the limits of that
check.** Step 5 (the persistence-header split) is about auditing
`MyPersistentData.h`'s 22 includers and splitting its `StorageHelperRK`-backed
persistence bag by concern (architecture review item 2). `AppBreadcrumb` is
declared in `Generalized-Core-Counter.cpp`, not `MyPersistentData.h`, and
the retained `appBreadcrumb` variable it initializes lives in `Generalized-Core-Counter.cpp`'s
own `retained` SRAM - a completely different retention mechanism from
`MyPersistentData.h`'s flash-file-backed bag (the same distinction
established during this week's `retainedHibernateCount` investigation). No
overlap identified. Caveat: this project's "Step 5" scope was agreed
interactively earlier this session and is not captured in a committed
document I could check directly - this assessment is my best reading of
what's been referenced about it in adjacent WOs, not a confirmation against
a source-of-truth spec. Flagging that limit rather than presenting this as
more certain than it is.

## `sleep_breadcrumb_sequence_test.py` - confirmed will need updating, and how

This test parses `State_Sleep.cpp` directly and searches for literal tokens
like `"setAppBreadcrumb(22)"`. It is **not** indifferent to this change -
switching the source to named constants (`setAppBreadcrumb(BREADCRUMB_SLEEP_GATE_START)`)
would make every one of its `find_all("setAppBreadcrumb(N)")` calls return
zero matches, failing the test, not passing it silently. This was confirmed
directly during the Step 2 attempt: the test file was actually updated (token
strings swapped from bare numbers to the named-constant spelling) and
verified passing, before the whole change was reverted together with the
source when the compile failure surfaced. That update is known-good and
can be reapplied verbatim once the header extraction lands - it does not
need to be re-derived.

## Scope estimate

Same-day fix, not a multi-day effort: one new header (a handful of lines,
copying the existing enum verbatim), three `#include` additions, 24
call-site edits (mechanical, one number replaced with one name each, using
comments already present at most `State_Sleep.cpp` sites as the mapping
key), and the one already-known `sleep_breadcrumb_sequence_test.py` update.

**Does need its own bench validation**, though, same as Step 2 did - it
touches `State_Sleep.cpp`, which is squarely inside the sleep/wake path
Step 1 and Step 2 both required bench nights for. Not a same-day
close-the-loop fix; a same-day *code* fix followed by the same two-night
soak discipline applied to every sleep-path change so far this session.

## Not authorized in this WO

Any code change - the header extraction, the call-site edits, and the test
update are all described above as the assessed shape of the fix, not
performed here.

## Provenance

Surfaced during Step 2 (`HibernateCycle`) implementation, 2026-09-16, when
folding in the Q6 breadcrumb cleanup "since `State_Sleep.cpp` was open
anyway" hit a real compile failure (`arm-none-eabi-gcc`: `'BREADCRUMB_SLEEP_ENTRY'
was not declared in this scope` and 13 more identical errors across the 22
`State_Sleep.cpp` call sites). Reverted cleanly rather than expanded
mid-Step-2, per that dispatch's explicit scope discipline ("if this turns
out to be nontrivial or touches more than `State_Sleep.cpp`, stop and
report rather than expanding"). `State_Sleep.cpp` and
`sleep_breadcrumb_sequence_test.py` are both confirmed byte-identical to
`main` as of that revert.

## Related work orders

- `docs/architecture-review-2026-09-03.md` - source of the Structural
  Ownership Map roadmap; Q6 is this WO's origin item.
- `WO-2026-09-03-004` - the MAFC-1 investigation whose breadcrumb-decode
  correction is the read-side half of this same underlying problem.
- `WO-2026-09-14-002` - Step 1, established the narrow-seam/single-owner
  pattern this WO's proposed header extraction would follow.
- Step 2 (`HibernateCycle`, not yet closed with its own WO doc as of this
  writing) - the dispatch this finding was surfaced under.

#!/usr/bin/env python3
"""Step 5 fan-in invariants for the persistence facade split (WO-2026-09-23-001).

Step 0's root problem recurring at header granularity was that every consumer
of `MyPersistentData.h` structurally depended on `StorageHelperRK.h` and on
every other consumer's fields. The four `src/persist/*.h` facades exist to
close that seam. Two fan-in counts are what actually prove it closed:

  1. `MyPersistentData.h` is included by exactly one file in src/ -
     `MyPersistentData.cpp`, its own implementation.
  2. `StorageHelperRK.h` is included by exactly one file in src/ -
     `MyPersistentData.h`, which is itself reachable only from
     `MyPersistentData.cpp` by invariant 1. So exactly one translation unit
     in the whole application sees the storage layer.

A third invariant guards the failure mode Codex's Finding 1 named: a facade
that includes the storage layer, the monolithic header, or a sibling facade
would silently reintroduce the coupling while every other check still passed.
That failure is invisible by inspection of any single facade in isolation.

MUTATION TESTING
----------------
A structural test that only ever runs against a passing tree proves nothing
about what it would catch. `check()` below is a pure function of a
{path: text} corpus, so this file runs it three ways: against the real tree
(must pass), and against each of several synthetic corpora carrying exactly
the regressions this test exists to catch (each must fail, for the expected
reason). If a mutation stops failing, this test has gone blind and says so.
"""
import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
SRC_ROOT = REPO_ROOT / "src"

OWNER_HEADER = "src/MyPersistentData.h"
OWNER_SOURCE = "src/MyPersistentData.cpp"
STORAGE_HEADER = "StorageHelperRK.h"
FACADE_DIR = "src/persist/"

# Include matching deliberately tolerates every spelling the preprocessor
# does - angle brackets, arbitrary whitespace after the hash, and relative
# prefixes. Stage 7 review falsified an earlier version of these patterns that
# only accepted `#include "..."`.
#
# This file is the fast, broad first-line check across all of src/. The
# authoritative per-facade check is facade_header_isolation_test.py, which
# reads the compiler's own dependency listing and therefore cannot be fooled
# by a spelling nobody anticipated.
INC = r'^[ \t]*#[ \t]*include[ \t]*'
OWNER_INCLUDE = re.compile(INC + r'["<](?:[^">]*/)?MyPersistentData\.h[">]', re.M)
STORAGE_INCLUDE = re.compile(INC + r'["<](?:[^">]*/)?StorageHelperRK\.h[">]', re.M)
# A facade must not reach a sibling, the monolith, the storage layer or
# Particle.h. Siblings are matched by BARE filename too, because that is how a
# file inside src/persist/ refers to its neighbour.
FACADE_FORBIDDEN = re.compile(
    INC + r'["<](?:[^">]*/)?(StorageHelperRK\.h|MyPersistentData\.h|Particle\.h|'
    r'SystemConfig\.h|RecoveryState\.h|PowerConfig\.h|CurrentReadings\.h)[">]',
    re.M,
)


def strip_comments(code: str) -> str:
    code = re.sub(r"/\*.*?\*/", "", code, flags=re.DOTALL)
    code = re.sub(r"//[^\n]*", "", code)
    return code


def check(corpus):
    """Return a list of failure strings for a {relative_path: text} corpus."""
    failures = []
    code = {p: strip_comments(t) for p, t in corpus.items()}

    owner_includers = sorted(p for p, t in code.items() if OWNER_INCLUDE.search(t))
    if owner_includers != [OWNER_SOURCE]:
        failures.append(
            f"MyPersistentData.h must be included by exactly one file "
            f"({OWNER_SOURCE}); found: {owner_includers or 'none'}"
        )

    storage_includers = sorted(p for p, t in code.items() if STORAGE_INCLUDE.search(t))
    if storage_includers != [OWNER_HEADER]:
        failures.append(
            f"{STORAGE_HEADER} must be included by exactly one file in src/ "
            f"({OWNER_HEADER}); found: {storage_includers or 'none'}"
        )

    facades = sorted(p for p in code if p.startswith(FACADE_DIR))
    if not facades:
        failures.append(f"no facade headers found under {FACADE_DIR}")
    for path in facades:
        own_name = path.rsplit("/", 1)[-1]
        for match in FACADE_FORBIDDEN.finditer(code[path]):
            if match.group(1) == own_name:
                continue  # a facade may not include itself, but #pragma once is fine
            failures.append(
                f"{path} includes {match.group(1)} - a facade must not depend "
                "on the storage layer, the monolithic header, Particle.h, or a "
                "sibling facade (Finding 1)"
            )
    return failures


def real_corpus():
    corpus = {}
    for path in SRC_ROOT.rglob("*"):
        if path.suffix in (".cpp", ".h"):
            corpus[str(path.relative_to(REPO_ROOT))] = path.read_text()
    return corpus


MUTATIONS = [
    (
        "a consumer re-acquires the monolithic header",
        lambda c: c.update(
            {"src/state/State_Idle.cpp": '#include "MyPersistentData.h"\n' + c["src/state/State_Idle.cpp"]}
        ),
        "MyPersistentData.h must be included by exactly one file",
    ),
    (
        "a consumer reaches the monolithic header by relative path",
        lambda c: c.update(
            {
                "src/observability/StartupSnapshotRuntime.cpp": '#include "../MyPersistentData.h"\n'
                + c["src/observability/StartupSnapshotRuntime.cpp"]
            }
        ),
        "MyPersistentData.h must be included by exactly one file",
    ),
    (
        "a second file picks up StorageHelperRK.h directly",
        lambda c: c.update(
            {"src/power/PowerManager.cpp": '#include "StorageHelperRK.h"\n' + c["src/power/PowerManager.cpp"]}
        ),
        f"{STORAGE_HEADER} must be included by exactly one file",
    ),
    (
        "a facade re-exposes the storage layer (Finding 1)",
        lambda c: c.update(
            {"src/persist/SystemConfig.h": '#include "StorageHelperRK.h"\n' + c["src/persist/SystemConfig.h"]}
        ),
        "a facade must not depend on the storage layer",
    ),
    (
        "a facade pulls in the monolithic header (Finding 1)",
        lambda c: c.update(
            {"src/persist/PowerConfig.h": '#include "MyPersistentData.h"\n' + c["src/persist/PowerConfig.h"]}
        ),
        "MyPersistentData.h must be included by exactly one file",
    ),
    (
        "a facade leans on a sibling facade (Finding 1)",
        lambda c: c.update(
            {"src/persist/RecoveryState.h": '#include "persist/CurrentReadings.h"\n' + c["src/persist/RecoveryState.h"]}
        ),
        "sibling facade",
    ),
    (
        "a facade leans on a sibling by BARE filename (Stage 7 case 1)",
        lambda c: c.update(
            {"src/persist/RecoveryState.h": '#include "CurrentReadings.h"\n' + c["src/persist/RecoveryState.h"]}
        ),
        "sibling facade",
    ),
    (
        "a facade reaches the monolith in angle-bracket form",
        lambda c: c.update(
            {"src/persist/PowerConfig.h": "#include <MyPersistentData.h>\n" + c["src/persist/PowerConfig.h"]}
        ),
        "MyPersistentData.h must be included by exactly one file",
    ),
    (
        "a consumer reaches the storage layer with whitespace after the hash",
        lambda c: c.update(
            {"src/time/Clock.cpp": '#  include "StorageHelperRK.h"\n' + c["src/time/Clock.cpp"]}
        ),
        f"{STORAGE_HEADER} must be included by exactly one file",
    ),
    (
        "a facade takes on Particle.h",
        lambda c: c.update(
            {"src/persist/CurrentReadings.h": '#include "Particle.h"\n' + c["src/persist/CurrentReadings.h"]}
        ),
        "Particle.h",
    ),
]


def main() -> None:
    corpus = real_corpus()

    failures = check(corpus)
    if failures:
        for f in failures:
            print(f"FAIL: {f}", file=sys.stderr)
        sys.exit(1)

    # Mutation testing: every planted regression must be caught.
    blind = []
    for name, mutate, expected in MUTATIONS:
        mutated = dict(corpus)
        mutate(mutated)
        caught = check(mutated)
        if not caught:
            blind.append(f"{name}: planted regression was NOT caught")
        elif not any(expected in f for f in caught):
            blind.append(
                f"{name}: caught something, but not for the expected reason "
                f"(wanted {expected!r}, got {caught!r})"
            )
    if blind:
        for b in blind:
            print(f"FAIL (mutation): {b}", file=sys.stderr)
        sys.exit(1)

    print(f"OK: MyPersistentData.h has exactly one includer in src/ ({OWNER_SOURCE})")
    print(f"OK: {STORAGE_HEADER} has exactly one includer in src/ ({OWNER_HEADER})")
    print("OK: no facade includes the storage layer, the monolithic header, Particle.h, or a sibling facade")
    print(f"OK: all {len(MUTATIONS)} planted regressions were caught (test is not blind)")
    print("persistence_facade_fanin_structural_test: all invariants hold")


if __name__ == "__main__":
    main()

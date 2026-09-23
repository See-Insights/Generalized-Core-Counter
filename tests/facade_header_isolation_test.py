#!/usr/bin/env python3
"""Facade header isolation (WO-2026-09-23-001, Step 5, revised criterion 8).

Each `src/persist/*.h` facade must be genuinely self-contained: it must
compile on its own, and - the part that actually matters - it must not reach
any other project header, by any route.

WHY THIS TEST WORKS THE WAY IT DOES
-----------------------------------
The first version checked for a fixed list of undeclared types plus a regex
over include lines. Stage 7 review proved that approach unsound: it only
recognises the spellings its author happened to think of. A facade including
a sibling by bare filename (`#include "CurrentReadings.h"`, which is how a
file inside src/persist/ naturally refers to its neighbour) passed cleanly,
and so did a leak routed through an intermediate header.

Enumerating spellings was never going to be complete. So the real check no
longer reads source at all: it asks the compiler for its own dependency
listing (`-MM`) and requires that the only project file a facade depends on
is the facade itself. That is closed by construction - a leak must appear in
the dependency listing to affect the build at all, whatever spelling,
nesting depth or include path produced it. Same principle as
persisted_layout_preservation_test.py diffing compiler output rather than
source text.

Run under host Clang and the real ARM device toolchain, because the two
disagree about what the standard headers drag in.

MUTATION TESTING
----------------
Every planted leak below is applied to a scratch copy of the facade tree
(production files are never touched) and must be caught. The first two are
the exact cases Stage 7 used to falsify the previous version.
"""
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
SRC_ROOT = REPO_ROOT / "src"
FACADE_DIR = SRC_ROOT / "persist"
ARM_GXX = Path("/Users/chipmc/.particle/toolchains/gcc-arm/10.2.1/bin/arm-none-eabi-g++")

HOST = ("host clang", ["clang++", "-std=c++17", "-Wall", "-Wextra"])
ARM = ("ARM gcc", [str(ARM_GXX), "-std=gnu++14", "-mcpu=cortex-m4", "-mthumb",
                   "-mabi=aapcs", "-mfloat-abi=softfp", "-mfpu=fpv4-sp-d16",
                   "-Wall", "-Wextra"])


def fail(msg):
    print(f"FAIL: {msg}", file=sys.stderr)
    sys.exit(1)


def run(cmd):
    return subprocess.run(cmd, capture_output=True, text=True)


def compile_standalone(toolchain, header, include_dirs, workdir):
    _, base = toolchain
    tu = workdir / "standalone.cpp"
    tu.write_text(f'#include "persist/{header}"\n')
    return run(base + ["-fsyntax-only"] + [f"-I{d}" for d in include_dirs] + [str(tu)])


def project_dependencies(toolchain, header, include_dirs, workdir):
    """Ask the compiler which PROJECT files the facade actually pulls in.

    `-MM` omits system/standard headers, so what remains is exactly the
    project surface the facade depends on.
    """
    _, base = toolchain
    tu = workdir / "deps.cpp"
    tu.write_text(f'#include "persist/{header}"\n')
    result = run(base + ["-MM"] + [f"-I{d}" for d in include_dirs] + [str(tu)])
    if result.returncode != 0:
        return False, set(), result.stderr

    text = result.stdout.replace("\\\n", " ")
    if ":" in text:
        text = text.split(":", 1)[1]
    deps = set()
    for token in text.split():
        p = Path(token)
        if not p.is_absolute():
            p = (workdir / token).resolve()
        else:
            p = p.resolve()
        if p == tu.resolve():
            continue
        roots = [str(Path(d).resolve()) for d in include_dirs] + [str(REPO_ROOT)]
        if any(str(p).startswith(r) for r in roots):
            deps.add(p)
    return True, deps, ""


def check_facade(toolchain, header, include_dirs, facade_root, workdir):
    """Return a list of failure strings for one facade under one toolchain."""
    name, _ = toolchain
    failures = []

    result = compile_standalone(toolchain, header, include_dirs, workdir)
    if result.returncode != 0:
        failures.append(f"{header} does not compile standalone under {name}:\n{result.stderr}")
        return failures

    ok, deps, stderr = project_dependencies(toolchain, header, include_dirs, workdir)
    if not ok:
        failures.append(f"{header} dependency listing failed under {name}:\n{stderr}")
        return failures

    allowed = {(Path(facade_root) / header).resolve()}
    leaked = sorted(str(d) for d in deps - allowed)
    if leaked:
        failures.append(
            f"{header} depends on project header(s) it must not reach, under "
            f"{name}: {leaked} - a facade must depend on nothing but itself "
            "(sibling facades, the monolithic header and the storage layer are "
            "all out of bounds, by any include spelling or nesting depth)"
        )
    return failures


# Each mutation edits a scratch copy of the facade tree; `extra_files` are
# written into the scratch persist/ directory first.
MUTATIONS = [
    {
        "name": "direct sibling include by bare filename (Stage 7 case 1)",
        "target": "RecoveryState.h",
        "inject": '#include "CurrentReadings.h"\n',
        "extra_files": {},
    },
    {
        "name": "sibling reached through an intermediate header (Stage 7 case 2)",
        "target": "RecoveryState.h",
        "inject": '#include "_leak_intermediate.h"\n',
        "extra_files": {"_leak_intermediate.h": '#pragma once\n#include "CurrentReadings.h"\n'},
    },
    {
        "name": "sibling include in angle-bracket form",
        "target": "PowerConfig.h",
        "inject": "#include <CurrentReadings.h>\n",
        "extra_files": {},
    },
    {
        "name": "sibling include with whitespace after the hash",
        "target": "PowerConfig.h",
        "inject": '#   include "SystemConfig.h"\n',
        "extra_files": {},
    },
    {
        "name": "sibling include via the persist/-prefixed spelling",
        "target": "CurrentReadings.h",
        "inject": '#include "persist/SystemConfig.h"\n',
        "extra_files": {},
    },
    {
        "name": "the monolithic header leaks back in",
        "target": "SystemConfig.h",
        "inject": '#include "MyPersistentData.h"\n',
        "extra_files": {},
    },
    {
        "name": "the storage layer leaks back in",
        "target": "SystemConfig.h",
        "inject": '#include "StorageHelperRK.h"\n',
        "extra_files": {},
    },
]


def build_scratch_tree(tmp, mutation):
    root = tmp / "mutated"
    if root.exists():
        shutil.rmtree(root)
    persist = root / "persist"
    persist.mkdir(parents=True)
    for header in FACADE_DIR.glob("*.h"):
        shutil.copy(header, persist / header.name)
    for fname, content in mutation["extra_files"].items():
        (persist / fname).write_text(content)
    target = persist / mutation["target"]
    text = target.read_text()
    if "#pragma once\n" not in text:
        fail(f"{mutation['target']} has no `#pragma once` to inject after - mutation harness is broken")
    target.write_text(text.replace("#pragma once\n", "#pragma once\n" + mutation["inject"], 1))
    return root


def main():
    facades = sorted(p.name for p in FACADE_DIR.glob("*.h"))
    if not facades:
        fail(f"no facade headers found in {FACADE_DIR}")

    toolchains = [HOST]
    if ARM_GXX.is_file():
        toolchains.append(ARM)
    else:
        print(f"NOTE: ARM toolchain not found at {ARM_GXX}; host-only run")

    with tempfile.TemporaryDirectory() as tmpname:
        tmp = Path(tmpname)
        workdir = tmp / "work"
        workdir.mkdir()

        # --- Clean run: every facade, every toolchain, must be self-contained.
        for header in facades:
            for toolchain in toolchains:
                problems = check_facade(toolchain, header, [SRC_ROOT], FACADE_DIR, workdir)
                if problems:
                    for p in problems:
                        print(f"FAIL: {p}", file=sys.stderr)
                    sys.exit(1)

        # --- Mutation run: every planted leak must be caught.
        blind = []
        for mutation in MUTATIONS:
            root = build_scratch_tree(tmp, mutation)
            includes = [root, root / "persist", SRC_ROOT]
            caught_by = []
            for toolchain in toolchains:
                if check_facade(toolchain, mutation["target"], includes, root / "persist", workdir):
                    caught_by.append(toolchain[0])
            if not caught_by:
                blind.append(f"{mutation['name']}: planted leak in {mutation['target']} was NOT caught")

        if blind:
            for b in blind:
                print(f"FAIL (mutation): {b}", file=sys.stderr)
            sys.exit(1)

    names = ", ".join(t[0] for t in toolchains)
    for header in facades:
        print(f"OK: persist/{header} compiles standalone; dependency listing contains "
              f"no project header but itself ({names})")
    print(f"OK: all {len(MUTATIONS)} planted leaks caught, including both Stage 7 "
          "falsification cases (bare-filename sibling, indirect chain)")
    print("facade_header_isolation_test: every facade is self-sufficient and genuinely narrow")


if __name__ == "__main__":
    main()

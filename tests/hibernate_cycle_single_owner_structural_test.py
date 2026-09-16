#!/usr/bin/env python3
"""
WO-2026-09-16 Step 2 - structural regression test for the hibernate-cycle
retained fields' single ownership.

Background: before this step, retainedHibernateRtcBefore/WakeTime/
RequestedSleep/Count/Pending were declared in Generalized-Core-Counter.cpp,
armed by State_Sleep.cpp, and read back by Generalized-Core-Counter.cpp on
the wake boot - three files sharing one lifecycle with no single owner (the
architecture review's "no single owner of what time is it after this wake"
finding). This step moved all four still-read fields (retainedHibernateWakeTime
had no readers anywhere in the tree and was not carried forward) into
src/time/HibernateCycle.cpp, declared in an anonymous namespace with no
`extern` anywhere else - so no other translation unit can even reference
them; single ownership is enforced by the language, not just convention,
as long as no `extern` declaration exists for them elsewhere.

This is a source-invariant check on the REAL checked-in files, not a
mirror. Comments are stripped before each check so prose (including this
file's own history, if pasted into source) cannot produce a false positive
or a false negative.

  1. Each of the four fields is `retained`-declared exactly once in the
     entire src/ tree, and that one declaration is in
     src/time/HibernateCycle.cpp.
  2. No `extern` declaration for any of the four fields exists anywhere in
     src/ - if one did, some other file could read or write the field
     directly, defeating the single-ownership move even though the
     declaration itself stayed put.
  3. retainedHibernateWakeTime - confirmed to have no readers anywhere in
     the tree during this step's baseline investigation - was not carried
     forward as a declaration anywhere (it may still be mentioned in prose
     explaining why).
"""
import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
SRC_ROOT = REPO_ROOT / "src"
OWNER_FILE = SRC_ROOT / "time" / "HibernateCycle.cpp"

CARRIED_FORWARD_FIELDS = [
    "retainedHibernateRtcBefore",
    "retainedHibernateRequestedSleep",
    "retainedHibernateCount",
    "retainedHibernatePending",
]
DELETED_FIELD = "retainedHibernateWakeTime"

SOURCE_SUFFIXES = (".cpp", ".h")


def strip_comments(code: str) -> str:
    """Remove // line comments and /* */ block comments so prose mentioning
    these identifiers (explaining this history, or the field's own doc
    comments) cannot produce a false positive or a false negative."""
    code = re.sub(r"/\*.*?\*/", "", code, flags=re.DOTALL)
    code = re.sub(r"//[^\n]*", "", code)
    return code


def fail(msg: str) -> None:
    print(f"FAIL: {msg}", file=sys.stderr)
    sys.exit(1)


def all_source_files():
    return sorted(p for p in SRC_ROOT.rglob("*") if p.suffix in SOURCE_SUFFIXES)


def main() -> None:
    if not OWNER_FILE.is_file():
        fail(f"{OWNER_FILE} does not exist")

    files = all_source_files()
    code_by_file = {f: strip_comments(f.read_text()) for f in files}

    # ---- Invariant 1: exactly one `retained ... <field>` declaration per
    # field, in HibernateCycle.cpp. ----
    for field in CARRIED_FORWARD_FIELDS:
        decl_pattern = re.compile(r"\bretained\b[^;{}\n]*\b" + re.escape(field) + r"\b\s*=")
        declaring_files = [f for f, code in code_by_file.items() if decl_pattern.search(code)]
        if len(declaring_files) != 1:
            fail(
                f"{field}: expected exactly 1 `retained` declaration in src/, "
                f"found {len(declaring_files)}: {[str(f.relative_to(REPO_ROOT)) for f in declaring_files]}"
            )
        if declaring_files[0] != OWNER_FILE:
            fail(
                f"{field}: sole `retained` declaration is in "
                f"{declaring_files[0].relative_to(REPO_ROOT)}, expected {OWNER_FILE.relative_to(REPO_ROOT)}"
            )
    print(f"OK: each of {len(CARRIED_FORWARD_FIELDS)} carried-forward fields is `retained`-declared exactly once, in {OWNER_FILE.relative_to(REPO_ROOT)}")

    # ---- Invariant 2: no `extern` declaration for any field anywhere -
    # otherwise another file could still read/write it directly regardless
    # of where the declaration itself lives. ----
    for field in CARRIED_FORWARD_FIELDS:
        extern_pattern = re.compile(r"\bextern\b[^;{}\n]*\b" + re.escape(field) + r"\b")
        extern_files = [f for f, code in code_by_file.items() if extern_pattern.search(code)]
        if extern_files:
            fail(
                f"{field}: found `extern` declaration(s) outside its owner - "
                f"{[str(f.relative_to(REPO_ROOT)) for f in extern_files]} - "
                "single ownership requires no other file be able to reference it directly"
            )
    print("OK: no `extern` declaration exists for any carried-forward field outside its owner")

    # ---- Invariant 3 (positive control): the deleted field was not
    # quietly re-added as a declaration anywhere. ----
    deleted_decl_pattern = re.compile(r"\bretained\b[^;{}\n]*\b" + re.escape(DELETED_FIELD) + r"\b\s*=")
    resurrected = [f for f, code in code_by_file.items() if deleted_decl_pattern.search(code)]
    if resurrected:
        fail(
            f"{DELETED_FIELD} was confirmed dead (no readers anywhere in the tree) and "
            f"deliberately not carried forward, but a `retained` declaration for it was "
            f"found in {[str(f.relative_to(REPO_ROOT)) for f in resurrected]}"
        )
    print(f"OK: {DELETED_FIELD} was not re-added as a declaration anywhere")

    print("hibernate_cycle_single_owner_structural_test: all invariants hold")


if __name__ == "__main__":
    main()

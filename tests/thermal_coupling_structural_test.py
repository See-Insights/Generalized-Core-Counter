#!/usr/bin/env python3
"""
WO-2026-08-25-001 Amendment C, Decision C1 - structural coupling regression
test (AC-C1/AC-C2/AC-C3/AC-C8).

This is a source-invariant check on the REAL production files
(src/sensors/SensorManager.h / .cpp), not a mirror or reimplementation. It is
a deliberate COMPLEMENT to (not a substitute for) a full runtime/link test:
SensorManager.cpp pulls in PublishQueuePosixRK, SensorFactory, StateMachine,
and Connectivity - a stub surface large enough that building a host-linkable
test harness for it was judged disproportionate to this dispatch's remaining
scope (see IMPLEMENTATION_REPORT_R4B.md). The mandatory
`particle compile boron` build is the actual integration-level validation
that this function compiles and links correctly on-device; this test instead
asserts, directly against the checked-in source text, the specific structural
properties Amendment C requires and that a future edit could otherwise
silently violate:

  1. There is exactly ONE function that reads enclosure temperature AND
     exactly one call site per platform (AC-C1 point 1 / AC-C8) - no public
     entry point exists to read temperature or evaluate the charge decision
     independently (the old `isItSafeToCharge()` public API must not exist).
  2. The decision consumes the LOCAL values read this call
     (`measuredThisCall`, `tempC`), never re-reading
     `current.get_internalTempC()` as the evaluation input (AC-C1 point 2).
  3. No `return` statement appears between the start of the function body and
     the decision call - i.e. no path can read a temperature and skip the
     decision (AC-C1 point 3 / AC-C3).
  4. The coupled function AND the genuine acquisition half
     (`readTmp112TemperatureC()`) are both declared `private` - not merely
     non-`public` - and no OTHER `public:` or `protected:` member in
     `SensorManager` can read the enclosure temperature sensor. `protected`
     was rejected as sufficient here: a derived class can call a protected
     member, so a `protected` coupled function or acquisition half would
     still be a separately reachable entry point from a future subclass. The
     class currently has no subclasses, but the ACCESS SPECIFIER itself, not
     the absence of a subclass today, is what this test asserts (AC-C1
     point 4 / AC-C8).

This test MUST FAIL if any of these invariants is violated by a future edit -
including the exact regressions Codex Stage 7 Round 3 found (early return
before the decision at old line 990; decision reading persisted
`current.internalTempC` instead of the just-read value; a separately
callable `isItSafeToCharge()` public entry point) AND the regression Codex
Stage 7 Final found (a separately callable PUBLIC `readTmp112TemperatureC()`
acquisition half - the coupled function being non-public did not close the
loophole, because the acquisition half itself was still reachable without it).

WO-2026-08-25-001 Decision C4 (AC-C9/AC-C10) additionally verifies that the
TMP36 sampling loop inside this same function takes all 8 samples INLINE,
within a single call, with NO `static` (or otherwise cross-call-persistent)
accumulator state:

  5. No `static` declaration exists anywhere inside the `if (!tmp112Present)`
     TMP36-sampling block (AC-C9). This is the specific regression this
     dispatch resolves: `static int sampleIndex` / `static int tmpRawSum`
     spread 8 samples across multiple coupled calls, which a hibernate-
     induced MCU reset could wipe mid-cycle, indefinitely deferring a
     genuine reading (AC-C5, resolved by Decision C4).
  6. The TMP36-sampling block contains a single bounded loop
     (`for (...; i < TMP36_SAMPLES; ...)`) that collects all samples before
     computing the average, rather than a partial-sample branch that
     collects one sample per call and falls through without a reading
     (AC-C10) - i.e. the old `sampleIndex < TMP36_SAMPLES` deferral pattern
     must not reappear.

WO-2026-08-25-001 Decision C5 additionally verifies (AC-C11):

  7. No `refreshInputProfile()` call occurs after the coupled
     `batteryState(BatterySampleContext::Setup)` call inside `setup()` in
     `src/Generalized-Core-Counter.cpp`. A trailing refresh there was found to
     be redundant (SensorManager's internal refresh always runs before the
     coupled decision on Boron) and, on a real profile transition, capable of
     clearing the thermal inhibit the coupled call just applied, with no
     bounded interval to the next thermal re-assertion.

WHAT THIS TEST DOES NOT PROVE (stated honestly, per Codex Stage 7 Final):
this is a regex-based parser over the checked-in source TEXT, not a compiler,
linker, or semantic analyzer. It cannot see macro expansion, preprocessor
conditionals actually taken for a given platform build, friend declarations,
pointer-to-member extraction, reflection, or any other language mechanism that
could theoretically expose a private member outside its class. It does not
prove the ELF contains only the expected symbols (the `nm` linkage check in
the implementation report does that, separately, for the shipped build). It
proves the SOURCE TEXT, as checked in, declares both halves `private` with no
other public/protected acquisition method present, and that the coupled
function's body has the specific control-flow properties above - not that no
exotic C++ construct could ever circumvent `private` in principle.
"""
import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
HEADER = REPO_ROOT / "src" / "sensors" / "SensorManager.h"
SOURCE = REPO_ROOT / "src" / "sensors" / "SensorManager.cpp"
SETUP_SOURCE = REPO_ROOT / "src" / "Generalized-Core-Counter.cpp"

FUNC_NAME = "measureTemperatureAndApplyChargeDecision"


def strip_comments(code: str) -> str:
    """Remove // line comments and /* */ block comments so prose mentioning
    keywords like `return` (there is plenty of it in this heavily-documented
    function) does not produce false positives."""
    code = re.sub(r"/\*.*?\*/", "", code, flags=re.DOTALL)
    code = re.sub(r"//[^\n]*", "", code)
    return code


def fail(msg):
    print(f"FAIL: {msg}")
    sys.exit(1)


def extract_function_body(source_text: str, qualified_name: str) -> str:
    """Extract the full body (between the outermost { and matching }) of a
    top-level function definition `bool SensorManager::qualified_name() {`."""
    marker = re.search(
        r"bool\s+SensorManager::" + re.escape(qualified_name) + r"\s*\(\s*\)\s*\{",
        source_text,
    )
    if not marker:
        fail(f"could not find definition of SensorManager::{qualified_name}() in {SOURCE}")
    start = marker.end() - 1  # position of the opening '{'
    depth = 0
    for i in range(start, len(source_text)):
        if source_text[i] == "{":
            depth += 1
        elif source_text[i] == "}":
            depth -= 1
            if depth == 0:
                return source_text[start + 1 : i]
    fail(f"unbalanced braces while extracting SensorManager::{qualified_name}() body")


def main():
    header_text = HEADER.read_text()
    source_text = SOURCE.read_text()

    # --- Invariant: no separately-callable old entry point. ---
    if "isItSafeToCharge" in header_text or "isItSafeToCharge" in source_text:
        fail(
            "isItSafeToCharge() (the old, separately-callable public entry "
            "point) still exists - it must be fully replaced by the coupled "
            f"{FUNC_NAME}()."
        )

    # --- Invariant: exactly one function definition. ---
    def_count = len(
        re.findall(r"bool\s+SensorManager::" + re.escape(FUNC_NAME) + r"\s*\(\s*\)\s*\{", source_text)
    )
    if def_count != 1:
        fail(f"expected exactly 1 definition of {FUNC_NAME}(), found {def_count}")

    # --- Invariant: both the coupled function and the genuine acquisition
    # half are declared PRIVATE (not merely non-public) in the header. ---
    def access_specifier_for(header_text, decl_regex, label):
        decl_match = re.search(decl_regex, header_text)
        if not decl_match:
            fail(f"could not find declaration of {label} in {HEADER}")
        preceding = header_text[: decl_match.start()]
        specifiers = re.findall(r"^\s*(public|protected|private)\s*:", preceding, re.MULTILINE)
        if not specifiers:
            fail(f"could not determine the access specifier governing {label}'s declaration")
        return specifiers[-1]

    coupled_access = access_specifier_for(
        header_text, r"bool\s+" + re.escape(FUNC_NAME) + r"\s*\(\s*\)\s*;", f"{FUNC_NAME}()"
    )
    if coupled_access != "private":
        fail(
            f"{FUNC_NAME}() is declared `{coupled_access}`, not `private` - "
            "`protected` is NOT sufficient (a derived class could still call "
            "it, separating the measurement half from the decision half "
            "without the code failing to compile). AC-C1 point 4 requires "
            "the coupling be structural, not conventional."
        )

    acquisition_access = access_specifier_for(
        header_text, r"bool\s+readTmp112TemperatureC\s*\(\s*float\s*&\s*\w+\s*\)\s*;",
        "readTmp112TemperatureC()"
    )
    if acquisition_access != "private":
        fail(
            f"readTmp112TemperatureC() is declared `{acquisition_access}`, not "
            "`private` - this is the genuine enclosure-temperature ACQUISITION "
            "half (Codex Stage 7 Final finding). A public or protected "
            "acquisition method lets any caller obtain an enclosure "
            "temperature reading WITHOUT the charge decision being evaluated, "
            "regardless of what access the coupled evaluation function has."
        )

    # --- Invariant: no OTHER public/protected member can read enclosure
    # temperature. This is the check the Codex Stage 7 Final finding says was
    # previously missing entirely: checking that one specific old name
    # (isItSafeToCharge) is gone is not the same as checking no other
    # acquisition door exists. Enumerate every public:/protected: member
    # declaration and reject any name that plausibly reads a temperature
    # sensor, with a narrow, explicitly-justified exception for
    # tmp36TemperatureC(int adcValue) - a pure ADC-code-to-Celsius
    # CONVERSION that takes an already-acquired raw value as its argument
    # and performs no hardware I/O itself (the actual acquisition,
    # `analogRead(TMP36_SENSE_PIN)`, only happens inside the private
    # sampling loop in measureTemperatureAndApplyChargeDecision()).
    ALLOWED_PUBLIC_TEMP_HELPERS = {"tmp36TemperatureC"}
    TEMP_ACQUISITION_NAME_RE = re.compile(
        r"\b(?:bool|float|int|void)\s+(\w*(?:Temp|TMP112|Tmp112)\w*)\s*\(([^;{}]*)\)\s*;"
    )

    access_sections = re.split(r"^\s*(public|protected|private)\s*:", header_text, flags=re.MULTILINE)
    # re.split with a capturing group yields [prefix, spec1, body1, spec2, body2, ...]
    forbidden_found = []
    for idx in range(1, len(access_sections), 2):
        spec = access_sections[idx]
        body_text = access_sections[idx + 1] if idx + 1 < len(access_sections) else ""
        if spec not in ("public", "protected"):
            continue
        for m in TEMP_ACQUISITION_NAME_RE.finditer(body_text):
            name, params = m.group(1), m.group(2)
            if name in ALLOWED_PUBLIC_TEMP_HELPERS:
                continue
            forbidden_found.append(f"{spec}: {name}({params.strip()})")
    if forbidden_found:
        fail(
            "found a public/protected member other than the allowed pure "
            "conversion helper that can plausibly read enclosure "
            f"temperature: {forbidden_found}. AC-C1 point 1 requires the "
            "coupled call to be the ONLY entry point - closing one named "
            "acquisition method is not sufficient if another remains "
            "reachable."
        )

    # --- Invariant: no other .cpp/.h in src/ calls this function. Comments
    # are stripped first so prose (e.g. rationale comments explaining WHY a
    # call site elsewhere runs measure.batteryState(), which internally
    # reaches this function) does not produce a false positive - this checks
    # for an actual CALL, not a mention of the name. ---
    call_sites_elsewhere = []
    for path in (REPO_ROOT / "src").rglob("*.cpp"):
        if path == SOURCE:
            continue
        text_code_only = strip_comments(path.read_text(errors="ignore"))
        if re.search(r"\b" + re.escape(FUNC_NAME) + r"\s*\(", text_code_only):
            call_sites_elsewhere.append(str(path))
    if call_sites_elsewhere:
        fail(f"{FUNC_NAME}() is called from outside SensorManager.cpp: {call_sites_elsewhere}")

    # --- Invariant: readTmp112TemperatureC() has exactly one call site,
    # inside SensorManager.cpp, and it is NOT called from any other .cpp/.h
    # in the repo (the exact gap Codex Stage 7 Final found: the coupled
    # function being non-public did not close the loophole because this
    # acquisition half was still separately callable). Comments are stripped
    # first for the same reason as above. ---
    acquisition_call_sites_elsewhere = []
    for path in REPO_ROOT.rglob("*.cpp"):
        if ".claude" in path.parts:
            continue
        if path == SOURCE:
            continue
        text_code_only = strip_comments(path.read_text(errors="ignore"))
        if re.search(r"\breadTmp112TemperatureC\s*\(", text_code_only):
            acquisition_call_sites_elsewhere.append(str(path))
    for path in REPO_ROOT.rglob("*.h"):
        if ".claude" in path.parts or path == HEADER:
            continue
        text_code_only = strip_comments(path.read_text(errors="ignore"))
        if re.search(r"\breadTmp112TemperatureC\s*\(", text_code_only):
            acquisition_call_sites_elsewhere.append(str(path))
    if acquisition_call_sites_elsewhere:
        fail(
            "readTmp112TemperatureC() is referenced from outside "
            f"SensorManager.h/.cpp: {acquisition_call_sites_elsewhere}"
        )

    # --- Extract the function body and check the read-to-decision path. ---
    body = extract_function_body(source_text, FUNC_NAME)
    # Strip comments from the WHOLE body up front - the function is heavily
    # documented, including prose that itself mentions
    # "ChargeInhibitPolicy::evaluateThermalWithValidity()" and "return" (to
    # explain these very invariants), which would otherwise be
    # mis-identified as the real call/statement below.
    body_code_only = strip_comments(body)

    decision_call = re.search(
        r"ChargeInhibitPolicy::evaluateThermalWithValidity\s*\(", body_code_only
    )
    if not decision_call:
        fail(
            "could not find the ChargeInhibitPolicy::evaluateThermalWithValidity(...) "
            f"decision call inside {FUNC_NAME}()'s body."
        )

    before_decision_code_only = body_code_only[: decision_call.start()]

    # Invariant 3: no `return` between the start of the function and the
    # decision call - i.e. no path can produce a reading (or fail to) and
    # skip the decision.
    if re.search(r"\breturn\b", before_decision_code_only):
        fail(
            f"a `return` statement exists inside {FUNC_NAME}() BEFORE the "
            "decision call - a partial/failed measurement could return "
            "without the charge decision being evaluated/applied (AC-C1 "
            "point 3 / AC-C3 regression)."
        )

    # Invariant 2: the decision call's arguments must be the LOCAL
    # measuredThisCall/tempC values, not a re-read of the persisted field.
    # Find the matching close-paren (depth-aware, since an argument like
    # current.get_internalTempC() has its own nested parens that a naive
    # first-")" search would stop at prematurely).
    depth = 1
    call_args_end = decision_call.end()
    for i in range(decision_call.end(), len(body_code_only)):
        if body_code_only[i] == "(":
            depth += 1
        elif body_code_only[i] == ")":
            depth -= 1
            if depth == 0:
                call_args_end = i
                break
    call_args = body_code_only[decision_call.end() : call_args_end]
    if "current.get_internalTempC()" in call_args or "current.internalTempC" in call_args:
        fail(
            "the decision call re-reads the persisted temperature field as "
            "its evaluation input, instead of consuming the just-read local "
            "(AC-C1 point 2 regression)."
        )
    if "measuredThisCall" not in call_args or "tempC" not in call_args:
        fail(
            "the decision call does not appear to consume the expected "
            "per-call locals (measuredThisCall, tempC) - re-check the "
            "coupling by hand; this test's pattern match may be stale."
        )

    # --- AC-C9 / AC-C10 (Decision C4): TMP36 sampling is inline, per-call,
    # with no cross-call accumulator state. ---
    tmp36_marker = re.search(r"if\s*\(\s*!\s*tmp112Present\s*\)\s*\{", body_code_only)
    if not tmp36_marker:
        fail(
            "could not find the `if (!tmp112Present) { ... }` TMP36-sampling "
            f"block inside {FUNC_NAME}()'s body."
        )
    tmp36_start = tmp36_marker.end() - 1  # position of the opening '{'
    depth = 0
    tmp36_block = None
    for i in range(tmp36_start, len(body_code_only)):
        if body_code_only[i] == "{":
            depth += 1
        elif body_code_only[i] == "}":
            depth -= 1
            if depth == 0:
                tmp36_block = body_code_only[tmp36_start + 1 : i]
                break
    if tmp36_block is None:
        fail("unbalanced braces while extracting the TMP36-sampling block")

    # AC-C9: no static (or otherwise named) cross-call accumulator anywhere
    # in the TMP36-sampling block. This must FAIL if `static int sampleIndex`
    # / `static int tmpRawSum` (or any other static accumulator) is
    # reintroduced.
    if re.search(r"\bstatic\b", tmp36_block):
        fail(
            "a `static` declaration exists inside the TMP36-sampling block - "
            "AC-C9 requires all 8 samples be taken inline as per-call LOCALS "
            "with no state surviving across calls (Decision C4). Do not "
            "reintroduce `static int sampleIndex` / `static int tmpRawSum` or "
            "any other cross-call accumulator."
        )

    # The specific historical accumulator names must not reappear anywhere in
    # the block either (belt-and-suspenders against a rename that dodges the
    # generic `static` check above via some other persistence mechanism).
    if "sampleIndex" in tmp36_block:
        fail(
            "`sampleIndex` (the old cross-call sample counter) still exists "
            "in the TMP36-sampling block - AC-C9 regression."
        )

    # AC-C10: all TMP36_SAMPLES samples are collected within a single bounded
    # loop in this one call - not deferred one-per-call across multiple
    # coupled calls. The old deferral pattern gated sample collection behind
    # `sampleIndex < TMP36_SAMPLES`; that identifier/pattern must be gone,
    # and a single bounded `for` loop over TMP36_SAMPLES must be present.
    if not re.search(r"for\s*\([^;]*;\s*\w+\s*<\s*TMP36_SAMPLES\s*;", tmp36_block):
        fail(
            "no single bounded loop over TMP36_SAMPLES was found in the "
            "TMP36-sampling block - AC-C10 requires all samples be collected "
            "inline within one call, not spread across multiple calls."
        )
    if re.search(r"if\s*\(\s*\w+\s*<\s*TMP36_SAMPLES\s*\)", tmp36_block):
        fail(
            "the TMP36-sampling block still contains an `if (... < "
            "TMP36_SAMPLES)` partial-sample deferral branch - this dead "
            "AC-C3 fall-through must be removed now that sampling is inline "
            "(Decision C4)."
        )

    # --- WO-2026-08-25-001 Decision C5 (AC-C11): no refreshInputProfile()
    # call may occur after the coupled batteryState(Setup) call inside
    # setup(). A trailing refresh there could clear the thermal inhibit the
    # coupled call just applied, on any real profile transition, with no
    # bounded interval to the next thermal re-assertion. ---
    setup_text = SETUP_SOURCE.read_text()
    setup_code_only = strip_comments(setup_text)
    battery_state_setup = re.search(
        r"batteryState\s*\(\s*BatterySampleContext::Setup\s*\)", setup_code_only
    )
    if not battery_state_setup:
        fail(
            "could not find the batteryState(BatterySampleContext::Setup) call "
            f"in {SETUP_SOURCE}."
        )
    after_battery_state_setup = setup_code_only[battery_state_setup.end():]
    if re.search(r"\brefreshInputProfile\s*\(", after_battery_state_setup):
        fail(
            "a refreshInputProfile() call occurs after "
            "batteryState(BatterySampleContext::Setup) in setup() - this was "
            "removed under WO-2026-08-25-001 Decision C5 (AC-C11) because on "
            "a profile transition it could clear the thermal inhibit the "
            "coupled call just applied, with no bounded interval to the next "
            "thermal re-assertion. Do not reintroduce it there."
        )

    print(f"PASS: {FUNC_NAME}() and readTmp112TemperatureC() are both `private`,")
    print(f"      not merely non-public; no other public/protected member can")
    print(f"      read enclosure temperature; no `return` separates measurement")
    print(f"      from the decision; the decision consumes this call's local")
    print(f"      reading, not the persisted field. TMP36 sampling is inline")
    print(f"      with no `static` cross-call accumulator (AC-C9) and all")
    print(f"      samples are collected within a single bounded loop (AC-C10).")


if __name__ == "__main__":
    main()

#!/bin/zsh
# WO-2026-10-04-001 item A2 - System.sleep() is only reached once the modem is
# really off.
#
# Device OS has in-between modem states where BOTH Cellular.isOn() and
# Cellular.isOff() are false (system_network_manager.cpp). The pre-v37 sleep
# gate asked Connectivity::isRadioPoweredOn(), i.e. Cellular.isOn(), so
# "not on" passed as "off" and System.sleep() was entered anyway - and
# System.sleep() then blocks up to 120 s waiting for the modem to power down
# (system_sleep.cpp), which is longer than the 60 s MCU awake watchdog
# (AWAKE_WATCHDOG_TIMEOUT_MS). That is the sleep-stage reset reason 60.
#
# Behavioural test. The three REAL gate decisions are extracted verbatim from
# src/state/State_Sleep.cpp and driven against a fake Device OS modem that can
# be put in the in-between state. No new timer, state or breadcrumb is
# involved: the existing teardown wait and its alert-15 timeout are what the
# blocked gate falls through to, and that is asserted here too.
set -euo pipefail

repo_root="${0:A:h:h}"
sleep_src="$repo_root/src/state/State_Sleep.cpp"
generated="${TMPDIR:-/tmp}/sleep_gate_modem_off_test.cpp"
binary="${TMPDIR:-/tmp}/sleep_gate_modem_off_test"

extract_line() {
  local marker="$1"
  local found
  found=$(grep -F -- "$marker" "$sleep_src" || true)
  if [[ -z "$found" ]]; then
    print -u2 "EXTRACT_FAILED: no line containing: $marker"
    exit 1
  fi
  if [[ $(print -r -- "$found" | wc -l) -ne 1 ]]; then
    print -u2 "EXTRACT_FAILED: marker is not unique: $marker"
    exit 1
  fi
  print -r -- "$found"
}

extract_braced_block() {
  local marker="$1"
  awk -v marker="$marker" '
    !active && index($0, marker) { active = 1 }
    active {
      print
      opens = gsub(/\{/, "{")
      closes = gsub(/\}/, "}")
      depth += opens - closes
      if (seen_open && depth == 0) exit
      if (opens > 0) seen_open = 1
    }
  ' "$sleep_src"
}

still_on_line=$(extract_line "stillOn = useNetworkStandbyEffective ? Particle.connected() :")
modem_off_logged_line=$(extract_line "!radioOffCompleteLogged && Cellular.isOff()) {")
preconditions=$(extract_braced_block "auto sleepPreconditionsSatisfied = [&]() -> bool {")

# --- Part 1: the gate must not be expressed as "not on" -----------------------
for forbidden in \
  "stillOn = useNetworkStandbyEffective ? Particle.connected() : (Particle.connected() || Connectivity::isRadioPoweredOn());" \
  "!radioOffCompleteLogged && !Connectivity::isRadioPoweredOn()) {" \
  "return !Particle.connected() && !Connectivity::isRadioPoweredOn();"
do
  if grep -qF -- "$forbidden" "$sleep_src"; then
    print -u2 "FAILED: a non-standby gate still decides on isOn(): $forbidden"
    exit 1
  fi
done

# Standby sleep keeps the modem on by design, so it must NOT have gained a
# modem-off condition.
grep -qF 'ulpConfig.network(' "$sleep_src" || {
  print -u2 "FAILED: the standby sleep configuration went missing"; exit 1; }

# The existing bounded wait and its timeout are the fallback path.
grep -qF 'RecoveryState::raiseAlert(15);' "$sleep_src" || {
  print -u2 "FAILED: the teardown timeout no longer raises alert 15"; exit 1; }
grep -qF 'transitionTo(ERROR_STATE, "sleep-disconnect-timeout");' "$sleep_src" || {
  print -u2 "FAILED: the teardown timeout no longer escalates to ERROR_STATE"; exit 1; }
grep -qF 'if (disconnectRequested && stillOn) {' "$sleep_src" || {
  print -u2 "FAILED: the bounded teardown wait is no longer keyed on stillOn"; exit 1; }

# --- Part 2: drive the real decisions ----------------------------------------
{
  cat <<'CPP'
#include <cassert>
#include <iostream>

#define Wiring_Cellular 1

// ----- Fake Device OS modem/cloud surface ------------------------------------
//
// isOn and isOff are INDEPENDENT here on purpose: that is exactly the
// in-between state (powering up, powering down, or a stuck NCP) that the
// pre-v37 gate could not see.

struct FakeCellular {
  bool on = false;
  bool off = true;
  bool isOn() const { return on; }
  bool isOff() const { return off; }
};
FakeCellular Cellular;

struct FakeParticle {
  bool cloud = false;
  bool connected() const { return cloud; }
};
FakeParticle Particle;

namespace Connectivity {
// The pre-v37 definition, kept so the extracted code still compiles if a
// future edit reintroduces it - and so this test fails loudly instead of
// failing to build.
inline bool isRadioPoweredOn() { return Cellular.isOn(); }
}  // namespace Connectivity

bool useNetworkStandbyEffective = false;

// ----- The three real gate decisions, extracted from State_Sleep.cpp ---------

bool evalStillOn() {
  bool stillOn;
CPP

  echo "// ===== verbatim from src/state/State_Sleep.cpp ====="
  print -r -- "$still_on_line"

  cat <<'CPP'
  return stillOn;
}

bool evalModemOffCompleteWouldLog(bool disconnectRequested, bool radioOffCompleteLogged) {
  // ===== verbatim condition from src/state/State_Sleep.cpp =====
CPP
  print -r -- "  if (disconnectRequested && !useNetworkStandbyEffective &&"
  print -r -- "$modem_off_logged_line"
  cat <<'CPP'
    return true;
  }
  return false;
}

bool evalSleepPreconditions() {
  // ===== verbatim from src/state/State_Sleep.cpp =====
CPP
  print -r -- "$preconditions"
  cat <<'CPP'
  return sleepPreconditionsSatisfied();
}

// ----- Test driver ------------------------------------------------------------

void setModem(bool on, bool off, bool cloud, bool standby) {
  Cellular.on = on;
  Cellular.off = off;
  Particle.cloud = cloud;
  useNetworkStandbyEffective = standby;
}

int main() {
  // --- 1. THE BUG: the in-between state, non-standby. isOn() is false, so the
  //        pre-v37 gate called this "off" and slept. System.sleep() then
  //        blocked on the modem for up to 120 s, past the 60 s watchdog.
  setModem(/*on=*/false, /*off=*/false, /*cloud=*/false, /*standby=*/false);
  assert(!Connectivity::isRadioPoweredOn());   // what the old gate asked
  assert(!evalSleepPreconditions());           // ... and what v37 answers: do not sleep
  assert(evalStillOn());                       // the bounded teardown wait is entered
  assert(!evalModemOffCompleteWouldLog(/*disconnectRequested=*/true,
                                       /*radioOffCompleteLogged=*/false));

  // --- 2. The modem is still fully on, non-standby: unchanged, do not sleep.
  setModem(/*on=*/true, /*off=*/false, /*cloud=*/false, /*standby=*/false);
  assert(!evalSleepPreconditions());
  assert(evalStillOn());
  assert(!evalModemOffCompleteWouldLog(true, false));

  // --- 3. The modem is genuinely off, non-standby: sleep, as before.
  setModem(/*on=*/false, /*off=*/true, /*cloud=*/false, /*standby=*/false);
  assert(evalSleepPreconditions());
  assert(!evalStillOn());                      // the wait is complete
  assert(evalModemOffCompleteWouldLog(true, false));
  assert(!evalModemOffCompleteWouldLog(true, true));   // logged once only
  assert(!evalModemOffCompleteWouldLog(false, false)); // only after a request

  // --- 4. Still cloud-connected blocks the gate whatever the modem says.
  setModem(/*on=*/false, /*off=*/true, /*cloud=*/true, /*standby=*/false);
  assert(!evalSleepPreconditions());
  assert(evalStillOn());

  // --- 5. STANDBY IS UNCHANGED: it keeps the modem on by design, so only the
  //        cloud session matters. An on, in-between or off modem must not
  //        change the standby answer.
  for (int i = 0; i < 3; ++i) {
    const bool on = (i == 0);
    const bool off = (i == 2);
    setModem(on, off, /*cloud=*/false, /*standby=*/true);
    assert(evalSleepPreconditions());
    assert(!evalStillOn());
    assert(!evalModemOffCompleteWouldLog(true, false));  // standby never logs modem-off

    setModem(on, off, /*cloud=*/true, /*standby=*/true);
    assert(!evalSleepPreconditions());
    assert(evalStillOn());
  }

  std::cout << "Sleep gate modem-off test passed "
               "(in-between modem state blocks non-standby sleep; standby unchanged)\n";
  return 0;
}
CPP
} > "$generated"

clang++ -std=c++17 -Wall -Wextra -pedantic \
  "$generated" \
  -o "$binary"

"$binary"

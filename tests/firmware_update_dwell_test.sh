#!/bin/zsh
# WO-2026-10-01-001 item A (round 2) - a device must not sleep in the middle of
# a firmware download that is still making progress.
#
# Behavioral test. The REAL `firmwareUpdateHandler()`, the REAL sleep-gate
# firmware-update check (extracted verbatim from `handleSleepingState()`) and
# the REAL `handleFirmwareUpdateState()` are extracted from the shipped sources
# and compiled against a fake Particle / Cloud / ThrashGuard surface, so the
# exits are exercised rather than pattern-matched. Only the collaborators are
# faked; the decision logic under test is the checked-in code, and the
# five-minute window is read from the real ConnectivityPolicy constant.
#
# Design (Particle's wake-publish-sleep reference pattern): the handler only
# records a flag, the sleep gate consults the flag before any teardown request,
# and FIRMWARE_UPDATE_STATE dwells until the flag clears or activity stops.
set -euo pipefail

repo_root="${0:A:h:h}"
generated="${TMPDIR:-/tmp}/firmware_update_dwell_test.cpp"
binary="${TMPDIR:-/tmp}/firmware_update_dwell_test"

extract_braced_block() {
  local file="$1"
  local marker="$2"
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
  ' "$file"
}

{
  cat <<'CPP'
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

#include "power/ConnectivityPolicy.h"  // real FIRMWARE_UPDATE_MAX_MS (and stub Log)

// ----- Fake Particle / application surface -----------------------------------

enum State {
  INITIALIZATION_STATE,
  ERROR_STATE,
  IDLE_STATE,
  SLEEPING_STATE,
  CONNECTING_STATE,
  REPORTING_STATE,
  FIRMWARE_UPDATE_STATE
};

State state = IDLE_STATE;
State oldState = IDLE_STATE;

unsigned long fakeNowMs = 0;
unsigned long millis() { return fakeNowMs; }

struct Transition {
  State target;
  std::string reason;
};
std::vector<Transition> transitions;

void transitionTo(State newState, const char *reason) {
  transitions.push_back({newState, reason ? reason : ""});
  state = newState;
}

void publishStateTransition() {}

struct FakeParticle {
  bool connectedValue = true;
  bool connectRequested = false;
  bool connected() const { return connectedValue; }
  void connect() { connectRequested = true; }
};
FakeParticle Particle;

// Present so that re-introducing the removed `!System.updatesPending()` exit
// still compiles - and then fails the "progress keeps the device awake" cases,
// because Device OS clears that flag as soon as the transfer starts.
struct FakeSystem {
  bool updatesPendingValue = false;
  bool updatesPending() const { return updatesPendingValue; }
};
FakeSystem System;

#define BUTTON_PIN 7
int buttonLevel = 1; // active low: 1 == not pressed
int digitalRead(int) { return buttonLevel; }

struct FakeSession {
  bool awaitingWebhookResponse = false;
  unsigned long webhookAwaitStartMs = 0;
};
FakeSession session;

struct FakeThrashGuard {
  std::vector<std::string> progressTags;
  void markProgress(const char *tag) { progressTags.push_back(tag ? tag : ""); }
};
FakeThrashGuard thrashGuard;

struct FakeCloudInstance {
  int configLoads = 0;
  bool loadConfigurationFromCloud() {
    configLoads++;
    return true;
  }
};

namespace Cloud {
inline FakeCloudInstance &instance() {
  static FakeCloudInstance inst;
  return inst;
}
} // namespace Cloud

// Mirrors the shipped alias in State_Connect.cpp; reads the real constant.
static const unsigned long firmwareUpdateMaxMs =
    ConnectivityPolicy::FIRMWARE_UPDATE_MAX_MS;

typedef int system_event_t;

// ----- Sleep-gate surrounding state (mirrors State_Sleep.cpp's statics) ------

bool disconnectRequested = false;
unsigned long cloudSyncStartMs = 0;
int teardownRequests = 0;   // any cloud-disconnect / radio-off request
bool cloudGateWaiting = false; // the cloud-operations gate is still blocking

CPP

  echo "// ===== Extracted verbatim from src/Generalized-Core-Counter.cpp ====="
  echo "volatile bool firmwareUpdateInProgress = false;"
  echo "volatile unsigned long firmwareUpdateLastActivityMs = 0;"
  echo
  extract_braced_block "$repo_root/src/Generalized-Core-Counter.cpp" \
    "void firmwareUpdateHandler(system_event_t event, int param) {"
  echo
  echo "// ===== Sleep-gate check extracted verbatim from src/state/State_Sleep.cpp ====="
  echo "// Wrapped in the surrounding order of handleSleepingState(): the check runs"
  echo "// first, then the cloud-operations gate (which returns while it blocks), then"
  echo "// the first teardown request."
  echo "void sleepGatePass() {"
  extract_braced_block "$repo_root/src/state/State_Sleep.cpp" \
    "if (!disconnectRequested && firmwareUpdateInProgress) {"
  cat <<'CPP'
  if (Particle.connected() && !disconnectRequested) {
    if (cloudSyncStartMs == 0) {
      cloudSyncStartMs = millis();
    }
    if (cloudGateWaiting) {
      return; // Stay in SLEEPING_STATE until complete or timeout
    }
    cloudSyncStartMs = 0;
  }
  if (!disconnectRequested) {
    teardownRequests++;   // Connectivity::requestFullDisconnectAndRadioOff()
    disconnectRequested = true;
  }
}
CPP

  echo
  echo "// ===== Extracted verbatim from src/state/State_Connect.cpp ====="
  extract_braced_block "$repo_root/src/state/State_Connect.cpp" \
    "// FIRMWARE_UPDATE_STATE: Stay connected for firmware/config updates"

  cat <<'CPP'

// ----- Test driver ------------------------------------------------------------

static void resetWorld() {
  transitions.clear();
  thrashGuard.progressTags.clear();
  firmwareUpdateInProgress = false;
  firmwareUpdateLastActivityMs = 0;
  buttonLevel = 1;
  Particle.connectedValue = true;
  System.updatesPendingValue = false;
  session.awaitingWebhookResponse = false;
  session.webhookAwaitStartMs = 0;
  disconnectRequested = false;
  cloudSyncStartMs = 0;
  teardownRequests = 0;
  cloudGateWaiting = false;
}

static void pump() {
  handleFirmwareUpdateState();
  oldState = state;
}

static void enterUpdateState() {
  state = FIRMWARE_UPDATE_STATE;
  oldState = IDLE_STATE;
  pump(); // entry pass
}

static void enterSleepState() {
  state = SLEEPING_STATE;
  oldState = SLEEPING_STATE;
}

static bool sawTransitionTo(State target) {
  for (const auto &t : transitions) {
    if (t.target == target) return true;
  }
  return false;
}

static const int EVENT_BEGIN = 0;
static const int EVENT_COMPLETE = 1;
static const int EVENT_PROGRESS = 2;
static const int EVENT_FAILED = -1;

int main() {
  const unsigned long FIVE_MIN = ConnectivityPolicy::FIRMWARE_UPDATE_MAX_MS;

  // --- 1. The handler records only: it sets and clears the flag, stamps
  //        activity on begin and progress, and makes no transitions.
  resetWorld();
  fakeNowMs = 10000;
  firmwareUpdateHandler(0, EVENT_BEGIN);
  assert(firmwareUpdateInProgress);
  assert(firmwareUpdateLastActivityMs == 10000);
  assert(transitions.empty());
  assert(state == IDLE_STATE);

  fakeNowMs = 40000;
  firmwareUpdateHandler(0, EVENT_PROGRESS);
  assert(firmwareUpdateInProgress); // progress does not disturb the flag
  assert(firmwareUpdateLastActivityMs == 40000);
  assert(transitions.empty());

  fakeNowMs = 50000;
  firmwareUpdateHandler(0, EVENT_COMPLETE);
  assert(!firmwareUpdateInProgress);
  assert(firmwareUpdateLastActivityMs == 40000); // terminal events are not activity
  assert(transitions.empty());

  firmwareUpdateHandler(0, EVENT_BEGIN);
  assert(firmwareUpdateInProgress);
  firmwareUpdateHandler(0, EVENT_FAILED);
  assert(!firmwareUpdateInProgress);
  assert(transitions.empty());

  // --- 2. The sleep gate leaves for FIRMWARE_UPDATE_STATE *before* any
  //        teardown request is issued (WO item A2 placement).
  resetWorld();
  fakeNowMs = 100000;
  enterSleepState();
  firmwareUpdateHandler(0, EVENT_BEGIN);
  sleepGatePass();
  assert(state == FIRMWARE_UPDATE_STATE);
  assert(transitions.size() == 1);
  assert(transitions[0].reason == "firmware update in progress");
  assert(teardownRequests == 0); // no cloud disconnect, no radio off
  assert(!disconnectRequested);

  // Control: with no download the same pass tears down as before.
  resetWorld();
  fakeNowMs = 100000;
  enterSleepState();
  sleepGatePass();
  assert(state == SLEEPING_STATE);
  assert(transitions.empty());
  assert(teardownRequests == 1);

  // --- 3. Stage 7 round 1 reproduction 1: a `begin` delivered between loop
  //        passes while the device sits in the sleep gate never reaches
  //        teardown.
  resetWorld();
  fakeNowMs = 200000;
  enterSleepState();
  cloudGateWaiting = true;
  sleepGatePass(); // gate blocks, nothing torn down
  assert(state == SLEEPING_STATE);
  assert(teardownRequests == 0);

  // Device OS delivers `begin` between loop passes.
  fakeNowMs += 5000;
  firmwareUpdateHandler(0, EVENT_BEGIN);
  sleepGatePass();
  assert(state == FIRMWARE_UPDATE_STATE);
  assert(teardownRequests == 0);
  assert(transitions.back().reason == "firmware update in progress");
  // The gate timer was cleared so a later return to sleep starts cleanly.
  assert(cloudSyncStartMs == 0);

  // --- 4. Progress keeps the device in the state well past five minutes in
  //        total, and each new activity stamp is reported to ThrashGuard.
  resetWorld();
  fakeNowMs = 10000;
  firmwareUpdateHandler(0, EVENT_BEGIN);
  enterUpdateState();
  for (int i = 0; i < 4; i++) {
    fakeNowMs += (FIVE_MIN - 1000); // just under the no-progress window
    firmwareUpdateHandler(0, EVENT_PROGRESS);
    pump();
    assert(state == FIRMWARE_UPDATE_STATE);
  }
  // More than three times the five-minute window has elapsed in total.
  assert(fakeNowMs - 10000 > 3 * FIVE_MIN);
  assert(!sawTransitionTo(SLEEPING_STATE));
  assert(!sawTransitionTo(IDLE_STATE));
  // Entry pass plus one per progress event.
  assert(thrashGuard.progressTags.size() == 5);
  for (const auto &tag : thrashGuard.progressTags) {
    assert(!tag.empty());
  }

  // --- 5. Five minutes without a progress event exits to sleep.
  resetWorld();
  fakeNowMs = 10000;
  firmwareUpdateHandler(0, EVENT_BEGIN);
  enterUpdateState();
  fakeNowMs += FIVE_MIN; // exactly at the window: still dwelling
  pump();
  assert(state == FIRMWARE_UPDATE_STATE);
  fakeNowMs += 1;
  pump();
  assert(state == SLEEPING_STATE);
  assert(transitions.back().reason == "firmware-update-no-progress");

  // --- 6. Clearing the flag exits to sleep. `complete` needs no special
  //        handling: Device OS resets the device after a completed update.
  for (int i = 0; i < 2; i++) {
    const int terminal = (i == 0) ? EVENT_COMPLETE : EVENT_FAILED;
    resetWorld();
    fakeNowMs = 10000;
    firmwareUpdateHandler(0, EVENT_BEGIN);
    enterUpdateState();
    fakeNowMs += 60000;
    firmwareUpdateHandler(0, terminal);
    assert(state == FIRMWARE_UPDATE_STATE); // the handler made no transition
    pump();
    assert(state == SLEEPING_STATE);
    assert(transitions.back().reason == "firmware-update-not-in-progress");

    // Reproduction 2: no re-entry into the update state once the download has
    // ended - the next sleep pass tears down normally.
    oldState = FIRMWARE_UPDATE_STATE;
    enterSleepState();
    sleepGatePass();
    assert(state == SLEEPING_STATE);
    assert(teardownRequests == 1);
  }

  // --- 7. Reproduction 3: a stale terminal event cannot cut short a later
  //        download's stay. A fresh `begin` after an earlier complete/failed
  //        dwells for as long as the new transfer keeps moving.
  for (int i = 0; i < 2; i++) {
    const int staleTerminal = (i == 0) ? EVENT_COMPLETE : EVENT_FAILED;
    resetWorld();
    fakeNowMs = 10000;
    firmwareUpdateHandler(0, EVENT_BEGIN);
    fakeNowMs += 30000;
    firmwareUpdateHandler(0, staleTerminal); // the earlier download ends

    fakeNowMs += 600000;                     // much later
    firmwareUpdateHandler(0, EVENT_BEGIN);   // a new download starts
    enterSleepState();
    sleepGatePass();
    assert(state == FIRMWARE_UPDATE_STATE);
    assert(teardownRequests == 0);
    oldState = SLEEPING_STATE;
    pump(); // entry pass for the new dwell
    assert(state == FIRMWARE_UPDATE_STATE);
    for (int p = 0; p < 6; p++) {
      fakeNowMs += (FIVE_MIN - 1000);
      firmwareUpdateHandler(0, EVENT_PROGRESS);
      pump();
      assert(state == FIRMWARE_UPDATE_STATE);
    }
    assert(!sawTransitionTo(SLEEPING_STATE));
    assert(!sawTransitionTo(IDLE_STATE));
  }

  // --- 8. Entry via System.updatesPending() with no download running leaves
  //        straight for SLEEPING_STATE (WO item A5); the sleep gate then
  //        catches a transfer that actually starts.
  resetWorld();
  fakeNowMs = 500000;
  enterUpdateState();
  assert(state == SLEEPING_STATE);
  assert(transitions.back().reason == "firmware-update-not-in-progress");

  // --- 9. No sleep transition happens while the transfer is moving, even
  //        though Device OS has already cleared its "updates pending" flag -
  //        the v30 Dev-09 failure mode.
  resetWorld();
  fakeNowMs = 10000;
  System.updatesPendingValue = false;
  firmwareUpdateHandler(0, EVENT_BEGIN);
  enterUpdateState();
  for (int i = 0; i < 10; i++) {
    fakeNowMs += 30000;
    firmwareUpdateHandler(0, EVENT_PROGRESS);
    pump();
  }
  assert(state == FIRMWARE_UPDATE_STATE);
  assert(transitions.empty());

  // --- 10. The button override still exits to IDLE.
  resetWorld();
  fakeNowMs = 10000;
  firmwareUpdateHandler(0, EVENT_BEGIN);
  enterUpdateState();
  buttonLevel = 0;
  pump();
  assert(state == IDLE_STATE);
  assert(transitions.back().reason == "firmware-update-button-exit");

  // --- 11. The webhook-ack window is held open while dwelling (WO item A4), so
  //         a long transfer cannot age out into alert 40.
  resetWorld();
  fakeNowMs = 10000;
  firmwareUpdateHandler(0, EVENT_BEGIN);
  enterUpdateState();
  session.awaitingWebhookResponse = true;
  session.webhookAwaitStartMs = 1;
  fakeNowMs += 120000;
  firmwareUpdateHandler(0, EVENT_PROGRESS);
  pump();
  assert(session.webhookAwaitStartMs == fakeNowMs);

  // --- 12. The configuration load still happens once per dwell.
  resetWorld();
  fakeNowMs = 10000;
  const int loadsBefore = Cloud::instance().configLoads;
  firmwareUpdateHandler(0, EVENT_BEGIN);
  enterUpdateState();
  pump();
  pump();
  assert(Cloud::instance().configLoads == loadsBefore + 1);

  std::cout << "Firmware update dwell test passed (flag-gated sleep, progress-aware exits)\n";
  return 0;
}
CPP
} > "$generated"

clang++ -std=c++17 -Wall -Wextra -pedantic \
  -I"$repo_root/tests/stubs" -I"$repo_root/src" \
  "$generated" \
  -o "$binary"

"$binary"

#include "time/HibernateWakeDiagnostics.h"

#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>

namespace {

using HibernateWakeDiagnostics::buildEventFields;
using HibernateWakeDiagnostics::buildEventPayload;
using HibernateWakeDiagnostics::classifyGateArm;
using HibernateWakeDiagnostics::EventFields;
using HibernateWakeDiagnostics::GateArm;
using HibernateWakeDiagnostics::GateInputs;
using HibernateWakeDiagnostics::gateArmName;

// A GateInputs that satisfies every condition, matching a well-formed
// hibernate wake: requested 3600s, RTC read 3605s after retainedHibernateRtcBefore.
GateInputs passingInputs() {
  GateInputs in{};
  in.resetReasonIsPowerManagement = true;
  in.wakeReasonIsAlarm = true;
  in.rtcReadOk = true;
  in.rtcBefore = 1000;
  in.requestedSleepSec = 3600;
  in.rtcAtWake = 1000 + 3605;
  return in;
}

void testGatePassesClassifiesNone() {
  const GateInputs in = passingInputs();
  assert(classifyGateArm(in) == GateArm::kNone);
}

// buildEventFields() is the exact function production calls immediately
// after classifyGateArm() to combine the gate outcome with the
// already-computed actual/error values (see Generalized-Core-Counter.cpp's
// call site). On a passing gate it must report the success values verbatim
// (WO requirement: event and status payload can never disagree, because
// this function never recomputes them - it only forwards what the caller,
// i.e. setup(), already computed).
void testBuildEventFieldsOnSuccessReportsProvidedActualAndError() {
  const GateInputs in = passingInputs();
  const EventFields f = buildEventFields(in, /*osResetReason=*/30, "ALARM",
                                          /*hibernateCount=*/7,
                                          /*actualSleepSecOnSuccess=*/3605,
                                          /*sleepErrorSecOnSuccess=*/5);
  assert(f.arm == GateArm::kNone);
  assert(f.actualSleepSec == 3605);
  assert(f.sleepErrorSec == 5);
  assert(f.hibernateCount == 7);
}

// Both-outcomes coverage (WO requirement 2, Finding 2): buildEventFields()
// must zero actual/error whenever the gate did NOT pass, even if the
// caller passes non-zero success-path values in - a failed gate can never
// leak a fabricated duration through this function.
void testBuildEventFieldsOnFailureZeroesActualAndErrorRegardlessOfInput() {
  GateInputs in = passingInputs();
  in.rtcReadOk = false; // fails the kRtcRead arm
  const EventFields f = buildEventFields(in, /*osResetReason=*/30, "ALARM",
                                          /*hibernateCount=*/7,
                                          /*actualSleepSecOnSuccess=*/9999,
                                          /*sleepErrorSecOnSuccess=*/9999);
  assert(f.arm == GateArm::kRtcRead);
  assert(f.actualSleepSec == 0);
  assert(f.sleepErrorSec == 0);
}

// Stage 7 (second round): buildEventFields() forwards osResetReason,
// wakeReasonName, requestedSleepSec, rtcBefore, and rtcAtWake straight
// through from its inputs - these ARE the forensic values this WO exists
// to preserve ("An event that reports the correct gateArm while carrying
// zeroed inputs would look correct in the ledger and tell us nothing
// about why a wake failed"). Every field is given a distinctive,
// non-zero, non-1, non-default value here so that a dropped or
// accidentally-defaulted assignment inside buildEventFields() cannot pass
// by coincidence - each of the five values below is unique and could not
// be confused with 0, a boolean, or any other field's value.
void testBuildEventFieldsOnSuccessForwardsEveryForensicInputFaithfully() {
  GateInputs in{};
  in.resetReasonIsPowerManagement = true;
  in.wakeReasonIsAlarm = true;
  in.rtcReadOk = true;
  in.rtcBefore = 87654321;         // distinctive int64_t
  in.requestedSleepSec = 424242;   // distinctive uint32_t
  in.rtcAtWake = 88154321;         // distinctive, > rtcBefore (gate passes)

  const EventFields f = buildEventFields(in, /*osResetReason=*/90211, "COUNTDOWN_TIMER",
                                          /*hibernateCount=*/13579,
                                          /*actualSleepSecOnSuccess=*/918273,
                                          /*sleepErrorSecOnSuccess=*/-31415);

  assert(f.arm == GateArm::kNone);
  assert(f.osResetReason == 90211);
  assert(strcmp(f.wakeReasonName, "COUNTDOWN_TIMER") == 0);
  assert(f.rtcReadOk == true);
  assert(f.requestedSleepSec == 424242u);
  assert(f.rtcBefore == 87654321);
  assert(f.rtcAtWake == 88154321);
  assert(f.hibernateCount == 13579u);
  assert(f.actualSleepSec == 918273u);
  assert(f.sleepErrorSec == -31415);

  char payload[256];
  const int written = buildEventPayload(payload, sizeof(payload), f);
  assert(written > 0 && (size_t)written < sizeof(payload));
  assert(strstr(payload, "\"osReason\":90211") != nullptr);
  assert(strstr(payload, "\"wakeReason\":\"COUNTDOWN_TIMER\"") != nullptr);
  assert(strstr(payload, "\"req\":424242") != nullptr);
  assert(strstr(payload, "\"rtcBefore\":87654321") != nullptr);
  assert(strstr(payload, "\"rtcAt\":88154321") != nullptr);
  assert(strstr(payload, "\"actual\":918273") != nullptr);
  assert(strstr(payload, "\"err\":-31415") != nullptr);
  assert(strstr(payload, "\"count\":13579") != nullptr);
  assert(strstr(payload, "\"rtcOk\":1") != nullptr);
}

// Same faithfulness requirement on the FAILURE branch: actual/error are
// correctly zeroed (already covered above), but osResetReason,
// wakeReasonName, requestedSleepSec, rtcBefore, and rtcAtWake must still
// be carried through untouched - a failed wake is exactly the case this
// WO's forensic event needs to explain, so these are the values Codex
// flagged as unprotected. Distinctive values again, different from the
// success-path test's, so no field's expected value could be produced by
// cross-copying another field.
void testBuildEventFieldsOnFailureForwardsEveryForensicInputFaithfully() {
  GateInputs in{};
  in.resetReasonIsPowerManagement = true;
  in.wakeReasonIsAlarm = true;
  in.rtcReadOk = false;             // fails the kRtcRead arm
  in.rtcBefore = 55555555;         // distinctive int64_t
  in.requestedSleepSec = 808080;   // distinctive uint32_t
  in.rtcAtWake = 66666666;         // distinctive int64_t

  const EventFields f = buildEventFields(in, /*osResetReason=*/12321, "DEEP_POWER_DOWN",
                                          /*hibernateCount=*/24680,
                                          /*actualSleepSecOnSuccess=*/555,
                                          /*sleepErrorSecOnSuccess=*/-555);

  assert(f.arm == GateArm::kRtcRead);
  assert(f.osResetReason == 12321);
  assert(strcmp(f.wakeReasonName, "DEEP_POWER_DOWN") == 0);
  assert(f.rtcReadOk == false);
  assert(f.requestedSleepSec == 808080u);
  assert(f.rtcBefore == 55555555);
  assert(f.rtcAtWake == 66666666);
  assert(f.hibernateCount == 24680u);
  assert(f.actualSleepSec == 0);  // zeroed - gate failed
  assert(f.sleepErrorSec == 0);   // zeroed - gate failed

  char payload[256];
  const int written = buildEventPayload(payload, sizeof(payload), f);
  assert(written > 0 && (size_t)written < sizeof(payload));
  assert(strstr(payload, "\"result\":\"fail\"") != nullptr);
  assert(strstr(payload, "\"gateArm\":\"rtc_read\"") != nullptr);
  assert(strstr(payload, "\"osReason\":12321") != nullptr);
  assert(strstr(payload, "\"wakeReason\":\"DEEP_POWER_DOWN\"") != nullptr);
  assert(strstr(payload, "\"req\":808080") != nullptr);
  assert(strstr(payload, "\"rtcBefore\":55555555") != nullptr);
  assert(strstr(payload, "\"rtcAt\":66666666") != nullptr);
  assert(strstr(payload, "\"rtcOk\":0") != nullptr);
  assert(strstr(payload, "\"actual\":0") != nullptr);
  assert(strstr(payload, "\"err\":0") != nullptr);
}

void testGatePassesPayloadReportsSuccess() {
  const GateInputs in = passingInputs();
  const EventFields f = buildEventFields(in, /*osResetReason=*/30, "ALARM",
                                          /*hibernateCount=*/7,
                                          /*actualSleepSecOnSuccess=*/3605,
                                          /*sleepErrorSecOnSuccess=*/5);

  char payload[256];
  const int written = buildEventPayload(payload, sizeof(payload), f);
  assert(written > 0 && (size_t)written < sizeof(payload));
  assert(strstr(payload, "\"result\":\"ok\"") != nullptr);
  assert(strstr(payload, "\"gateArm\":\"none\"") != nullptr);
  assert(strstr(payload, "\"actual\":3605") != nullptr);
  assert(strstr(payload, "\"err\":5") != nullptr);
  assert(strstr(payload, "\"count\":7") != nullptr);
}

// Each gate arm failing, one condition at a time, must be identified by
// classifyGateArm() and must show up verbatim in the built payload.
void testEachGateArmFailureIsIdentified() {
  {
    GateInputs in = passingInputs();
    in.resetReasonIsPowerManagement = false;
    assert(classifyGateArm(in) == GateArm::kResetReason);
    assert(strcmp(gateArmName(classifyGateArm(in)), "reset_reason") == 0);
  }
  {
    GateInputs in = passingInputs();
    in.wakeReasonIsAlarm = false;
    assert(classifyGateArm(in) == GateArm::kWakeReason);
    assert(strcmp(gateArmName(classifyGateArm(in)), "wake_reason") == 0);
  }
  {
    GateInputs in = passingInputs();
    in.rtcReadOk = false;
    assert(classifyGateArm(in) == GateArm::kRtcRead);
    assert(strcmp(gateArmName(classifyGateArm(in)), "rtc_read") == 0);
  }
  {
    GateInputs in = passingInputs();
    in.rtcBefore = 0;
    assert(classifyGateArm(in) == GateArm::kRtcBeforeZero);
    assert(strcmp(gateArmName(classifyGateArm(in)), "rtc_before_zero") == 0);
  }
  {
    GateInputs in = passingInputs();
    in.requestedSleepSec = 0;
    assert(classifyGateArm(in) == GateArm::kRequestedZero);
    assert(strcmp(gateArmName(classifyGateArm(in)), "requested_zero") == 0);
  }
  {
    GateInputs in = passingInputs();
    in.rtcAtWake = in.rtcBefore - 1; // woke "before" the recorded sleep time
    assert(classifyGateArm(in) == GateArm::kRtcOrder);
    assert(strcmp(gateArmName(classifyGateArm(in)), "rtc_order") == 0);
  }
}

// Mirrors the real gate's short-circuit order: if the FIRST failing
// condition in evaluation order is reset_reason, a simultaneously-false
// later condition (e.g. rtcReadOk) must not be reported instead - matching
// what the real `&&` chain would actually short-circuit on.
void testFirstFailingArmInEvaluationOrderWins() {
  GateInputs in = passingInputs();
  in.resetReasonIsPowerManagement = false;
  in.wakeReasonIsAlarm = false;
  in.rtcReadOk = false;
  assert(classifyGateArm(in) == GateArm::kResetReason);
}

// Stage 7 (round 5): classifyGateArm()'s ORDER of checks - not just each
// check's individual correctness - must match the real gate's
// short-circuit order (see the ordering contract in classifyGateArm()'s
// doc comment). Codex demonstrated that swapping two ADJACENT checks
// (specifically wake_reason <-> rtc_read) passed every existing test,
// because testEachGateArmFailureIsIdentified() only ever fails ONE
// condition at a time (so an internal reordering can't be observed - each
// arm is still reachable, just via a different position), and
// testFirstFailingArmInEvaluationOrderWins() only exercises the
// reset_reason-vs-later-conditions boundary, never discriminating among
// the arms after it.
//
// Fix: one test per ADJACENT pair of checks, each failing BOTH conditions
// in that pair simultaneously (with every earlier condition left passing),
// asserting the EARLIER one in the documented order is what gets
// reported. This is control-flow coverage, not another value-mutation
// test: it would catch an accidental swap of any two adjacent `if`s
// inside classifyGateArm(), which no single-arm-failure test can, because
// a swap only changes behavior when the swapped pair fails together.
void testAdjacentArmOrdering_ResetReasonBeforeWakeReason() {
  GateInputs in = passingInputs();
  in.resetReasonIsPowerManagement = false; // fails check 1
  in.wakeReasonIsAlarm = false;             // fails check 2
  assert(classifyGateArm(in) == GateArm::kResetReason);
}

void testAdjacentArmOrdering_WakeReasonBeforeRtcRead() {
  GateInputs in = passingInputs();
  in.wakeReasonIsAlarm = false; // fails check 2
  in.rtcReadOk = false;         // fails check 3
  assert(classifyGateArm(in) == GateArm::kWakeReason);
}

void testAdjacentArmOrdering_RtcReadBeforeRtcBeforeZero() {
  GateInputs in = passingInputs();
  in.rtcReadOk = false; // fails check 3
  in.rtcBefore = 0;     // fails check 4
  assert(classifyGateArm(in) == GateArm::kRtcRead);
}

void testAdjacentArmOrdering_RtcBeforeZeroBeforeRequestedZero() {
  GateInputs in = passingInputs();
  in.rtcBefore = 0;         // fails check 4
  in.requestedSleepSec = 0; // fails check 5
  assert(classifyGateArm(in) == GateArm::kRtcBeforeZero);
}

void testAdjacentArmOrdering_RequestedZeroBeforeRtcOrder() {
  GateInputs in = passingInputs();
  in.requestedSleepSec = 0;             // fails check 5
  in.rtcAtWake = in.rtcBefore - 1;      // fails check 6
  assert(classifyGateArm(in) == GateArm::kRequestedZero);
}

void testFailurePayloadIdentifiesArmAndOmitsFabricatedTiming() {
  GateInputs in = passingInputs();
  in.rtcReadOk = false;

  // Same "both outcomes" contract as testBuildEventFieldsOnFailureZeroesActualAndErrorRegardlessOfInput():
  // pass deliberately non-zero success-path values to prove buildEventFields()
  // - not the caller - is what zeroes them on a failed gate.
  const EventFields f = buildEventFields(in, /*osResetReason=*/30, "ALARM",
                                          /*hibernateCount=*/2,
                                          /*actualSleepSecOnSuccess=*/1234,
                                          /*sleepErrorSecOnSuccess=*/1234);
  assert(f.actualSleepSec == 0);
  assert(f.sleepErrorSec == 0);

  char payload[256];
  const int written = buildEventPayload(payload, sizeof(payload), f);
  assert(written > 0 && (size_t)written < sizeof(payload));
  assert(strstr(payload, "\"result\":\"fail\"") != nullptr);
  assert(strstr(payload, "\"gateArm\":\"rtc_read\"") != nullptr);
  assert(strstr(payload, "\"rtcOk\":0") != nullptr);
  assert(strstr(payload, "\"actual\":0") != nullptr);
  assert(strstr(payload, "\"err\":0") != nullptr);
}

// Worst-case payload size check: longest gateArm name, longest wakeReason
// name (ab1805WakeReasonName()'s longest case is "DEEP_POWER_DOWN", 16
// chars, not "COUNTDOWN_TIMER"), and full-width signed minima for every
// numeric field (osResetReason is `int`/%d, rtcBefore/rtcAtWake are
// int64_t/%lld, sleepErrorSec is int32_t/%ld) - not merely 13-digit
// placeholder values. Must comfortably fit both the chosen 256-byte
// buffer and Device OS 6.4.1's 1024-byte MAX_EVENT_DATA_LENGTH.
void testWorstCasePayloadFitsBudget() {
  EventFields f{};
  f.arm = GateArm::kRtcBeforeZero;    // "rtc_before_zero" - longest gateArm name
  f.osResetReason = std::numeric_limits<int32_t>::min(); // widest plausible int (-2147483648)
  f.wakeReasonName = "DEEP_POWER_DOWN"; // longest AB1805 wake-reason name (16 chars)
  f.rtcReadOk = false;
  f.requestedSleepSec = std::numeric_limits<uint32_t>::max();
  f.rtcBefore = std::numeric_limits<int64_t>::min();
  f.rtcAtWake = std::numeric_limits<int64_t>::min();
  f.actualSleepSec = std::numeric_limits<uint32_t>::max();
  f.sleepErrorSec = std::numeric_limits<int32_t>::min();
  f.hibernateCount = std::numeric_limits<uint32_t>::max();

  char payload[256];
  const int written = buildEventPayload(payload, sizeof(payload), f);
  assert(written > 0);
  assert((size_t)written < sizeof(payload)); // not truncated
  assert(written == 245);                   // exact full-width-minima worst case
  assert(written < 1024);                   // fits Device OS 6.4.1 MAX_EVENT_DATA_LENGTH
  std::cout << "Worst-case hibernate_wake payload length: " << written << " bytes\n";
}

} // namespace

int main() {
  testGatePassesClassifiesNone();
  testBuildEventFieldsOnSuccessReportsProvidedActualAndError();
  testBuildEventFieldsOnFailureZeroesActualAndErrorRegardlessOfInput();
  testBuildEventFieldsOnSuccessForwardsEveryForensicInputFaithfully();
  testBuildEventFieldsOnFailureForwardsEveryForensicInputFaithfully();
  testGatePassesPayloadReportsSuccess();
  testEachGateArmFailureIsIdentified();
  testFirstFailingArmInEvaluationOrderWins();
  testAdjacentArmOrdering_ResetReasonBeforeWakeReason();
  testAdjacentArmOrdering_WakeReasonBeforeRtcRead();
  testAdjacentArmOrdering_RtcReadBeforeRtcBeforeZero();
  testAdjacentArmOrdering_RtcBeforeZeroBeforeRequestedZero();
  testAdjacentArmOrdering_RequestedZeroBeforeRtcOrder();
  testFailurePayloadIdentifiesArmAndOmitsFabricatedTiming();
  testWorstCasePayloadFitsBudget();

  std::cout << "All hibernate_wake_diagnostics_test assertions passed\n";
  return 0;
}

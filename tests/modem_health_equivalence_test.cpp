// WO-2026-10-19-004: runs the real power/ModemHealth.cpp on a fixed set of
// cases and prints a transcript of the four latch fields plus every log line
// (level and text). The .sh requires it to equal, byte for byte, the frozen
// output of the pre-move latch code in tests/fixtures/modem_health_transcript.txt.
#include <cstdio>
#include "state/State_Common.h"
#include "observability/WakeCycleStats.h"
#include "power/ModemHealth.cpp"

static void opConnectTimeout() { noteConnectTimeout(); }
static void opConnectSuccess() { noteConnectSuccess(); }
static void opDailyClose() { clearAtDailyClose(); }
static void opMark(int r, unsigned long ms) { markModemUnstable((ModemUnstableReason)r, ms); }
static void opMaybeClear(unsigned long ms) { maybeClearModemUnstable(ms); }

using CR = Observability::WakeCycleStats::ConnectResult;
static int cases = 0;

static void setState(bool unstable, bool suppressed, int reason, int count) {
  session.modemUnstable = unstable;
  session.modemStandbySuppressed = suppressed;
  session.modemUnstableReason = (uint8_t)reason;
  session.modemConnectTimeoutCount = (uint8_t)count;
  logLines.clear();
}

static void dump(const char *name, int a, int b, int c, int d) {
  printf("%s %d %d %d %d -> unstable=%d suppressed=%d reason=%u count=%u\n", name, a, b, c, d,
         session.modemUnstable, session.modemStandbySuppressed,
         (unsigned)session.modemUnstableReason, (unsigned)session.modemConnectTimeoutCount);
  for (const auto &l : logLines) printf("  LOG %s\n", l.c_str());
  cases++;
}

int main() {
  const int counts[] = {0, 1, 2, 3, 255};
  for (int count : counts)
    for (int reason : {0, 2, 1})  // latch not set / set CONNECT_TIMEOUT / set SLOW_TEARDOWN
      for (int supp = 0; supp < 2; supp++) {
        setState(reason != 0, supp, reason, count);
        opConnectTimeout();
        dump("connectTimeout", count, reason, supp, 0);
      }
  for (int count : {0, 1, 255})
    for (int un = 0; un < 2; un++) {
      setState(un, un, un, count);
      opConnectSuccess();
      dump("connectSuccess", count, un, 0, 0);
    }
  for (int un = 0; un < 2; un++)
    for (int supp = 0; supp < 2; supp++)
      for (int reason = 0; reason < 3; reason++)
        for (int count : {0, 2}) {
          setState(un, supp, reason, count);
          opDailyClose();
          dump("dailyClose", un, supp, reason, count);
        }
  for (int reason = 0; reason < 3; reason++)
    for (int prior = 0; prior < 3; prior++)  // none / same reason set / other reason set
      for (int supp = 0; supp < 2; supp++)
        for (int count : {0, 3}) {
          setState(prior != 0, supp, prior == 1 ? reason : (reason + 1) % 3, count);
          opMark(reason, 12345UL);
          dump("mark", reason, prior, supp, count);
        }
  const CR results[] = {CR::NOT_ATTEMPTED, CR::SUCCESS, CR::TIMEOUT, CR::ABORTED};
  for (int un = 0; un < 2; un++)
    for (int supp = 0; supp < 2; supp++)
      for (CR r : results)
        for (unsigned long ms : {4999UL, 5000UL, 5001UL, 10000UL, 10001UL}) {
          Observability::cycleStats().connect_result = r;
          setState(un, supp, 1, 2);
          opMaybeClear(ms);
          dump("maybeClear", un * 2 + supp, (int)r, (int)(ms % 1000), (int)(ms / 1000));
        }
  printf("CASES %d\n", cases);
  return 0;
}

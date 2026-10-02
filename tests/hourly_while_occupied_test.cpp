// WO-2026-10-02-002 (v33-HourlyWhileOccupied): the device makes its scheduled
// report every hour whether or not the site is occupied.
//
// HOW THIS TEST IS BEHAVIORAL AGAINST THE REAL SOURCE
// ---------------------------------------------------
// Same two-part pattern as connectivity_failsafe_open_hours_test:
//
//   * The due test itself is NOT re-implemented here. The real
//     reportDueThisInterval() body is lifted verbatim out of
//     src/state/State_Common.h by the driver script and #included below
//     (REAL_DUE_HELPER_HEADER), compiled against tiny shims for
//     Config::reportingIntervalSecForRuntime(), SystemConfig::get_lastReport()
//     and Time.now(). Change the rule in src/ and this binary's behavior
//     changes with it.
//
//   * The three decision sites live inside handleSleepingState() /
//     handleIdleState(), which cannot be linked on the host. So the driver
//     script parses the real occupied-branch of each site out of
//     State_Sleep.cpp / State_Idle.cpp and injects what it found as
//     -DSITE*_* flags. The simulations below then run the real due helper
//     through the structure the sources actually have. Restoring the
//     suppression at site 1 or site 2 flips a flag and fails an assertion.
//
// The unoccupied models are the literal pre-v33 rules; Part 2 of the driver
// script asserts that the real else-branches still spell them that way, so
// "unoccupied is unchanged" is pinned from both directions.

#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <ctime>

// --- Shims the real helper is compiled against ---------------------------
namespace {
uint16_t g_intervalSec = 3600;
time_t g_lastReport = 0;
time_t g_now = 0;
} // namespace

namespace Config {
uint16_t reportingIntervalSecForRuntime() { return g_intervalSec; }
} // namespace Config

namespace SystemConfig {
time_t get_lastReport() { return g_lastReport; }
} // namespace SystemConfig

namespace {
struct TimeShim {
	time_t now() const { return g_now; }
};
TimeShim Time;
} // namespace

// The real src/state/State_Common.h body, extracted by the driver script.
#include REAL_DUE_HELPER_HEADER

namespace {

// --- What the real sources were found to do ------------------------------
constexpr bool kSite1GatedOnDue = (SITE1_GATED_ON_DUE != 0);
constexpr bool kSite1FallsThroughToReport = (SITE1_FALLTHROUGH_REPORTS != 0);
constexpr bool kSite2ReportsWhenDue = (SITE2_REPORTS_WHEN_DUE != 0);
constexpr bool kSite3ReportsWhenDue = (SITE3_REPORTS_WHEN_DUE != 0);
constexpr time_t kFailsafeStaleSec = REAL_STALE_SEC;

enum Decision {
	DECISION_SLEEP_SUPPRESS,
	DECISION_REPORT,
	DECISION_RETURN_TO_SLEEP,
	DECISION_NO_REPORT,
};

// --- The occupancy session (never touched by a scheduled report) ---------
struct Session {
	bool occupied = false;
	time_t occupancyStartTime = 0;
	uint32_t totalOccupiedSeconds = 0;
};

Session session;
time_t g_lastConnection = 0;
int g_reportCount = 0;

void resetWorld(time_t now, uint16_t intervalSec, time_t lastReport) {
	g_now = now;
	g_intervalSec = intervalSec;
	g_lastReport = lastReport;
	g_lastConnection = lastReport;
	g_reportCount = 0;
	session = Session();
}

// A scheduled report: stamps lastReport (State_Report.cpp's
// SystemConfig::set_lastReport(now)), refreshes lastConnection through its
// successful connection, and leaves the session strictly alone - the report
// path closes a session only at the daily close.
void applyScheduledReport() {
	g_lastReport = g_now;
	g_lastConnection = g_now;
	++g_reportCount;
}

int payloadOccupancy() { return session.occupied ? 1 : 0; }

// --- Site 1: State_Sleep.cpp timer wake ----------------------------------
Decision site1TimerWakeOccupied() {
	if (!kSite1GatedOnDue || !reportDueThisInterval()) {
		return DECISION_SLEEP_SUPPRESS; // "sleep-timer-occupied-suppress-report"
	}
	return DECISION_REPORT; // falls through to "sleep-timer-report"
}

Decision site1TimerWakeUnoccupied() {
	// Unchanged: a timer wake is a scheduled report, no checks, no gates.
	return DECISION_REPORT;
}

// --- Site 2: State_Sleep.cpp PIR wake ------------------------------------
Decision site2PirWakeOccupied() {
	if (kSite2ReportsWhenDue && reportDueThisInterval()) {
		return DECISION_REPORT; // "sleep-pir-overdue-report"
	}
	return DECISION_RETURN_TO_SLEEP; // "sleep-pir-return-to-sleep"
}

Decision site2PirWakeUnoccupied() {
	// Unchanged pre-v33 elapsed-time overdue rule.
	if (g_lastReport > 0 && (g_now - g_lastReport) >= (time_t)g_intervalSec) {
		return DECISION_REPORT;
	}
	return DECISION_RETURN_TO_SLEEP;
}

// --- Site 3: State_Idle.cpp scheduled reporting --------------------------
Decision site3IdleOccupied() {
	if (kSite3ReportsWhenDue && reportDueThisInterval()) {
		return DECISION_REPORT; // "report interval"
	}
	return DECISION_NO_REPORT;
}

Decision site3IdleUnoccupied() {
	// Unchanged pre-v33 elapsed-time rule.
	if (g_lastReport == 0 || (g_now - g_lastReport) >= (time_t)g_intervalSec) {
		return DECISION_REPORT;
	}
	return DECISION_NO_REPORT;
}

// --- The v32 long-duration connectivity failsafe (unchanged) -------------
// Age is measured from the later of lastConnection and today's opening; the
// failsafe acts once that age reaches CONNECTIVITY_FAILSAFE_STALE_SEC.
bool failsafeWouldAct(time_t openedAt) {
	if (g_lastConnection <= 0) {
		return false;
	}
	const time_t ageBase = (openedAt > g_lastConnection) ? openedAt : g_lastConnection;
	return (g_now - ageBase) >= kFailsafeStaleSec;
}

// --- Fixtures ------------------------------------------------------------
// 2026-10-02 in whole hours; the absolute epoch does not matter, only that
// the hour boundaries are exact multiples of the 3600 s interval.
constexpr time_t kDay = 1790000000L - (1790000000L % 86400L); // midnight
constexpr time_t kOpenedAt = kDay + 6 * 3600;                 // 06:00
constexpr time_t kHour07 = kDay + 7 * 3600;
constexpr time_t kHour08 = kDay + 8 * 3600;

[[noreturn]] void fail(const char *what) {
	std::printf("FAILED: %s\n", what);
	std::fflush(stdout);
	std::exit(1);
}

// =========================================================================
// 1. The due rule itself (the real helper, straight out of src/).
// =========================================================================
void testDueRuleIsTheClockHour() {
	resetWorld(kHour07 + 10, 3600, 0);
	if (!reportDueThisInterval()) fail("lastReport == 0 must be due");

	g_lastReport = kHour07 + 5;
	if (reportDueThisInterval()) fail("a report already made this hour must not be due again");

	g_now = kHour07 + 3599;
	if (reportDueThisInterval()) fail("still inside the same clock hour must not be due");

	g_now = kHour08;
	if (!reportDueThisInterval()) fail("the first instant of the next clock hour must be due");

	// An occupancy-change report late in the hour counts: it is still the
	// same interval bucket, so nothing more is due until the boundary.
	g_lastReport = kHour07 + 3300; // 07:55
	g_now = kHour07 + 3540;        // 07:59
	if (reportDueThisInterval()) fail("an occupancy-change report must satisfy the hour");
	g_now = kHour08 + 1; // 08:00:01
	if (!reportDueThisInterval()) fail("the next boundary after a late report must be due");

	// The rule follows the configured interval, not a hardcoded hour.
	resetWorld(kHour07 + 10, 300, kHour07 + 5);
	if (reportDueThisInterval()) fail("same 5-minute bucket must not be due");
	g_now = kHour07 + 300;
	if (!reportDueThisInterval()) fail("the next 5-minute bucket must be due");

	std::printf("OK: the due test is the clock-interval rule from src/state/State_Common.h\n");
}

// =========================================================================
// 2. Hourly while occupied (acceptance 1).
// =========================================================================
void testHourlyWhileOccupied() {
	resetWorld(kHour08, 3600, kHour07 + 5);
	session.occupied = true;
	session.occupancyStartTime = kDay + 6 * 3600 + 1800; // 06:30
	session.totalOccupiedSeconds = 5400;

	const time_t startBefore = session.occupancyStartTime;
	const uint32_t accumulatedBefore = session.totalOccupiedSeconds;

	if (site1TimerWakeOccupied() != DECISION_REPORT) {
		fail("a timer wake at the report time while occupied must report "
		     "(site 1 suppression still present?)");
	}
	applyScheduledReport();

	if (payloadOccupancy() != 1) fail("the report must carry occupancy: 1");
	if (session.occupancyStartTime != startBefore) fail("occupancyStartTime must be unchanged");
	if (!session.occupied) fail("the session must not be closed by a scheduled report");

	// The session keeps accumulating afterwards.
	g_now = kHour08 + 600;
	session.totalOccupiedSeconds += 600;
	if (session.totalOccupiedSeconds != accumulatedBefore + 600) fail("accumulation must continue");
	if (session.occupancyStartTime != startBefore) fail("occupancyStartTime must stay unchanged");

	std::printf("OK: a timer wake at the hour while occupied reports, session untouched\n");
}

// =========================================================================
// 3. No more than due (acceptance 2, part one).
// =========================================================================
void testNoMoreThanDueWithinTheInterval() {
	resetWorld(kHour08 + 2, 3600, 0);
	session.occupied = true;
	session.occupancyStartTime = kDay + 6 * 3600 + 1800;

	if (site1TimerWakeOccupied() != DECISION_REPORT) fail("the first wake of the hour must report");
	applyScheduledReport();

	for (time_t t = kHour08 + 300; t < kHour08 + 3600; t += 300) {
		g_now = t;
		if (site1TimerWakeOccupied() != DECISION_SLEEP_SUPPRESS) {
			fail("a debounce timer wake inside an already-reported hour must not report");
		}
		if (site2PirWakeOccupied() != DECISION_RETURN_TO_SLEEP) {
			fail("a PIR wake inside an already-reported hour must not report");
		}
		if (site3IdleOccupied() != DECISION_NO_REPORT) {
			fail("idle inside an already-reported hour must not report");
		}
	}
	if (g_reportCount != 1) fail("exactly one report in the interval");

	std::printf("OK: after a report, further wakes in the same interval do not report\n");
}

// =========================================================================
// 4. Four hours of continuous occupancy (acceptance 2 part two, and 3).
// =========================================================================
void testFourHourOccupiedSession() {
	const time_t start = kDay + 6 * 3600 + 1800; // 06:30, occupied already
	const time_t end = start + 4 * 3600;         // 10:30

	resetWorld(start, 3600, kOpenedAt + 60); // last report 06:01, connection fresh
	session.occupied = true;
	session.occupancyStartTime = start;
	session.totalOccupiedSeconds = 0;

	const time_t sessionStartBefore = session.occupancyStartTime;
	int reportsPerHour[24] = {0};
	bool failsafeFired = false;

	for (time_t t = start + 1; t <= end; ++t) {
		g_now = t;
		session.totalOccupiedSeconds += 1;

		// PIR wakes every few seconds all through the session.
		bool reported = false;
		if ((t - start) % 5 == 0) {
			if (site2PirWakeOccupied() == DECISION_REPORT) {
				applyScheduledReport();
				reported = true;
			}
		}
		// Debounce timer wakes every 300 s (the product's setting1 while occupied).
		if (!reported && (t - start) % 300 == 0) {
			if (site1TimerWakeOccupied() == DECISION_REPORT) {
				applyScheduledReport();
				reported = true;
			}
		}
		if (reported) {
			if (payloadOccupancy() != 1) fail("every mid-session report must carry occupancy: 1");
			reportsPerHour[(t - kDay) / 3600] += 1;
		}

		if (failsafeWouldAct(kOpenedAt)) {
			failsafeFired = true;
			break;
		}
	}

	if (failsafeFired) fail("the v32 failsafe must never act across a 4 h occupied session");
	if (g_reportCount != 4) {
		std::printf("  scheduled reports: %d (expected 4)\n", g_reportCount);
		fail("a 4 h occupied session must produce exactly one scheduled report per hour boundary");
	}
	for (int hour = 7; hour <= 10; ++hour) {
		if (reportsPerHour[hour] != 1) {
			std::printf("  hour %02d had %d scheduled reports\n", hour, reportsPerHour[hour]);
			fail("each crossed hour boundary must produce exactly one scheduled report");
		}
	}
	if (reportsPerHour[6] != 0) fail("06:00 was already reported; no second report in that hour");

	if (session.occupancyStartTime != sessionStartBefore) {
		fail("the session must not be restarted by the hourly reports");
	}
	if (!session.occupied) fail("the session must still be open after 4 h of reports");
	if (session.totalOccupiedSeconds != 4u * 3600u) {
		fail("the session must keep accumulating across the hourly reports");
	}

	std::printf("OK: 4 h continuous occupancy -> 4 scheduled reports, one per hour boundary\n");
	std::printf("OK: the v32 failsafe never acts (each report refreshes lastConnection)\n");
}

// The same 4 hours with the v32 hold-back in place is exactly the failure
// this work order exists to remove - proof that the check above has teeth.
void testFourHourSessionWouldHaveTrippedTheFailsafeBeforeV33() {
	const time_t start = kDay + 6 * 3600 + 1800;
	resetWorld(start, 3600, kOpenedAt + 60);
	session.occupied = true;

	bool failsafeFired = false;
	for (time_t t = start + 1; t <= start + 4 * 3600; ++t) {
		g_now = t;
		// No reports at all: the pre-v33 behavior at every site.
		if (failsafeWouldAct(kOpenedAt)) {
			failsafeFired = true;
			break;
		}
	}
	if (!failsafeFired) {
		fail("sanity: with no reports for 4 h the v32 failsafe must act - "
		     "if it does not, the failsafe check above proves nothing");
	}
	std::printf("OK: sanity - with the reports suppressed the v32 failsafe does act\n");
}

// =========================================================================
// 5. Site 2 on its own: the first wake after the boundary may be a PIR wake.
// =========================================================================
void testSite2ReportsTheHourWhenPirWakesFirst() {
	resetWorld(kHour08 + 1, 3600, kHour07 + 3300); // last report 07:55
	session.occupied = true;
	session.occupancyStartTime = kDay + 6 * 3600;

	if (site2PirWakeOccupied() != DECISION_REPORT) {
		fail("a PIR wake just after the hour boundary while occupied must report "
		     "(site 2 suppression still present?)");
	}
	applyScheduledReport();
	if (session.occupancyStartTime != kDay + 6 * 3600) fail("the PIR report must not restart the session");

	// And the very next PIR wake must not report again.
	g_now = kHour08 + 6;
	if (site2PirWakeOccupied() != DECISION_RETURN_TO_SLEEP) fail("no second report in the hour");

	std::printf("OK: site 2 carries the hour when a PIR wake comes first\n");
}

void testSite3ReportsTheHourFromIdle() {
	resetWorld(kHour08 + 1, 3600, kHour07 + 3300);
	session.occupied = true;
	if (site3IdleOccupied() != DECISION_REPORT) {
		fail("idle just after the hour boundary while occupied must report "
		     "(site 3 suppression still present?)");
	}
	applyScheduledReport();
	g_now = kHour08 + 60;
	if (site3IdleOccupied() != DECISION_NO_REPORT) fail("no second idle report in the hour");
	std::printf("OK: site 3 reports once at the boundary from idle\n");
}

// =========================================================================
// 6. Unoccupied is unchanged at all three sites (acceptance 4).
// =========================================================================
void testUnoccupiedUnchanged() {
	// Site 1: a timer wake reports, due or not - no gate at all.
	resetWorld(kHour08 + 1800, 3600, kHour08 + 5); // already reported this hour
	if (reportDueThisInterval()) fail("fixture: should not be due");
	if (site1TimerWakeUnoccupied() != DECISION_REPORT) {
		fail("unoccupied: a timer wake must still report unconditionally");
	}
	if (!kSite1FallsThroughToReport) {
		fail("unoccupied: the timer wake must still fall through to sleep-timer-report");
	}

	// Sites 2 and 3 keep the ELAPSED rule, which is deliberately not the
	// clock-hour rule: last report 07:55, now 08:05 is due by the clock hour
	// but must NOT report while unoccupied.
	resetWorld(kHour08 + 300, 3600, kHour07 + 3300);
	if (!reportDueThisInterval()) fail("fixture: the clock-hour rule should call this due");
	if (site2PirWakeUnoccupied() != DECISION_RETURN_TO_SLEEP) {
		fail("unoccupied site 2 must keep the elapsed-time overdue rule");
	}
	if (site3IdleUnoccupied() != DECISION_NO_REPORT) {
		fail("unoccupied site 3 must keep the elapsed-time rule");
	}

	// ...and they still fire on the elapsed rule when it is satisfied.
	resetWorld(kHour08 + 3600, 3600, kHour07 + 3300);
	if (site2PirWakeUnoccupied() != DECISION_REPORT) fail("unoccupied site 2 overdue must still report");
	if (site3IdleUnoccupied() != DECISION_REPORT) fail("unoccupied site 3 overdue must still report");

	// lastReport == 0 keeps its pre-v33 asymmetry: site 3 reports, site 2 does not.
	resetWorld(kHour08, 3600, 0);
	if (site2PirWakeUnoccupied() != DECISION_RETURN_TO_SLEEP) {
		fail("unoccupied site 2 must still require lastReport > 0");
	}
	if (site3IdleUnoccupied() != DECISION_REPORT) fail("unoccupied site 3 must still report when lastReport == 0");

	std::printf("OK: unoccupied behavior is unchanged at all three sites\n");
}

} // namespace

int main() {
	std::printf("--- Part 1: behavior, with the real due test from src/ ---\n");
	std::printf("sites as found in src/: site1-gated-on-due=%d site2-reports-when-due=%d "
	            "site3-reports-when-due=%d failsafe-stale=%lds\n",
	            (int)kSite1GatedOnDue, (int)kSite2ReportsWhenDue, (int)kSite3ReportsWhenDue,
	            (long)kFailsafeStaleSec);

	testDueRuleIsTheClockHour();
	testHourlyWhileOccupied();
	testNoMoreThanDueWithinTheInterval();
	testFourHourOccupiedSession();
	testFourHourSessionWouldHaveTrippedTheFailsafeBeforeV33();
	testSite2ReportsTheHourWhenPirWakesFirst();
	testSite3ReportsTheHourFromIdle();
	testUnoccupiedUnchanged();

	std::printf("hourly_while_occupied_test Part 1 passed\n");
	return 0;
}

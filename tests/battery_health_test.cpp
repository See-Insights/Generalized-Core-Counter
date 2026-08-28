// Host-side replay test for F1 (BatteryHealth) against the real 378-sample
// field fixture. Confirms the pure trust signal cleanly separates the known
// Dev-14 terminal-incident window (dev14_terminal_incident=1, 20 rows) from
// routine operation, and that BatteryHealth::evaluate() has no hidden state
// (repeated calls with the same inputs produce identical output).
#include "power/BatteryHealth.h"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

struct FixtureRow {
  std::string device;
  std::string chgState;
  float vcell;
  float soc;
  bool incident;
};

std::vector<std::string> splitCsvLine(const std::string &line) {
  std::vector<std::string> fields;
  std::stringstream ss(line);
  std::string field;
  while (std::getline(ss, field, ',')) {
    fields.push_back(field);
  }
  return fields;
}

std::vector<FixtureRow> loadFixture(const std::string &path) {
  std::vector<FixtureRow> rows;
  std::ifstream in(path);
  if (!in.is_open()) {
    std::cerr << "Could not open fixture: " << path << "\n";
    std::exit(1);
  }

  std::string line;
  bool first = true;
  while (std::getline(in, line)) {
    if (first) { // header
      first = false;
      continue;
    }
    if (line.empty()) {
      continue;
    }
    const std::vector<std::string> f = splitCsvLine(line);
    // device,timestamp_utc,chg_state,chg_code,ichg_ma,fault_reg,vcell_v,
    // reported_soc_pct,power_src,power_profile,adjacent_powerdiag_soc_pct,
    // dev14_terminal_incident
    if (f.size() < 12) {
      continue;
    }
    FixtureRow row;
    row.device = f[0];
    row.chgState = f[2];
    row.vcell = std::strtof(f[6].c_str(), nullptr);
    row.soc = std::strtof(f[7].c_str(), nullptr);
    row.incident = (f[11] == "1");
    rows.push_back(row);
  }
  return rows;
}

bool chargingActiveFor(const std::string &chgState) {
  return chgState == "FAST" || chgState == "PRE";
}

void testOcvCurveMonotonicAndClamped() {
  assert(BatteryHealth::restingSocFromVcell(2.0f) == 0.0f);
  assert(BatteryHealth::restingSocFromVcell(5.0f) == 100.0f);
  assert(BatteryHealth::restingSocFromVcell(3.0f) == 0.0f);
  assert(BatteryHealth::restingSocFromVcell(4.2f) == 100.0f);

  float previous = -1.0f;
  for (float v = 3.0f; v <= 4.2001f; v += 0.01f) {
    const float soc = BatteryHealth::restingSocFromVcell(v);
    assert(soc >= previous - 1e-4f); // monotonically non-decreasing
    previous = soc;
  }
}

void testEvaluateIsPureAndDeterministic() {
  const BatteryHealth::Reading a = BatteryHealth::evaluate(80.0f, 4.05f, false, false);
  const BatteryHealth::Reading b = BatteryHealth::evaluate(80.0f, 4.05f, false, false);
  assert(a.trust == b.trust);
  assert(a.residual == b.residual);
  assert(a.restingSocEstimate == b.restingSocEstimate);

  // Calling with different inputs then repeating the first inputs must
  // reproduce the exact same result - no memory across calls.
  (void)BatteryHealth::evaluate(5.0f, 3.2f, true, true);
  const BatteryHealth::Reading c = BatteryHealth::evaluate(80.0f, 4.05f, false, false);
  assert(c.trust == a.trust);
  assert(c.residual == a.residual);
}

void testUnusableVoltageIsUntrusted() {
  const BatteryHealth::Reading r = BatteryHealth::evaluate(50.0f, 0.0f, false, false);
  assert(!r.vcellUsable);
  assert(r.trust == BatteryHealth::SocTrust::Untrusted);
}

void testConsistentSampleIsTrusted() {
  // DONE (not charging), soc/vcell pair well inside the OCV curve's band.
  const BatteryHealth::Reading r = BatteryHealth::evaluate(87.6f, 4.084f, false, false);
  assert(r.vcellUsable);
  assert(r.trust == BatteryHealth::SocTrust::Trusted);
}

void testDev14IncidentWindowAllUntrusted(const std::vector<FixtureRow> &rows) {
  int incidentCount = 0;
  int incidentUntrusted = 0;

  for (const FixtureRow &row : rows) {
    if (!row.incident) {
      continue;
    }
    incidentCount++;
    const BatteryHealth::Reading r =
        BatteryHealth::evaluate(row.soc, row.vcell, chargingActiveFor(row.chgState), false);
    if (r.trust == BatteryHealth::SocTrust::Untrusted) {
      incidentUntrusted++;
    }
  }

  std::cout << "Dev-14 incident rows: " << incidentCount
            << ", flagged Untrusted: " << incidentUntrusted << "\n";
  assert(incidentCount == 20); // fixture is documented as 20 flagged rows
  assert(incidentUntrusted == incidentCount); // 100% recall on the known incident
}

void testRoutineOperationMostlyTrusted(const std::vector<FixtureRow> &rows) {
  int routineCount = 0;
  int trusted = 0;
  int untrusted = 0;

  for (const FixtureRow &row : rows) {
    if (row.incident) {
      continue;
    }
    routineCount++;
    const BatteryHealth::Reading r =
        BatteryHealth::evaluate(row.soc, row.vcell, chargingActiveFor(row.chgState), false);
    if (r.trust == BatteryHealth::SocTrust::Trusted) {
      trusted++;
    } else if (r.trust == BatteryHealth::SocTrust::Untrusted) {
      untrusted++;
    }
  }

  const double trustedRate = 100.0 * trusted / routineCount;
  const double untrustedRate = 100.0 * untrusted / routineCount;
  std::cout << "Routine (non-incident) rows: " << routineCount
            << ", Trusted=" << trusted << " (" << trustedRate << "%)"
            << ", Untrusted=" << untrusted << " (" << untrustedRate << "%)\n";

  // Regression guard against a generic (documented, non-fleet-tuned) OCV
  // curve: routine operation must remain mostly Trusted, and the Untrusted
  // bucket - reserved for genuine divergence like the Dev-14 incident - must
  // stay a minority. See BatteryHealth.h for why this curve is a labeled
  // standard reference rather than a device-validated one.
  assert(trustedRate >= 60.0);
  assert(untrustedRate <= 20.0);
}

} // namespace

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cerr << "usage: " << argv[0] << " <path-to-fixture-csv>\n";
    return 1;
  }

  testOcvCurveMonotonicAndClamped();
  testEvaluateIsPureAndDeterministic();
  testUnusableVoltageIsUntrusted();
  testConsistentSampleIsTrusted();

  const std::vector<FixtureRow> rows = loadFixture(argv[1]);
  assert(!rows.empty());

  testDev14IncidentWindowAllUntrusted(rows);
  testRoutineOperationMostlyTrusted(rows);

  std::cout << "BatteryHealth (F1) replay tests passed\n";
  return 0;
}

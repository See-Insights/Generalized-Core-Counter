#pragma once

#include <cstdint>

// Narrow read seam between the reporting adapter (RuntimeReportingPolicy.cpp)
// and the shared configuration module - see WO-2026-09-14-001 Step 0.5.
// Declares only the one accessor the adapter needs
// (Config::reportingIntervalSecForRuntime(), in production), so this header
// - and anything that includes only it - stays host-compilable without
// needing that module's own transitive dependency on the persistence bag
// (reportingIntervalSecForRuntime() reads a stored setting internally).
//
// This header must never include the shared configuration header directly;
// the implementation (ReportingIntervalStore.cpp) does that instead, using
// the same relative include every other src/reporting/, src/state/, and
// src/cloud/ file uses for it, to avoid a name collision with a Device OS
// header of the same name under the local toolchain's include order.
// tests/battery_tier_store_seam_structural_test.py enforces that this
// header stays narrow and that RuntimeReportingPolicy.cpp does not
// reintroduce a direct relative include of its own.
//
// Non-relative include: unlike the module it wraps, this name is unique and
// does not collide with a Device OS header, so callers use a plain
// #include "reporting/ReportingIntervalStore.h" - no relative-path
// workaround needed, and a test's -I override directory can shadow it (a
// relative include cannot be shadowed this way, which is what broke
// tests/reporting_policy_adapter_test.sh a second time, after Step 0, until
// this fix).

namespace ReportingIntervalStore {

uint16_t reportingIntervalSec();

} // namespace ReportingIntervalStore

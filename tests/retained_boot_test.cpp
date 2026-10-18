// WO-2026-10-17-001: the retained boot-state magic/version check (the real
// persist/RetainedBoot.h), and the post-OTA boot's boot-storm path.
#include <cstddef>
#include <cstdio>
#include <cstring>

#include "persist/RetainedBoot.h"

static int failures = 0;
#define CHECK(cond, what) do { if (!(cond)) { std::printf("FAIL: %s\n", what); failures++; } } while (0)

static void fillGarbage(RetainedBoot &r) {
  std::memset(&r, 0xA5, sizeof(r));
}

static bool allInitial(const RetainedBoot &r) {
  return r.bootStormCount == 0 && !r.bootInProgress && r.bootStormWindowStart == 0 &&
         r.bootStormLastResetReason == 0 && r.bootStormLastResetReasonData == 0 &&
         r.bootStormTripCount == 0 && !r.bootStormAlertPending && r.appBreadcrumb == 0 &&
         r.appBreadcrumbMs == 0;
}

// Mirrors setup()'s boot-storm block; tests/retained_boot_structural_test.py pins these lines to the source.
static bool bootStormTrips(RetainedBoot &r, int reason, bool clockTrusted, time_t now) {
  const bool previousBootFailedEarly = r.bootInProgress;
  r.bootInProgress = true;
  const bool qualifies = reason == 0 || reason == 10 || reason == 60 || reason == 130;
  if (previousBootFailedEarly && qualifies && r.bootStormCount < 255) r.bootStormCount++;
  if (clockTrusted) {
    if (r.bootStormWindowStart == 0) r.bootStormWindowStart = now;
    else if ((now - r.bootStormWindowStart) > 600) {
      r.bootStormCount = (previousBootFailedEarly && qualifies) ? 1 : 0;
      r.bootStormWindowStart = now;
    }
  }
  return r.bootStormCount >= 6 && previousBootFailedEarly && qualifies;
}

int main() {
  static_assert(offsetof(RetainedBoot, magic) == 0, "magic is the first member");
  static_assert(offsetof(RetainedBoot, version) == 4, "version follows the magic");

  RetainedBoot r;
  fillGarbage(r);
  validateRetainedBoot(r);
  CHECK(r.magic == kRetainedBootMagic && r.version == kRetainedBootVersion, "garbage: magic and version written");
  CHECK(allInitial(r), "garbage: every member at its initial value");

  fillGarbage(r);
  r.magic = kRetainedBootMagic;
  r.version = kRetainedBootVersion + 1;
  validateRetainedBoot(r);
  CHECK(r.version == kRetainedBootVersion && allInitial(r), "wrong version: reset and rewritten");

  fillGarbage(r);
  r.magic = kRetainedBootMagic + 1;
  r.version = kRetainedBootVersion;
  validateRetainedBoot(r);
  CHECK(r.magic == kRetainedBootMagic && allInitial(r), "wrong magic: reset and rewritten");

  r = RetainedBoot();
  validateRetainedBoot(r);
  r.bootStormCount = 3; r.bootInProgress = true; r.bootStormWindowStart = 1234567;
  r.bootStormLastResetReason = 130; r.bootStormLastResetReasonData = 77; r.bootStormTripCount = 2;
  r.bootStormAlertPending = true; r.appBreadcrumb = 9; r.appBreadcrumbMs = 4242;
  const RetainedBoot kept = r;
  validateRetainedBoot(r);
  CHECK(std::memcmp(&r, &kept, sizeof(r)) == 0, "magic and version right: values kept");

  for (int trusted = 0; trusted < 2; trusted++) {
    for (int reason = 0; reason < 256; reason++) {
      RetainedBoot post;
      fillGarbage(post);
      validateRetainedBoot(post);
      CHECK(!bootStormTrips(post, reason, trusted != 0, 1000000), "post-OTA boot: storm condition false for every reason");
      CHECK(post.bootStormCount == 0 && post.bootStormTripCount == 0 && !post.bootStormAlertPending,
            "post-OTA boot: no count, trip or pending alert");
    }
  }

  RetainedBoot storm;
  validateRetainedBoot(storm);
  bool tripped = false;
  for (int boot = 0; boot < 8; boot++) tripped = bootStormTrips(storm, 130, true, 1000000) || tripped;
  CHECK(tripped, "a genuine storm still trips after the reset boot");

  if (failures == 0) std::printf("PASS retained_boot_test\n");
  return failures == 0 ? 0 : 1;
}

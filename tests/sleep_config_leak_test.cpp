/*
 * WO-2026-10-02-003 (v34-SleepConfigLeak) - per-wake memory leak check.
 *
 * Compiled against Device OS 6.4.1's REAL
 * `system/inc/system_sleep_configuration.h` (see
 * tests/sleep_config_leak_test.sh for the include paths and the two-line
 * host shim). The wake sources configured at each of the four sites are
 * extracted from the real `src/state/State_Sleep.cpp` by the test script
 * and injected as REAL_SITES_HEADER, so this exercises the production
 * configuration, not a hand-written imitation.
 *
 * Allocation accounting is a counting allocator: global operator new /
 * delete are replaced and track outstanding bytes and blocks. LeakSanitizer
 * is unavailable on macOS/arm64 hosts, and a counting allocator is both
 * portable and exact (it reports bytes lost per cycle, not just "a leak").
 *
 * Two patterns are run over the same number of cycles:
 *
 *   new  - a fresh local SystemSleepConfiguration per sleep (what this WO
 *          ships). The destructor runs at the end of each scope and frees
 *          the wake-source list: zero bytes lost per cycle.
 *   old  - one long-lived object reset by `cfg = SystemSleepConfiguration()`
 *          before each sleep (what this WO removes). Device OS's move
 *          assignment memcpys over `config_` without freeing the old
 *          `wakeup_sources` list, so each cycle orphans the whole list. This
 *          is the MUTATION: it must lose bytes on every cycle, and the
 *          zero-leak assertion applied to it must fail.
 */

#include <cstdio>
#include <cstdlib>
#include <cstddef>
#include <new>

// ---------------------------------------------------------------------------
// Counting allocator
// ---------------------------------------------------------------------------
namespace {

struct AllocStats {
  long long outstandingBytes = 0;
  long long outstandingBlocks = 0;
  long long totalAllocs = 0;
};

AllocStats g_stats;
bool g_counting = false;

struct Header {
  size_t size;
  size_t magic;
};

constexpr size_t kMagic = 0x5731C0DEu;

}  // namespace

static void *countingAlloc(size_t size) {
  void *raw = std::malloc(size + sizeof(Header));
  if (!raw) return nullptr;
  Header *h = static_cast<Header *>(raw);
  h->size = size;
  h->magic = kMagic;
  if (g_counting) {
    g_stats.outstandingBytes += (long long)size;
    g_stats.outstandingBlocks += 1;
    g_stats.totalAllocs += 1;
  } else {
    h->magic = 0;  // allocated while counting was off - not accounted
  }
  return static_cast<char *>(raw) + sizeof(Header);
}

static void countingFree(void *p) {
  if (!p) return;
  Header *h = reinterpret_cast<Header *>(static_cast<char *>(p) - sizeof(Header));
  if (h->magic == kMagic) {
    g_stats.outstandingBytes -= (long long)h->size;
    g_stats.outstandingBlocks -= 1;
    h->magic = 0;
  }
  std::free(h);
}

void *operator new(size_t size) {
  void *p = countingAlloc(size);
  if (!p) throw std::bad_alloc();
  return p;
}
void *operator new[](size_t size) {
  void *p = countingAlloc(size);
  if (!p) throw std::bad_alloc();
  return p;
}
void *operator new(size_t size, const std::nothrow_t &) noexcept { return countingAlloc(size); }
void *operator new[](size_t size, const std::nothrow_t &) noexcept { return countingAlloc(size); }
void operator delete(void *p) noexcept { countingFree(p); }
void operator delete[](void *p) noexcept { countingFree(p); }
void operator delete(void *p, size_t) noexcept { countingFree(p); }
void operator delete[](void *p, size_t) noexcept { countingFree(p); }
void operator delete(void *p, const std::nothrow_t &) noexcept { countingFree(p); }
void operator delete[](void *p, const std::nothrow_t &) noexcept { countingFree(p); }

// ---------------------------------------------------------------------------
// Device OS 6.4.1, verbatim
// ---------------------------------------------------------------------------
#include "system_sleep_configuration.h"

using particle::SystemSleepConfiguration;
using particle::SystemSleepMode;
using particle::SystemSleepNetworkFlag;

// The pin identifiers the production sites name. Values are irrelevant to
// allocation behavior; only the shape of the wake-source list matters.
static const hal_pin_t WAKEUP_PIN = 8;
static const hal_pin_t BUTTON_PIN = 5;
static const hal_pin_t intPin = 10;
static const uint32_t wakeInSeconds = 300;

#include REAL_SITES_HEADER

// ---------------------------------------------------------------------------
// Harness
// ---------------------------------------------------------------------------
namespace {

int g_failures = 0;

void check(bool cond, const char *what) {
  if (!cond) {
    std::printf("FAILED: %s\n", what);
    g_failures++;
  }
}

size_t wakeupSourceCount(const SystemSleepConfiguration &cfg) {
  size_t n = 0;
  for (auto *s = cfg.halConfig()->wakeup_sources; s; s = s->next) n++;
  return n;
}

/// Stand-in for System.sleep(): reads the configuration the way the HAL
/// would, so nothing can be optimized away.
volatile size_t g_sink = 0;
void simulateSleep(const SystemSleepConfiguration &cfg) {
  g_sink += wakeupSourceCount(cfg) + (size_t)cfg.halConfig()->mode;
}

struct SiteResult {
  const char *name;
  long long leakedBytes;
  long long leakedBlocks;
  size_t wakeSources;
};

/// The pattern this WO ships: a fresh local per sleep.
SiteResult runFreshLocal(const RealSite &site, int cycles, bool standby) {
  size_t sources = 0;
  g_counting = true;
  const long long before = g_stats.outstandingBytes;
  const long long beforeBlocks = g_stats.outstandingBlocks;
  for (int i = 0; i < cycles; i++) {
    SystemSleepConfiguration cfg;
    site.configure(cfg, standby);
    if (i == 0) sources = wakeupSourceCount(cfg);
    simulateSleep(cfg);
  }
  const long long leaked = g_stats.outstandingBytes - before;
  const long long blocks = g_stats.outstandingBlocks - beforeBlocks;
  g_counting = false;
  return SiteResult{site.name, leaked, blocks, sources};
}

/// The pattern this WO removes: one long-lived object, reset by move
/// assignment before each sleep. `storage` is deliberately never destroyed,
/// exactly like the removed file-scope global.
SiteResult runSharedReset(const RealSite &site, int cycles, bool standby) {
  g_counting = true;
  // Placement-new into raw storage so the destructor never runs, which is
  // what made the global leak: only ~SystemSleepConfiguration() frees the
  // wake-source list.
  alignas(SystemSleepConfiguration) static unsigned char storage[sizeof(SystemSleepConfiguration)];
  SystemSleepConfiguration *cfg = new (static_cast<void *>(storage)) SystemSleepConfiguration();
  const long long before = g_stats.outstandingBytes;
  const long long beforeBlocks = g_stats.outstandingBlocks;
  size_t sources = 0;
  for (int i = 0; i < cycles; i++) {
    *cfg = SystemSleepConfiguration();  // the leaking move assignment
    site.configure(*cfg, standby);
    if (i == 0) sources = wakeupSourceCount(*cfg);
    simulateSleep(*cfg);
  }
  const long long leaked = g_stats.outstandingBytes - before;
  const long long blocks = g_stats.outstandingBlocks - beforeBlocks;
  g_counting = false;
  return SiteResult{site.name, leaked, blocks, sources};
}

}  // namespace

int main() {
  const int cycles = 1000 * 5;  // comfortably above the WO's 1,000 minimum

  std::printf("Device OS header: %s\n", DEVICE_OS_HEADER_DESCRIPTION);
  std::printf("Wake sources extracted from: %s\n", REAL_SITES_SOURCE);
  std::printf("Cycles per site: %d\n\n", cycles);

  // Sanity: the allocator is actually seeing the builders' allocations.
  {
    g_counting = true;
    const long long before = g_stats.totalAllocs;
    {
      SystemSleepConfiguration cfg;
      cfg.mode(SystemSleepMode::STOP).gpio(BUTTON_PIN, FALLING).duration(1000UL);
      check(wakeupSourceCount(cfg) == 2, "a GPIO + a duration make two wake sources");
    }
    const long long allocs = g_stats.totalAllocs - before;
    g_counting = false;
    check(allocs >= 2, "the counting allocator sees the builders' allocations");
    check(g_stats.outstandingBytes == 0,
          "the destructor frees everything the builders allocated");
  }

  std::printf("--- New pattern: a fresh local SystemSleepConfiguration per sleep ---\n");
  bool anySites = false;
  for (const RealSite &site : kRealSites) {
    anySites = true;
    for (int s = 0; s < (site.hasNetworkStandby ? 2 : 1); s++) {
      const bool standby = (s == 1);
      SiteResult r = runFreshLocal(site, cycles, standby);
      std::printf("  %-16s standby=%d wake-sources=%zu leaked=%lld B in %lld blocks\n",
                  r.name, standby ? 1 : 0, r.wakeSources, r.leakedBytes, r.leakedBlocks);
      check(r.wakeSources == (size_t)(site.expectedWakeSources + (standby ? 1 : 0)),
            "the site configures the expected number of wake sources");
      check(r.leakedBytes == 0, "the fresh-local pattern must lose zero bytes");
      check(r.leakedBlocks == 0, "the fresh-local pattern must lose zero blocks");
    }
  }
  check(anySites, "at least one real sleep site was extracted");

  std::printf("\n--- Mutation: one long-lived object reset by move assignment ---\n");
  bool mutationCaught = true;
  for (const RealSite &site : kRealSites) {
    SiteResult r = runSharedReset(site, cycles, site.hasNetworkStandby);
    const double perCycle = (double)r.leakedBytes / (double)cycles;
    std::printf("  %-16s leaked=%lld B in %lld blocks over %d cycles (%.1f B/cycle)\n",
                r.name, r.leakedBytes, r.leakedBlocks, cycles, perCycle);
    if (r.leakedBytes <= 0) {
      std::printf("FAILED: the old pattern did not leak at %s - the mutation "
                  "is not exercising Device OS's move assignment\n", r.name);
      g_failures++;
      mutationCaught = false;
      continue;
    }
    // Every cycle orphans the whole list: one block per wake source.
    if (r.leakedBlocks < (long long)cycles) {
      std::printf("FAILED: the old pattern leaked %lld blocks over %d cycles at "
                  "%s - expected at least one orphaned node per cycle\n",
                  r.leakedBlocks, cycles, r.name);
      g_failures++;
      mutationCaught = false;
    }
    // The zero-leak assertion, applied to the old pattern, must fail.
    if (r.leakedBytes == 0) {
      mutationCaught = false;
    }
  }
  check(mutationCaught,
        "the mutation (restoring the long-lived reset object) fails the "
        "zero-leak assertion");

  std::printf("\n");
  if (g_failures) {
    std::printf("sleep_config_leak_test FAILED (%d check(s))\n", g_failures);
    return 1;
  }
  std::printf("sleep_config_leak_test passed: zero bytes lost per cycle with a "
              "fresh local; the old shared-reset pattern leaks every cycle\n");
  return 0;
}

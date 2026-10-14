// WO-2026-10-14-001 (Step 6 2b-1): golden test for publishData()'s webhook payload.
// run() is the REAL publishData() body, extracted verbatim by the .sh from
// batteryContext[] through the publish call (gen_publish_region.inc). Each vector
// drives it through counted fakes and an observer of PublishQueuePosix::publish().
// Usage: report_payload_golden_test <fixture>   compares every vector to the fixture
//        report_payload_golden_test --dump       prints the lines (used once to record)
// The fixture is never written by the test; it was recorded from the unchanged src/
// on the host below. NaN/Inf text is host libc.
#include <cfloat>
#include <clocale>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <new>
#include <string>
#include <vector>

static long g_heap = 0;
void *operator new(size_t n) { ++g_heap; void *p = malloc(n ? n : 1); if (!p) abort(); return p; }
void operator delete(void *p) noexcept { free(p); }
void operator delete(void *p, size_t) noexcept { free(p); }

struct In {
  uint8_t mode = 0; time_t stamp = 0; int minute = 27, second = 14; uint8_t bs = 2;
  float soc = 87.5f; bool vcOk = true; float vc = 3.91f; float temp = 23.4f;
  int8_t alert = 7; uint8_t resets = 3; uint16_t conn = 45; bool occ = true; uint32_t occSec = 7384;
  uint16_t hourly = 12, daily = 88; uint32_t freeMem = 61234, lfb = 40960, cycles = 5, sleeps = 3;
  const char *name = "my-hook"; bool queued = true;
};
static In in;
enum { NOW, MIN, SEC, FREE, RTINFO, CYC, SLP, MILLIS, BSTATE, SOC, VC, MODE, CONN, ALERT, RESETS, OCC, TOTOCC,
       HOURLY, DAILY, TEMP, NAME, PUB, BREAD, NC };
static const char *const cn[NC] = {"Time.now", "Time.minute", "Time.second", "freeMemory", "HAL_Core_Runtime_Info",
  "AwakeCycles::cycles", "AwakeCycles::sleeps", "millis", "batteryState", "soc", "cachedBatteryVoltage",
  "get_sensorMode", "get_lastConnectionDuration", "get_alertCode", "get_resetCount", "get_occupied",
  "get_totalOccupiedSeconds", "get_hourlyCount", "get_dailyCount", "get_internalTempC", "getWebhookName",
  "publish", "setAppBreadcrumb"};
static int calls[NC];

// Device OS values (lib/StorageHelperRK/automated-test/UnitTestLib/Particle.h:203-206).
struct PublishFlags { uint8_t v; constexpr PublishFlags(uint8_t x = 0) : v(x) {} };
constexpr PublishFlags operator|(PublishFlags a, PublishFlags b) { return PublishFlags(a.v | b.v); }
__attribute__((unused)) const PublishFlags PRIVATE(0x1), WITH_ACK(0x8);

static struct { time_t now() { ++calls[NOW]; return 1791200000; } int minute() { ++calls[MIN]; return in.minute; }
                int second() { ++calls[SEC]; return in.second; } } Time;
static struct { uint32_t freeMemory() { ++calls[FREE]; return in.freeMem; } } System;
struct runtime_info_t { uint16_t size; uint32_t largest_free_block_heap; };
static bool rtSizeOk = true;
static void HAL_Core_Runtime_Info(runtime_info_t *r, void *) {
  ++calls[RTINFO]; rtSizeOk = rtSizeOk && r->size == sizeof(runtime_info_t); r->largest_free_block_heap = in.lfb; }
struct Ctr { int id; operator uint32_t() { ++calls[id]; return id == CYC ? in.cycles : in.sleeps; } };
namespace AwakeCycles { static Ctr cycles{CYC}, sleeps{SLP}; }
static unsigned long millis() { ++calls[MILLIS]; return 1000; }
struct PowerManager { static PowerManager &instance() { static PowerManager p; return p; }
  uint8_t batteryState() const { ++calls[BSTATE]; return in.bs; } float soc() const { ++calls[SOC]; return in.soc; } };
struct SensorManager { static SensorManager &instance() { static SensorManager s; return s; }
  bool cachedBatteryVoltage(float &v) const { ++calls[VC]; if (!in.vcOk) return false; v = in.vc; return true; } };
namespace SystemConfig { enum { COUNTING = 0, OCCUPANCY = 1, MEASUREMENT = 2 };
  static uint8_t get_sensorMode() { ++calls[MODE]; return in.mode; }
  static uint16_t get_lastConnectionDuration() { ++calls[CONN]; return in.conn; } }
namespace RecoveryState { static int8_t get_alertCode() { ++calls[ALERT]; return in.alert; }
  static uint8_t get_resetCount() { ++calls[RESETS]; return in.resets; } }
namespace CurrentReadings {
  static bool get_occupied() { ++calls[OCC]; return in.occ; }
  static uint32_t get_totalOccupiedSeconds() { ++calls[TOTOCC]; return in.occSec; }
  static uint16_t get_hourlyCount() { ++calls[HOURLY]; return in.hourly; }
  static uint16_t get_dailyCount() { ++calls[DAILY]; return in.daily; }
  static float get_internalTempC() { ++calls[TEMP]; return in.temp; } }
struct Cloud { static Cloud &instance() { static Cloud c; return c; }
  const char *getWebhookName() const { ++calls[NAME]; return in.name; } };
static void setAppBreadcrumb(uint8_t) { ++calls[BREAD]; }
static const uint8_t BREADCRUMB_REPORT_QUEUE_START = 1;

// Observer of the real queue call; static storage only so heap use stays observable.
static struct Obs { const char *name; char data[512]; size_t len; uint8_t f1, f2;
  bool publish(const char *n, const char *d, PublishFlags a, PublishFlags b = PublishFlags()) {
    ++calls[PUB]; name = n; len = strlen(d); memcpy(data, d, len < 511 ? len + 1 : 511); data[511] = 0;
    f1 = a.v; f2 = b.v; return in.queued; } } obs;
struct PublishQueuePosix { static Obs &instance() { return obs; } };

static bool run(time_t stampOverride) {
#include "gen_publish_region.inc"
  (void)queueStartMs;
  return queued;
}

struct V { std::string id; In in; };
static std::string S(const char *f, int a = 0, int b = 0, int c = 0) { char t[96]; snprintf(t, sizeof t, f, a, b, c); return t; }

static std::vector<V> vectors() {
  std::vector<V> v;
  auto add = [&](const std::string &id, In i) { v.push_back({id, i}); };
  const int8_t alerts[] = {-128, -1, 0, 19, 42, 127};
  const float socs[] = {NAN, INFINITY, -INFINITY, -0.01f, 0.0f, 87.5f, 100.0f, 100.01f};
  const uint8_t states[] = {0, 2, 6, 7, 255};
  const float temps[] = {23.4f, -999.99f, NAN, INFINITY, -INFINITY, FLT_MAX, -FLT_MAX};
  const char *names[] = {"", "my-hook", "counting-webhook-v1"};
  for (int m = 0; m < 3; m++) for (int s = 0; s < 2; s++) for (int t = 0; t < 2; t++) {
    In i; i.mode = m; i.stamp = s ? 1791100000 : 0; i.minute = t ? 0 : 27; i.second = t ? 0 : 14;
    add(S("mode%d_stamp%d_ms%d", m, s, t), i); }
  for (int m = 0; m < 2; m++) for (int a = 0; a < 6; a++) { In i; i.mode = m; i.alert = alerts[a]; add(S("alert_m%d_%d", m, a), i); }
  for (int s = 0; s < 8; s++) for (int b = 0; b < 5; b++) { In i; i.soc = socs[s]; i.bs = states[b]; add(S("soc%d_bs%d", s, b), i); }
  for (int k = 0; k < 3; k++) { In i; i.vcOk = k != 0; i.vc = k == 1 ? 3.91f : 4.20f; add(S("vc%d", k), i); }
  for (int m = 0; m < 2; m++) for (int k = 0; k < 7; k++) { In i; i.mode = m; i.temp = temps[k]; add(S("temp_m%d_%d", m, k), i); }
  In wide;
  for (int m = 0; m < 2; m++) {  // all fields at their widest; %lu inputs stay within 32 bits
    In i; i.mode = m; i.stamp = 4294967295U; i.minute = 59; i.second = 59; i.bs = 6; i.soc = 100.0f; i.vc = 4.20f;
    i.temp = -99.99f; i.alert = -128; i.resets = 255; i.conn = 65535; i.occSec = 4294967295U; i.hourly = i.daily = 65535;
    i.freeMem = i.lfb = i.cycles = i.sleeps = 4294967295U; add(S("maxwidth_m%d", m), i);
    if (m == 1) wide = i;
    i.temp = FLT_MAX; add(S("maxwidth_fltmax_m%d", m), i); }
  for (int k = 0; k <= 38; k++) {  // temp = 1e0..1e38 on the widest occupancy payload: straddles 255/256 bytes
    In i = wide; i.temp = strtof(S("1e%d", k).c_str(), nullptr); add(S("sweep_1e%d", k), i); }
  for (int n = 0; n < 3; n++) { In i; i.name = names[n]; add(S("name%d", n), i); }
  for (int m = 0; m < 2; m++) { In i; i.mode = m; i.queued = false; add(S("queuefalse_m%d", m), i); }
  // Occupancy-mode mirrors of the SOC/battState, vc and name vectors (appended; queue false and alert/temp already cover mode 1).
  for (int s = 0; s < 8; s++) for (int b = 0; b < 5; b++) { In i; i.mode = 1; i.soc = socs[s]; i.bs = states[b]; add(S("occ_soc%d_bs%d", s, b), i); }
  for (int k = 0; k < 3; k++) { In i; i.mode = 1; i.vcOk = k != 0; i.vc = k == 1 ? 3.91f : 4.20f; add(S("occ_vc%d", k), i); }
  for (int n = 0; n < 3; n++) { In i; i.mode = 1; i.name = names[n]; add(S("occ_name%d", n), i); }
  return v;
}

int main(int argc, char **argv) {
  setlocale(LC_ALL, "C");
  const bool dump = argc > 1 && !strcmp(argv[1], "--dump");
  std::vector<std::string> got, want;
  int fails = 0;
  for (const V &vec : vectors()) {
    in = vec.in; memset(calls, 0, sizeof calls); obs = Obs{}; rtSizeOk = true;
    const long heap0 = g_heap;
    const bool queued = run(vec.in.stamp);
    const long heapUsed = g_heap - heap0;
    int expect[NC]; for (int &e : expect) e = 1;
    expect[OCC] = expect[TOTOCC] = in.mode == 1; expect[HOURLY] = expect[DAILY] = in.mode != 1;
    for (int c = 0; c < NC; c++) if (calls[c] != expect[c]) { printf("FAIL %s: %s called %d times\n", vec.id.c_str(), cn[c], calls[c]); fails++; }
    if (obs.name != in.name || obs.f1 != 0x9 || obs.f2 != 0 || queued != in.queued || obs.len > 255 || heapUsed != 0 || !rtSizeOk) {
      printf("FAIL %s: name/flags/return/length/heap (%p f1=%u f2=%u len=%zu heap=%ld)\n", vec.id.c_str(), (const void *)obs.name, obs.f1, obs.f2, obs.len, heapUsed); fails++; }
    char head[160]; snprintf(head, sizeof head, "%s|%s|%u|%u|%zu|%d|", vec.id.c_str(), obs.name, obs.f1, obs.f2, obs.len, queued);
    got.push_back(std::string(head) + obs.data);
  }
  if (dump) { for (auto &l : got) puts(l.c_str()); return fails ? 1 : 0; }
  std::ifstream f(argc > 1 ? argv[1] : ""); std::string line;
  while (std::getline(f, line)) if (!line.empty() && line[0] != '#') want.push_back(line);
  if (want.size() != got.size()) { printf("FAIL: %zu vectors, fixture has %zu\n", got.size(), want.size()); fails++; }
  for (size_t i = 0; i < got.size() && i < want.size(); i++) if (got[i] != want[i]) {
    printf("FAIL golden mismatch\n  want: %s\n  got:  %s\n", want[i].c_str(), got[i].c_str()); fails++; }
  printf("%zu vectors, %d failures\n", got.size(), fails);
  return fails ? 1 : 0;
}

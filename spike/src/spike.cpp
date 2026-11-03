// WO-2026-11-01-001 spike: AB1805 native-driver bench image for Dev-14.
// UNMERGED, NEVER RELEASED, NEVER UPLOADED. Flash over USB only.
//
// One image, one serial command per proof (type `help`). Order on the bench:
//   p7 (vendored deepPowerDown, BEFORE any bind), then bind, p5 (two 8 h native hibernates).
// Diagnostics are Log lines; results are also kept in EEPROM and published as "spike" events.
// It does not use the GCC payload, retained layout or persisted layout.

#include "Particle.h"
#include "AB1805_RK.h"      // vendored library: used for P7 only
#include <time.h>
#include <sys/time.h>

SYSTEM_MODE(AUTOMATIC);
SYSTEM_THREAD(ENABLED);
SerialLogHandler logHandler(LOG_LEVEL_INFO);

using namespace particle;

static const uint32_t SPIKE_MAGIC = 0x53504B31;   // "SPK1"
static const uint32_t P5_HIBERNATE_SEC = 14400;
static const uint32_t P5_AWAKE_MS = 10UL * 60UL * 1000UL;
static const uint8_t  P5_ROUNDS = 2;

enum Phase : uint8_t { PH_IDLE = 0, PH_P5_HIBERNATING = 1, PH_P5_AWAKE = 2, PH_P5_DONE = 3, PH_WDT_STOPPED = 4, PH_P7_ARMED = 5 };

struct SpikeRec {
  uint32_t magic;
  uint8_t phase;
  uint8_t round;          // 1-based round currently hibernating / awake
  uint16_t wakes;         // count of boots recorded by this image
  uint32_t reqEpoch;      // external RTC time at the hibernate request
  uint32_t reqSec;        // requested hibernate seconds
  int32_t  err[P5_ROUNDS];        // actual - req, per round (s); INT32_MIN = not recorded
  int32_t  uptimeMs[P5_ROUNDS];   // app uptime when the RTC was read at boot
  uint8_t  reason[P5_ROUNDS];     // System.resetReason() at that wake
  uint8_t  statusReg[P5_ROUNDS];  // raw AM18x5 STATUS (0x0F) read at the start of setup()
  uint32_t wdtStopEpoch;  // P4: external RTC time when refreshing stopped
  uint32_t wdtTimeoutMs;
  uint32_t p7Epoch;       // P7: external/system time at the deepPowerDown call
  uint32_t p7Sec;
};

static SpikeRec rec;
static TimeClass extTime(TimeSource::EXTERNAL);     // TimeClass(TimeSource::EXTERNAL)
static uint8_t bootStatusReg = 0xFF;
static uint32_t bootRtcEpoch = 0;
static uint32_t bootMillis = 0;
static system_tick_t lastSyncSeen = 0;
static bool firstSyncLogged = false;
static uint32_t awakeCloudStartMs = 0;
static bool wdtRunning = false;
static uint32_t wdtLastRefresh = 0;
static char cmd[64];
static size_t cmdLen = 0;
static uint32_t p3StartMs = 0, p3NextMs = 0;
static bool p3Active = false;
static bool pendingPublish = false;
static char pubBuf[200];
static AB1805 ab1805(Wire);

static void saveRec() { EEPROM.put(0, rec); }

static void loadRec() {
  EEPROM.get(0, rec);
  if (rec.magic != SPIKE_MAGIC) {
    memset(&rec, 0, sizeof(rec));
    rec.magic = SPIKE_MAGIC;
    for (int i = 0; i < P5_ROUNDS; i++) rec.err[i] = INT32_MIN;
    saveRec();
  }
}

static uint8_t readRtcReg(uint8_t reg) {
  Wire.lock();
  Wire.beginTransmission(0x69);
  Wire.write(reg);
  uint8_t v = 0xFF;
  if (Wire.endTransmission(false) == 0 && Wire.requestFrom((uint8_t)0x69, (uint8_t)1) == 1) v = Wire.read();
  Wire.unlock();
  return v;
}

static void logStatus(const char* tag) {
  auto st = ExternalTime.status();
  Log.info("%s: ext status valid=%d err=%d type=%d bound=%d present=%d ready=%d clkSrc=%d caps_en=0x%lx xtalCal=%d",
           tag, (int)st.valid(), st.error(), (int)st.type(), (int)st.bound(), (int)st.present(), (int)st.ready(),
           (int)st.clockSource(), (unsigned long)st.enabledCapabilities().value(), (int)st.xtalCalibration());
}

static void logTimes(const char* tag) {
  struct timeval tvh = {};
  hal_rtc_option_t opt = {};
  opt.size = sizeof(opt);
  opt.source = HAL_RTC_SOURCE_EXTERNAL;
  int r = hal_rtc_get_time(&tvh, &opt);
  Log.info("%s: sys=%ld valid=%d | TimeClass(EXTERNAL)=%ld | hal_rtc EXTERNAL=%ld r=%d | millis=%lu",
           tag, (long)Time.now(), (int)Time.isValid(), (long)extTime.now(), (long)tvh.tv_sec, r, (unsigned long)millis());
}

static void doBind() {
  Am18x5Configuration cfg;
  cfg.i2c(Wire)                       // Wire, 0x69
     .interruptPin(WKP)
     .watchdogPin(D5)
     .capabilities(RtcCap::AUTO_CLOCK_SOURCE_INTERNAL_ON_FAIL)   // rc_fallback on; ..._ON_BATTERY left out = always-on RC off
     .clockSource(RtcClockSource::EXTERNAL);
  int r = Am18x5.enable(cfg);
  Log.info("bind: Am18x5.enable() -> %d (persists across reboots; reboot to confirm)", r);
  logStatus("bind");
}

static void doUnbind() {
  int r = ExternalTime.disable();
  Log.info("unbind: ExternalTime.disable() -> %d (reboot and run `status` to confirm not bound)", r);
}

static void startP5Round(uint8_t round) {
  rec.round = round;
  rec.phase = PH_P5_HIBERNATING;
  rec.reqSec = P5_HIBERNATE_SEC;
  rec.reqEpoch = (uint32_t)extTime.now();
  saveRec();
  Log.info("P5 round %u: HIBERNATE %lu s requested at ext RTC %lu (sys %ld). Wake due at ext RTC %lu",
           round, (unsigned long)rec.reqSec, (unsigned long)rec.reqEpoch, (long)Time.now(),
           (unsigned long)(rec.reqEpoch + rec.reqSec));
  delay(1500);
  SystemSleepConfiguration cfg;
  cfg.mode(SystemSleepMode::HIBERNATE).duration(rec.reqSec * 1000UL);
  SystemSleepResult res = System.sleep(cfg);
  // Only reached if the sleep call failed.
  Log.error("P5: System.sleep returned (error %d)", (int)res.error());
  rec.phase = PH_IDLE;
  saveRec();
}

static void doP7(uint32_t sec) {
  // Vendored AB1805_RK path exactly as v48 failsafe stage 3 (ConnectivityFailsafe.cpp:314).
  ab1805.withFOUT(WKP).setup();
  rec.phase = PH_P7_ARMED;
  rec.p7Epoch = (uint32_t)Time.now();
  rec.p7Sec = sec;
  saveRec();
  Log.info("P7: vendored deepPowerDown(%lu) at sys %lu. WATCH: modem LED dark? device offline for the period? timer boot?",
           (unsigned long)sec, (unsigned long)rec.p7Epoch);
  delay(1500);
  bool ok = ab1805.deepPowerDown((int)sec);
  Log.error("P7: deepPowerDown returned %d: it did NOT power down. v48 stage 3 would fall through. Resetting in 5 s.", (int)ok);
  delay(5000);
  System.reset();
}

static void doWdt(uint32_t sec) {
  if (ExternalTime.status().bound() == false) { Log.error("wdt: bind first"); return; }
  int r = ExternalWatchdog.init(WatchdogConfiguration().timeout(sec * 1000UL));
  int s = ExternalWatchdog.start();
  Log.info("wdt: ExternalWatchdog init=%d start=%d timeout=%lu s; refreshing every %lu s. `wdtstop` stops refreshing.",
           r, s, (unsigned long)sec, (unsigned long)(sec / 3 ? sec / 3 : 1));
  rec.wdtTimeoutMs = sec * 1000UL;
  wdtRunning = true;
  wdtLastRefresh = millis();
}

static void printHelp() {
  Log.info("cmds: help status time | p7 [sec=120] | bind unbind | settime <epoch> <ext|hal> | skew <sec> | p3 | wdt <sec> wdtstop | p5 p5abort res | reboot");
}

static void printResults() {
  Log.info("rec: phase=%u round=%u wakes=%u", rec.phase, rec.round, rec.wakes);
  for (int i = 0; i < P5_ROUNDS; i++) {
    if (rec.err[i] == INT32_MIN) Log.info("P5 round %d: no wake recorded", i + 1);
    else Log.info("P5 round %d: actual-req=%ld s reason=%u uptimeAtRead=%ld ms bootSTATUS=0x%02x",
                  i + 1, (long)rec.err[i], rec.reason[i], (long)rec.uptimeMs[i], rec.statusReg[i]);
  }
}

static void handleCommand(char* line) {
  char* a = strtok(line, " ");
  if (!a) return;
  char* b = strtok(nullptr, " ");
  char* c = strtok(nullptr, " ");
  if (!strcmp(a, "help")) printHelp();
  else if (!strcmp(a, "status")) { logStatus("status"); logTimes("status"); printResults(); }
  else if (!strcmp(a, "time")) logTimes("time");
  else if (!strcmp(a, "p7")) doP7(b ? strtoul(b, nullptr, 10) : 120);
  else if (!strcmp(a, "bind")) doBind();
  else if (!strcmp(a, "unbind")) doUnbind();
  else if (!strcmp(a, "settime") && b) {
    time_t t = strtoul(b, nullptr, 10);
    if (c && !strcmp(c, "hal")) {
      struct timeval tv = {t, 0};
      hal_rtc_option_t opt = {};
      opt.size = sizeof(opt);
      opt.source = HAL_RTC_SOURCE_EXTERNAL;
      Log.info("settime hal_rtc_set_time(EXTERNAL) -> %d", hal_rtc_set_time(&tv, &opt));
    } else {
      extTime.setTime(t);
      Log.info("settime TimeClass(EXTERNAL).setTime(%ld) done", (long)t);
    }
    logTimes("settime");
  } else if (!strcmp(a, "skew") && b) {
    long d = strtol(b, nullptr, 10);
    time_t t = Time.now() - d;
    extTime.setTime(t);
    Log.info("P3: external RTC set %ld s behind system. Now `p3` to sync and watch", d);
    logTimes("skew");
  } else if (!strcmp(a, "p3")) {
    Particle.syncTime();
    p3Active = true; p3StartMs = millis(); p3NextMs = 0;
    Log.info("P3: syncTime requested; logging ext vs sys every 5 s for 5 min");
  } else if (!strcmp(a, "wdt") && b) doWdt(strtoul(b, nullptr, 10));
  else if (!strcmp(a, "wdtstop")) {
    wdtRunning = false;
    rec.phase = PH_WDT_STOPPED;
    rec.wdtStopEpoch = (uint32_t)extTime.now();
    saveRec();
    Log.info("P4: refreshing STOPPED at ext RTC %lu; expect a reset within timeout+1 s", (unsigned long)rec.wdtStopEpoch);
  } else if (!strcmp(a, "p5")) {
    if (!Time.isValid()) { Log.error("p5: no valid time yet: wait for the cloud sync (or `settime <epoch> ext`)"); return; }
    if (!Particle.connected()) Log.warn("p5: cloud not connected; continuing on the valid RTC time");
    if (!ExternalTime.status().ready()) { Log.error("p5: external RTC not ready: run `bind`, reboot, then retry"); return; }
    for (int i = 0; i < P5_ROUNDS; i++) { rec.err[i] = INT32_MIN; }
    startP5Round(1);
  } else if (!strcmp(a, "p5abort")) { rec.phase = PH_IDLE; saveRec(); Log.info("p5 aborted"); }
  else if (!strcmp(a, "res")) printResults();
  else if (!strcmp(a, "reboot")) System.reset();
  else printHelp();
}

void setup() {
  bootMillis = millis();
  bootStatusReg = readRtcReg(0x0F);        // raw STATUS at the first moment the app runs (stale-alarm flag evidence)
  waitFor(Serial.isConnected, 15000);
  loadRec();
  rec.wakes++;
  const int reason = System.resetReason();
  const uint32_t rdata = System.resetReasonData();
  bootRtcEpoch = (uint32_t)extTime.now();
  const uint32_t uptimeAtRead = millis() - bootMillis;
  Log.info("=== ab1805-spike boot: resetReason=%d data=%lu phase=%u round=%u wakes=%u bootSTATUS=0x%02x ===",
           reason, (unsigned long)rdata, rec.phase, rec.round, rec.wakes, bootStatusReg);
  logStatus("boot");
  Log.info("boot: RTC at boot (TimeClass EXTERNAL)=%lu sys=%ld sysValid=%d uptime=%lu ms", (unsigned long)bootRtcEpoch,
           (long)Time.now(), (int)Time.isValid(), (unsigned long)uptimeAtRead);
  logTimes("boot");
  ExternalTime.onEvent([](RtcEvents ev) { Log.info("RTC EVENT 0x%lx", (unsigned long)ev.value()); });

  if (rec.phase == PH_P5_HIBERNATING) {
    int idx = rec.round - 1;
    long err = (long)bootRtcEpoch - (long)(rec.reqEpoch + rec.reqSec);
    rec.err[idx] = err;
    rec.uptimeMs[idx] = (int32_t)uptimeAtRead;
    rec.reason[idx] = (uint8_t)reason;
    rec.statusReg[idx] = bootStatusReg;
    rec.phase = PH_P5_AWAKE;
    Log.info("P5 round %u WAKE: actual-req = %ld s (ext RTC %lu vs due %lu), reason=%d, uptime at read %lu ms, bootSTATUS=0x%02x (bits: TIM 0x08, ALM 0x04 set = alarm flag still pending)",
             rec.round, err, (unsigned long)bootRtcEpoch, (unsigned long)(rec.reqEpoch + rec.reqSec), reason,
             (unsigned long)uptimeAtRead, bootStatusReg);
    snprintf(pubBuf, sizeof(pubBuf), "{\"p5round\":%u,\"err\":%ld,\"reason\":%d,\"status\":%u,\"upms\":%lu}",
             rec.round, err, reason, bootStatusReg, (unsigned long)uptimeAtRead);
    pendingPublish = true;
  } else if (rec.phase == PH_P5_AWAKE) {
    Log.info("P5: reset during the awake window (round %u); awake timer restarts", rec.round);
  } else if (rec.phase == PH_WDT_STOPPED) {
    long el = (long)bootRtcEpoch - (long)rec.wdtStopEpoch;
    Log.info("P4: reset after refreshing stopped: elapsed %ld s (timeout %lu ms), reason=%d data=%lu", el,
             (unsigned long)rec.wdtTimeoutMs, reason, (unsigned long)rdata);
    rec.phase = PH_IDLE;
  } else if (rec.phase == PH_P7_ARMED) {
    long off = (long)Time.now() - (long)rec.p7Epoch;
    Log.info("P7: BOOT after deepPowerDown(%lu): reason=%d data=%lu (power-on reset expected, reason 1=RESET_REASON_POWER_DOWN? 130-ish=PIN). RTC at boot %lu vs call %lu = %ld s off",
             (unsigned long)rec.p7Sec, reason, (unsigned long)rdata, (unsigned long)bootRtcEpoch, (unsigned long)rec.p7Epoch,
             (long)bootRtcEpoch - (long)rec.p7Epoch);
    (void)off;
    rec.phase = PH_IDLE;
  }
  saveRec();
  printHelp();
}

void loop() {
  // first cloud time sync after boot: external RTC (read at boot) against cloud time
  system_tick_t synced = Particle.timeSyncedLast();
  if (!firstSyncLogged && synced != 0 && synced != lastSyncSeen && Particle.connected()) {
    firstSyncLogged = true;
    long sysNow = (long)Time.now();
    long extNow = (long)extTime.now();
    long elapsed = (long)((millis() - bootMillis) / 1000UL);
    long bootRtcProjected = (long)bootRtcEpoch + elapsed;   // boot RTC read, advanced by app uptime
    Log.info("FIRST SYNC: cloud/sys=%ld, ext RTC now=%ld (ext-sys=%ld s), boot RTC read=%lu projected to now=%ld => boot RTC error vs cloud = %ld s (uptime %ld s)",
             sysNow, extNow, extNow - sysNow, (unsigned long)bootRtcEpoch, bootRtcProjected, bootRtcProjected - sysNow, elapsed);
    snprintf(pubBuf, sizeof(pubBuf), "{\"firstsync\":1,\"bootrtc_err\":%ld,\"ext_minus_sys\":%ld,\"reason\":%d}",
             bootRtcProjected - sysNow, extNow - sysNow, (int)System.resetReason());
    pendingPublish = true;
  }
  lastSyncSeen = synced;

  if (pendingPublish && Particle.connected()) {
    Particle.publish("spike", pubBuf, PRIVATE);
    pendingPublish = false;
  }

  if (p3Active && millis() >= p3NextMs) {
    p3NextMs = millis() + 5000;
    Log.info("P3: +%lu s sys=%ld ext=%ld ext-sys=%ld syncedLast=%lu", (unsigned long)((millis() - p3StartMs) / 1000),
             (long)Time.now(), (long)extTime.now(), (long)extTime.now() - (long)Time.now(), (unsigned long)Particle.timeSyncedLast());
    if (millis() - p3StartMs > 300000UL) p3Active = false;
  }

  if (wdtRunning && millis() - wdtLastRefresh >= (rec.wdtTimeoutMs / 3)) {
    ExternalWatchdog.refresh();
    wdtLastRefresh = millis();
  }

  // Poor-connectivity bench (Singtel): if the cloud has not connected 20 min after boot, start the awake window anyway.
  if (rec.phase == PH_P5_AWAKE && (Particle.connected() || millis() - bootMillis >= 20UL * 60UL * 1000UL)) {
    if (awakeCloudStartMs == 0) { awakeCloudStartMs = millis(); Log.info("P5: awake window starts (cloud connected=%d)", (int)Particle.connected()); }
    if (millis() - awakeCloudStartMs >= P5_AWAKE_MS) {
      if (rec.round < P5_ROUNDS) startP5Round(rec.round + 1);
      else {
        rec.phase = PH_P5_DONE; saveRec();
        Log.info("P5 COMPLETE. Run `res`.");
        printResults();
        snprintf(pubBuf, sizeof(pubBuf), "{\"p5done\":1,\"e1\":%ld,\"e2\":%ld}", (long)rec.err[0], (long)rec.err[1]);
        pendingPublish = true;
      }
    }
  }

  while (Serial.available()) {
    char ch = Serial.read();
    if (ch == '\n' || ch == '\r') { if (cmdLen) { cmd[cmdLen] = 0; cmdLen = 0; handleCommand(cmd); } }
    else if (cmdLen < sizeof(cmd) - 1) cmd[cmdLen++] = ch;
  }
}

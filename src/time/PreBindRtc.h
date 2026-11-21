#pragma once
// v50 S1b (after Particle.h, time/Clock.h): before enable() on an unbound boot, seed Time from the chip and set WRTC. L: replace the AB1805_RK calls with raw I2C.
namespace PreBindRtc {
template <typename Ab, typename W> bool run(Ab &rtc, W &wire) {
  std::lock_guard<W> hold(wire);
  if (!wire.isEnabled()) wire.begin();
  uint8_t c1 = 0;
  const bool read = rtc.readRegister(0x10, c1);
  const bool ok = read && (!(c1 & 0x01) || rtc.writeRegister(0x10, c1 & ~0x01));  // the library's time read fails while WRTC is set
  time_t chip = 0;
  const bool timeOk = rtc.getRtcAsTime(chip);
  if (timeOk && Clock::isPlausibleEpoch(chip) && !Time.isValid()) Time.setTime(chip);
  return (read && rtc.writeRegister(0x10, (c1 | 0x01) & ~0x40)) & ok & timeOk;  // CONTROL1: WRTC=1, 24-hour mode; written even if the time read failed (& keeps it unconditional)
}
}  // namespace PreBindRtc

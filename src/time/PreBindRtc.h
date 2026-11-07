#pragma once
// v50 S1b (after Particle.h, time/Clock.h): before enable() on an unbound boot, seed Time from the chip and set WRTC. L: replace the AB1805_RK calls with raw I2C.
namespace PreBindRtc {
template <typename Ab, typename W> bool run(Ab &rtc, W &wire) {
  std::lock_guard<W> hold(wire);
  if (!wire.isEnabled()) wire.begin();
  time_t chip = 0;
  if (rtc.getRtcAsTime(chip) && Clock::isPlausibleEpoch(chip) && !Time.isValid()) Time.setTime(chip);
  uint8_t c1 = 0;
  return rtc.readRegister(0x10, c1) && rtc.writeRegister(0x10, (c1 | 0x01) & ~0x40);  // CONTROL1: WRTC=1, 24-hour mode
}
}  // namespace PreBindRtc

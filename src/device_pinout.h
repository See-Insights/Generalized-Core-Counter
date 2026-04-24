/**
 * @file   device_pinout.h
 * @author Chip McClelland
 * @email  chip@seeinsights.com
 * @brief  Pinout definitions for the carrier board and sensors
 *
 * @details Logical pin names used by the firmware are defined here so
 *          the rest of the code does not depend on specific pin numbers.
 *          This makes it easy to swap sensors or carrier revisions by
 *          changing only this file.
 *
 * Carrier board header (PIR sensor on carrier)
 * -------------------------------------------------------------
 * Left Side (16 pins)
 * !RESET -
 * 3.3V  -
 * !MODE -
 * GND   -
 * D19 - A0 -
 * D18 - A1 -
 * D17 - A2 -
 * D16 - A3 -
 * D15 - A4 -               TMP32 temp sensor on carrier
 * D14 - A5 / SPI SS -
 * D13 - S2 - SCK  - SPI Clock -  intPin (PIR interrupt)
 * D12 - S0 - MOSI - SPI MOSI -   disableModule (enable line to sensor)
 * D11 - S1 - MISO - SPI MISO -   ledPower (indicator LED power)
 * D10 - UART RX -
 * D9  - UART TX -
 *
 * Right Side (12 pins)
 * Li+
 * ENABLE
 * VUSB -
 * D8  -                  wakeUpPin (watchdog wake)
 * D7  -                  blueLED (status LED)
 * D6  -                  deep-sleep enable (to EN)
 * D5  -                  watchdog DONE pin
 * D4  -                  userSwitch (front-panel button)
 * D3  -
 * D2  -
 * D1  - SCL - I2C Clock - FRAM / RTC / I2C bus
 * D0  - SDA - I2C Data  - FRAM / RTC / I2C bus
 *
 * @copyright Copyright (c) 2026 Chip McClelland
 * @license MIT License
 */

// ******************* Not a Singleton - Just a header file for pin definitions **********************

#ifndef DEVICE_PINOUT_H
#define DEVICE_PINOUT_H

#include "Particle.h"
// Logical pin names used throughout the application.
//
// The intent is to keep all *device-specific* pin numbers (D12 vs MOSI vs S1,
// etc.) inside device_pinout.cpp.  The rest of the firmware should only use
// these logical names so that swapping between Boron, P2, or future
// devices only requires changes in one place.

// ---------------------------------------------------------------------------
// Carrier-board common pins (independent of specific sensor)
// ---------------------------------------------------------------------------
extern const pin_t BUTTON_PIN;        // User switch on carrier (front-panel)
extern const pin_t TMP36_SENSE_PIN;   // Carrier board temperature sensor (A4)
extern const pin_t BLUE_LED;          // On-module blue status LED (D7)
extern const pin_t WAKEUP_PIN;        // AB1805 FOUT/nIRQ wake pin (WKP = D10 on Photon2, was D8 on Argon/Boron)

// Optional hardware override macros are documented in Settings.h.

// ---------------------------------------------------------------------------
// Sensor-specific logical pins (PIR-on-carrier configuration)
// ---------------------------------------------------------------------------
// NOTE: The actual hardware mapping (D13 vs S2 vs SCK, etc.) is handled in
// device_pinout.cpp using PLATFORM_ID (Boron vs P2, etc.). Application code
// should always use these names, never raw Dxx/Sx/MISO/MOSI identifiers.
extern const pin_t intPin;            // PIR interrupt pin (SPI clock line)
extern const pin_t disableModule;     // Sensor enable line
extern const pin_t ledPower;          // Sensor LED power

/**
 * @brief Initialize carrier-board pin modes used by the firmware.
 *
 * @return true once all required pins have been configured.
 */
bool initializePinModes();

/**
 * @brief Initialize platform power configuration.
 *
 * @details The active implementation currently leaves the system power profile
 *          unchanged, but this hook remains as the single place to enable a
 *          board-specific power configuration in the future.
 *
 * @return true when power-configuration initialization completes.
 */
bool initializePowerCfg();

/**
 * @brief Set the on-module blue LED state with an optional auto-off duration.
 *
 * @param state `true` turns the LED on and `false` turns it off.
 * @param durationMs Optional on-time in milliseconds. A value of `0` leaves the
 *        LED on indefinitely until another caller changes it.
 */
void signalLED(bool state, uint32_t durationMs = 0);

/**
 * @brief Process any pending automatic LED timeout.
 *
 * @details Call from the main loop to turn the LED off once the retained RTC
 *          deadline has expired.
 */
void signalLEDUpdate();

/**
 * @brief Get the remaining LED timeout in seconds.
 *
 * @return Seconds remaining until the LED should be turned off, or `0` if the
 *         LED is off, indefinite, expired, or time is not valid.
 */
uint32_t signalLEDTimeRemaining();

/**
 * @brief Report whether the blue status LED is currently on.
 *
 * @return true if the LED output is currently driven high.
 */
bool signalLEDStatus();

#endif
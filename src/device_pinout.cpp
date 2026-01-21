// Particle Functions
#include "Particle.h"
#include "device_pinout.h"

// Carrier board pin definitions
// See device_pinout.h for the full header mapping and documentation.

// ---------------------------------------------------------------------------
// Carrier-board common pins (same logical role on all platforms)
// ---------------------------------------------------------------------------
// TMP36 temperature sensor:
//  - On Boron carrier, wired to A4.
//  - On Photon 2 / P2 carrier, silk is "S4"; map explicitly for that
//    platform so we don't rely on A4 aliasing.

// Optional override (for example, a Muon carrier) that wires the TMP36 to a
// different analog-capable pin.
#if defined(MUON_TMP36_SENSE_PIN)
const pin_t TMP36_SENSE_PIN   = MUON_TMP36_SENSE_PIN;
#elif PLATFORM_ID == PLATFORM_P2
const pin_t TMP36_SENSE_PIN   = S4;
#else
const pin_t TMP36_SENSE_PIN   = A4;
#endif

const pin_t BUTTON_PIN        = D4;
const pin_t BLUE_LED          = D7;
const pin_t WAKEUP_PIN        = WKP;  // D10 on Photon2 (was D8 on Argon/Boron)

// Convenience aliases for carrier functions
// (No additional aliases; use the base names directly in application code.)

// ---------------------------------------------------------------------------
// Sensor-specific pins (PIR-on-carrier) with platform-specific mapping
// ---------------------------------------------------------------------------
// All device-specific identifiers (D12 vs MOSI vs S1, etc.) are handled here
// so the rest of the firmware only ever uses intPin/disableModule/ledPower.

// P2 uses S0/S1/S2 for the primary SPI header, while Boron exposes them as
// D11/D12/D13 (MISO/MOSI/SCK).  We select the correct mapping based on
// PLATFORM_ID so this same firmware can target both.

#if PLATFORM_ID == PLATFORM_P2
const pin_t intPin        = S2;   // PIR interrupt on SPI SCK-equivalent
const pin_t disableModule = S0;   // Sensor enable (SPI MOSI-equivalent)
const pin_t ledPower      = S1;   // Sensor LED power (SPI MISO-equivalent)

#elif PLATFORM_ID == PLATFORM_BORON
const pin_t intPin        = SCK;  // D13 on Boron
const pin_t disableModule = MOSI; // D12 on Boron
const pin_t ledPower      = MISO; // D11 on Boron

#else
// Fallback: assume SPI pins follow the common SCK/MOSI/MISO aliases.
const pin_t intPin        = SCK;
const pin_t disableModule = MOSI;
const pin_t ledPower      = MISO;
#endif

bool initializePinModes() {
    Log.info("Initalizing the pinModes");
    // Define as inputs or outputs
    pinMode(BUTTON_PIN, INPUT);    // User button on the carrier board - external pull-up on carrier
    pinMode(WAKEUP_PIN, INPUT_PULLUP);    // AB1805 FOUT/nIRQ (open-drain, active-LOW, needs pull-up)
    pinMode(BLUE_LED, OUTPUT);     // On-module status LED
     return true;
}


bool initializePowerCfg() {
    /*
    Log.info("Initializing Power Config");
    const int maxCurrentFromPanel = 900;            // Not currently used (100,150,500,900,1200,2000 - will pick closest) (550mA for 3.5W Panel, 340 for 2W panel)
    SystemPowerConfiguration conf;
    System.setPowerConfiguration(SystemPowerConfiguration());  // To restore the default configuration

    conf.powerSourceMaxCurrent(maxCurrentFromPanel) // Set maximum current the power source can provide  3.5W Panel (applies only when powered through VIN)
        .powerSourceMinVoltage(5080) // Set minimum voltage the power source can provide (applies only when powered through VIN)
        .batteryChargeCurrent(maxCurrentFromPanel) // Set battery charge current
        .batteryChargeVoltage(4208) // Set battery termination voltage
        .feature(SystemPowerFeature::USE_VIN_SETTINGS_WITH_USB_HOST); // For the cases where the device is powered through VIN
                                                                     // but the USB cable is connected to a USB host, this feature flag
                                                                     // enforces the voltage/current limits specified in the configuration
                                                                     // (where by default the device would be thinking that it's powered by the USB Host)
    int res = System.setPowerConfiguration(conf); // returns SYSTEM_ERROR_NONE (0) in case of success
    return res;
    */
    return true;
}
                

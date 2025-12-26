/**
 * @file   device_pinout.h
 * @author Chip McClelland
 * @date   7-5-2022
 * @brief  File containing the pinout documentation and initializations
 * */

 // ******************* Not a Singleton - Just a header file for pin definitions **********************

#ifndef DEVICE_PINOUT_H
#define DEVICE_PINOUT_H

#include "Particle.h"

// Pin definitions
// These pin definitions come from the carrier board schematic
extern const pin_t BUTTON_PIN;
extern const pin_t TMP36_SENSE_PIN;
// These are the pins that are standard for all Particle devices
extern const pin_t BLUE_LED;
extern const pin_t WAKEUP_PIN;   
// Specific to the sensor
// None

bool initializePinModes();
bool initializePowerCfg();

#endif
// src/PIRSensor.cpp
#include "PIRSensor.h"
#include "device_pinout.h"

// Static ISR flag and a simple counter so we can see
// in the main loop whether the ISR is ever firing.
volatile bool PIRSensor::_motionDetected = false;
volatile uint32_t PIRSensor::_isrCount = 0;

// Static ISR handler
void PIRSensor::pirISR() {
    _motionDetected = true;
    _isrCount++;
}

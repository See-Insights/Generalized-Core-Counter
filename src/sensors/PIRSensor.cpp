// src/PIRSensor.cpp
#include "sensors/PIRSensor.h"
#include "device_pinout.h"

volatile bool PIRSensor::_motionDetected = false;

// Static ISR handler
void PIRSensor::pirISR() {
    _motionDetected = true;
}

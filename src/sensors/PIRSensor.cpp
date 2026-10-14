// src/PIRSensor.cpp
#include "sensors/PIRSensor.h"
#include "device_pinout.h"
#include "persist/CurrentReadings.h"
#include "persist/SystemConfig.h"

volatile bool PIRSensor::_motionDetected = false;

// Static ISR handler
void PIRSensor::pirISR() {
    _motionDetected = true;
}

void PIRSensor::reportFields(char* out, size_t len, uint8_t sensorMode) const {
    if (sensorMode == SystemConfig::OCCUPANCY) {
        snprintf(out, len, "\"occupancy\":%d,\"dailyoccupancy\":%lu,",
                 CurrentReadings::get_occupied() ? 1 : 0,
                 (unsigned long)(CurrentReadings::get_totalOccupiedSeconds() / 60UL));
    } else {
        snprintf(out, len, "\"hourly\":%i,\"daily\":%i,",
                 CurrentReadings::get_hourlyCount(), CurrentReadings::get_dailyCount());
    }
}

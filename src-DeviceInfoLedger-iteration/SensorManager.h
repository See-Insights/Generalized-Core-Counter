/*
 * @file SensorManager.h
 * @author Chip McClelland (chip@seeinisghts.com)
 * @brief This function will take measurements at intervals defined int he
 * sleep_helper_config file. The libraries and functions needed will depend the
 * spectifics of the device and sensors
 *
 * @version 0.2
 * @date 2022-12-23
 *
 */

#ifndef SENSORMANAGER_H
#define SENSORMANAGER_H

#include "Particle.h"
#include "ISensor.h"

extern char internalTempStr[16];
extern char signalStr[64];

#define measure SensorManager::instance()

class SensorManager {
public:
    static SensorManager &instance();
    
    void setup();
    bool loop();
    
    void setSensor(ISensor* sensor);
    SensorData getSensorData() const;
    bool isSensorReady() const;
    
    // Utility functions
    float tmp36TemperatureC(int adcValue);
    bool batteryState();
    bool isItSafeToCharge();
    void getSignalStrength();
    void softDelay(uint32_t t);
    
protected:
    SensorManager();
    virtual ~SensorManager();
    SensorManager(const SensorManager &) = delete;
    SensorManager &operator=(const SensorManager &) = delete;
    
    static SensorManager *_instance;
    ISensor* _sensor;
    unsigned long _lastPollTime;
};

#endif /* SENSORMANAGER_H */
/**
 * @file Cloud.h
 * @brief Cloud Configuration Management - Particle Ledger integration for device configuration
 * 
 * @details Manages device configuration using Particle Ledger for offline device updates.
 *          Implements hierarchical configuration with product defaults and device-specific overrides.
 *          Uses simple manual merging logic (no external merging library).
 * 
 * @author Chip McClelland
 * @date December 12, 2025
 * 
 * @license MIT License
 * 
 * Copyright (c) 2025 Chip McClelland
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef __CLOUD_H
#define __CLOUD_H

#include "Particle.h"
#include "device_pinout.h"
#include "MyPersistentData.h"

/**
 * This class is a singleton; you do not create one as a global, on the stack, or with new.
 * 
 * From global application setup you must call:
 * Cloud::instance().setup();
 */
class Cloud {
public:
    /**
     * @brief Gets the singleton instance of this class, allocating it if necessary
     * 
     * Use Cloud::instance() to instantiate the singleton.
     */
    static Cloud &instance();

    /**
     * @brief Perform setup operations; call this from global application setup()
     * 
     * You typically use Cloud::instance().setup();
     */
    void setup();

    /**
     * @brief Load and apply configuration from cloud ledgers
     * 
     * Reads merged configuration from default-settings (Product scope) and 
     * device-settings (Device scope), with device settings overriding defaults.
     * 
     * @return true if configuration was successfully applied
     */
    bool loadConfigurationFromCloud();

    /**
     * @brief Write current device configuration to device-status ledger
     * 
     * Updates Device→Cloud ledger with current configuration for Console visibility.
     * 
     * @return true if successful
     */
    bool writeDeviceStatusToCloud();

    /**
     * @brief Publish latest sensor data to device-data ledger
     * 
     * Updates the Device→Cloud ledger with current sensor readings,
     * making data visible in Particle Console even when device is offline
     * 
     * @return true if successful
     */
    bool publishDataToLedger();

    /**
     * @brief Service deferred cloud work; call from main loop.
     *
     * Used to keep any ledger/status publishing out of callback context
     * and to avoid stacking multiple expensive operations in a single
     * loop() iteration.
     */
    void loop();

private:
    /**
     * @brief Apply configuration from DeviceInfoLedger to persistent storage
     * 
     * Reads merged configuration (product defaults + device overrides) from
     * DeviceConfigLedger and applies to sysStatus and sensorConfig.
     * 
     * @return true if successful
     */
    bool applyConfigurationFromLedger();

    /**
     * @brief Apply sensor configuration section
     * 
     * @return true if successful
     */
    bool applySensorConfig();

    /**
     * @brief Apply timing configuration section
     * 
     * @return true if successful
     */
    bool applyTimingConfig();

    /**
     * @brief Apply power configuration section
     * 
     * @return true if successful
     */
    bool applyPowerConfig();
    
    /**
     * @brief Callback when default-settings ledger syncs
     */
    static void onDefaultSettingsSync(Ledger ledger);
    
    /**
     * @brief Callback when device-settings ledger syncs
     */
    static void onDeviceSettingsSync(Ledger ledger);
    
    /**
     * @brief Merge default and device settings into mergedConfig
     */
    void mergeConfiguration();

    /**
     * @brief Apply messaging configuration section
     * 
     * @return true if successful
     */
    bool applyMessagingConfig();

    /**
     * @brief Apply modes configuration section
     * 
     * @return true if successful
     */
    bool applyModesConfig();

    /**
     * @brief Validate configuration value is within acceptable range
     * 
     * @param value Value to validate
     * @param min Minimum acceptable value
     * @param max Maximum acceptable value
     * @param name Parameter name for logging
     * @return true if valid
     */
    template<typename T>
    bool validateRange(T value, T min, T max, const String& name);

    /**
     * @brief Check if device configuration differs from product defaults
     * 
     * @return true if any values differ from defaults
     */
    bool hasNonDefaultConfig();

    /**
     * @brief Ledger for product default settings (Product scope, Cloud → Device)
     */
    Ledger defaultSettingsLedger;
    
    /**
     * @brief Ledger for device-specific overrides (Device scope, Cloud → Device)
     */
    Ledger deviceSettingsLedger;
    
    /**
     * @brief Ledger for device status reporting (Device → Cloud)
     */
    Ledger deviceStatusLedger;
    
    /**
     * @brief Ledger for sensor data reporting (Device → Cloud)
     */
    Ledger deviceDataLedger;
    
    /**
     * @brief Merged configuration data (defaults + device overrides)
     */
    LedgerData mergedConfig;
    
    /**
     * @brief Flag indicating if ledgers have synced from cloud
     */
    bool ledgersSynced;
    
    /**
     * @brief Store last published device status to detect changes
     */
    String lastPublishedStatus;

    /**
     * @brief Tracks success of last applyConfigurationFromLedger() call
     * 
     * Used by loadConfigurationFromCloud() to report whether configuration
     * was successfully applied during the most recent merge.
     */
    bool lastApplySuccess;

    // Deferred work flags
    bool pendingStatusPublish;
    bool pendingConfigApply;

protected:
    /**
     * @brief The constructor is protected because the class is a singleton
     * 
     * Use Cloud::instance() to instantiate the singleton.
     */
    Cloud();

    /**
     * @brief The destructor is protected because the class is a singleton and cannot be deleted
     */
    virtual ~Cloud();

    /**
     * This class is a singleton and cannot be copied
     */
    Cloud(const Cloud&) = delete;

    /**
     * This class is a singleton and cannot be copied
     */
    Cloud& operator=(const Cloud&) = delete;

    /**
     * @brief Singleton instance of this class
     * 
     * The object pointer to this class is stored here. It's NULL at system boot.
     */
    static Cloud *_instance;

};
#endif  /* __CLOUD_H */

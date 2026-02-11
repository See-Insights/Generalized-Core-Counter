#include "cloud/Cloud.h"

void Cloud::mergeConfiguration() {
    // Get data from both ledgers
    LedgerData defaults = defaultSettingsLedger.get();
    LedgerData device = deviceSettingsLedger.get();
    
    // Start with defaults as base
    mergedConfig = defaults;
    
    // Manually merge sensor configuration - copy ALL sensor keys then override with device settings
    //
    // Supported keys: type, setting1, setting2, setting3, setting4, threshold1, threshold2
    //   plus legacy sensorThreshold (applies to both thresholds)
    {
        VariantMap mergedSensor;

        // Start with defaults sensor config - copy ALL keys
        if (defaults.has("sensor") && defaults.get("sensor").isMap()) {
            Variant defaultSensor = defaults.get("sensor");
            if (defaultSensor.has("type")) mergedSensor["type"] = defaultSensor.get("type");
            if (defaultSensor.has("setting1")) mergedSensor["setting1"] = defaultSensor.get("setting1");
            if (defaultSensor.has("setting2")) mergedSensor["setting2"] = defaultSensor.get("setting2");
            if (defaultSensor.has("setting3")) mergedSensor["setting3"] = defaultSensor.get("setting3");
            if (defaultSensor.has("setting4")) mergedSensor["setting4"] = defaultSensor.get("setting4");
            if (defaultSensor.has("threshold1")) mergedSensor["threshold1"] = defaultSensor.get("threshold1");
            if (defaultSensor.has("threshold2")) mergedSensor["threshold2"] = defaultSensor.get("threshold2");
        } else {
            // Fallback defaults if no sensor section in ledger
            mergedSensor["threshold1"] = Variant(60);
            mergedSensor["threshold2"] = Variant(60);
        }

        // Allow a single generic default threshold that applies to both channels
        if (defaults.has("sensorThreshold")) {
            int base = defaults.get("sensorThreshold").toInt();
            mergedSensor["threshold1"] = Variant(base);
            mergedSensor["threshold2"] = Variant(base);
        }

        // Override with device-specific sensor config - copy ALL keys
        if (device.has("sensor") && device.get("sensor").isMap()) {
            Variant deviceSensor = device.get("sensor");
            if (deviceSensor.has("type")) mergedSensor["type"] = deviceSensor.get("type");
            if (deviceSensor.has("setting1")) mergedSensor["setting1"] = deviceSensor.get("setting1");
            if (deviceSensor.has("setting2")) mergedSensor["setting2"] = deviceSensor.get("setting2");
            if (deviceSensor.has("setting3")) mergedSensor["setting3"] = deviceSensor.get("setting3");
            if (deviceSensor.has("setting4")) mergedSensor["setting4"] = deviceSensor.get("setting4");
            if (deviceSensor.has("threshold1")) mergedSensor["threshold1"] = deviceSensor.get("threshold1");
            if (deviceSensor.has("threshold2")) mergedSensor["threshold2"] = deviceSensor.get("threshold2");
        }

        if (device.has("sensorThreshold")) {
            int override = device.get("sensorThreshold").toInt();
            mergedSensor["threshold1"] = Variant(override);
            mergedSensor["threshold2"] = Variant(override);
        }

        mergedConfig.set("sensor", Variant(mergedSensor));
        
        // Log merged sensor config for debugging
        int type = mergedSensor.has("type") ? mergedSensor["type"].toInt() : -1;
        int setting1 = mergedSensor.has("setting1") ? mergedSensor["setting1"].toInt() : -1;
        Log.info("Merged sensor config: type=%d, setting1=%d", type, setting1);
    }
    
    // Manually merge webhook configuration - copy ALL webhook keys then override with device settings
    {
        VariantMap mergedReporting;
        VariantMap mergedWebhook;

        // Start with defaults webhook config - copy ALL keys
        if (defaults.has("reporting") && defaults.get("reporting").isMap()) {
            Variant defaultReporting = defaults.get("reporting");
            if (defaultReporting.has("webhook") && defaultReporting.get("webhook").isMap()) {
                Variant defaultWebhook = defaultReporting.get("webhook");
                if (defaultWebhook.has("name")) mergedWebhook["name"] = defaultWebhook.get("name");
                if (defaultWebhook.has("enabled")) mergedWebhook["enabled"] = defaultWebhook.get("enabled");
                if (defaultWebhook.has("timeoutMs")) mergedWebhook["timeoutMs"] = defaultWebhook.get("timeoutMs");
            }
        }

        // Override with device-specific webhook config - copy ALL keys
        if (device.has("reporting") && device.get("reporting").isMap()) {
            Variant deviceReporting = device.get("reporting");
            if (deviceReporting.has("webhook") && deviceReporting.get("webhook").isMap()) {
                Variant deviceWebhook = deviceReporting.get("webhook");
                if (deviceWebhook.has("name")) mergedWebhook["name"] = deviceWebhook.get("name");
                if (deviceWebhook.has("enabled")) mergedWebhook["enabled"] = deviceWebhook.get("enabled");
                if (deviceWebhook.has("timeoutMs")) mergedWebhook["timeoutMs"] = deviceWebhook.get("timeoutMs");
            }
        }

        if (!mergedWebhook.isEmpty()) {
            mergedReporting["webhook"] = Variant(mergedWebhook);
            mergedConfig.set("reporting", Variant(mergedReporting));
            
            // Log merged webhook config for debugging
            String name = mergedWebhook.has("name") ? mergedWebhook["name"].toString() : "none";
            bool enabled = mergedWebhook.has("enabled") ? mergedWebhook["enabled"].toBool() : false;
            Log.info("Merged webhook config: name=%s, enabled=%d", name.c_str(), enabled);
        }
    }
    
    // Apply other top-level device overrides (these aren't nested objects)
    if (device.has("timing")) mergedConfig.set("timing", device.get("timing"));
    if (device.has("power")) mergedConfig.set("power", device.get("power"));
    if (device.has("messaging")) mergedConfig.set("messaging", device.get("messaging"));
    if (device.has("modes")) mergedConfig.set("modes", device.get("modes"));
    
    lastApplySuccess = applyConfigurationFromLedger();

    if (!lastApplySuccess) {
        Log.warn("Configuration apply failed");
    }
}

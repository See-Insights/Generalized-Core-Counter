#pragma once

// Project-wide configuration hooks for product-specific settings
// (webhook names, defaults, etc.) so that different products can
// share the same core firmware with minimal changes.
namespace ProjectConfig {

// Webhook event name used for hourly data publishes. Change this in
// one place when reusing the firmware in a new product.
static inline const char *webhookEventName() {
    return "Ubidots-Counter-Hook-v1";
}

} // namespace ProjectConfig

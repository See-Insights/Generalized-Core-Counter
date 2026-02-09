#pragma once

// Project-wide configuration hooks for product-specific settings
// (webhook names, defaults, etc.) so that different products can
// share the same core firmware with minimal changes.
//
// NOTE: Webhook event names are now configured via cloud (default-settings JSON)
// with convention-based fallback. This static function is kept for reference but
// is no longer actively used. See Cloud::getWebhookName() for the runtime resolution.
namespace ProjectConfig {

// Legacy webhook event name - now superseded by Cloud::getWebhookName()
// which provides 3-tier resolution: cloud config → convention → legacy
static inline const char *webhookEventName() {
    return "Ubidots-Counter-Hook-v1";
}

} // namespace ProjectConfig

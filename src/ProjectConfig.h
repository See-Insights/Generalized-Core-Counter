/**
 * @file ProjectConfig.h
 * @brief Project-wide configuration hooks for product-specific settings.
 *
 * @details
 * Provides legacy configuration helpers (for example, a webhook name) so
 * multiple products can share the same core firmware with minimal changes.
 * Cloud configuration now supersedes most of these values, but this header
 * remains for compatibility and documentation. See Settings.h for the
 * centralized catalog of project and build settings.
 */
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
/**
 * @brief Return the legacy fallback webhook event name for this product family.
 *
 * @return Stable C string used only when the runtime cloud configuration does
 *         not provide a webhook name and no convention-based match is available.
 */
static inline const char *webhookEventName() {
    return "Ubidots-Counter-Hook-v1";
}

} // namespace ProjectConfig

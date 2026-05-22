/**
 * @file Config.h
 * @brief Backward-compatible wrapper for shared project settings.
 *
 * @details
 * New code should include Settings.h directly when it needs shared build or
 * project settings. This wrapper remains so existing includes do not need to
 * change all at once.
 *
 * Safety note:
 * - CONNECTIVITY_FAILSAFE_TEST_MODE is a compile-time build-profile flag.
 * - It must not be sourced from cloud config and should not be toggled in
 *   this wrapper header.
 */

#ifndef CONFIG_H
#define CONFIG_H

#include "Settings.h"

#endif /* CONFIG_H */

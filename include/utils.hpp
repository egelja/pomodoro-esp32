#pragma once

#include <Arduino.h>

// espressif/arduino-esp32 - examples/ResetReason/ResetReason.ino
#ifdef ESP_IDF_VERSION_MAJOR  // IDF 4+
#  if CONFIG_IDF_TARGET_ESP32 // ESP32/PICO-D4
#    include "esp32/rom/rtc.h"
#  elif CONFIG_IDF_TARGET_ESP32S2
#    include "esp32s2/rom/rtc.h"
#  elif CONFIG_IDF_TARGET_ESP32C3
#    include "esp32c3/rom/rtc.h"
#  elif CONFIG_IDF_TARGET_ESP32S3
#    include "esp32s3/rom/rtc.h"
#  else
#    error Target CONFIG_IDF_TARGET is not supported
#  endif
#else // ESP32 Before IDF 4.0
#  include "rom/rtc.h"
#endif

/**
 * Get the current time as an ISO8601 string.
 */
inline String
iso8601_str() noexcept
{
    constexpr size_t ISO8601_LEN = 26; // 2022-12-31T20:06:38-0600

    struct tm tm;
    if (!getLocalTime(&tm))
        return "";

    char buf[ISO8601_LEN];
    strftime(buf, ISO8601_LEN, "%FT%T%z", &tm);

    return buf;
}

/**
 * Get the reason for our last "reset".
 *
 * @param core The core to get the reset reason for.
 *
 * @returns A string describing the reset reason.
 */
const char* get_reset_reason(int core) noexcept;

/**
 * Print debug info about our chip.
 */
void print_chip_debug_info() noexcept;

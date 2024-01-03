#include "utils.hpp"

#include "config.h"

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

const char*
get_reset_reason(int core) noexcept
{
    RESET_REASON reason = rtc_get_reset_reason(core);
    switch (reason) {
        case POWERON_RESET:
            return "Vbat power on reset (POWERON_RESET)";
        case SW_RESET:
            return "Software reset digital core (SW_RESET)";
        case OWDT_RESET:
            return "Legacy watch dog reset digital core (OWDT_RESET)";
        case DEEPSLEEP_RESET:
            return "Deep Sleep reset digital core (DEEPSLEEP_RESET)";
        case SDIO_RESET:
            return "Reset by SLC module, reset digital core (SDIO_RESET)";
        case TG0WDT_SYS_RESET:
            return "Timer Group0 Watch dog reset digital core (TG0WDT_SYS_RESET)";
        case TG1WDT_SYS_RESET:
            return "Timer Group1 Watch dog reset digital core (TG1WDT_SYS_RESET)";
        case RTCWDT_SYS_RESET:
            return "RTC Watch dog Reset digital core (RTCWDT_SYS_RESET)";
        case INTRUSION_RESET:
            return "Instrusion tested to reset CPU (INTRUSION_RESET)";
        case TGWDT_CPU_RESET:
            return "Time Group reset CPU (TGWDT_CPU_RESET)";
        case SW_CPU_RESET:
            return "Software reset CPU (SW_CPU_RESET)";
        case RTCWDT_CPU_RESET:
            return "RTC Watch dog Reset CPU (RTCWDT_CPU_RESET)";
        case EXT_CPU_RESET:
            return "for APP CPU, reset by PRO CPU (EXT_CPU_RESET)";
        case RTCWDT_BROWN_OUT_RESET:
            return "Reset when the vdd voltage is not stable (RTCWDT_BROWN_OUT_RESET)";
        case RTCWDT_RTC_RESET:
            return "RTC Watch dog reset digital core and rtc module (RTCWDT_RTC_RESET)";

        case NO_MEAN:
        default:
            return "No meaning for reset reason (NO_MEAN)";
    }
}

void
print_chip_debug_info() noexcept
{
    // Chip info
    log_d("%s v%d", ESP.getChipModel(), ESP.getChipRevision());
    log_d("%u x CPU @ %lu MHz", ESP.getChipCores(), ESP.getCpuFreqMHz());
    log_d(
        "%.2f MB flash @ %lu MHz, mode: %#2x",
        ESP.getFlashChipSize() / 1024.0 / 1024.0,
        ESP.getFlashChipSpeed() / 1000 / 1000,
        ESP.getFlashChipMode()
    );
    log_d("Chip ID: %llX", ESP.getEfuseMac());

    // Sketch info
    log_d("Sketch MD5: %s", ESP.getSketchMD5().c_str());
    log_d(
        "Used sketch space: %lu B/%lu B",
        ESP.getSketchSize(),
        ESP.getFreeSketchSpace() + ESP.getSketchSize()
    );

    // Memory info
    log_d(
        "Used heap: %lu B/%lu B",
        ESP.getHeapSize() - ESP.getFreeHeap(),
        ESP.getHeapSize()
    );
    log_d(
        "Used PSRAM: %lu B/%lu B",
        ESP.getPsramSize() - ESP.getFreePsram(),
        ESP.getPsramSize()
    );
    // Library versions
    log_d("ESP-IDF %s", ESP.getSdkVersion());
    log_d(
        "Arduino v%u.%u.%u",
        ESP_ARDUINO_VERSION_MAJOR,
        ESP_ARDUINO_VERSION_MAJOR,
        ESP_ARDUINO_VERSION_PATCH
    );

    // Reset reasons
    log_d("Core 0 reset reason: %s", get_reset_reason(0));
    if (ESP.getChipCores() >= 2)
        log_d("Core 1 reset reason: %s", get_reset_reason(1));
}

void
print_centered(const char* text, uint16_t cursor_y, MatrixPanel_I2S_DMA* display)
{
    // Find text size
    int16_t x, y;
    uint16_t w, h;

    display->getTextBounds(text, 0, 0, &x, &y, &w, &h);

    log_d("Text starts at (%d, %d) with width %u and height %u", x, y, w, h);

    // Get cursor position
    assert(x == 0 && y == 0);

    uint16_t cursor_x = (MAT_RES_X - w) / 2;
    log_d("Drawing at (%u, %u)", cursor_x, cursor_y);

    // Print text
    display->setCursor(cursor_x, cursor_y);
    display->print(text);
}

#include "utils.hpp"

#include <Arduino.h>

void
print_chip_debug_info()
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
setup()
{
    // Initialize serial communication
    Serial.begin(115200);
    Serial.setDebugOutput(true);

    while (!Serial) // Wait for init
        ;

    // Log information
    print_chip_debug_info();

    Serial.print("Hello world\n");
}

void
loop()
{
    while (Serial.available())
        Serial.write(Serial.read());
}

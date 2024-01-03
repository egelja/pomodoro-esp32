// This needs to go first because ezTime is stupid sometimes
#include <Arduino.h>

// Other includes
#include "clock.hpp"
#include "config.h"
#include "connections.hpp"
#include "utils.hpp"

#include <WiFi.h>

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

/*
 * Two patches must be made to the ezTime library.
 *
 * 1. Move `#ifdef __cplusplus` under the "warranty void" line in ezTime.h
 * 2. Add `using namespace ezt;` to ezTime.cpp
 */

#include <ezTime.h>

#include <cstdint>

enum display_mode_t : int8_t {
    DISP_MODE_NONE = -1,
    //
    DISP_MODE_CLOCK = 0,
    DISP_MODE_POMODORO,
    //
    DISP_MODE_LAST,
};

namespace {

// Connection variables
bool should_reconnect_wifi = false;
bool should_reconnect_mqtt = false;

// Timezone info
bool timezones_need_refresh = true; // refresh on boot
Timezone local_tz;

// Display settings
display_mode_t display_mode = DISP_MODE_NONE;
uint8_t display_brightness = 127;

uint8_t display_color[3] = {0xff, 0xff, 0xff}; // r, g, b
uint16_t display_color_565 = 0xffff;

} // namespace

/*****************************************************************************/

namespace {

constexpr size_t MQTT_TOPIC_LEN = sizeof("display/") - 1;

void
on_mqtt_connect(bool session_present)
{
    auto id = mqtt::subscribe("display/#", 1); // all display notifications
    if (!id) {
        log_e("Error subscribing to MQTT topic");
        ESP.restart();
    }
}

void
on_mqtt_message(
    String* topic,
    uint8_t* payload,
    size_t length,
    AsyncMqttClientMessageProperties props
)
{
    String payload_str(payload, length);
    log_d("Payload: \"%s\"", payload_str.c_str());

    // Get subtopic
    assert(topic->length() > MQTT_TOPIC_LEN);

    String subtopic = topic->substring(MQTT_TOPIC_LEN);
    log_i("Subtopic: \"%s\"", subtopic.c_str());

    // Decide what to do
    switch (subtopic[0]) {
        case 'm':
            {
                assert(subtopic == "mode");

                long val = std::strtol(payload_str.c_str(), NULL, 10);
                if (val <= DISP_MODE_NONE || val >= DISP_MODE_LAST) {
                    log_e("Invalid display mode %d", val);
                    return;
                }

                display_mode = static_cast<display_mode_t>(val);
                log_i("Updated display mode to %d", val);

                break;
            }

        case 'c':
            {
                assert(subtopic == "color");

                uint32_t color = std::strtoul(payload_str.c_str(), NULL, 16);
                if (color > 0xffffff) {
                    log_e("Invalid display color %#lx", color);
                    return;
                }

                display_color[0] = (color >> 16) & 0xff; // red
                display_color[1] = (color >> 8) & 0xff;  // green
                display_color[2] = color & 0xff;         // blue

                display_color_565 = MatrixPanel_I2S_DMA::color565(
                    display_color[0], display_color[1], display_color[2]
                );

                log_i("Updated display color to %#lx", color);
                break;
            }

        case 'b':
            {
                assert(subtopic == "brightness");

                long val = std::strtol(payload_str.c_str(), NULL, 10);
                if (val < 0 || val > UINT8_MAX) {
                    log_e("Invalid display brightness %ld", val);
                    return;
                }

                display_brightness = val;
                log_i("Updated display brightness to %u", display_brightness);

                break;
            }

        default:
            break;
    }
}

} // namespace

/*****************************************************************************/

namespace {

MatrixPanel_I2S_DMA* display = nullptr;

void
setup_led_matrix()
{
    // Set up pins
    HUB75_I2S_CFG::i2s_pins pins = {
        MAT_PIN_R1,
        MAT_PIN_G1,
        MAT_PIN_B1,
        MAT_PIN_R2,
        MAT_PIN_G2,
        MAT_PIN_B2,
        MAT_PIN_A,
        MAT_PIN_B,
        MAT_PIN_C,
        MAT_PIN_D,
        MAT_PIN_E,
        MAT_PIN_LAT,
        MAT_PIN_OE,
        MAT_PIN_CLK,
    };

    // Set up matrix config
    HUB75_I2S_CFG config(MAT_RES_X, MAT_RES_Y, MAT_CHAIN, pins);
    config.clkphase = false;
    config.i2sspeed = HUB75_I2S_CFG::HZ_20M;

#ifdef MAT_DOUBLE_BUFF
    config.double_buff = true;
#endif

    // Create display
    display = new MatrixPanel_I2S_DMA(config);
    display->begin();
    display->setBrightness8(display_brightness); // 0 - 255
    display->clearScreen();

    // Enable bugfix
    display->cp437(true);
}

} // namespace

/*****************************************************************************/

void
setup()
{
    // Initialize serial communication
    Serial.begin(115200);
    Serial.setDebugOutput(true);

    while (!Serial) // Wait until ready
        ;

    // Log information
    print_chip_debug_info();

    // Set MQTT callbacks
    mqtt::set_connect_cb(on_mqtt_connect);
    mqtt::set_message_cb(on_mqtt_message);

    // Setup LED matrix
    setup_led_matrix();

    // Test matrix
    display->clearScreen();
    for (size_t x = 0; x < MAT_RES_X; ++x) {
        for (size_t y = 0; y < MAT_RES_Y; ++y) {
            display->drawPixel(x, y, display->color565(x << 2, y << 3, 0));
        }
    }

    display->flipDMABuffer();

    // Start connections
    connections::begin(&should_reconnect_wifi, &should_reconnect_mqtt);
}

void
loop()
{
    // Run callbacks
    if (should_reconnect_wifi) {
        wifi::connect();
        timezones_need_refresh = true;
    }

    if (should_reconnect_mqtt)
        mqtt::connect();

    // Process commands
    while (Serial.available()) {
        switch (Serial.read()) {
            case 'w':
                wifi::print_status();
                break;

            case 'm':
                mqtt::print_status();
                break;

            case 's':
                print_chip_debug_info();
                break;

            case 'r':
                ESP.restart();
                __builtin_unreachable();

            case 'u':
                local_tz.setLocation("Europe/Belgrade");
                break;

            case 'c':
                local_tz.setLocation(TIME_TIMEZONE);
                break;

            default:
                break;
        }
    }

    // Only do the rest if we have WiFi
    if (WiFi.status() != WL_CONNECTED)
        return;

    // Refresh timezones
    if (timezones_need_refresh) {
        log_i("Refreshing timezones");
        local_tz.setLocation(TIME_TIMEZONE);

        log_i("Timezone refresh completed");
        timezones_need_refresh = false;
    }

    // Run ezt events
    ezt::events();

    // Update display
    if (ezt::secondChanged()) {
        log_i("Updating display");

        // Update settings
        display->setTextColor(display_color_565);
        display->setBrightness8(display_brightness);

        // Update text
        switch (display_mode) {
            case DISP_MODE_NONE:
                break;

            case DISP_MODE_CLOCK:
                matrix_clock::draw(display, &local_tz);
                break;

            case DISP_MODE_POMODORO:
                local_tz.dateTime(); // dummy

                // TODO
                display->clearScreen();
                print_centered("POMODORO", 12, display);

                break;

            default:
                log_e("Invalid display mode");
                abort();
        }

#ifdef MAT_DOUBLE_BUFF
        // Show the updates
        display->flipDMABuffer();
#endif
    }
}

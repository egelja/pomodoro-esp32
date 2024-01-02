#include "config.h"
#include "connections.hpp"
#include "utils.hpp"

#include <Arduino.h>
#include <WiFi.h>

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

/*
 * Two patches must be made to the ezTime library.
 *
 * 1. Move `#ifdef __cplusplus` under the "warranty void" line in ezTime.h
 * 2. Add `using namespace ezt;` to ezTime.cpp
 */
#include <ezTime.h>

namespace {

bool should_reconnect_wifi = false;
bool should_reconnect_mqtt = false;

bool timezones_need_refresh = true; // refresh on boot
Timezone local_tz;

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
    // config.driver = HUB75_I2S_CFG::ICN2038S;
    config.clkphase = false;
    config.i2sspeed = HUB75_I2S_CFG::HZ_20M;

    // Create display
    display = new MatrixPanel_I2S_DMA(config);
    display->begin();
    display->setBrightness8(127); // 0 - 255
    display->clearScreen();
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
            // delay(10);
        }
    }

    // Start connections
    connections::begin(&should_reconnect_wifi, &should_reconnect_mqtt);
}

void
loop()
{
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

        timezones_need_refresh = false;
    }

    // Run ezt events
    log_i("Running ezTime events");
    ezt::events();
    log_i("Finished ezTime events");

    // Print current time
    log_i("Getting date time");
    String time = local_tz.dateTime();
    log_i("Finished getting time");

    Serial.println("Chicago Time: " + time);
}

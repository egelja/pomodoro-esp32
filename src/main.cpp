#include "connections.hpp"
#include "utils.hpp"

#include <Arduino.h>

bool should_reconnect_wifi = false;
bool should_reconnect_mqtt = false;

constexpr static size_t MQTT_TOPIC_LEN = sizeof("display/") - 1;

static void
on_mqtt_connect(bool session_present)
{
    auto id = mqtt::subscribe("display/#", 1); // all display notifications
    if (!id) {
        log_e("Error subscribing to MQTT topic");
        ESP.restart();
    }
}

static void
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

    // Start connections
    connections::begin(&should_reconnect_wifi, &should_reconnect_mqtt);
}

void
loop()
{
    if (should_reconnect_wifi)
        wifi::connect();

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
}

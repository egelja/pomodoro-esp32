#pragma once

#include <Arduino.h>
#include <AsyncMqttClient.hpp>

/**
 * WiFi connection related functions.
 */
namespace wifi {

/**
 * Attempt to connect to WiFi.
 */
void connect() noexcept;

/**
 * Print stats about the connected WiFi network.
 */
void print_status() noexcept;

} // namespace wifi

/*****************************************************************************/

/**
 * MQTT connection related functions.
 */
namespace mqtt {

typedef void (*on_connect_cb)(bool);
typedef void (*on_message_cb)(
    String*, uint8_t*, size_t, AsyncMqttClientMessageProperties
);

/**
 * Set the connect callback.
 *
 * Subscriptions are done here.
 */
void set_connect_cb(on_connect_cb cb);

/**
 * Set the message callback.
 */
void set_message_cb(on_message_cb cb);

/**
 * Attempt to connect to MQTT.
 */
void connect() noexcept;

/**
 * Print stats about the connection to MQTT.
 */
void print_status() noexcept;

/**
 * Subscribe to a MQTT topic.
 *
 * @returns the subscribe packet ID, or 0 on error.
 */
[[nodiscard]] uint16_t subscribe(const char* topic, uint8_t qos);

/**
 * Unsubscribe from an MQTT topic.
 *
 * @returns the unsubscribe packet ID, or 0 on error.
 */
[[nodiscard]] uint16_t unsubscribe(const char* topic);

/**
 * Publish a message to the MQTT broker.
 *
 * @returns the publish packet ID, or 0 on error.
 */
[[nodiscard]] uint16_t publish(
    const char* topic,
    uint8_t qos,
    bool retain,
    const char* payload = nullptr,
    size_t length = 0,
    bool dup = false,
    uint16_t message_id = 0
);

} // namespace mqtt

/*****************************************************************************/

namespace connections {

/**
 * Start our connections.
 *
 * Takes two flags that are set when a reconnection needs to happen.
 */
void begin(bool* rec_wifi, bool* rec_mqtt);

} // namespace connections

#pragma once

namespace connections {

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

/**
 * Attempt to connect to MQTT.
 */
void connect() noexcept;

/**
 * Print stats about the connection to MQTT.
 */
void print_status() noexcept;

} // namespace mqtt

/*****************************************************************************/

/**
 * Start our connections.
 *
 * Takes two flags that are set when a reconnection needs to happen.
 */
void begin(bool* rec_wifi, bool* rec_mqtt);

} // namespace connections

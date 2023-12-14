#include "connections.hpp"

#include "AsyncMqttClient/DisconnectReasons.hpp"
#include "AsyncMqttClient/MessageProperties.hpp"
#include "config.h"
#include "IPAddress.h"
#include "sys/_stdint.h"

#include <Arduino.h>
#include <AsyncMqttClient.h>
#include <WiFi.h>

constexpr static int RECONNECT_DELAY_MS = 10 * 1000; // 10 seconds

static bool* should_reconnect_wifi;
static bool* should_reconnect_mqtt;

static TimerHandle_t wifi_reconnect_timer;
static TimerHandle_t mqtt_reconnect_timer;

namespace wifi {

static void
on_event(arduino_event_id_t event)
{
    log_i("[WiFi-event] event: %d", event);

    switch (event) {
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            log_i("WiFi Connected");
            print_status();

            xTimerStop(wifi_reconnect_timer, 0);
            *should_reconnect_wifi = false;

            xTimerStart(mqtt_reconnect_timer, 0);
            break;

        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            log_w("WiFi lost connection");

            xTimerStop(mqtt_reconnect_timer, 0);  // can't connect to MQTT w/o WiFi
            xTimerStart(wifi_reconnect_timer, 0); // reconnect to WiFi

            break;

        default: // don't care
            break;
    }
}

static void
setup()
{
    WiFi.onEvent(wifi::on_event);
    WiFi.setAutoReconnect(false);
    WiFi.setAutoConnect(false);
}

void
connect() noexcept
{
    *should_reconnect_wifi = false;
    log_i("Connecting to WiFi...");

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PSK);
}

void
print_status() noexcept
{
    const char* modes[] = {"NULL", "STA", "AP", "STA+AP"};

    log_d("Wifi mode: %s", modes[WiFi.getMode()]);

    const char* status;
    switch (WiFi.status()) {
        case WL_IDLE_STATUS:
            status = "IDLE";
            break;
        case WL_NO_SSID_AVAIL:
            status = "NO SSID AVAILABLE";
            break;
        case WL_SCAN_COMPLETED:
            status = "SCAN COMPLETED";
            break;
        case WL_CONNECTED:
            status = "CONNECTED";
            break;
        case WL_CONNECT_FAILED:
            status = "CONNECTION FAILED";
            break;
        case WL_CONNECTION_LOST:
            status = "CONNECTION LOST";
            break;
        case WL_DISCONNECTED:
            status = "DISCONNECTED";
            break;
        default:
            status = "UNKNOWN";
            break;
    }
    log_d("Status: %s", status);

    log_d("Mac address: %s", WiFi.macAddress().c_str());

    String ssid = WiFi.SSID();
    log_d("SSID (%lu): %s", ssid.length(), ssid.c_str());

    String pass = WiFi.psk();
    log_d("Passphrase (%lu): %s", pass.length(), pass.c_str());

    log_d("Channel: %u (primary)", WiFi.channel());
    log_d("BSSID: %s", WiFi.BSSIDstr().c_str());
    log_d("Strength: %d dB", WiFi.RSSI());

    log_d("IPv4: %s", WiFi.localIP().toString().c_str());
    log_d("IPv6: %s", WiFi.localIPv6().toString().c_str());
    log_d("Gateway: %s", WiFi.gatewayIP().toString().c_str());
    log_d("Subnet Mask: %s", WiFi.subnetMask().toString().c_str());
    log_d("DNS: %s", WiFi.dnsIP().toString().c_str());

    log_d("Broadcast IP: %s", WiFi.broadcastIP().toString().c_str());
    log_d("Network ID: %s", WiFi.networkID().toString().c_str());
}

} // namespace wifi

/*****************************************************************************/

namespace mqtt {

static const IPAddress MQTT_IP(MQTT_HOST);

static AsyncMqttClient mqtt_client;

static on_connect_cb user_connect_cb{};
static on_message_cb user_message_cb{};

static_assert(
    sizeof(AsyncMqttClientMessageProperties) <= sizeof(void*),
    "MQTT msg properties can fit in a register"
);

/*      CALLBACKS      */

static void
on_connect(bool session_present)
{
    log_i("MQTT Connected, session: %s", session_present ? "YES" : "NO");
    print_status();

    if (user_connect_cb)
        user_connect_cb(session_present);
}

static void
on_disconnect(AsyncMqttClientDisconnectReason reason)
{
    log_w("MQTT Disconnected, reason: %d");

    if (WiFi.isConnected())
        xTimerStart(mqtt_reconnect_timer, 0);
}

static void
on_subscribe(uint16_t packet_id, uint8_t qos)
{
    log_i("New MQTT Subscription with ID %d at QOS Level %d", packet_id, qos);
}

static void
on_unsubscribe(uint16_t packet_id)
{
    log_i("MQTT unsubscribed with ID %d", packet_id);
}

static void
on_publish(uint16_t packet_id)
{
    log_i("MQTT publish with ID %d", packet_id);
}

static void
on_message(
    char* topic,
    char* payload,
    AsyncMqttClientMessageProperties props,
    size_t len,
    size_t idx,
    size_t total
)
{
    log_i(
        "MQTT message received from topic \"%s\" (%d/%d bytes)", topic, idx + len, total
    );
    log_d("Length: %zu, Index: %zu, Total: %zu", len, idx, total);
    log_d("Qos: %d, Dup: %d, Retain: %d", props.qos, props.dup, props.retain);

    // TODO: support calling this function multiple times
    if (idx != 0) {
        log_e("Long messages not supported");
        ESP.restart();
    }

    assert(len == total);

    // Call user callback
    if (user_message_cb) {
        String topic_str(topic);
        user_message_cb(&topic_str, reinterpret_cast<uint8_t*>(payload), len, props);
    }
}

static void
setup()
{
    // Setup server
    mqtt_client.setServer(MQTT_IP, MQTT_PORT);

    // Set callbacks
    mqtt_client.onConnect(on_connect);
    mqtt_client.onDisconnect(on_disconnect);

    mqtt_client.onSubscribe(on_subscribe);
    mqtt_client.onUnsubscribe(on_unsubscribe);

    mqtt_client.onPublish(on_publish);
    mqtt_client.onMessage(on_message);
}

/*      PUBLIC FUNCTIONS      */

void
set_connect_cb(on_connect_cb cb)
{
    user_connect_cb = cb;
}

void
set_message_cb(on_message_cb cb)
{
    user_message_cb = cb;
}

void
connect() noexcept
{
    log_i("Connecting to MQTT");
    *should_reconnect_mqtt = false;

    mqtt_client.connect();
}

void
print_status() noexcept
{
    log_i(
        "Connected: %s, Client ID: %s",
        mqtt_client.connected() ? "YES" : "NO",
        mqtt_client.getClientId()
    );
}

uint16_t
subscribe(const char* topic, uint8_t qos)
{
    return mqtt_client.subscribe(topic, qos);
}

uint16_t
unsubscribe(const char* topic)
{
    return mqtt_client.unsubscribe(topic);
}

uint16_t
publish(
    const char* topic,
    uint8_t qos,
    bool retain,
    const char* payload,
    size_t length,
    bool dup,
    uint16_t message_id
)
{
    return mqtt_client.publish(topic, qos, retain, payload, length, dup, message_id);
}

} // namespace mqtt

/*****************************************************************************/

namespace connections {

static void
wifi_timer_cb(TimerHandle_t handle) noexcept
{
    (void)handle;
    *should_reconnect_wifi = true;
}

static void
mqtt_timer_cb(TimerHandle_t handle) noexcept
{
    (void)handle;
    *should_reconnect_mqtt = true;
}

void
begin(bool* rec_wifi, bool* rec_mqtt)
{
    should_reconnect_wifi = rec_wifi;
    should_reconnect_mqtt = rec_mqtt;

    // Set up timers
    wifi_reconnect_timer = xTimerCreate(
        "wifi_timer",
        pdMS_TO_TICKS(RECONNECT_DELAY_MS),
        pdFALSE,
        (void*)0,
        wifi_timer_cb
    );

    mqtt_reconnect_timer = xTimerCreate(
        "mqtt_timer",
        pdMS_TO_TICKS(RECONNECT_DELAY_MS),
        pdFALSE,
        (void*)1,
        mqtt_timer_cb
    );

    // Setup MQTT
    mqtt::setup();

    // Connect to wifi
    wifi::setup();
    wifi::connect();
}

} // namespace connections

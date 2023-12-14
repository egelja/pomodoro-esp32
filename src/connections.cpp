#include "connections.hpp"

#include "config.h"

#include <Arduino.h>
#include <AsyncMqttClient.h>
#include <WiFi.h>

constexpr static int RECONNECT_DELAY_MS = 10 * 1000; // 10 seconds

static bool* should_reconnect_wifi;
static bool* should_reconnect_mqtt;

static TimerHandle_t wifi_reconnect_timer;
static TimerHandle_t mqtt_reconnect_timer;

namespace connections {

namespace wifi {

void
on_event(arduino_event_id_t event)
{
    log_i("[WiFi-event] event: %d", event);

    switch (event) {
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            log_i("WiFi Connected");
            print_status();

            xTimerStop(wifi_reconnect_timer, 0);

            mqtt::connect();
            break;

        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            log_i("WiFi lost connection");

            xTimerStop(mqtt_reconnect_timer, 0);  // can't connect to MQTT w/o WiFi
            xTimerStart(wifi_reconnect_timer, 0); // reconnect to WiFi

            break;

        default: // don't care
            break;
    }
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

void
connect() noexcept
{
    *should_reconnect_mqtt = false;
}

void
print_status() noexcept
{}

} // namespace mqtt

/*****************************************************************************/

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

    // Connect to wifi
    WiFi.onEvent(wifi::on_event);
    WiFi.setAutoReconnect(false);
    WiFi.setAutoConnect(false);
    wifi::connect();
}

} // namespace connections

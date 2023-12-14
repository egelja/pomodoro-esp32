#include "connections.hpp"
#include "utils.hpp"

#include <Arduino.h>

bool should_reconnect_wifi = false;
bool should_reconnect_mqtt = false;

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

    // Start connections
    connections::begin(&should_reconnect_wifi, &should_reconnect_mqtt);
}

void
loop()
{
    if (should_reconnect_wifi)
        connections::wifi::connect();

    if (should_reconnect_mqtt)
        connections::wifi::connect();
}

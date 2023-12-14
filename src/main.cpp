#include "utils.hpp"

#include <Arduino.h>

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

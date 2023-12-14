#include <Arduino.h>

void setup()
{
    // Initialize serial communication
    Serial.begin(115200);
    Serial.setDebugOutput(true);

    while (!Serial) // Wait for init
        ;

    Serial.print("Hello world\n");
}

void
loop()
{
    // Nothing...
}

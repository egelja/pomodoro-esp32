#include <Arduino.h>
#include <AsyncMqttClient.hpp>
#include <ezTime.h>

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

namespace pomodoro {

void on_mqtt_message(
    String& subtopic, String& payload, AsyncMqttClientMessageProperties props
);

void draw(MatrixPanel_I2S_DMA* display, Timezone* local_tz);

} // namespace pomodoro

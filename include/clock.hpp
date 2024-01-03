#pragma once

#include <Arduino.h>
#include <ezTime.h>

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

namespace matrix_clock {

void draw(MatrixPanel_I2S_DMA* display, Timezone* local_tz);

}

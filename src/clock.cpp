#include "clock.hpp"

#include "utils.hpp"

namespace matrix_clock {

void
draw(MatrixPanel_I2S_DMA* display, Timezone* local_tz)
{
    log_i("Drawing clock on display");

    // Get our time strings
    String day = local_tz->dateTime("l");
    String date = local_tz->dateTime("n/j/Y");
    String time = local_tz->dateTime("G:i:s");

    // Update display
    display->clearScreen();

    print_centered(day, 2, display);
    display->print("\n");
    print_centered(date, display->getCursorY() + 1, display);
    display->print("\n");
    print_centered(time, display->getCursorY() + 3, display);
}

} // namespace matrix_clock

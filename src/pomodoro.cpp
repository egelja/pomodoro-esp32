#include "pomodoro.hpp"

#include "connections.hpp"
#include "utils.hpp"

#include <cstdint>

enum pomo_mode_t : int8_t {
    POMO_MODE_NONE = -1,
    POMO_MODE_WORK,
    POMO_MODE_SHORT_BREAK,
    POMO_MODE_LONG_BREAK,
};

namespace {

// Config
uint8_t work_minutes = 25;
uint8_t short_break_minutes = 15;
uint8_t long_break_minutes = 5;

// Status
pomo_mode_t mode = POMO_MODE_NONE;
size_t time_remaining = work_minutes * 60;

uint8_t blinks_remaining = 0;
size_t num_pomodoros_completed = 0;

// Helpers
void
on_work_completed()
{
    ++num_pomodoros_completed;

    if (num_pomodoros_completed % 4 == 0) {
        // Long break every 4th pomodoro
        mode = POMO_MODE_LONG_BREAK;
        time_remaining = long_break_minutes * 60;
    }
    else {
        mode = POMO_MODE_SHORT_BREAK;
        time_remaining = short_break_minutes * 60;
    }
}

void
on_break_completed()
{
    mode = POMO_MODE_WORK;
    time_remaining = work_minutes * 60;
}

const char*
mode_string()
{
    switch (mode) {
        case POMO_MODE_WORK:
            return "WORK!";

        case POMO_MODE_LONG_BREAK:
        case POMO_MODE_SHORT_BREAK:
            return "BREAK!";

        default:
            log_e("Invalid pomodoro mode %u!", mode);
            abort();
    }
}

void
publish_count()
{
    String count_str;
    count_str += num_pomodoros_completed;

    auto id = mqtt::publish(
        "display/pomodoro/count", 1, true, count_str.c_str(), count_str.length()
    );

    if (!id)
        log_w("Error publishing count message to MQTT");
}

void
reset_state()
{
    mode = POMO_MODE_WORK;
    time_remaining = work_minutes * 60;

    num_pomodoros_completed = 0;
    publish_count();
}

} // namespace

namespace pomodoro {

void
on_mqtt_message(
    String& subtopic, String& payload, AsyncMqttClientMessageProperties props
)
{
    String pomo_topic = subtopic.substring(9);

    // Parse value
    long val = strtol(payload.c_str(), NULL, 10);
    if (val < 0 || val > UINT8_MAX) {
        log_e("Invalid value %ld for %s", val, pomo_topic.c_str());
        return;
    }

    // Decide what to do
    switch (pomo_topic[0]) {
        case 'w':
            assert(pomo_topic == "work");

            work_minutes = static_cast<uint8_t>(val);
            log_i("Set work minutes to %u", work_minutes);

            break;

        case 's':
            assert(pomo_topic == "short_break");

            short_break_minutes = static_cast<uint8_t>(val);
            log_i("Set short break minutes to %u", short_break_minutes);

            break;

        case 'l':
            assert(pomo_topic == "long_break");

            long_break_minutes = static_cast<uint8_t>(val);
            log_i("Set long break minutes to %u", long_break_minutes);

            break;

        case 'r':
            assert(pomo_topic == "reset");

            reset_state();
            break;

        default:
            log_w("Invalid pomodoro topic %s", pomo_topic.c_str());
            break;
    }
}

void
draw(MatrixPanel_I2S_DMA* display, Timezone* local_tz)

{
    // Handle cold boot condition
    if (mode == POMO_MODE_NONE)
        reset_state();

    // Poll time for eztime
    local_tz->tzTime();

    // Blink screen if needed
    if (blinks_remaining != 0) {
        display->clearScreen();
        display->setBrightness8(255);

        if (blinks_remaining % 2 == 1)
            display->fillScreenRGB888(0xff, 0xff, 0xff);

        --blinks_remaining;
        return;
    }

    // Called once per second
    --time_remaining;

    // Get time string
    int minutes = time_remaining / 60;
    int seconds = time_remaining % 60;

    String time;
    time += ezt::zeropad(minutes, 2);
    time += ":";
    time += ezt::zeropad(seconds, 2);

    // Show time
    display->clearScreen();

    print_centered(mode_string(), 6, display);
    display->print("\n");
    print_centered(time, display->getCursorY() + 5, display);

    // Update mode
    if (time_remaining == 0) {
        blinks_remaining = 5;

        if (mode == POMO_MODE_WORK) // Completed a pomodoro
            on_work_completed();
        else // finished our break
            on_break_completed();

        // Publish info to MQTT
        publish_count();
    }
}

} // namespace pomodoro

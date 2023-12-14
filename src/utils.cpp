#include "utils.hpp"

const char*
get_reset_reason(int core) noexcept
{
    RESET_REASON reason = rtc_get_reset_reason(core);
    switch (reason) {
        case POWERON_RESET:
            return "Vbat power on reset (POWERON_RESET)";
        case SW_RESET:
            return "Software reset digital core (SW_RESET)";
        case OWDT_RESET:
            return "Legacy watch dog reset digital core (OWDT_RESET)";
        case DEEPSLEEP_RESET:
            return "Deep Sleep reset digital core (DEEPSLEEP_RESET)";
        case SDIO_RESET:
            return "Reset by SLC module, reset digital core (SDIO_RESET)";
        case TG0WDT_SYS_RESET:
            return "Timer Group0 Watch dog reset digital core (TG0WDT_SYS_RESET)";
        case TG1WDT_SYS_RESET:
            return "Timer Group1 Watch dog reset digital core (TG1WDT_SYS_RESET)";
        case RTCWDT_SYS_RESET:
            return "RTC Watch dog Reset digital core (RTCWDT_SYS_RESET)";
        case INTRUSION_RESET:
            return "Instrusion tested to reset CPU (INTRUSION_RESET)";
        case TGWDT_CPU_RESET:
            return "Time Group reset CPU (TGWDT_CPU_RESET)";
        case SW_CPU_RESET:
            return "Software reset CPU (SW_CPU_RESET)";
        case RTCWDT_CPU_RESET:
            return "RTC Watch dog Reset CPU (RTCWDT_CPU_RESET)";
        case EXT_CPU_RESET:
            return "for APP CPU, reset by PRO CPU (EXT_CPU_RESET)";
        case RTCWDT_BROWN_OUT_RESET:
            return "Reset when the vdd voltage is not stable (RTCWDT_BROWN_OUT_RESET)";
        case RTCWDT_RTC_RESET:
            return "RTC Watch dog reset digital core and rtc module (RTCWDT_RTC_RESET)";

        case NO_MEAN:
        default:
            return "No meaning for reset reason (NO_MEAN)";
    }
}

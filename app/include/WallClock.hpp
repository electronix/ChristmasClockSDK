#pragma once

#include <cstdint>
#include <string>

namespace ChristmasClock {

// Time of day / date, set from outside (USB serial today; DCF77 or NTP could feed it too).
//
// There is no battery-backed RTC on the board, so the time is lost at power-off and has to be
// set again after every boot; between two Set() calls it is counted with the 1MHz system timer
// (quartz accuracy, a few seconds per day).
//
// The value is plain *local* civil time - whatever the sender considers "now" (the PC sends its
// local time including DST). No time-zone handling happens here.
class WallClock {
public:
    struct DateTime {
        int year, month, day, hour, minute, second;
    };

    bool IsValid() const { return _valid; }

    // Sets "now". Returns false (and leaves the clock untouched) if `t` is not a real date/time.
    bool Set(const DateTime& t);
    // Same, as seconds since 1970-01-01 00:00:00 civil time (no time zone).
    void SetSeconds(int64_t civil_seconds);
    // Current time; false if the clock has never been set.
    bool Now(DateTime& out) const;

    // "YYYY-MM-DD HH:MM:SS" (a 'T' instead of the space is accepted, too).
    static bool Parse(const char* text, DateTime& out);
    static std::string Format(const DateTime& t);

    static bool IsValidDateTime(const DateTime& t);
    static int64_t ToSeconds(const DateTime& t);
    static DateTime FromSeconds(int64_t civil_seconds);

private:
    bool _valid = false;
    int64_t _base_seconds = 0;  // civil seconds at the moment of the last Set()
    uint64_t _base_us = 0;      // time_us_64() at the moment of the last Set()
};

}  // namespace ChristmasClock

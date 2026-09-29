#include "WallClock.hpp"

#include <cstdio>

#include "pico/time.h"

namespace ChristmasClock {

namespace {
bool IsLeapYear(int y) {
    return (y % 4 == 0 && y % 100 != 0) || y % 400 == 0;
}

int DaysInMonth(int y, int m) {
    static const int days[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    return (m == 2 && IsLeapYear(y)) ? 29 : days[m - 1];
}

// Howard Hinnant's civil-date algorithms (public domain): days since 1970-01-01 <-> y/m/d.
int64_t DaysFromCivil(int64_t y, int m, int d) {
    y -= m <= 2;
    const int64_t era = (y >= 0 ? y : y - 399) / 400;
    const int64_t yoe = y - era * 400;
    const int64_t doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    const int64_t doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + doe - 719468;
}

void CivilFromDays(int64_t z, int& y, int& m, int& d) {
    z += 719468;
    const int64_t era = (z >= 0 ? z : z - 146096) / 146097;
    const int64_t doe = z - era * 146097;
    const int64_t yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    const int64_t yy = yoe + era * 400;
    const int64_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const int64_t mp = (5 * doy + 2) / 153;
    d = static_cast<int>(doy - (153 * mp + 2) / 5 + 1);
    m = static_cast<int>(mp < 10 ? mp + 3 : mp - 9);
    y = static_cast<int>(yy + (m <= 2));
}
}  // namespace

bool WallClock::IsValidDateTime(const DateTime& t) {
    if (t.year < 2020 || t.year > 2099) return false;
    if (t.month < 1 || t.month > 12) return false;
    if (t.day < 1 || t.day > DaysInMonth(t.year, t.month)) return false;
    if (t.hour < 0 || t.hour > 23) return false;
    if (t.minute < 0 || t.minute > 59) return false;
    if (t.second < 0 || t.second > 59) return false;
    return true;
}

int64_t WallClock::ToSeconds(const DateTime& t) {
    return DaysFromCivil(t.year, t.month, t.day) * 86400 + t.hour * 3600 + t.minute * 60 + t.second;
}

WallClock::DateTime WallClock::FromSeconds(int64_t s) {
    int64_t days = s / 86400;
    int64_t rem = s % 86400;
    if (rem < 0) {
        rem += 86400;
        days -= 1;
    }
    DateTime t;
    CivilFromDays(days, t.year, t.month, t.day);
    t.hour = static_cast<int>(rem / 3600);
    t.minute = static_cast<int>((rem % 3600) / 60);
    t.second = static_cast<int>(rem % 60);
    return t;
}

bool WallClock::Set(const DateTime& t) {
    if (!IsValidDateTime(t)) return false;
    SetSeconds(ToSeconds(t));
    return true;
}

void WallClock::SetSeconds(int64_t civil_seconds) {
    _base_seconds = civil_seconds;
    _base_us = time_us_64();
    _valid = true;
}

bool WallClock::Now(DateTime& out) const {
    if (!_valid) return false;
    const int64_t elapsed_s = static_cast<int64_t>((time_us_64() - _base_us) / 1000000ULL);
    out = FromSeconds(_base_seconds + elapsed_s);
    return true;
}

bool WallClock::Parse(const char* text, DateTime& out) {
    DateTime t;
    int n = std::sscanf(text, "%d-%d-%d%*[ T]%d:%d:%d", &t.year, &t.month, &t.day, &t.hour, &t.minute,
                        &t.second);
    if (n != 6 || !IsValidDateTime(t)) return false;
    out = t;
    return true;
}

std::string WallClock::Format(const DateTime& t) {
    char buf[24];
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d", t.year, t.month, t.day, t.hour,
                  t.minute, t.second);
    return buf;
}

}  // namespace ChristmasClock

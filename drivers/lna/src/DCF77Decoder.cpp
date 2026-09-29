#include "DCF77Decoder.hpp"

namespace ChristmasClock {

namespace {
// Pulse width classes in ms. The pin is sampled every ~10ms, so widths are quantized to +-10ms.
constexpr uint32_t kZeroMinMs = 50;
constexpr uint32_t kZeroMaxMs = 150;  // exclusive, 0 = ~100ms
constexpr uint32_t kOneMaxMs = 260;   // 1 = ~200ms

// Distance between two pulse starts.
constexpr uint32_t kSecondMinMs = 900;
constexpr uint32_t kSecondMaxMs = 1100;
constexpr uint32_t kMinuteMarkMinMs = 1750;  // second 58 -> minute mark is 2s
constexpr uint32_t kMinuteMarkMaxMs = 2250;

constexpr uint64_t Field(uint64_t bits, int lo, int n) {
    return (bits >> lo) & ((1ULL << n) - 1);
}

bool EvenParity(uint64_t bits, int lo, int hi_inclusive) {
    return (__builtin_popcountll(Field(bits, lo, hi_inclusive - lo + 1)) & 1) == 0;
}

// BCD field: `n_low` bits binary (must be <=9), followed by `n_high` bits of tens.
bool Bcd(uint64_t bits, int lo, int n_low, int n_high, int& out) {
    int low = static_cast<int>(Field(bits, lo, n_low));
    int high = n_high ? static_cast<int>(Field(bits, lo + n_low, n_high)) : 0;
    if (low > 9) return false;
    out = high * 10 + low;
    return true;
}

bool IsLeapYear(int year) {
    return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}

int DaysInMonth(int year, int month) {
    static const uint8_t days[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    return (month == 2 && IsLeapYear(year)) ? 29 : days[month - 1];
}
}  // namespace

bool DCF77Decoder::Time::operator==(const Time& o) const {
    return year == o.year && month == o.month && day == o.day && weekday == o.weekday &&
           hour == o.hour && minute == o.minute && cest == o.cest;
}

DCF77Decoder::Time DCF77Decoder::AddMinute(Time t) {
    if (++t.minute < 60) return t;
    t.minute = 0;
    if (++t.hour < 24) return t;
    t.hour = 0;
    t.weekday = t.weekday % 7 + 1;
    if (++t.day <= DaysInMonth(t.year, t.month)) return t;
    t.day = 1;
    if (++t.month <= 12) return t;
    t.month = 1;
    ++t.year;
    return t;
}

// Bit layout (DCF77 spec, bit index == second within the minute):
//   0        always 0
//   1-14     weather / civil warning (ignored)
//   15-19    call bit, DST change announce (16), Z1 (17), Z2 (18), leap second announce (19)
//   20       always 1 (start of time information)
//   21-27    minute (BCD 1,2,4,8,10,20,40),  28 = even parity P1
//   29-34    hour   (BCD 1,2,4,8,10,20),     35 = even parity P2
//   36-41    day,  42-44 weekday (1=Mon), 45-49 month, 50-57 year (00-99),  58 = even parity P3
// The frame describes the minute that starts at the *following* minute mark.
bool DCF77Decoder::DecodeFrame(uint64_t bits, Time& out) {
    if (Field(bits, 0, 1) != 0 || Field(bits, 20, 1) != 1) return false;
    // Z1 (CEST) and Z2 (CET) are always complementary.
    if (Field(bits, 17, 1) == Field(bits, 18, 1)) return false;
    if (!EvenParity(bits, 21, 28) || !EvenParity(bits, 29, 35) || !EvenParity(bits, 36, 58))
        return false;

    int minute, hour, day, month, year;
    if (!Bcd(bits, 21, 4, 3, minute) || !Bcd(bits, 29, 4, 2, hour) || !Bcd(bits, 36, 4, 2, day) ||
        !Bcd(bits, 45, 4, 1, month) || !Bcd(bits, 50, 4, 4, year))
        return false;
    int weekday = static_cast<int>(Field(bits, 42, 3));

    if (minute > 59 || hour > 23 || weekday < 1 || weekday > 7 || month < 1 || month > 12 ||
        day < 1)
        return false;
    if (day > DaysInMonth(2000 + year, month)) return false;

    out.year = static_cast<uint16_t>(2000 + year);
    out.month = static_cast<uint8_t>(month);
    out.day = static_cast<uint8_t>(day);
    out.weekday = static_cast<uint8_t>(weekday);
    out.hour = static_cast<uint8_t>(hour);
    out.minute = static_cast<uint8_t>(minute);
    out.cest = Field(bits, 17, 1) != 0;
    return true;
}

void DCF77Decoder::Feed(bool level, uint32_t now_ms) {
    if (level == _level) {
        _candidate_count = 0;
        return;
    }
    // A new level only counts once it has been stable for kDebounceSamples samples. The edge time
    // is that of the first differing sample, so both edges of a pulse are shifted equally and the
    // measured width stays accurate.
    if (_candidate_count == 0) _candidate_ms = now_ms;
    if (++_candidate_count >= kDebounceSamples) {
        _candidate_count = 0;
        OnEdge(level, _candidate_ms);
    }
}

void DCF77Decoder::OnEdge(bool new_level, uint32_t t_ms) {
    _stats.edges++;
    if (_have_edge) {
        // The phase that just ended was at the old level and started at _last_edge_ms.
        uint32_t width = t_ms - _last_edge_ms;
        if (width < kZeroMinMs) _stats.w_noise++;
        else if (width < kZeroMaxMs) _stats.w_bit0++;
        else if (width <= kOneMaxMs) _stats.w_bit1++;
        else if (width < 700) _stats.w_odd++;
        else if (width <= 1100) _stats.w_sec++;
        else _stats.w_min++;

        bool bit;
        bool is_pulse = true;
        if (width >= kZeroMinMs && width < kZeroMaxMs) {
            bit = false;
        } else if (width >= kZeroMaxMs && width <= kOneMaxMs) {
            bit = true;
        } else {
            is_pulse = false;  // long phase (800-900ms / minute gap) or noise
            bit = false;
        }

        if (is_pulse) {
            // Only the carrier-reduction phase is ever 100/200ms long, so whichever level shows
            // such widths is the pulse level (handles active-high and active-low receivers).
            uint8_t& v = _votes[_level ? 1 : 0];
            if (++v >= 60) {
                _votes[0] >>= 1;
                _votes[1] >>= 1;
            }
            bool pulse_level = _votes[1] > _votes[0];
            if (_level == pulse_level) OnPulse(_last_edge_ms, bit);
        }
    }
    _level = new_level;
    _last_edge_ms = t_ms;
    _have_edge = true;
}

void DCF77Decoder::OnPulse(uint32_t start_ms, bool bit) {
    _stats.pulses++;
    if (_have_pulse) {
        uint32_t period = start_ms - _last_start_ms;
        if (period >= kMinuteMarkMinMs && period <= kMinuteMarkMaxMs) {
            OnMinuteMark(start_ms);
            _index = 0;
            _bits = 0;
        } else if (period >= kSecondMinMs && period <= kSecondMaxMs) {
            if (_index >= 0 && ++_index >= kFrameBits) {
                _stats.frames_lost++;
                _index = -1;
                _prev_valid = false;
            }
        } else {
            // Missing or spurious pulse: wait for the next minute mark.
            if (_index >= 0) _stats.frames_lost++;
            _index = -1;
            _prev_valid = false;
        }
    }
    _have_pulse = true;
    _last_start_ms = start_ms;
    if (_index >= 0 && bit) _bits |= 1ULL << _index;
}

void DCF77Decoder::OnMinuteMark(uint32_t mark_ms) {
    if (_index != kFrameBits - 1) {
        if (_index >= 0) _stats.frames_lost++;
        _prev_valid = false;
        return;
    }

    Time t;
    if (!DecodeFrame(_bits, t)) {
        _stats.frames_bad++;
        _prev_valid = false;
        return;
    }
    _stats.frames_ok++;

    // Indoor reception is noisy: only trust a time that the following frame confirms. (Across a
    // DST change the hour jumps, so that one confirmation fails and the chain restarts.)
    if (_prev_valid && AddMinute(_prev) == t) {
        _sync.time = t;
        _sync.mark_ms = mark_ms;
        _sync_pending = true;
    }
    _prev = t;
    _prev_valid = true;
}

bool DCF77Decoder::TakeSync(Sync& out) {
    if (!_sync_pending) return false;
    out = _sync;
    _sync_pending = false;
    return true;
}

}  // namespace ChristmasClock

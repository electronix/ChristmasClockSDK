#pragma once

#include <cstdint>

namespace ChristmasClock {

// Hardware-independent DCF77 frame decoder.
//
// Feed() takes the raw level of the DCF77 pin at a fixed rate (~10ms). The decoder measures the
// carrier-reduction pulses (~100ms = 0, ~200ms = 1), finds the minute mark (2s gap between pulse
// starts), decodes the 59-bit frame incl. all 3 parity bits and only publishes a time once two
// consecutive frames agree (frame N+1 == frame N + 1 minute).
//
// The pulse polarity (active-high or active-low on the pin) is detected automatically: only the
// carrier-reduction phase is ever 100ms or 200ms long, the other phase is 800-900ms.
class DCF77Decoder {
public:
    struct Time {
        uint16_t year;    // full year, e.g. 2026
        uint8_t month;    // 1-12
        uint8_t day;      // 1-31
        uint8_t weekday;  // 1 = Monday ... 7 = Sunday
        uint8_t hour;     // 0-23
        uint8_t minute;   // 0-59
        bool cest;        // true = summer time (CEST, UTC+2), false = CET (UTC+1)

        bool operator==(const Time& o) const;
    };

    struct Sync {
        Time time;         // wall-clock time (second 0) at the moment of the minute mark
        uint32_t mark_ms;  // Feed() timestamp of that minute mark
    };

    struct Stats {
        uint32_t frames_ok = 0;    // frames that passed all checks
        uint32_t frames_bad = 0;   // complete frames rejected (parity / range / fixed bits)
        uint32_t frames_lost = 0;  // frames broken by a missing/extra pulse
    };

    static constexpr int kFrameBits = 59;

    // Call at a fixed rate (~10ms). now_ms may wrap (only differences are used).
    void Feed(bool level, uint32_t now_ms);

    // Returns true (once) after a newly confirmed minute mark.
    bool TakeSync(Sync& out);

    const Stats& GetStats() const { return _stats; }

    // Exposed for tests.
    static bool DecodeFrame(uint64_t bits, Time& out);
    static Time AddMinute(Time t);

private:
    static constexpr uint32_t kDebounceSamples = 2;

    void OnEdge(bool new_level, uint32_t t_ms);
    void OnPulse(uint32_t start_ms, bool bit);
    void OnMinuteMark(uint32_t mark_ms);

    // debounce
    bool _level = false;
    uint32_t _candidate_count = 0;
    uint32_t _candidate_ms = 0;

    // edge tracking
    bool _have_edge = false;
    uint32_t _last_edge_ms = 0;

    // polarity vote: which level is the (short) carrier-reduction phase
    uint8_t _votes[2] = {0, 0};

    // pulse / frame tracking
    bool _have_pulse = false;
    uint32_t _last_start_ms = 0;
    int _index = -1;  // bit index of the last pulse, -1 = not synchronized to a minute mark yet
    uint64_t _bits = 0;

    // two-frame confirmation
    bool _prev_valid = false;
    Time _prev{};

    bool _sync_pending = false;
    Sync _sync{};

    Stats _stats;
};

}  // namespace ChristmasClock

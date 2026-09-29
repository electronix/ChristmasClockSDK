#pragma once

#include <cstdint>
#include <optional>

#include "DCF77Decoder.hpp"
#include "pico/stdlib.h"

namespace ChristmasClock {

// DCF77 time-signal receiver. The analog front end (LNA sheet) already demodulates the carrier
// into a digital pulse train on the DCF77 net, which arrives at the RP2040 via R (47 ohm) on
// "A0" = GPIO26. A 10ms repeating timer samples the pin and feeds DCF77Decoder.
class LNA {
public:
    static constexpr uint kDefaultPin = 26;
    static constexpr int32_t kSamplePeriodMs = 10;

    struct Synced {
        DCF77Decoder::Time time;  // wall-clock time at second 0 of that minute
        uint32_t age_ms;          // how long ago that minute mark happened (add to time.second)
    };

    explicit LNA(uint pin = kDefaultPin);
    ~LNA();
    LNA(const LNA&) = delete;
    LNA& operator=(const LNA&) = delete;

    // Returns a newly confirmed time (once per confirmed minute), or nullopt.
    std::optional<Synced> PollSync();

    DCF77Decoder::Stats GetStats();

private:
    static bool SampleCallback(repeating_timer_t* timer);

    uint _pin;
    DCF77Decoder _decoder;
    repeating_timer_t _timer;
};
}

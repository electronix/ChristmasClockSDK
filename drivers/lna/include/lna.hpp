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

    // Diagnostic: counts level changes on the pin in a 1ms window with a tight polling loop
    // (interrupts off, ~MHz resolution, far beyond the 10ms decoder sampling). A 77.5kHz carrier
    // gives ~155 edges/ms, a clean DCF77 envelope gives 0. `with_pulldown` repeats the count with
    // the internal pull-down enabled: a driven pin doesn't care, a floating pin drops to 0.
    struct FastProbe {
        uint32_t edges_per_ms;
        uint32_t edges_per_ms_pulldown;
    };
    FastProbe ProbeFastEdges();

    // Diagnostic: the DCF77 pin (GPIO26 = ADC0) is read with the ADC in parallel to the digital
    // input. Each 10ms tick averages 16 conversions (0-4095, 3.3V full scale). Returns the values
    // collected since the previous call and starts a new window.
    static constexpr int kAdcBins = 20;  // 20 bins x 250ms = the last 5s
    struct AdcWindow {
        uint16_t mean;       // mean of all 10ms averages
        uint16_t min;        // lowest 10ms average
        uint16_t max;        // highest 10ms average
        uint16_t noise_p2p;  // largest peak-to-peak spread inside one 10ms tick (fast noise)
        uint16_t bins[kAdcBins];  // 250ms averages, oldest first
    };
    AdcWindow TakeAdcWindow();

    // Index of the last bit of the current frame (0-58), -1 if not synced to a minute mark.
    int GetBitIndex() const { return _decoder.BitIndex(); }

    // Raw level of the DCF77 pin right now (for diagnostics).
    bool GetPinLevel() const { return gpio_get(_pin); }

private:
    static bool SampleCallback(repeating_timer_t* timer);
    void SampleAdc();

    static constexpr int kTicksPerBin = 25;  // 25 x 10ms
    uint32_t _adc_sum = 0;
    uint32_t _adc_count = 0;
    uint16_t _adc_min = 4095;
    uint16_t _adc_max = 0;
    uint16_t _adc_noise = 0;
    uint32_t _bin_sum = 0;
    int _bin_ticks = 0;
    uint16_t _bins[kAdcBins] = {};
    int _bin_next = 0;  // ring index of the oldest bin

    uint _pin;
    DCF77Decoder _decoder;
    repeating_timer_t _timer;
};
}

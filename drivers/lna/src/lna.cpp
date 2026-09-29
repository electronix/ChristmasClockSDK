#include "lna.hpp"

#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "hardware/sync.h"

ChristmasClock::LNA::LNA(uint pin) : _pin(pin) {
    gpio_init(_pin);
    gpio_set_dir(_pin, GPIO_IN);
    gpio_disable_pulls(_pin);
    // Negative period: fixed interval between callback starts, independent of callback duration.
    // Only the ADC mux is selected, not adc_gpio_init(): that would switch off the digital input
    // buffer, and the digital path is still used by the decoder.
    adc_init();
    adc_select_input(_pin - 26);
    add_repeating_timer_ms(-kSamplePeriodMs, SampleCallback, this, &_timer);
}

void ChristmasClock::LNA::SampleAdc() {
    uint32_t sum = 0;
    uint16_t lo = 4095, hi = 0;
    for (int i = 0; i < 16; i++) {
        uint16_t v = adc_read();
        sum += v;
        if (v < lo) lo = v;
        if (v > hi) hi = v;
    }
    uint16_t avg = sum / 16;

    _adc_sum += avg;
    _adc_count++;
    if (avg < _adc_min) _adc_min = avg;
    if (avg > _adc_max) _adc_max = avg;
    if (hi - lo > _adc_noise) _adc_noise = hi - lo;

    _bin_sum += avg;
    if (++_bin_ticks >= kTicksPerBin) {
        _bins[_bin_next] = _bin_sum / kTicksPerBin;
        _bin_next = (_bin_next + 1) % kAdcBins;
        _bin_sum = 0;
        _bin_ticks = 0;
    }
}

ChristmasClock::LNA::AdcWindow ChristmasClock::LNA::TakeAdcWindow() {
    AdcWindow w;
    uint32_t irq = save_and_disable_interrupts();
    w.mean = _adc_count ? _adc_sum / _adc_count : 0;
    w.min = _adc_count ? _adc_min : 0;
    w.max = _adc_max;
    w.noise_p2p = _adc_noise;
    for (int i = 0; i < kAdcBins; i++) w.bins[i] = _bins[(_bin_next + i) % kAdcBins];
    _adc_sum = 0;
    _adc_count = 0;
    _adc_min = 4095;
    _adc_max = 0;
    _adc_noise = 0;
    restore_interrupts(irq);
    return w;
}

ChristmasClock::LNA::~LNA() {
    cancel_repeating_timer(&_timer);
}

// Runs in IRQ context: keep it short (no printing, no allocation).
bool ChristmasClock::LNA::SampleCallback(repeating_timer_t* timer) {
    auto* self = static_cast<LNA*>(timer->user_data);
    self->_decoder.Feed(gpio_get(self->_pin), to_ms_since_boot(get_absolute_time()));
    self->SampleAdc();
    return true;
}

static uint32_t CountEdges1ms(uint pin) {
    uint32_t irq = save_and_disable_interrupts();
    bool last = gpio_get(pin);
    uint32_t edges = 0;
    uint32_t start = time_us_32();
    while (time_us_32() - start < 1000) {
        bool level = gpio_get(pin);
        if (level != last) {
            edges++;
            last = level;
        }
    }
    restore_interrupts(irq);
    return edges;
}

ChristmasClock::LNA::FastProbe ChristmasClock::LNA::ProbeFastEdges() {
    FastProbe probe;
    probe.edges_per_ms = CountEdges1ms(_pin);

    gpio_pull_down(_pin);
    busy_wait_us(100);  // let a floating pin discharge
    probe.edges_per_ms_pulldown = CountEdges1ms(_pin);
    gpio_disable_pulls(_pin);
    return probe;
}

std::optional<ChristmasClock::LNA::Synced> ChristmasClock::LNA::PollSync() {
    DCF77Decoder::Sync sync;
    uint32_t irq = save_and_disable_interrupts();
    bool have = _decoder.TakeSync(sync);
    restore_interrupts(irq);
    if (!have) return std::nullopt;

    return Synced{sync.time, to_ms_since_boot(get_absolute_time()) - sync.mark_ms};
}

ChristmasClock::DCF77Decoder::Stats ChristmasClock::LNA::GetStats() {
    uint32_t irq = save_and_disable_interrupts();
    auto stats = _decoder.GetStats();
    restore_interrupts(irq);
    return stats;
}

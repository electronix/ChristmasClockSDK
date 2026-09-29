#include "lna.hpp"

#include "hardware/gpio.h"
#include "hardware/sync.h"

ChristmasClock::LNA::LNA(uint pin) : _pin(pin) {
    gpio_init(_pin);
    gpio_set_dir(_pin, GPIO_IN);
    gpio_disable_pulls(_pin);
    // Negative period: fixed interval between callback starts, independent of callback duration.
    add_repeating_timer_ms(-kSamplePeriodMs, SampleCallback, this, &_timer);
}

ChristmasClock::LNA::~LNA() {
    cancel_repeating_timer(&_timer);
}

// Runs in IRQ context: keep it short (no printing, no allocation).
bool ChristmasClock::LNA::SampleCallback(repeating_timer_t* timer) {
    auto* self = static_cast<LNA*>(timer->user_data);
    self->_decoder.Feed(gpio_get(self->_pin), to_ms_since_boot(get_absolute_time()));
    return true;
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

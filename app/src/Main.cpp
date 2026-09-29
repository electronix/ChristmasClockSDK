#include "ChristmasClock.hpp"
#include <iostream>
#include <iomanip>
#include "pico/stdlib.h"

#include "ErrorCorrection.hpp"
#include "Receiver.hpp"
#include "Transmitter.hpp"
#include "NECEventMapper.hpp"
#include "lna.hpp"
#include "SerialCommands.hpp"

static const uint32_t HEARTBEAT_INTERVAL_S = 5;

// IR loopback test: transmit the current time every second and print what the receiver decodes.
// Off by default because it floods the console; remote-control reception works either way.
static const bool IR_LOOPBACK_TEST = false;

void countdown(uint n) {
    for (uint i = 0; i < n; i++) {
        std::cout << n -i << std::endl;
        sleep_ms(1000);
    }
}

void IRQCallback(uint32_t data){
    std::cout << "IRQ: Manchester encoded Data received" << std::endl;
    std::cout << "Receiving Data: 0x" << std::hex << std::setfill('0') << std::setw(8) << data << " decoded to:   0x" << std::setfill('0') << std::setw(8) << ChristmasClock::IR::ErrorCorrection::DecodeMessage(data) << std::endl;
}

int main() {
    stdio_init_all();

    countdown(4);
    
    ChristmasClock::ChristmasClock clock;
    ChristmasClock::IR::Transmitter trans(pio0);
    ChristmasClock::IR::Receiver recv(pio1);

    ChristmasClock::IR::NECEventMapper mapper(recv);
    ChristmasClock::LNA dcf77;
    ChristmasClock::SerialCommands commands(clock.GetWallClock());

    if(IR_LOOPBACK_TEST){
        recv.UseReceivedCallback(IRQCallback);
    }

    int next_tick = time_us_32();
    int next_update = 0;
    uint32_t uptime_s = 0;
    ChristmasClock::DCF77Decoder::Stats last_stats;
    while (true) {
        auto tick = time_us_32();
        if(tick >= next_tick){
            clock.Tick();
            commands.Tick();
            next_tick += 1000000;
            uptime_s++;

            if(uptime_s % HEARTBEAT_INTERVAL_S == 0){
                auto stats = dcf77.GetStats();
                auto probe = dcf77.ProbeFastEdges();
                // Phase widths since the previous heartbeat. Clean DCF77 gives per 5s roughly:
                // noise=0 odd=0 sec=5 bit0+bit1=5 (and one min=1 per minute); lots of noise/odd = interference.
                std::cout << "[heartbeat] up=" << std::dec << uptime_s << "s"
                          << " DCF77 pin=" << dcf77.GetPinLevel()
                          << " | last " << HEARTBEAT_INTERVAL_S << "s:"
                          << " noise=" << stats.w_noise - last_stats.w_noise
                          << " bit0=" << stats.w_bit0 - last_stats.w_bit0
                          << " bit1=" << stats.w_bit1 - last_stats.w_bit1
                          << " odd=" << stats.w_odd - last_stats.w_odd
                          << " sec=" << stats.w_sec - last_stats.w_sec
                          << " min=" << stats.w_min - last_stats.w_min
                          << " | fast edges/ms=" << probe.edges_per_ms << " (pulldown " << probe.edges_per_ms_pulldown << ")"
                          << " | bit=" << dcf77.GetBitIndex() << "/58"
                          << " frames ok/bad/lost=" << stats.frames_ok << "/" << stats.frames_bad << "/" << stats.frames_lost
                          << " | total edges=" << stats.edges << " pulses=" << stats.pulses
                          << std::endl;
                last_stats = stats;

                // Analog view of the same pin (0-4095 = 0-3.3V; VGND idle level would be ~2048).
                // A working receiver shows the 1Hz pulse pattern in the 250ms bins.
                auto adc = dcf77.TakeAdcWindow();
                std::cout << "[adc] mean=" << adc.mean << " min=" << adc.min << " max=" << adc.max
                          << " noise_p2p=" << adc.noise_p2p << " | 250ms bins:";
                for(int i = 0; i < ChristmasClock::LNA::kAdcBins; i++){
                    std::cout << " " << adc.bins[i];
                }
                std::cout << std::endl;
            }

            if(IR_LOOPBACK_TEST){
                trans.Transmit(clock.GetTime());
            }
        }
        commands.Poll();
        if(auto sync = dcf77.PollSync()){
            auto& t = sync->time;
            // A confirmed DCF77 time (parity-checked, 2 consecutive frames) also sets the clock.
            clock.GetWallClock().SetSeconds(ChristmasClock::WallClock::ToSeconds(
                {t.year, t.month, t.day, t.hour, t.minute, 0}) + sync->age_ms / 1000);
            auto stats = dcf77.GetStats();
            std::cout << "DCF77 sync: " << std::dec << std::setfill('0')
                      << std::setw(4) << t.year << "-" << std::setw(2) << (int)t.month << "-" << std::setw(2) << (int)t.day
                      << " " << std::setw(2) << (int)t.hour << ":" << std::setw(2) << (int)t.minute
                      << (t.cest ? " CEST" : " CET") << " (+" << sync->age_ms << "ms)"
                      << " frames ok/bad/lost: " << stats.frames_ok << "/" << stats.frames_bad << "/" << stats.frames_lost
                      << std::endl;
        }
        next_update--;
        if(next_update <= 0){
            next_update = 20;
            clock.Update();
        }
        sleep_ms(10);
        if(auto event = mapper.GetEvent(); event != ChristmasClock::IR::NECEvent::NO_EVENT){
            if(clock.EvaluateEvent(event)){
                next_tick = time_us_32() + 1000000;
                std::cout << "New NEC Event: " << event << std::endl;
            }
        }
    }
    return 0;
}

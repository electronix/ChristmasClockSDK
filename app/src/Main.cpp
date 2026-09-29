#include "ChristmasClock.hpp"
#include <iostream>
#include <iomanip>
#include "pico/stdlib.h"

#include "ErrorCorrection.hpp"
#include "Receiver.hpp"
#include "Transmitter.hpp"
#include "NECEventMapper.hpp"
#include "SerialCommands.hpp"

// NOTE: DCF77 (time signal receiver) does NOT work on this board, do not try it again.
// The on-board front end (LNA sheet: 2 CE stages + 2 op-amp stages, ~10^6-10^7 total gain) amplifies
// broadband noise into rail-to-rail swings on the DCF77 net (GPIO26/ADC0); no 77.5kHz carrier or 1Hz
// pulse pattern was ever visible, neither on the pin nor on the scope, with the original tank, a
// single 1.2nF tank cap, or shorted coil. A decoder (pulse widths, parities, 2-frame confirmation) was
// written and host-tested, and ADC sampling was tried - it is in the git history (last commit that
// still has drivers/lna: 6b1f1fd) - but the hardware never delivered a usable signal.
// The time of day is therefore set from a PC over USB serial (SerialCommands, tools/settime.html).
// No external DCF77 module is planned either; if the time should survive power loss, a battery-backed
// RTC chip over I2C/SPI is the option to consider.

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
    ChristmasClock::SerialCommands commands(clock.GetWallClock());

    if(IR_LOOPBACK_TEST){
        recv.UseReceivedCallback(IRQCallback);
    }

    int next_tick = time_us_32();
    int next_update = 0;
    uint32_t uptime_s = 0;
    while (true) {
        auto tick = time_us_32();
        if(tick >= next_tick){
            clock.Tick();
            commands.Tick();
            next_tick += 1000000;
            uptime_s++;

            if(uptime_s % HEARTBEAT_INTERVAL_S == 0){
                std::cout << "[heartbeat] up=" << std::dec << uptime_s << "s" << std::endl;
            }

            if(IR_LOOPBACK_TEST){
                trans.Transmit(clock.GetTime());
            }
        }
        commands.Poll();
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

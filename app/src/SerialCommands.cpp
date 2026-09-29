#include "SerialCommands.hpp"

#include <strings.h>

#include <iostream>

#include "pico/stdio_usb.h"
#include "pico/stdlib.h"

namespace ChristmasClock {

SerialCommands::SerialCommands(WallClock& wall) : _wall(wall) {}

void SerialCommands::Poll() {
    // Bounded work per call so a flood of input can't starve the display/IR handling.
    for (int i = 0; i < 64; i++) {
        int c = getchar_timeout_us(0);
        if (c == PICO_ERROR_TIMEOUT) return;

        if (c == '\r' || c == '\n') {
            if (!_discard_line && !_line.empty()) Execute(_line);
            _line.clear();
            _discard_line = false;
            continue;
        }
        if (_line.size() >= kMaxLine) {
            _discard_line = true;  // too long: ignore the whole line rather than parse a fragment
            continue;
        }
        _line.push_back(static_cast<char>(c));
    }
}

void SerialCommands::Execute(const std::string& line) {
    const char* s = line.c_str();
    while (*s == ' ' || *s == '\t') s++;

    if (strncasecmp(s, "TIME ", 5) == 0) {
        WallClock::DateTime t;
        if (WallClock::Parse(s + 5, t) && _wall.Set(t)) {
            std::cout << "OK TIME " << WallClock::Format(t) << std::endl;
        } else {
            std::cout << "ERR bad time, expected: TIME YYYY-MM-DD HH:MM:SS" << std::endl;
        }
    } else if (strcasecmp(s, "GET TIME") == 0) {
        WallClock::DateTime t;
        if (_wall.Now(t)) {
            std::cout << "TIME " << WallClock::Format(t) << std::endl;
        } else {
            std::cout << "TIME NOTSET" << std::endl;
        }
    } else if (strcasecmp(s, "HELP") == 0) {
        std::cout << "Commands: TIME YYYY-MM-DD HH:MM:SS | GET TIME | HELP" << std::endl;
    } else {
        std::cout << "ERR unknown command, try HELP" << std::endl;
    }
}

void SerialCommands::Request() {
    std::cout << "REQ TIME" << std::endl;
    _since_request = 0;
}

void SerialCommands::Tick() {
    // A terminal (DTR) that just connected always gets asked once, so a re-plugged clock is
    // re-synchronized; while the clock is unset it is asked every few seconds so a page that
    // was opened later still gets the request.
    bool connected = stdio_usb_connected();
    if (connected && !_was_connected) Request();
    _was_connected = connected;

    if (_wall.IsValid()) return;
    if (++_since_request >= kRequestPeriodS) Request();
}

}  // namespace ChristmasClock

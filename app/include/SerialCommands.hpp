#pragma once

#include <string>

#include "WallClock.hpp"

namespace ChristmasClock {

// Line-based command interface on the USB serial console (stdin/stdout), used to set the
// wall clock from a PC (see tools/settime.html, or just type the commands into a terminal).
//
//   host -> device   TIME 2026-09-29 17:45:00    set the clock (local time)   -> "OK TIME ..."
//                    GET TIME                    -> "TIME 2026-09-29 17:45:01" or "TIME NOTSET"
//                    HELP
//   device -> host   REQ TIME                    "please send me the time": printed once when a
//                                                terminal connects and every 2s while the clock
//                                                is not set
class SerialCommands {
public:
    explicit SerialCommands(WallClock& wall);

    // Call often (every main-loop iteration): reads pending characters without blocking and
    // executes complete lines.
    void Poll();

    // Call once per second.
    void Tick();

private:
    static constexpr size_t kMaxLine = 80;
    static constexpr int kRequestPeriodS = 2;

    void Execute(const std::string& line);
    void Request();

    WallClock& _wall;
    std::string _line;
    bool _discard_line = false;
    bool _was_connected = false;
    int _since_request = 0;
};

}  // namespace ChristmasClock

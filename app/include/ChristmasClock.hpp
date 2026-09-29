#pragma once

#include "led.hpp"
#include "LEDvertical.hpp"
#include "SevenSeg.hpp"
#include "MatrixEffect.hpp"
#include "SnakeEffect.hpp"
#include "NECEventMapper.hpp"
#include "touch.hpp"
#include "WallClock.hpp"

namespace ChristmasClock {
class ChristmasClock {
public:
    ChristmasClock();
    void Tick();
    void Update();

    bool EvaluateEvent(IR::NECEvent event);

    void Reset();

    void SetCountdown(std::time_t countdown);
    std::time_t GetTime();
    void SetTime(std::time_t time);

    // Wall-clock time of day (set over USB serial, see SerialCommands). While it is set and the
    // countdown is not running, the display shows HH:MM instead of the countdown.
    WallClock& GetWallClock() { return _wall; }

private:
    LED _led;
    SevenSeg _seg;
    MatrixEffect _matrix;
    SnakeEffect _snake;
    Touch _touch;
    WallClock _wall;

    bool _is_on;
    bool _running;
    int _vol_index;
    bool _is_in_menu;
    int _menu_number;

    int _idle_seconds;
    int _adjust_seconds;   // seconds since the last pad/IR input (slider does not count)
    bool _matrix_active;
    bool _snake_active;

    std::time_t _time;
    std::time_t _countdown;
    std::time_t _countdown_warning;
    std::time_t _countdown_finishing;

    static const uint8_t _brightness[20];
    static const int MAX_VOL_INDEX = 8;
    static const int MATRIX_IDLE_TIMEOUT_S = 10;
    // After this many idle seconds (no touch/IR, countdown not running) the time of day replaces the
    // countdown value; until then the countdown stays visible so adjusting it gives feedback.
    static const int CLOCK_SHOW_DELAY_S = 3;
    // Brightness of the cyan time of day relative to full (0-255), so it is about as bright as
    // the single-channel green countdown.
    static const int CLOCK_COLOR_GAIN = 128;
    bool EvaluateEventInMenu(IR::NECEvent event);
    int ConvertTimeToNumber(std::time_t time);
    std::time_t NumberToConvertTime(int number);
};
}
#pragma once

#include "led.hpp"
#include "LEDvertical.hpp"
#include "SevenSeg.hpp"
#include "MatrixEffect.hpp"
#include "SnakeEffect.hpp"
#include "NECEventMapper.hpp"
#include "touch.hpp"

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

private:
    LED _led;
    SevenSeg _seg;
    MatrixEffect _matrix;
    SnakeEffect _snake;
    Touch _touch;

    bool _is_on;
    bool _running;
    int _vol_index;
    bool _is_in_menu;
    int _menu_number;

    int _idle_seconds;
    bool _matrix_active;
    bool _snake_active;

    std::time_t _time;
    std::time_t _countdown;
    std::time_t _countdown_warning;
    std::time_t _countdown_finishing;

    static const uint8_t _brightness[20];
    static const int MAX_VOL_INDEX = 8;
    static const int MATRIX_IDLE_TIMEOUT_S = 10;

    bool EvaluateEventInMenu(IR::NECEvent event);
    int ConvertTimeToNumber(std::time_t time);
    std::time_t NumberToConvertTime(int number);
};
}
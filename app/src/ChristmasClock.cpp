#include "ChristmasClock.hpp"
#include "ColorGRBa.hpp"
#include <iostream>

namespace ChristmasClock{
    
const uint8_t ChristmasClock::_brightness[20] = { 2, 4, 5, 7, 9, 11, 14, 18, 22, 28, 35, 43, 54, 67, 84, 105, 131, 164, 204, 255 };

ChristmasClock::ChristmasClock() :
    _led(pio0),
    _seg(_led),
    _matrix(_led),
    _snake(_led),
    _touch(),
    _is_on(true),
    _running(false),
    _vol_index(6),
    _is_in_menu(false),
    _menu_number(0),
    _idle_seconds(0),
    _adjust_seconds(CLOCK_SHOW_DELAY_S),
    _matrix_active(false),
    _snake_active(false),
    _countdown(300),
    _countdown_warning(100),
    _countdown_finishing(50),
    _time(300)
{
    _seg.SetGain(_brightness[_vol_index]);
}

bool ChristmasClock::EvaluateEvent(IR::NECEvent event){
    if(event != IR::NECEvent::NO_EVENT){
        _idle_seconds = 0;
        _adjust_seconds = 0;
        _matrix_active = false;
    }
    switch(event){
        case(IR::NECEvent::NO_EVENT): return false;
        case(IR::NECEvent::ON_OFF):
            {
                _is_on = !_is_on;
                if(_is_on){
                    _seg.SetGain(_brightness[_vol_index]);
                }else{
                    _seg.SetGain(0x00);
                }
            }
            return false;
        case(IR::NECEvent::VOL_UP):
            {
                _vol_index++;
                if(_vol_index > MAX_VOL_INDEX){
                    _vol_index = MAX_VOL_INDEX;
                }
                if(_is_on){
                    _seg.SetGain(_brightness[_vol_index]);
                }
            }
            return false;
        case(IR::NECEvent::VOL_DOWN): 
            {
                _vol_index--;
                if(_vol_index < 0){
                    _vol_index = 0;
                }
                if(_is_on){
                    _seg.SetGain(_brightness[_vol_index]);
                }
            }
            return false;
        case(IR::NECEvent::ENTER_MENU): 
            {
                if(_is_in_menu){
                    if(_menu_number > 0){
                        SetCountdown(NumberToConvertTime(_menu_number));
                    }
                }else{
                    _menu_number = 0;
                    _seg.SetForeground(ColorGRBa::WHITE);
                    _seg.SetBCDNumber(ConvertTimeToNumber(_countdown), 3);
                    _seg.SetDoublePoint();
                    _seg.Update();
                }
                _is_in_menu = !_is_in_menu;
            }
            return false;
    }
    if(_is_in_menu) return EvaluateEventInMenu(event);
    Reset();
    return true;
}

int ChristmasClock::ConvertTimeToNumber(std::time_t time){
    int minutes = ((int)time) /60;
    int seconds = ((int)time) -(minutes *60);

    int bcd_minutes_high = minutes /10;
    int bcd_minutes_low  = minutes -(bcd_minutes_high *10);

    int bcd_seconds_high = seconds /10;
    int bcd_seconds_low  = seconds -(bcd_seconds_high *10);
    return (bcd_minutes_high << 12) | (bcd_minutes_low << 8) | (bcd_seconds_high << 4) | bcd_seconds_low;
}

std::time_t ChristmasClock::NumberToConvertTime(int number){
    int minutes = (number >> 12) * 10;
    minutes += (number >> 8) & 0x0F;

    number &= 0x00FF;
    int seconds = (number >> 4) * 10;
    seconds += number & 0x0F;

    return minutes *60 +seconds;
}

bool ChristmasClock::EvaluateEventInMenu(IR::NECEvent event){
    switch(event){
        case(IR::NECEvent::NUM_0): { _menu_number <<= 4; _menu_number |= 0; } break;
        case(IR::NECEvent::NUM_1): { _menu_number <<= 4; _menu_number |= 1; } break;
        case(IR::NECEvent::NUM_2): { _menu_number <<= 4; _menu_number |= 2; } break;
        case(IR::NECEvent::NUM_3): { _menu_number <<= 4; _menu_number |= 3; } break;
        case(IR::NECEvent::NUM_4): { _menu_number <<= 4; _menu_number |= 4; } break;
        case(IR::NECEvent::NUM_5): { _menu_number <<= 4; _menu_number |= 5; } break;
        case(IR::NECEvent::NUM_6): { _menu_number <<= 4; _menu_number |= 6; } break;
        case(IR::NECEvent::NUM_7): { _menu_number <<= 4; _menu_number |= 7; } break;
        case(IR::NECEvent::NUM_8): { _menu_number <<= 4; _menu_number |= 8; } break;
        case(IR::NECEvent::NUM_9): { _menu_number <<= 4; _menu_number |= 9; } break;
    }
    _menu_number &= 0x0000FFFF;
    _seg.SetBCDNumber(_menu_number, 3);
    return false;
}

void ChristmasClock::Reset() {
    _time = _countdown;
}

void ChristmasClock::SetCountdown(std::time_t countdown){
    _countdown = countdown;
    _countdown_warning = countdown / 3;
    _countdown_finishing = _countdown_warning / 2;
    _time = countdown;
}

std::time_t ChristmasClock::GetTime(){
    return _time;
}

void ChristmasClock::SetTime(std::time_t time){
    _time = time;
}

void ChristmasClock::Tick(){
    if(_running){
        _time--;
        _idle_seconds = 0;
    }else{
        _idle_seconds++;
        if(_adjust_seconds < CLOCK_SHOW_DELAY_S) _adjust_seconds++;
        // With a valid wall clock the idle state shows the time of day instead of the screensaver.
        if(_idle_seconds >= MATRIX_IDLE_TIMEOUT_S && !_wall.IsValid()){
            _matrix_active = true;
        }
    }
}

void ChristmasClock::Update() {
    if(auto slider = _touch.GetSliderPosition()){
        int index = (int)(*slider *MAX_VOL_INDEX +0.5f);
        if(index < 0) index = 0;
        if(index > MAX_VOL_INDEX) index = MAX_VOL_INDEX;
        _vol_index = index;
        if(_is_on){
            _seg.SetGain(_brightness[_vol_index]);
        }
        _idle_seconds = 0;
        _matrix_active = false;
    }

    uint16_t pressed = _touch.GetPressedPads();
    if(pressed != 0){
        _idle_seconds = 0;
        _adjust_seconds = 0;
        _matrix_active = false;
    }

    bool tp1 = pressed & (uint16_t)TouchPad::TP1;
    bool tp6 = pressed & (uint16_t)TouchPad::TP6;
    if(tp1 && tp6){
        // TP1+TP6 held together is the Snake-effect toggle gesture, not "-1 minute"/"-1 second".
        _snake_active = !_snake_active;
    }else{
        if(tp1){ _time -= 60; }
        if(pressed & (uint16_t)TouchPad::TP2){ _time += 60; }
        if(pressed & (uint16_t)TouchPad::TP3){ Reset(); }
        if(pressed & (uint16_t)TouchPad::TP4){ _running = !_running; }
        if(pressed & (uint16_t)TouchPad::TP5){ _time += 1; }
        if(tp6){ _time -= 1; }
    }

    if(_snake_active){
        _snake.Update();
        return;
    }

    if(_matrix_active){
        _matrix.Update();
        return;
    }

    if(_is_in_menu) {
        _seg.Update();
        return;
    }

    WallClock::DateTime now;
    if(!_running && _adjust_seconds >= CLOCK_SHOW_DELAY_S && _wall.Now(now)){
        // Cyan: its active channels are equal, so the hue stays the same at every brightness (mixed
        // colours like warm white shift because the small gains round the channels unequally).
        // Halved so two channels are not brighter than the single-channel green countdown.
        _seg.SetForeground(ColorGRBa::CYAN * CLOCK_COLOR_GAIN);
        _seg.ClearPoints();
        _seg.SetNumber(now.hour * 100 + now.minute, 4);
        if(now.second % 2 == 0){
            _seg.SetDoublePoint();  // blinking colon, once per second
        }
        _seg.Update();
        return;
    }

    if(_time > _countdown_warning){
        _seg.SetForeground(ColorGRBa::GREEN);
    }else if(_time > _countdown_finishing){
        _seg.SetForeground(ColorGRBa::ORANGE);
    }else{
        _seg.SetForeground(ColorGRBa::RED);
    }
    _seg.SetTime(_time);
    _seg.Update();
}
}

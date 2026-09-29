#include "MatrixEffect.hpp"
#include "led.hpp"

#include "pico/time.h"
#include <cstdlib>

namespace ChristmasClock {

const uint8_t MatrixEffect::_columnX[8] = { 0, 6, 8, 14, 18, 24, 26, 32 };
const uint8_t MatrixEffect::_offsets[4] = { 0, 8, 18, 26 };

MatrixEffect::MatrixEffect(LED& led) :
    _led(led),
    _bmp(LED::SCREEN_WIDTH, LED::SCREEN_HIGHT)
{
    srand(static_cast<unsigned int>(time_us_64()));

    for(int n = 0; n < 8; n++){
        Drop drop;
        drop.position = rand() % COLUMN_LENGTH;
        drop.speed = 1;
        drop.counter = 0;
        _drops.push_back(drop);
    }
}

int MatrixEffect::ToBitmapY(int position){
    if(position < 5) return 1 +position;
    return 7 +(position -5);
}

void MatrixEffect::DrawColumn(const Drop& drop, int x){
    _bmp(x, ToBitmapY(drop.position)) = ColorGRBa::GREEN;

    static const int tailGain[TAIL_LENGTH] = { 140, 80, 40 };
    for(int t = 1; t <= TAIL_LENGTH; t++){
        int pos = drop.position -t;
        if(pos < 0) pos += COLUMN_LENGTH;
        _bmp(x, ToBitmapY(pos)) = ColorGRBa::GREEN * tailGain[t -1];
    }
}

void MatrixEffect::DrawBackgroundFlicker(){
    for(auto offset : _offsets){
        for(int x = 1; x <= 5; x++){
            if(rand() % 40 == 0) _bmp(offset +x, 0)  = ColorGRBa::GREEN * 30;
            if(rand() % 40 == 0) _bmp(offset +x, 6)  = ColorGRBa::GREEN * 30;
            if(rand() % 40 == 0) _bmp(offset +x, 12) = ColorGRBa::GREEN * 30;
        }
    }
    if(rand() % 40 == 0) _bmp(16, 6) = ColorGRBa::GREEN * 30;
    if(rand() % 40 == 0) _bmp(16, 9) = ColorGRBa::GREEN * 30;
}

void MatrixEffect::Update(){
    for(int x = 0; x < LED::SCREEN_WIDTH; x++){
        for(int y = 0; y < LED::SCREEN_HIGHT; y++){
            _bmp(x, y) = ColorGRBa::BLACK;
        }
    }

    DrawBackgroundFlicker();

    for(int n = 0; n < 8; n++){
        Drop& drop = _drops[n];
        drop.counter++;
        if(drop.counter >= drop.speed){
            drop.counter = 0;
            drop.position = (drop.position +1) % COLUMN_LENGTH;
        }
        DrawColumn(drop, _columnX[n]);
    }

    _led.Update(_bmp);
}
}

#pragma once

#include "bitmap.hpp"
#include "ColorGRBa.hpp"
#include <cstdint>
#include <vector>

namespace ChristmasClock {

class LED;

// "Matrix rain" over the seven-segment display. The board is not a dense dot-matrix - LEDs only
// exist on the digit strokes - so this drives a falling green drop down each of the 8 vertical
// segment strokes (2 per digit x 4 digits), each stroke really being two separate 5-pixel runs
// (above/below the middle horizontal segment) that are treated here as one continuous 10-step
// virtual column. The horizontal bars (top/middle/bottom of each digit, plus the colon) flicker
// faintly and randomly in the background for the "falling code" texture.
class MatrixEffect {
public:
    MatrixEffect(LED& led);

    // Call periodically (e.g. once per ~50-100ms tick) to advance and redraw the effect.
    void Update();

private:
    LED& _led;
    Bitmap _bmp;

    struct Drop {
        int position; // 0..(COLUMN_LENGTH-1) along the virtual column, see ToBitmapY()
        int speed;    // ticks per step - randomized per column so they don't fall in lockstep
        int counter;
    };

    std::vector<Drop> _drops;

    static const int COLUMN_LENGTH = 10; // 5px top segment + 5px bottom segment
    static const int TAIL_LENGTH = 3;
    static const uint8_t _columnX[8];
    static const uint8_t _offsets[4];

    static int ToBitmapY(int position);
    void DrawColumn(const Drop& drop, int x);
    void DrawBackgroundFlicker();
};
}

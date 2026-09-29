#pragma once

#include "bitmap.hpp"
#include "ColorGRBa.hpp"
#include <memory>
#include <cstdint>
#include <ctime>
#include <string>

namespace ChristmasClock {

class LED;

class SevenSeg {
public:
    SevenSeg(LED& led, ColorGRBa color = ColorGRBa::WHITE, ColorGRBa background = ColorGRBa::BLACK);

    void SetDigit1(int number) { SetDigit(0, number); }
    void SetDigit2(int number) { SetDigit(1, number); }
    void SetDigit3(int number) { SetDigit(2, number); }
    void SetDigit4(int number) { SetDigit(3, number); }

    void SetDigit(int position, int number);
    void ClearDigit(int position);

    void SetNumber(int number, int minimal = 0);
    void SetBCDNumber(int number, int minimal = 0);
    void SetTime(std::time_t t);

    void SetPoint();
    void SetComma();
    void SetDoublePoint();
    void ClearPoints();

    // Marquee/ticker: shows the 4 characters of `text` starting at `offset` (wrapping around,
    // modulo text.size()) across the 4 digit positions, left digit = position 0 = offset+0.
    // Only what a seven-segment digit can actually render is supported: '0'-'9' and the hex
    // letters a-f/A-F; anything else (e.g. a space used as a loop separator) is shown blank.
    // Call with an incrementing offset (e.g. one step per timer tick) to scroll the text
    // right-to-left across the display - position 0 is physically the left-most digit and
    // position 3 the right-most when the board is held with the middle screw on its long edge
    // at the bottom (confirmed against the PCB/Gerber mounting-hole layout: increasing bitmap
    // x runs left-to-right and increasing bitmap y runs top-to-bottom in that orientation, same
    // as the existing MM:SS digit layout already relies on - no flip needed).
    void SetMarqueeWindow(const std::string& text, int offset);

    void SetForeground(ColorGRBa color) { _foreground = color; }
    void SetBackground(ColorGRBa color) { _background = color; }

    void SetGain(uint8_t gain);

    void Update();

private:
    LED& _led;
    Bitmap _bmp;
    bool _draw_leading_zero;
    bool _comma_drawn;

    ColorGRBa _foreground;
    ColorGRBa _background;

    void setSegment(int number, int offsetX, ColorGRBa color);

    static int CharToHexValue(char c);

    void SetNegative();
    void ClearNegative();

    static const uint8_t _offsets[4];
    static const uint8_t _seg0X[5];
    static const uint8_t _seg0Y[5];
    static const uint8_t _seg1X[5];
    static const uint8_t _seg1Y[5];
    static const uint8_t _seg2X[5];
    static const uint8_t _seg2Y[5];
    static const uint8_t _seg3X[5];
    static const uint8_t _seg3Y[5];
    static const uint8_t _seg4X[5];
    static const uint8_t _seg4Y[5];
    static const uint8_t _seg5X[5];
    static const uint8_t _seg5Y[5];
    static const uint8_t _seg6X[5];
    static const uint8_t _seg6Y[5];
    static const uint8_t* _segmentsX[7];
    static const uint8_t* _segmentsY[7];
    static const uint8_t _bcdDecode[16];
};
}

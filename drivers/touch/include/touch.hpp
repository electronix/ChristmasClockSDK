#pragma once

#include "hardware/i2c.h"

#include <cstddef>
#include <cstdint>
#include <optional>

namespace ChristmasClock {

// Electrodes ELE0-2 and ELE9-11 are the six round touch pads (TP1-TP6).
enum class TouchPad : uint16_t {
    TP1 = 1u << 0,
    TP2 = 1u << 1,
    TP3 = 1u << 2,
    TP4 = 1u << 9,
    TP5 = 1u << 10,
    TP6 = 1u << 11,
};

// Driver for the on-board MPR121 capacitive touch controller (U4).
// Wiring per hardware/1-Schematic_ChristmasClock.json (Touch/RP2040/GPIO sheets):
//   SDA  -> GPIO18 (I2C1 SDA)
//   SCL  -> GPIO19 (I2C1 SCL)
//   ADDR -> GND    (-> I2C address 0x5A)
// Electrodes ELE3..ELE8 form the linear touch slider (S1).
class Touch {
public:
    Touch(i2c_inst_t* i2c = i2c1, uint sda_pin = 18, uint scl_pin = 19, uint8_t address = 0x5A);

    // Position on the slider, from 0.0 (ELE3 end) to 1.0 (ELE8 end),
    // or std::nullopt if the slider is not currently touched.
    std::optional<float> GetSliderPosition();

    // Bitmask (OR of TouchPad values) of pads that just transitioned from
    // untouched to touched since the last call, i.e. one hit per tap.
    uint16_t GetPressedPads();

private:
    static const uint8_t SLIDER_FIRST_ELECTRODE = 3;
    static const uint8_t SLIDER_ELECTRODE_COUNT = 6;
    static const uint16_t PAD_MASK = 0b0000111000000111;

    i2c_inst_t* _i2c;
    uint8_t _address;
    uint16_t _previous_pad_touch;

    void Init();
    void WriteRegister(uint8_t reg, uint8_t value);
    uint8_t ReadRegister(uint8_t reg);
    void ReadRegisters(uint8_t reg, uint8_t* buffer, size_t length);

    uint16_t ReadTouchStatus();
    uint16_t ReadFilteredData(uint8_t electrode);
    uint16_t ReadBaseline(uint8_t electrode);
};
}

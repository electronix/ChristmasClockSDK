#include "touch.hpp"

#include "pico/stdlib.h"

namespace {
    const uint8_t MPR121_TOUCHSTATUS_L = 0x00;
    const uint8_t MPR121_FILTDATA_0L   = 0x04;
    const uint8_t MPR121_BASELINE_0    = 0x1E;
    const uint8_t MPR121_MHDR          = 0x2B;
    const uint8_t MPR121_NHDR          = 0x2C;
    const uint8_t MPR121_NCLR          = 0x2D;
    const uint8_t MPR121_FDLR          = 0x2E;
    const uint8_t MPR121_MHDF          = 0x2F;
    const uint8_t MPR121_NHDF          = 0x30;
    const uint8_t MPR121_NCLF          = 0x31;
    const uint8_t MPR121_FDLF          = 0x32;
    const uint8_t MPR121_NHDT          = 0x33;
    const uint8_t MPR121_NCLT          = 0x34;
    const uint8_t MPR121_FDLT          = 0x35;
    const uint8_t MPR121_TOUCHTH_0     = 0x41;
    const uint8_t MPR121_RELEASETH_0   = 0x42;
    const uint8_t MPR121_DEBOUNCE      = 0x5B;
    const uint8_t MPR121_CONFIG1       = 0x5C;
    const uint8_t MPR121_CONFIG2       = 0x5D;
    const uint8_t MPR121_ECR           = 0x5E;
    const uint8_t MPR121_SOFTRESET     = 0x80;
}

namespace ChristmasClock {

Touch::Touch(i2c_inst_t* i2c, uint sda_pin, uint scl_pin, uint8_t address) :
    _i2c(i2c),
    _address(address),
    _previous_pad_touch(0)
{
    i2c_init(_i2c, 400 * 1000);
    gpio_set_function(sda_pin, GPIO_FUNC_I2C);
    gpio_set_function(scl_pin, GPIO_FUNC_I2C);
    gpio_pull_up(sda_pin);
    gpio_pull_up(scl_pin);

    Init();
}

void Touch::Init() {
    WriteRegister(MPR121_SOFTRESET, 0x63);
    sleep_ms(1);
    WriteRegister(MPR121_ECR, 0x00);

    for (uint8_t electrode = 0; electrode < 12; electrode++) {
        WriteRegister(MPR121_TOUCHTH_0 + electrode * 2, 12);
        WriteRegister(MPR121_RELEASETH_0 + electrode * 2, 6);
    }

    WriteRegister(MPR121_MHDR, 0x01);
    WriteRegister(MPR121_NHDR, 0x01);
    WriteRegister(MPR121_NCLR, 0x0E);
    WriteRegister(MPR121_FDLR, 0x00);

    WriteRegister(MPR121_MHDF, 0x01);
    WriteRegister(MPR121_NHDF, 0x05);
    WriteRegister(MPR121_NCLF, 0x01);
    WriteRegister(MPR121_FDLF, 0x00);

    WriteRegister(MPR121_NHDT, 0x00);
    WriteRegister(MPR121_NCLT, 0x00);
    WriteRegister(MPR121_FDLT, 0x00);

    WriteRegister(MPR121_DEBOUNCE, 0x00);
    WriteRegister(MPR121_CONFIG1, 0x10);
    WriteRegister(MPR121_CONFIG2, 0x20);

    WriteRegister(MPR121_ECR, 0x8C);
}

void Touch::WriteRegister(uint8_t reg, uint8_t value) {
    uint8_t buf[2] = { reg, value };
    i2c_write_blocking(_i2c, _address, buf, 2, false);
}

uint8_t Touch::ReadRegister(uint8_t reg) {
    uint8_t value = 0;
    ReadRegisters(reg, &value, 1);
    return value;
}

void Touch::ReadRegisters(uint8_t reg, uint8_t* buffer, size_t length) {
    i2c_write_blocking(_i2c, _address, &reg, 1, true);
    i2c_read_blocking(_i2c, _address, buffer, length, false);
}

uint16_t Touch::ReadTouchStatus() {
    uint8_t buf[2];
    ReadRegisters(MPR121_TOUCHSTATUS_L, buf, 2);
    return (uint16_t)buf[0] | ((uint16_t)buf[1] << 8);
}

uint16_t Touch::ReadFilteredData(uint8_t electrode) {
    uint8_t buf[2];
    ReadRegisters(MPR121_FILTDATA_0L + electrode * 2, buf, 2);
    return (uint16_t)buf[0] | ((uint16_t)buf[1] << 8);
}

uint16_t Touch::ReadBaseline(uint8_t electrode) {
    return (uint16_t)ReadRegister(MPR121_BASELINE_0 + electrode) << 2;
}

std::optional<float> Touch::GetSliderPosition() {
    uint16_t touched = ReadTouchStatus();
    uint16_t slider_mask = ((1u << SLIDER_ELECTRODE_COUNT) - 1) << SLIDER_FIRST_ELECTRODE;
    if ((touched & slider_mask) == 0) {
        return std::nullopt;
    }

    float weighted_sum = 0.0f;
    float weight_total = 0.0f;
    for (uint8_t i = 0; i < SLIDER_ELECTRODE_COUNT; i++) {
        uint8_t electrode = SLIDER_FIRST_ELECTRODE + i;
        if (!(touched & (1u << electrode))) {
            continue;
        }

        int32_t delta = (int32_t)ReadBaseline(electrode) - (int32_t)ReadFilteredData(electrode);
        if (delta < 0) {
            delta = 0;
        }

        weighted_sum += (float)i * (float)delta;
        weight_total += (float)delta;
    }

    if (weight_total <= 0.0f) {
        return std::nullopt;
    }

    return (weighted_sum / weight_total) / (float)(SLIDER_ELECTRODE_COUNT - 1);
}

uint16_t Touch::GetPressedPads() {
    uint16_t touched = ReadTouchStatus() & PAD_MASK;
    uint16_t pressed = touched & ~_previous_pad_touch;
    _previous_pad_touch = touched;
    return pressed;
}
}

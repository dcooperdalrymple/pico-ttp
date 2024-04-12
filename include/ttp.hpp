#pragma once
#include "pico/stdlib.h"
#include "ttp_serial.pio.h"

#define TTP_FREQUENCY 484

enum class TTPMode : uint8_t {
    MODE_8BIT = 0,
    MODE_16BIT
};

enum class TTPState : uint8_t {
    ERROR = 0,
    PRESS,
    RELEASE
};

typedef void(* ttp_callback_t) (uint8_t input, TTPState state);

const pio_program_t * ttp_serial_get_program(TTPMode mode, bool invert_clk);
bool ttp_serial_program_get_default_config(TTPMode mode, bool invert_clk, uint offset, pio_sm_config * cfg);

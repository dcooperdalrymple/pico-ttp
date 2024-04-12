#include "ttp.hpp"

const pio_program_t * ttp_serial_get_program(TTPMode mode, bool invert_clk) {
    switch (mode) {
        case TTPMode::MODE_8BIT:
            return invert_clk ? &ttp_serial_8inv_program : &ttp_serial_8_program;
        case TTPMode::MODE_16BIT:
            return invert_clk ? &ttp_serial_16inv_program : &ttp_serial_16_program;
    }
    return NULL;
};

bool ttp_serial_program_get_default_config(TTPMode mode, bool invert_clk, uint offset, pio_sm_config * cfg) {
    switch (mode) {
        case TTPMode::MODE_8BIT:
            *cfg = invert_clk ? ttp_serial_8inv_program_get_default_config(offset) : ttp_serial_8_program_get_default_config(offset);
            return true;
        case TTPMode::MODE_16BIT:
            *cfg = invert_clk ? ttp_serial_16inv_program_get_default_config(offset) : ttp_serial_16_program_get_default_config(offset);
            return true;
    }
    return false;
};

#pragma once
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"

#include "ttp.hpp"

class TTP229 {

public:

    TTP229(uint sdo, uint scl, TTPMode mode, bool invert_clk);
    TTP229(uint sdo, uint scl);
    ~TTP229();

    void enable();
    void disable();

    void set_callback(ttp_callback_t callback);
    void clear_callback();

    void update();
    uint get_pio_index();

private:
    void init(uint sdo, uint scl, TTPMode mode, bool invert_clk);

    uint sdo, scl;
    uint8_t bits;
    uint32_t data;
    ttp_callback_t cb;

    uint pio_index;
    PIO pio;
    const pio_program_t * prg;
    uint sm;
    uint offset;
    pio_sm_config cfg;

};

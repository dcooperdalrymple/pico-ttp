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

    void enable(bool interrupt);
    void enable();
    void disable();

    void set_callback(ttp_callback_t callback);
    void clear_callback();

    uint32_t get();
    uint32_t get_blocking();

    bool value(uint8_t input, uint32_t data);
    bool value(uint8_t input);

    TTPState state(uint8_t input, uint32_t current, uint32_t previous);
    TTPState state(uint8_t input);

    void update();

private:
    void init(uint sdo, uint scl, TTPMode mode, bool invert_clk);

    uint sdo, scl;
    uint8_t bits;
    uint32_t previous;
    ttp_callback_t cb;

    uint pio_index;
    PIO pio;
    const pio_program_t * prg;
    uint sm;
    uint offset;
    pio_sm_config cfg;

};

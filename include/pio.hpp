#pragma once
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "ttp_serial.pio.h"

static PIO pio_instances[NUM_PIOS] = {pio0, pio1};

static PIO pio_get_available(const pio_program_t * program) {
    for (uint8_t i = 0; i < NUM_PIOS; i++) {
        if (!pio_can_add_program(pio_instances[i], program)) continue;
        return pio_instances[i];
    }
    return NULL;
};

static uint pio_sm_get_availabe(PIO pio) {
    for (uint i = 0; i < NUM_PIO_STATE_MACHINES; i++) {
        if (!pio_sm_is_claimed(pio, i)) return i;
    }
    return -1;
};

static uint32_t pio_set_frequency(PIO pio, uint sm, uint32_t frequency) {
    if (frequency == 0) frequency = clock_get_hz(clk_sys);

    uint64_t frequency256 = (uint64_t)clock_get_hz(clk_sys) * 256;
    uint64_t div256 = frequency256 / frequency;
    if (frequency256 % div256 > 0) div256 += 1;
    
    // 0 is interpreted as 0x10000 so it's valid.
    if (div256 / 256 > 0x10000 || frequency > clock_get_hz(clk_sys)) return 0;

    pio_sm_set_clkdiv_int_frac(pio, sm, div256 / 256, div256 % 256);
    pio_sm_clkdiv_restart(pio, sm);

    return frequency256 / div256; // actual frequency
};

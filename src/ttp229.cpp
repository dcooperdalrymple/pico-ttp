#include "ttp229.hpp"
#include "pio.hpp"

TTP229::TTP229(uint sdo, uint scl, TTPMode mode, bool invert_clk) {
    this->data = 0;
    this->bits = mode == TTPMode::MODE_16BIT ? 16 : 8;

    this->sdo = sdo;
    this->scl = scl;

    // Load appropriate pio program
    this->prg = ttp_serial_get_program(mode, invert_clk);

    // Get available PIO and state machine index
    this->pio = pio_get_available(this->prg);
    this->sm = pio_sm_get_availabe(this->pio);

    // Load program
    this->offset = pio_add_program(this->pio, this->prg);
    ttp_serial_program_get_default_config(mode, invert_clk, this->offset, &this->cfg);

    // Setup GPIO
    pio_gpio_init(this->pio, this->sdo); // NOTE: May not be necessary?
    gpio_pull_up(this->sdo);
    pio_gpio_init(this->pio, this->scl);

    sm_config_set_in_pins(&this->cfg, this->sdo);
    sm_config_set_set_pins(&this->cfg, this->scl, 1);

    pio_sm_set_pindirs_with_mask(this->pio, this->sm, (1 << this->scl), (1 << this->sdo) | (1 << this->scl));
    pio_sm_set_pins_with_mask(this->pio, this->sm, invert_clk ? (1 << this->scl) : 0, (1 << this->scl));

    // Setup the State Machine
    pio_set_frequency(this->pio, this->sm, 2000000); // 2MHz, cycle = 0.5us
    pio_sm_init(this->pio, this->sm, this->offset, &this->cfg); // 16 = program counter after jump table
    pio_sm_set_enabled(this->pio, this->sm, true);

    /**
     * Timing Details:
     * Clock Cycle (F_SCL) = 8 pio cycles = 4us = 250KHz
     * Word Cycle = 64us = ~15.6KHz
     * Delay (Tout) = 2ms
     * Frequency (T_resp) = 2064us = ~484.5Hz
     */
};

TTP229::TTP229(uint sdo, uint scl) {
    TTP229(sdo, scl, TTPMode::MODE_16BIT, true);
};

TTP229::~TTP229() {
    pio_sm_set_enabled(this->pio, this->sm, false);
    pio_sm_unclaim(this->pio, this->sm);
    pio_remove_program(this->pio, this->prg, this->offset);
    gpio_deinit(this->sdo);
    gpio_deinit(this->scl);
};

// TODO: Use interrupts?
void TTP229::update() {
    uint32_t word, mask;
    TTPState state;
    while (!pio_sm_is_rx_fifo_empty(this->pio, this->sm)) {
        word = pio_sm_get(this->pio, this->sm);
        if (this->cb) {
            for (uint8_t i = 0; i < this->bits; i++) {
                mask = (1 << i);
                if (this->data & mask == word & mask) continue;
                this->cb(i, (word & mask) ? TTPState::PRESS : TTPState::RELEASE);
            }
        }
        this->data = word;
    }
};

void TTP229::set_callback(ttp_callback_t callback) {
    this->cb = callback;
};

void TTP229::clear_callback() {
    this->cb = NULL;
};

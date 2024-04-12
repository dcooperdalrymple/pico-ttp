#include "ttp229.hpp"
#include "pio.hpp"
#include "hardware/irq.h"

static TTP229 * ttp229_instances[NUM_PIOS] = { NULL, NULL };

static void ttp229_interrupt() {
    uint8_t i;

    if (pio0_hw->irq & 1) {
        if (ttp229_instances[0]) ttp229_instances[0]->update();
        pio0_hw->irq = 3; // Clear interrupt
    }

    if (pio1_hw->irq & 1) {
        if (ttp229_instances[1]) ttp229_instances[1]->update();
        pio1_hw->irq = 3; // Clear interrupt
    }
};

TTP229::TTP229(uint sdo, uint scl, TTPMode mode, bool invert_clk) {
    this->init(sdo, scl, mode, invert_clk);
};

TTP229::TTP229(uint sdo, uint scl) {
    this->init(sdo, scl, TTPMode::MODE_16BIT, true);
};

void TTP229::init(uint sdo, uint scl, TTPMode mode, bool invert_clk) {
    this->bits = mode == TTPMode::MODE_16BIT ? 16 : 8;
    this->previous = 0;

    this->sdo = sdo;
    this->scl = scl;

    // Load appropriate pio program
    this->prg = ttp_serial_get_program(mode, invert_clk);

    // Get available PIO and state machine index
    this->pio = pio_get_available(this->prg);
    this->sm = pio_claim_unused_sm(this->pio, true);
    this->pio_index = pio_get_index(this->pio);

    // Load program
    this->offset = pio_add_program(this->pio, this->prg);
    ttp_serial_program_get_default_config(mode, invert_clk, this->offset, &this->cfg);

    // Setup GPIO
    sm_config_set_set_pins(&this->cfg, this->scl, 1);
    pio_gpio_init(this->pio, this->scl);
    pio_sm_set_consecutive_pindirs(this->pio, this->sm, this->scl, 1, true);
    pio_sm_set_pins_with_mask(this->pio, this->sm, invert_clk ? (1 << this->scl) : 0, (1 << this->scl));

    sm_config_set_in_pins(&this->cfg, this->sdo);
    pio_gpio_init(this->pio, this->sdo);
    pio_sm_set_consecutive_pindirs(this->pio, this->sm, this->sdo, 1, false);
    gpio_pull_up(this->sdo);
    sm_config_set_in_shift(&this->cfg, true, false, this->bits);

    // Setup the State Machine
    pio_sm_init(this->pio, this->sm, this->offset, &this->cfg); // 16 = program counter after jump table
    pio_set_frequency(this->pio, this->sm, 2000000); // 2MHz, cycle = 0.5us

    /**
     * Timing Details:
     * Clock Cycle (F_SCL) = 8 pio cycles = 4us = 250KHz
     * Word Cycle = 64us = ~15.6KHz
     * Delay (Tout) = 2ms
     * Frequency (T_resp) = 2064us = ~484.5Hz
     */
};

TTP229::~TTP229() {
    this->disable();
    pio_sm_unclaim(this->pio, this->sm);
    pio_remove_program(this->pio, this->prg, this->offset);
    gpio_deinit(this->sdo);
    gpio_deinit(this->scl);
};

void TTP229::enable(bool interrupt) {
    this->disable();

    if (interrupt) {
        ttp229_instances[this->pio_index] = this;

        if (!this->pio_index) {
            irq_set_exclusive_handler(PIO0_IRQ_0, &ttp229_interrupt);
            pio0_hw->inte0 = PIO_IRQ0_INTE_SM0_BITS;
        } else {
            irq_set_exclusive_handler(PIO1_IRQ_0, &ttp229_interrupt);
            pio1_hw->inte0 = PIO_IRQ0_INTE_SM0_BITS;
        }
        irq_set_enabled(!this->pio_index ? PIO0_IRQ_0 : PIO1_IRQ_0, true);
    }

    pio_sm_set_enabled(this->pio, this->sm, true);
};

void TTP229::enable() {
    this->enable(false);
};

void TTP229::disable() {
    irq_set_enabled(!this->pio_index ? PIO0_IRQ_0 : PIO1_IRQ_0, false);
    pio_sm_set_enabled(this->pio, this->sm, false);
    pio_sm_clear_fifos(this->pio, this->sm);
    pio_sm_restart(this->pio, this->sm);
};

void TTP229::set_callback(ttp_callback_t callback) {
    this->cb = callback;
};

void TTP229::clear_callback() {
    this->cb = NULL;
};

uint32_t TTP229::get() {
    if (pio_sm_is_rx_fifo_empty(this->pio, this->sm)) return -1;
    return pio_sm_get(this->pio, this->sm) >> (32 - this->bits);
};

uint32_t TTP229::get_blocking() {
    return pio_sm_get_blocking(this->pio, this->sm) >> (32 - this->bits);
};

bool TTP229::value(uint8_t input, uint32_t data) {
    if (input > this->bits) return false;
    return data & (1 << input);
};

bool TTP229::value(uint8_t input) {
    pio_sm_clear_fifos(this->pio, this->sm);
    return this->value(input, this->get_blocking());
};

TTPState TTP229::state(uint8_t input, uint32_t current, uint32_t previous) {
    if (input > this->bits) return TTPState::ERROR;
    uint32_t mask = (1 << input);
    if ((previous & mask) == (current & mask)) return TTPState::NONE;
    return current & mask ? TTPState::PRESS : TTPState::RELEASE;
};

TTPState TTP229::state(uint8_t input) {
    uint32_t current = this->get_blocking();
    TTPState state = this->state(input, current, this->previous);
    this->previous = current;
    pio_sm_clear_fifos(this->pio, this->sm);
    return state;
};

void TTP229::update() {
    uint32_t current = this->get_blocking();
    if (this->cb) {
        TTPState state;
        for (uint8_t i = 0; i < this->bits; i++) {
            state = this->state(i, current, this->previous);
            if (state == TTPState::ERROR || state == TTPState::NONE) continue;
            this->cb(i, state == TTPState::PRESS);
        }
    }
    this->previous = current;
    pio_sm_clear_fifos(this->pio, this->sm);
};

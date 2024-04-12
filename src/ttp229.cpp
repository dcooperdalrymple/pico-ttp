#include "ttp229.hpp"
#include "pio.hpp"
#include "hardware/irq.h"

// NOTE: Max of 4 instances is arbitrary
static uint8_t ttp229_count = 0;
static TTP229 * ttp229_instances[4] = { NULL, NULL, NULL, NULL };

static void ttp229_interrupt() {
    uint8_t i;
    if (pio0_hw->irq & 1) {
        for (i = 0; i < ttp229_count; i++) {
            if (ttp229_instances[i] && !ttp229_instances[i]->get_pio_index()) ttp229_instances[i]->update();
        }
        pio0_hw->irq = 3; // Clear interrupt
    }

    if (pio1_hw->irq & 1) {
        for (i = 0; i < ttp229_count; i++) {
            if (ttp229_instances[i] && ttp229_instances[i]->get_pio_index()) ttp229_instances[i]->update();
        }
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
    if (ttp229_count < 4) ttp229_instances[ttp229_count++] = this;

    this->bits = mode == TTPMode::MODE_16BIT ? 16 : 8;
    this->data = 0;

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

    // Setup Interrupt
    // TODO: Allow multiple interrupts
    if (!this->pio_index) {
        irq_set_exclusive_handler(PIO0_IRQ_0, &ttp229_interrupt);
        pio0_hw->inte0 = PIO_IRQ0_INTE_SM0_BITS;
    } else {
        irq_set_exclusive_handler(PIO1_IRQ_0, &ttp229_interrupt);
        pio1_hw->inte0 = PIO_IRQ0_INTE_SM0_BITS;
    }

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

void TTP229::enable() {
    this->disable();
    irq_set_enabled(!this->pio_index ? PIO0_IRQ_0 : PIO0_IRQ_0, true);
    pio_sm_set_enabled(this->pio, this->sm, true);
};

void TTP229::disable() {
    irq_set_enabled(!this->pio_index ? PIO0_IRQ_0 : PIO0_IRQ_0, false);
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

void TTP229::update() {
    if (pio_sm_is_rx_fifo_empty(this->pio, this->sm)) return;
    uint32_t word = pio_sm_get(this->pio, this->sm) >> (32 - this->bits);
    if (this->cb) {
        uint32_t mask;
        for (uint8_t i = 0; i < this->bits; i++) {
            mask = (1 << i);
            if ((this->data & mask) == (word & mask)) continue;
            this->cb(i, (word & mask) ? TTPState::PRESS : TTPState::RELEASE);
        }
    }
    this->data = word;
    pio_sm_clear_fifos(this->pio, this->sm);
};

uint TTP229::get_pio_index() {
    return this->pio_index;
};

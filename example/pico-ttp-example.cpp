#include <stdio.h>
#include "pico/stdlib.h"
#include "ttp229.hpp"

#define TTP_SDO 2
#define TTP_SCL 3

// Defaults to use all 16 bits with an inverted clock signal
static TTP229 ttp(TTP_SDO, TTP_SCL);

void ttp_event(uint8_t input, TTPState state) {
    printf("%d: %s", input, state == TTPState::PRESS ? "Press" : "Release");
}

int main() {
    stdio_init_all();
    printf("Pico C/C++ TTP Library Example\n");
    ttp.set_callback(ttp_event);
    while (1) {
        ttp.update();
        sleep_ms(100);
    }
    return 0;
}

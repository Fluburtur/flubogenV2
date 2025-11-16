// Protobrain V2 for RP2040

// OSD chip might not work if powered by long/shitty USB cable, requires 5V
// On startup all GPIOs are in hi-z with a pull-down

// TODO: Current sense, auto brightness scale down if above max current
// TOOD: Interactive console to set values, save/load to txt file

// NOTE: The old MAX7456 driver assumes that the chip was loaded with a modified charset to match the standard ASCII table,
// this won't work with a brand new MAX7456.

// ADC values for automatic brightness adjustment (GL5528 photoresistor + 10k pulldown)

#include <stdint.h>

#include <pico/assert.h>
#include <pico/stdlib.h>
#include <pico/time.h>

int main(void) {
    hard_assert(stdio_init_all());

    while (1) {}
}

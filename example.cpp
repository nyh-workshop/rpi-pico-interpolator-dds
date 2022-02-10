#include <stdio.h>
#include <limits.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/interp.h"

// This example code uses the RP2040's interpolator to generate two sine waves simultaneously using DDS method.

int main()
{
    stdio_init_all();

    const uint LED_PIN = 2;
    const uint LED_PIN_1 = 3;
    gpio_init(LED_PIN);
    gpio_init(LED_PIN_1);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_set_dir(LED_PIN_1, GPIO_OUT);

     // Initialise UART 0
    uart_init(uart0, 115200);
 
    // Set the GPIO pin mux to the UART - 0 is TX, 1 is RX
    gpio_set_function(0, GPIO_FUNC_UART);
    gpio_set_function(1, GPIO_FUNC_UART);

    int16_t wavetable1024[1024];

    // Generate a 1024-point wavetable with amplitude of 162.97 (as an example):
    for (int i = 0; i < 1024; i++) {
        wavetable1024[i] = (int16_t)(162.97 * sin(((float)i / (float)(1024.0) * 2 * M_PI)));
    }

    // print N-point wavetable:
    for (int i = 0; i < 1024; i++) {
          printf("%d, ", wavetable1024[i]);
    }

    printf("Interpolator test begins!\n");
    // Interpolator example code
    interp_config cfg = interp_default_config();
    // Now use the various interpolator library functions for your use case
    // e.g. interp_config_clamp(&cfg, true);
    //      interp_config_shift(&cfg, 2);
    // Then set the config 
    // Lane 0 settings:
    interp_config_set_add_raw(&cfg, true);
    interp_config_set_cross_input(&cfg, 0);
    interp_config_set_mask(&cfg, 22, 31);
    interp_config_set_shift(&cfg, 0);
    interp_config_set_signed(&cfg, false);
    interp_set_config(interp0, 0, &cfg);
    // Lane 1 settings:
    interp_config_set_add_raw(&cfg, true);
    interp_config_set_cross_input(&cfg, 0);
    interp_config_set_mask(&cfg, 0, 9);
    interp_config_set_shift(&cfg, 22);
    interp_config_set_signed(&cfg, false);
    interp_set_config(interp0, 1, &cfg);

    // Lane 0:
    // tuning word = 42852281 - 44.1khz sampling rate, 440hz, 32-bit accumulator.
    interp_set_base(interp0, 0, 42852281);
    interp_set_accumulator(interp0, 0, 0);

    // Lane 1:
    // tuning word = 42852281*2 - 44.1khz sampling rate, 880hz, 32-bit accumulator.
    interp_set_base(interp0, 1, 42852281*2);
    interp_set_accumulator(interp0, 1, 0);

    // These lanes respectively accumulates and then packs them into the 32-bit variable at the "result2" register.
    // Only get the 10-bits from the accumulator for wave table lookup!
    // The top lane (lane 0) has a 10-bit mask from MSB onwards: 0xffc000000.
    // While the bottom lane (lane 1) shifts 22 bits, then a 10-bit mask: 0x000003ff.
    // Therefore, pop both lanes, we get the calculation immediately into the result2.
    // Only thing manually needs doing is to unpack that 32-bit result2 and put it into the wavetable array.
    printf("printing the interpolator results!\n");
    for(int i = 0; i < 128; i++) {
        _result2 = interp_pop_full_result(interp0);
        
        //printf("result0: %08x, result1: %08x, result2: %08x\n", interp_peek_lane_result(interp0, 0),
        //interp_peek_lane_result(interp0, 1), _result2);
        
        // Prints the sine table result from lane 0:
        printf("%d, ", wavetable1024[_result2 >> 22]);
        // and then the second sine table result from lane 1:
        printf("%d, ", wavetable1024[_result2 & 0x000003ff]);
    }

    while(1) {
        gpio_put(LED_PIN,1);
        sleep_ms(250);
        gpio_put(LED_PIN,0);
        sleep_ms(250);
    }
}

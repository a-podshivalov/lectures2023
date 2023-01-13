#include <stdio.h>

#include "board.h"
#include "periph/gpio.h"
#include "xtimer.h"

int main(void) {
    puts("Hello, world!");
    gpio_init(LED0_PIN, GPIO_OUT);
    while(1) {
        xtimer_msleep(500);
        gpio_set(LED0_PIN);
        xtimer_msleep(500);
        gpio_clear(LED0_PIN);
    }

    return 0;
}

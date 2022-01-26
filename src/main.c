#include "picos.h"

#include "pico/stdlib.h"
#include <stdio.h>

PICOS_STACK(test, 128);
void test() {
    for (;;) {
    }
}

PICOS_STACK(test2, 128);
void test2() {
    for (;;) {
    }
}

int main() {
    stdio_init_all();

    picos_init();

    PICOS_THREAD(test);
    PICOS_THREAD(test2);

    picos_start();
}
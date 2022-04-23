#include "picos.h"

#include "pico/stdlib.h"
#include <stdio.h>

PICOS_STACK(test, 128);
void test() {
    volatile uint8_t i = 0;
    for (;;) {
        i++;
    }
}

PICOS_STACK(test2, 128);
void test2() {
    // Example from: https://wiki.segger.com/Cortex-M_Fault#Illegal_Memory_Write
    int r = 0;
    volatile unsigned int *p = (unsigned int *)0x00100000;

    *p = 0x00BADA55;

    for (;;) {
    }
}

PICOS_STACK(test3, 128);
void test3() {
    volatile uint8_t i = 0;
    for (;;) {
        i++;
    }
}

int main() {
    stdio_init_all();

    picos_init();

    PICOS_THREAD(test);
    PICOS_THREAD(test2);
    PICOS_THREAD(test3);

    picos_start();
}
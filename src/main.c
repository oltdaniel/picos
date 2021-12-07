#include "picos.h"

PICOS_STACK(test2, 128);
void test2() { return; }

PICOS_STACK(test, 128);
void test() {
    PICOS_THREAD(test2);
    return;
}

int main() {
    picos_init();

    PICOS_THREAD(test);

    picos_start();
}
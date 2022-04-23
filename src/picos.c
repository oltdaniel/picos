#include "picos.h"

#define PICOS_IDLE_STACK_SIZE 100

// Define the declared variables from the header
picos_thread_t picos_threads[PICOS_MAX_THREADS];
picos_thread_t *picos_current[PICOS_CORES];

// Declare functions that will be later defined
void picos_setup_idle();
void picos_suicide();
void picos_scheduler_main();

// Functions from our assembler code
void picos_exec_stack(uint32_t sp);
void picos_set_psp(uint32_t sp, uint32_t ctrl);

void picos_init() {
#ifndef PICOS_NO_LED
    for (uint8_t i = PICOS_LED_START; i < PICOS_LED_START + 4; i++) {
        gpio_init(i);
        gpio_set_dir(i, GPIO_OUT);
    }
#endif
    picos_setup_idle();
}

picos_pid picos_exec(picos_thread_func func, picos_thread_stack_t *s) {
    picos_pid thread_slot;

    spin_lock_blocking(PICOS_SCHEDULE_SPINLOCK);

    // find free slot
    for(thread_slot = PICOS_CORES; thread_slot < PICOS_MAX_THREADS; thread_slot++) {
        if(picos_threads[thread_slot].state == PICOS_UNKNOWN) break;
    }

    // check if we found a free slot
    if(thread_slot > PICOS_MAX_THREADS - 1) {
        spin_unlock_unsafe(PICOS_SCHEDULE_SPINLOCK);
        return PICOS_INVALID_PID;
    }

    // initialize free slot
    picos_thread_t *slot = &picos_threads[thread_slot];
    slot->pid = thread_slot;
    slot->cpu = 0xFF;
    // calculate given address to a stack address (switch direction)
    slot->sp = (uint32_t)(s->data + s->size - 16);
    s->data[s->size - 1] =
        0x01000000; // This will set the T-Bit in the EPSR Part of the xPSR
                    // Register. As mentioned in the Armv6m Datasheet, the
                    // architecture only supports thumb instruction mode. So
                    // we need to maintain this bit always wit hthe value 1.
    s->data[s->size - 2] = (uint32_t)func; // The PC position to start exectuing
    s->data[s->size - 3] =
        (uint32_t)&picos_suicide; // if the actual thread function should exit,
                                  // this is the function to call next.
    slot->state = PICOS_RUNNING;

    spin_unlock_unsafe(PICOS_SCHEDULE_SPINLOCK);

    return thread_slot;
}

void picos_start() {
    // launch scheduler on second core
    multicore_launch_core1(picos_scheduler_main);
    // launch scheduler on this core
    picos_scheduler_main();

    // make sure we never return to main
    for (;;)
        ;
}

void isr_systick() {
    // Trigger the in assembly defined isr_pendsv via interrupt
    *(volatile uint32_t *)(0xe0000000 | M0PLUS_ICSR_OFFSET) = (1L << 28);
}

void isr_hardfault() {
    uint8_t cpu = *(uint32_t *)(SIO_BASE);

    // Update state to hardfault
    picos_thread_t *current = picos_current[cpu];
    current->state = PICOS_HARDFAULT;

    // Trigger scheduling routine (will store context)
    isr_systick();

    for(;;)
        ;
}

void picos_schedule() {
    uint8_t cpu = *(uint32_t *)(SIO_BASE);
    picos_thread_t *current = picos_current[cpu];

#ifndef PICOS_NO_LED
    // toggle led status
    gpio_put(PICOS_LED_START + cpu, !gpio_get(PICOS_LED_START + cpu));
#endif

    // entering exclusive section
    spin_lock_blocking(PICOS_SCHEDULE_SPINLOCK);

    // check if there is actually a thread we could shedule
    for (picos_pid i = PICOS_CORES; i < PICOS_MAX_THREADS; i++) {
        picos_thread_t *c = &picos_threads[i];
        if (c->state == PICOS_RUNNING && (c->cpu == cpu || c->cpu == 0xFF)) {
            goto selectNext;
        }
    }

    goto selectIdle;

selectNext:;
    // select a next thread to execute and assign it
    for (picos_pid i = 1; i < PICOS_MAX_THREADS; i++) {
        picos_thread_t *c = &picos_threads[(((current->pid - PICOS_CORES) + i) %
                                            PICOS_USER_THREADS) +
                                           PICOS_CORES];
        if (c->state == PICOS_RUNNING && (c->cpu == cpu || c->cpu == 0xFF)) {
            picos_current[cpu] = c;
            if (c->cpu == 0xFF) {
                c->cpu = cpu;
            }
            // found a new thread, stop searching
            break;
        }
    }

#ifndef PICOS_NO_LED
    // turn off idle led
    gpio_put(PICOS_LED_START + PICOS_CORES + cpu, 0);
#endif

    goto end;

selectIdle:;
#ifndef PICOS_NO_LED
    // turn on idle led
    gpio_put(PICOS_LED_START + PICOS_CORES + cpu, 1);
#endif
    // assign the idle process if nothing there to continue
    picos_current[cpu] = &picos_threads[cpu];

end:;
    spin_unlock_unsafe(PICOS_SCHEDULE_SPINLOCK);
}

// idle structure
PICOS_STACK(idle0, PICOS_IDLE_STACK_SIZE)
PICOS_STACK(idle1, PICOS_IDLE_STACK_SIZE)
void picos_idle() {
    for (;;)
        asm("wfi"); // wait for the next interrupt
}

static picos_thread_stack_t *picos_idle_stack[PICOS_CORES] = {
    &picos_stack_idle0, &picos_stack_idle1};

void picos_setup_idle() {
    for (uint8_t i = 0; i < PICOS_CORES; i++) {
        picos_thread_t *t = &picos_threads[i];
        picos_thread_stack_t *s = picos_idle_stack[i];
        t->pid = i;
        t->cpu = i;
        t->state = PICOS_RUNNING;

        // Setting the correct stack data and position
        s->data[s->size - 1] =
            0x01000000; // This will set the T-Bit in the EPSR Part of the xPSR
                        // Register. As mentioned in the Armv6m Datasheet, the
                        // architecture only supports thumb instruction mode. So
                        // we need to maintain this bit always wit hthe value 1.
        s->data[s->size - 2] =
            (uint32_t)picos_idle; // The PC position to start exectuing
        // NOTE: We never expect the idle process to finish. So keep that in
        // mind, if that is not the case the controller will eventually crash.

        // calculate given address to a stack address (switch direction)
        t->sp = (uint32_t)(s->data + s->size - 16);

        picos_current[i] = t;
    }
}

void picos_suicide() {
    uint8_t cpu = *(uint32_t *)(SIO_BASE);

    spin_lock_blocking(PICOS_SCHEDULE_SPINLOCK);

    // Cleanup the thread slot
    picos_thread_t *current = picos_current[cpu];
    current->state = PICOS_UNKNOWN;
    current->pid = 0;
    current->sp = 0;
    current->cpu = 0xFF;

    spin_unlock_unsafe(PICOS_SCHEDULE_SPINLOCK);

    // avoid continiung with an return which would cause a crash if we return to
    // here
    for (;;)
        ;
}

void picos_scheduler_main() {
    uint8_t cpu = *(uint32_t *)(SIO_BASE);

    // This will set the wanted counter value which defines the delays between
    // scheduler calls
    *(volatile unsigned int *)(0xe0000000 | M0PLUS_SYST_RVR_OFFSET) =
        (clock_get_hz(clk_sys) / 1000000) *
        PICOS_SCHEDULER_INTERVAL_US; // the counter value to set in CSR when 0
                                     // is eached
    // This will configure the systick timer so we have the behavior required
    // for scheduling
    *(volatile unsigned int *)(0xe0000000 | M0PLUS_SYST_CSR_OFFSET) =
        (1 << 0)    // enable counter
        | (1 << 1)  // counter at 0 causes systick exception status pending
        | (1 << 2); // use processor clock
    // This will change the priority of the system handlers we utilize for our
    // scheduler calls
    *(volatile unsigned int *)(0xe0000000 | M0PLUS_SHPR3_OFFSET) =
        (0 << 30) |
        (3 << 22); // priority systick=0(high), priority pendsv=3(low)

    // Start execution of the initial process (here idle process)
    picos_exec_stack(picos_current[cpu]->sp);
}

void picos_enter_critical() {
    // This will disable the systick counter
    *(volatile unsigned int *)(0xe0000000 | M0PLUS_SYST_CSR_OFFSET) &=
        ~(1 << 0);
}

void picos_leave_critical() {
    // This will enable the systick counter
    *(volatile unsigned int *)(0xe0000000 | M0PLUS_SYST_CSR_OFFSET) |= (1 << 0);
}
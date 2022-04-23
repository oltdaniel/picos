/**
 * @file picos.h
 * @author oltdaniel
 * @brief All relevant PicOS functions and types.
 *
 * @copyright Copyright (c) 2021
 *
 */

#pragma once

#include "hardware/clocks.h"
#include "hardware/irq.h"
#include "hardware/sync.h"
#include "hardware/timer.h"
#include "pico/multicore.h"
#include "pico/stdlib.h"

/**
 * @brief Number of cores the RP2040 Chip has.
 */
#define PICOS_CORES 2

/**
 * @brief Maximum number of threads a user can execute at once.
 */
#define PICOS_USER_THREADS 8

/**
 * @brief Maximum number of total threads.
 *
 * Each core will have its own idle process, which means we need to add
 * #PICOS_CORES to the number of threads the user can execute
 * #PICOS_USER_THREADS.
 */
#define PICOS_MAX_THREADS (PICOS_CORES + PICOS_USER_THREADS)

#ifndef PICOS_NO_LED
#ifndef PICOS_LED_START
/**
 * @brief The first GPIO pin of the LED pins for visualization.
 *
 * PicOS will use LEDs to visualize the activity. We use a total of 4 LEDs (2
 * for both core schedulers + 2 for each idle process). The default Raspberry PI
 * Pico board has a nice Pinout which allows multiple combinations of activity
 * LEDs next to each other. Note: These pins will be overwritten with with
 * #picos_init and should not be used by any other functionality.
 */
#define PICOS_LED_START 2
#endif
#endif

/**
 * @brief Create a new stack with name and size.
 * @param NAME Needs to be unique to the scope. Generally the name of the
 * #picos_thread_func is a good starting point.
 * @param SIZE Stack size. For a simple thread, 128 is a good starting point.
 *
 * This macro will create an data space in `picos_stack_NAME_data` and reference
 * this in an #picos_thread_stack_t and SIZE in `picos_stack_NAME`.
 */
#define PICOS_STACK(NAME, SIZE)                                                \
    static uint32_t picos_stack_##NAME##_data[SIZE];                           \
    static picos_thread_stack_t picos_stack_##NAME = {                         \
        .data = picos_stack_##NAME##_data, .size = SIZE};

/**
 * @brief Will call #picos_exec with NAME as #picos_thread_func and
 * `picos_stack_NAME` as the stack.
 */
#define PICOS_THREAD(NAME) picos_exec(NAME, &picos_stack_##NAME);

/**
 * @brief Return PID when the there was an execution error.
 */
#define PICOS_INVALID_PID UINT8_MAX

/**
 * @brief Spinlock used to lock both CPUs for critical work.
 *
 * If we are in a schedule process, we want to exclusively have access to the
 * #picos_threads and #picos_current to make changes.
 */
#define PICOS_SCHEDULE_SPINLOCK ((spin_lock_t *) (SIO_BASE + SIO_SPINLOCK0_OFFSET + PICO_SPINLOCK_ID_OS1 * 4))

/**
 * @brief How many µs do we want to wait between scheduler calls
 */
#define PICOS_SCHEDULER_INTERVAL_US 10000

/**
 * @brief Process ID type used in the PicOS.
 *
 * As this is a micrcontroller and we need to allocate the full array of
 * possible threads anyway, we can keep the number low.
 */
typedef uint8_t picos_pid;

/**
 * @brief Different states that the threads can have during their lifetime.
 *
 * Generally, a thread will be initialized with the #PICOS_RUNNING state. If a
 * thread has ended, it will get assigned the #PICOS_DONE state.
 */
typedef enum picos_thread_state_t {
    /**
     * @brief Describes an never used slot.
     */
    PICOS_UNKNOWN,
    /**
     * @brief Describes a thread that is ready to run or already in a running
     * state.
     *
     * This state is assigned to every process that has been newly created or is
     * currently running. Based on this state, a thread will be selected to be
     * scheduled.
     */
    PICOS_RUNNING,
    /**
     * @brief Describes a thread that had an hardfault. Information will be preserved.
     *
     * If a process produces and hardfault, it will receive this process state and the
     * scheduler will receive a new call and switch to a new process. If no new process
     * can be found, the idle process will kick in. 
     */
    PICOS_HARDFAULT
} picos_thread_state_t;

/**
 * @brief Describes a thread in the PicOS.
 *
 */
typedef struct picos_thread_t {
    /**
     * @brief The current Stackpointer of this specific thread.
     *
     * This needs to be at the first position, as the scheduler will use this
     * pointer to jump back to the work position of the thread. If we would have
     * this at another position, we would be required to add a specific offset
     * to the sheduler which would make things a bit complicated.
     */
    uint32_t sp;
    /**
     * @brief The assigned CPU of this thread.
     *
     * If the thread hasn't been claimed by any core yet, the default value is
     * UINT8_MAX.
     */
    uint8_t cpu;
    /**
     * @brief The process id of this thread.
     *
     * This PID also describes the index in the #picos_threads
     * array.
     */
    picos_pid pid;
    /**
     * @brief The state currently assigned to this thread.
     */
    picos_thread_state_t state;
} picos_thread_t;

/**
 * @brief Describe a thread stack with data and specified size.
 *
 * Each thread will require its own stack space. In order to execute a specific
 * thread, you will need to pass an initialized struct to the #picos_exec
 * function. You can simply use #PICOS_STACK to initialized such a thing.
 */
typedef struct picos_thread_stack_t {
    uint32_t *data;
    size_t size;
} picos_thread_stack_t;

/**
 * @brief The function that will be executed at the first schedule of the
 * thread.
 */
typedef void (*picos_thread_func)(void *);

/**
 * @brief Index of all threads managed by PicOS.
 *
 * The #picos_thread_t.pid of each thread will be equal to its index in this
 * array. Do not change any of the values in here, and always use the exposed
 * PicOS functions, except for when you exactly know what you are doing.
 */
extern picos_thread_t picos_threads[PICOS_MAX_THREADS];

/**
 * @brief The threads that are currently assigned to each core.
 *
 * Do not change any of the values in here, and always use the exposed PicOS
 * functions, except for when you exactly know what you are doing.
 */
extern picos_thread_t *picos_current[PICOS_CORES];

/**
 * @brief Initialized the PicOS.
 *
 * This will prepare the activity LEDs, if not disabled, and create two idle
 * processes (one for each core).
 */
void picos_init();

/**
 * @brief Registers a new thread in a free slot.
 *
 * @param func Function to initially call for the thread execution.
 * @param stack Allocated stack space of this thread.
 * @return picos_pid The assigned PID for the new thread.
 */
picos_pid picos_exec(picos_thread_func func, picos_thread_stack_t *stack);

/**
 * @brief Start the PicOS on both cores.
 *
 * This function won't return to the point where it has been called, as the
 * current execution in both cores will be replaced by the idle threads and
 * interrupted by the scheduler.
 */
void picos_start();

/**
 * @brief Block the scheduler from firing.
 *
 * If you have critical behavior that should not be interrupted by the
 * scheduler, the scheduler can be temporarly disabled with this call. We simply
 * disable the countdown that fires the scheduler if counted down.
 */
void picos_enter_critical();

/**
 * @brief Unblock the scheduler from firing.
 *
 * This will redo the changes done by the enter_critcial function.
 */
void picos_leave_critical();
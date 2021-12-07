.syntax unified
.cpu cortex-m0
.fpu softvfp

.extern picos_schedule

.thumb

/**
 * The pico-sdk already registered the vector table. If we declare a function
 * with a given vector table function name, this will be used instead of the
 * sdk default function.
 */
.global isr_pendsv
.type isr_pendsv, %function
isr_pendsv:
    // We will be automatically using the MSP here
    cpsid i // Disable interrupts to avoid interference
    mov r3, sp // Save the msp to jump back
    // Now we will make sure to save the current context to the process stack
    mrs r2, psp // Read the current process stack pointer
    mov sp, r2 // Make sure the stack pointer has the same value
    // Save the current context
    // * r0-r3 are not preserved across call
    // * r0-r7 are only supported by the push operation
    push {r4-r7} // Save r4-r7
    mov r4, r8
    mov r5, r9
    mov r6, r10
    mov r7, r11
    push {r4-r7} // Save r8-r11
    // Save the current process stack pointer to the process stack of the current process of this core
    mov r2, sp // Read the current stack position
    ldr r1, =#0xd0000000 // Go to CPU offset position
    ldr r1, [r1] // Read the core number to r1
    lsls r1, r1, #2 // Multiply r1 by 4 for the correct array offset
    ldr r0, =picos_current // Get the current process array address
    ldr r0, [r0, r1] // Get the current process in picos_threads from picos_current by the calculated offset
    str r2, [r0] // As the sp of the process lives at the firs tstruct position ,we can directly write to it
    // Process stuff done. Now back to the kernel.
    msr msp, r3 // Jump back to the main process  stack saved from earlier
    ldr r0, =picos_schedule_done // Read the position of the function after the schedule call
    mov lr, r0 // Set the return link for the call to the scheduler
    bl picos_schedule
picos_schedule_done:
    // We will be still using the MSP here
    mov r3, sp // Store the current main stack pointer
    // Read the current process stack pointer of this core
    ldr r1, =#0xd0000000 // Go to CPU offset position
    ldr r1, [r1] // Read the core number to r1
    lsls r1, r1, #2 // Multiply r1 by 4 for the correct array offset
    ldr r0, =picos_current // Get the current process array address
    ldr r0, [r0, r1] // Get the current process in picos_threads from picos_current by the calculated offset
    ldr r2, [r0] // As the sp of the process lives at the firs struct position, we can directly read ir from there
    mov sp, r2 // Jump to the process stack
    // Restore the process context from its stack
    pop {r4-r7} // This will return us the pushed r8-r11 registers
    mov r8, r4
    mov r9, r5
    mov r10, r6
    mov r11, r7
    pop {r4-r7} // Read the actual r4-r7
    // Update the process stack to the current stack position
    mov r1, sp
    msr psp, r1
    // Restore the main process stack saved from earlier
    mov sp, r3
    // Finalize interrupt call
    ldr r0, =0xfffffffd // EXEC_RETURN for returing back to the thread mode with process stack use
    cpsie i // Enable interrupts again
    bx r0 // Branch to target

/**
 * This function will execute a given thread by the passed stack pointer, but
 * ignores its context. Additionally we move into privileged thread mode.
 */
.global picos_exec_stack
.type picos_exec_stack, %function
picos_exec_stack:
    cpsid i // Disable interrupts to avoid interference
    // Put the given process stack into pivledged thread mode
    // * will utilize the process stack pointer
    // * will run in privledged thread mode
    ldr r1, =2 // nPriv = 0 (privileged access); SPSEL = 1 (use PSP as SP)
    msr control, r1 // put config into control register
    msr psp, r0 // put stack pointer into process stack pointer
    mov sp, r0  // make sure the stack pointer is equal to the process stack pointer
    isb // make sure the control register change is flushed
    // We need to read the "fake" context from the process
    // * we will simply read the context but ignore the values
    pop {r4-r7} // ignore read registers
    pop {r4-r7} // ignore read registers
    pop {r0-r3} // ignore read registers
    pop {r4-r5} // ignore read registers
    pop {r3} // Read the thread function address to jump to
    mov lr, r3 // Set the return link for the call to the thread function
    pop {r3} // ignore read register
    // Finalize call
    cpsie i // Enable interrupts again
    bx lr // Branch to target
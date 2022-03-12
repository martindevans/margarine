#include "picosystem.hpp"

#include "pico/stdlib.h"
#include "pico/multicore.h"

void core1_entry()
{
    while (true)
    {
        // Function pointer is passed to us via the FIFO
        // We have one incoming int32_t as a parameter, and will provide an
        // int32_t return value by simply pushing it back on the FIFO
        // which also indicates the result is ready.

        // Pop a function pointer from FIFO
        int32_t (*func)(int32_t) = (int32_t(*)(int32_t))multicore_fifo_pop_blocking();

        // Pop a single integer parameter from FIFO
        int32_t p = multicore_fifo_pop_blocking();

        // Execute function pointer with parameter
        int32_t result = (*func)(p);

        // Push integer result
        multicore_fifo_push_blocking(result);
    }
}

void launch_multicore()
{
    multicore_launch_core1(core1_entry);
}

void start_multicore_work(int32_t (*function)(int32_t), int32_t parameter)
{   
    multicore_fifo_push_blocking((uintptr_t)function);
    multicore_fifo_push_blocking(parameter);
}

int32_t end_multicore_work_blocking()
{
    return multicore_fifo_pop_blocking();
}
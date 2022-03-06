#include "picosystem.hpp"

#include "profiler.h"

#ifndef ENABLE_PROFILER

    int32_t profiler_value;

    void profiler_init()
    {
    }

    void profiler_clear()
    {
    }

    int32_t* profiler_get(ProfilerValue value)
    {
        return &profiler_value;
    }

#else

    static int32_t* profiler_values;

    void profiler_init()
    {
        profiler_values = (int32_t*)malloc((int)ProfilerValue_Max);
    }

    void profiler_clear()
    {
        for (int i = 0; i < ProfilerValue_Max; i++)
            profiler_values[i] = 0;
    }

    int32_t* profiler_get(ProfilerValue value)
    {
        return &profiler_values[int(value)];
    }

#endif
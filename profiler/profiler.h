#pragma once

enum ProfilerValue
{
    ProfilerValue_InitTime,
    ProfilerValue_PaintedWallPixels,

    // Add all new values **before** this one!
    ProfilerValue_Max
};

void profiler_init();

void profiler_clear();

int32_t* profiler_get(ProfilerValue value);

#if ENABLE_PROFILER
    #define PROFILER_EMIT(NAME, VALUE) \
        *profiler_get((NAME))=(VALUE);
#else
    #define PROFILER_EMIT(NAME, VALUE);
#endif

#if ENABLE_PROFILER
    #define PROFILER_ADD(NAME, VALUE) \
        *profiler_get((NAME))+=(VALUE);
#else
    #define PROFILER_ADD(NAME, VALUE);
#endif

#if ENABLE_PROFILER
    #define PROFILER_BEGIN_TIMING(NAME) \
        int32_t PROFILER_TIMER_TEMP_BEGIN_##NAME=time_us();
#else
    #define PROFILER_BEGIN_TIMING(NAME);
#endif

#if ENABLE_PROFILER
    #define PROFILER_END_TIMING(NAME) \
        *profiler_get((NAME))=PROFILER_TIMER_TEMP_BEGIN_##NAME;
#else
    #define PROFILER_END_TIMING(NAME);
#endif
#pragma once

#include "pico/stdlib.h"

void launch_multicore();

void start_multicore_work(int32_t (*function)(int32_t), int32_t parameter);

int32_t end_multicore_work_blocking();
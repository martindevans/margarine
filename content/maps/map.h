#pragma once

#include "pico/stdlib.h"

#include "../../render/texture.h"

typedef struct map
{
    uint8_t map_width;
    uint8_t map_height;
    uint8_t *wall_map;
} map_t;
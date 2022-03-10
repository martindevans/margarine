#pragma once

#include "camera.h"
#include "texture.h"
#include "../content/maps/map.h"

typedef struct dda_in
{
    int32_t w;
    int32_t h;
} dda_in_t;

typedef struct dda_out
{
    uint8_t wall_x;
    uint8_t wall_y;
    uint8_t wall_type;
    uint8_t side;
    float lineHeight;

    // u texture coord (0 -> 8192)
    uint16_t texture_coord;

    // Distance from camera to wall * 8192
    int32_t depth;
} dda_out_t;

dda_out_t dda(int xpos, dda_in_t *dda_in, camera_state_t *cam_in, map_t *map);

void render_walls_in_range(int min_x, int max_x, camera_state_t *cam_state, map_t *map, texture_mipmap **wall_textures, uint8_t *out_wall_heights, int32_t *out_wall_depths);
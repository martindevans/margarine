#pragma once

#include "texture.h"

typedef struct camera_state
{
    float posX;
    float posY;
    float dirX;
    float dirY;
    float planeX;
    float planeY;
} camera_state_t;

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
    float texture_coord;
} dda_out_t;

dda_out_t dda(int xpos, dda_in_t *dda_in, camera_state_t *cam_in, uint8_t *map, uint8_t map_width);

void render_walls_in_range(int min_x, int x_range, camera_state_t *cam_state, uint8_t* worldMap, int mapWidth, texture_mipmap **wall_textures);
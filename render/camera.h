#pragma once

#include "../content/maps/map.h"

typedef struct camera_state
{
    float posX;
    float posY;
    float dirX;
    float dirY;
    float planeX;
    float planeY;
} camera_state_t;

void rotate(camera_state_t *camera, float amount);

void move(camera_state_t *camera, map_t *map, float amount);
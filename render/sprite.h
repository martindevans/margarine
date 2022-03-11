#pragma once

#include "camera.h"

typedef struct sprite3d
{
    float posX;
    float posY;
    int texture;

    float distanceSqr;
} sprite3d_t;

void sort_sprites(camera_state_t *camera, sprite3d_t *sprites, int count);

/// Draw a list of sprites, must be presorted with sort_sprites
/// - camera: ptr to camera
/// - sprites: ptr to sprite
/// - count: number of items in `sprites` ptr to draw
/// - wall_depths: depth_buffer (wall distance * 8192)
void render_sprites(camera_state_t *camera, sprite3d_t *sprites, int count, int32_t *wall_depths);
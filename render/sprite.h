#pragma once

typedef struct sprite3d
{
    float posX;
    float posY;
    int texture;

    float distanceSqr;
} sprite3d_t;

void sort_sprites(sprite3d_t *sprites, int count);
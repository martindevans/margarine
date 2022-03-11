#include "pico/float.h"

#include "camera.h"

inline void rot(float* x, float* y, float cosrs, float sinrs)
{
    float old_x = *x;
    *x = *x * cosrs - *y * sinrs;
    *y = old_x * sinrs + *y * cosrs;
}

void rotate(camera_state_t *camera, float amount)
{
    float cosrs = cos(amount);
    float sinrs = sin(amount);

    rot(&camera->dirX, &camera->dirY, cosrs, sinrs);
    rot(&camera->planeX, &camera->planeY, cosrs, sinrs);
}

uint8_t sample_world_map(map_t *map, float x, float y)
{
    return map->wall_map[int(x) + int(y) * map->map_width];
}

void move(camera_state_t *camera, map_t *map, float amount)
{
    if (sample_world_map(map, camera->posX + camera->dirX * amount, camera->posY) == 0)
        camera->posX += camera->dirX * amount;
    if (sample_world_map(map, camera->posX, camera->posY + camera->dirY * amount) == 0)
        camera->posY += camera->dirY * amount;
}
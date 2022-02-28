#include <cmath>

#include "picosystem.hpp"

#include "pico/float.h"
#include "hardware/dma.h"
#include "hardware/interp.h"

#include "render/dda.h"
#include "render/texture_mapping.h"
#include "render/hud.h"

using namespace picosystem;

#define mapWidth 24
#define mapHeight 24
uint8_t worldMap2[] =
{
  4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,7,7,7,7,7,7,7,
  4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,7,0,0,0,0,0,0,7,
  4,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,7,
  4,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,7,
  4,0,3,0,0,0,0,0,0,0,0,0,0,0,0,0,7,0,0,0,0,0,0,7,
  4,0,4,0,0,0,0,5,5,5,5,5,5,5,5,5,7,7,0,7,7,7,7,7,
  4,0,5,0,0,0,0,5,0,5,0,5,0,5,0,5,7,0,0,0,7,7,7,1,
  4,0,6,0,0,0,0,5,0,0,0,0,0,0,0,5,7,0,0,0,0,0,0,8,
  4,0,7,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,7,7,7,1,
  4,0,8,0,0,0,0,5,0,0,0,0,0,0,0,5,7,0,0,0,0,0,0,8,
  4,0,0,0,0,0,0,5,0,0,0,0,0,0,0,5,7,0,0,0,7,7,7,1,
  4,0,0,0,0,0,0,5,5,5,5,0,5,5,5,5,7,7,7,7,7,7,7,1,
  6,6,6,6,6,6,6,6,6,6,6,0,6,6,6,6,6,6,6,6,6,6,6,6,
  8,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,
  6,6,6,6,6,6,0,6,6,6,6,0,6,6,6,6,6,6,6,6,6,6,6,6,
  4,4,4,4,4,4,0,4,4,4,6,0,6,2,2,2,2,2,2,2,3,3,3,3,
  4,0,0,0,0,0,0,0,0,4,6,0,6,2,0,0,0,0,0,2,0,0,0,2,
  4,0,0,0,0,0,0,0,0,0,0,0,6,2,0,0,5,0,0,2,0,0,0,2,
  4,0,0,0,0,0,0,0,0,4,6,0,6,2,0,0,0,0,0,2,2,0,2,2,
  4,0,6,0,6,0,0,0,0,4,6,0,0,0,0,0,5,0,0,0,0,0,0,2,
  4,0,0,5,0,0,0,0,0,4,6,0,6,2,0,0,0,0,0,2,2,0,2,2,
  4,0,6,0,6,0,0,0,0,4,6,0,6,2,0,0,5,0,0,2,0,0,0,2,
  4,0,0,0,0,0,0,0,0,4,6,0,6,2,0,0,0,0,0,2,0,0,0,2,
  4,4,4,4,4,4,4,4,4,4,1,1,1,2,2,2,2,2,2,3,3,3,3,3
};

color_t worldCol[] = {
    0,
    rgb(13, 0, 0),
    rgb(0, 13, 0),
    rgb(0, 0, 13),
    rgb(13, 13, 13),
    rgb(13, 13, 0),
    rgb(0, 13, 13),
    rgb(13, 0, 13),
    rgb(9, 0, 13),
};

#define texWidth 32
#define texHeight 32
uint16_t texture[texWidth * texHeight];

camera_state_t cam_state = 
{
    .posX = 4,
    .posY = 4,
    .dirX = -1,
    .dirY = 0,
    .planeX = 0,
    .planeY = 0.66,
};

color_t bg = 0;

void init()
{
    // generate floor texture
    for(int x = 0; x < texWidth; x++)
    for(int y = 0; y < texHeight; y++)
        texture[x + y * texWidth] = rgb(x<(texWidth/2) ? 15 : 3, y<(texHeight/2) ? 15 : 3, 4);
}

uint8_t sample_world_map(int x, int y)
{
    return worldMap2[x + y * mapWidth];
}

void __time_critical_func(update)(uint32_t tick)
{
    float rotSpeed = _PI * 0.8 / 40.0;

    if(button(RIGHT))
    {
        float cosrs = cos(-rotSpeed);
        float sinrs = sin(-rotSpeed);

        //both camera direction and camera plane must be rotated
        float oldDirX = cam_state.dirX;
        cam_state.dirX = cam_state.dirX * cosrs - cam_state.dirY * sinrs;
        cam_state.dirY = oldDirX * sinrs + cam_state.dirY * cosrs;
        float oldPlaneX = cam_state.planeX;
        cam_state.planeX = cam_state.planeX * cosrs - cam_state.planeY * sinrs;
        cam_state.planeY = oldPlaneX * sinrs + cam_state.planeY * cosrs;
    }

    if(button(LEFT))
    {
        float cosrs = cos(rotSpeed);
        float sinrs = sin(rotSpeed);

        float oldDirX = cam_state.dirX;
        cam_state.dirX = cam_state.dirX * cosrs - cam_state.dirY * sinrs;
        cam_state.dirY = oldDirX * sinrs + cam_state.dirY * cosrs;
        float oldPlaneX = cam_state.planeX;
        cam_state.planeX = cam_state.planeX * cosrs - cam_state.planeY * sinrs;
        cam_state.planeY = oldPlaneX * sinrs + cam_state.planeY * cosrs;
    }

    float moveSpeed = 2 * 1.0 / 40.0;

    if (button(UP))
    {
        if (sample_world_map(int(cam_state.posX + cam_state.dirX * moveSpeed), int(cam_state.posY)) == 0)
            cam_state.posX += cam_state.dirX * moveSpeed;
        if (sample_world_map(int(cam_state.posX), int(cam_state.posY + cam_state.dirY * moveSpeed)) == 0)
            cam_state.posY += cam_state.dirY * moveSpeed;
    }

    if (button(DOWN))
    {
        if(sample_world_map(int(cam_state.posX - cam_state.dirX * moveSpeed), int(cam_state.posY)) == 0)
            cam_state.posX -= cam_state.dirX * moveSpeed;
        if(sample_world_map(int(cam_state.posX), int(cam_state.posY - cam_state.dirY * moveSpeed)) == 0)
            cam_state.posY -= cam_state.dirY * moveSpeed;
    }
}

void __time_critical_func(draw_walls)()
{
    render_walls_in_range(0, 120, &cam_state, worldMap2, mapWidth, &worldCol[0]);
    render_walls_in_range(120, 240, &cam_state, worldMap2, mapWidth, &worldCol[0]);
}

void __time_critical_func(draw_floor)()
{
    uint16_t w = _dt->w;
    uint16_t h = _dt->h;
    uint16_t half_h = h / 2;

    // rayDir for leftmost ray (x = 0) and rightmost ray (x = w)
    float rayDirX0 = cam_state.dirX - cam_state.planeX;
    float rayDirY0 = cam_state.dirY - cam_state.planeY;
    float rayDirX1 = cam_state.dirX + cam_state.planeX;
    float rayDirY1 = cam_state.dirY + cam_state.planeY;

    for (int y = 0; y < half_h; y++)
    {
        // Current y position compared to the center of the screen (the horizon)
        int p = y - half_h;

        // Vertical position of the camera.
        float posZ = 1 - 0.5 * h;

        // Horizontal distance from the camera to the floor for the current row.
        // 0.5 is the z position exactly in the middle between floor and ceiling.
        float rowDistance = posZ / p;

        // calculate the real world step vector we have to add for each x (parallel to camera plane)
        // adding step by step avoids multiplications with a weight in the inner loop
        float floorStepX = rowDistance * (rayDirX1 - rayDirX0) / w;
        float floorStepY = rowDistance * (rayDirY1 - rayDirY0) / w;

        // real world coordinates of the leftmost column. This will be updated as we step to the right.
        float floorX = cam_state.posX + rowDistance * rayDirX0;
        float floorY = cam_state.posY + rowDistance * rayDirY0;

        for(int x = 0; x < w; ++x)
        {
            // the cell coord is simply got from the integer parts of floorX and floorY
            int cellX = int(floorX);
            int cellY = int(floorY);

            // get the texture coordinate from the fractional part
            int tx = (int)(texWidth * (floorX - cellX)) & (texWidth - 1);
            int ty = (int)(texHeight * (floorY - cellY)) & (texHeight - 1);

            floorX += floorStepX;
            floorY += floorStepY;

            // ceiling
            uint16_t color = texture[texWidth * ty + tx];
            *(_dt->p(x, y)) = color;

            // floor (symmetrical, at screenHeight - y - 1 instead of y)
            color = texture[texWidth * ty + tx];
            *(_dt->p(x, h - y - 1)) = color;
        }

        // // This definitely explodes when values are negative!
        // float u = floorX * texWidth;
        // float v = floorY * texHeight;
        // float du = floorStepX * texWidth;
        // float dv = floorStepY * texHeight;
        // texture_mapping_setup(5, 16);
        // texture_mapped_span_begin(uint32_t(65536 * u), uint32_t(65536 * v), uint32_t(65536 * du), uint32_t(65536 * dv));
        // uint16_t *dst = _dt->p(0, y);
        // for (int x = 0; x < w; x++)
        // {
        //     *dst = *(texture + texture_mapped_span_next());
        //     dst++;
        // }
    }
}

void __time_critical_func(draw)(uint32_t tick)
{
    // blend(COPY);
    // pen(rgb(0, 0, 0));
    // clear();

    draw_floor();
    draw_walls();

    draw_battery_indicator();

    pen(rgb(14, 14, 14));
    text(str(stats.fps, 1), 0, 0);
    text(str(stats.update_us, 2), 0, 15);
    text(str(stats.draw_us, 2), 0, 30);
    text(str(stats.flip_us, 2), 0, 45);
}
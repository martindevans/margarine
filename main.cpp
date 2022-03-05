#include <cmath>

#include "picosystem.hpp"

#include "pico/float.h"
#include "hardware/dma.h"
#include "hardware/interp.h"

#include "content/sprites/test_tile/tile.h"
#include "content/sprites/walls/wood/wood.h"
#include "content/sprites/walls/stone3/stone3.h"
#include "content/sprites/walls/stone2/stone2.h"

#include "render/dda.h"
#include "render/planes.h"
#include "render/texture_mapping.h"
#include "render/texture.h"
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

texture_mipmap_t floor_texture_mip = {
    .pixels = &testtile_mip_chain[0],
    .size_bits = 5,
    .size = 32,
    .mip_chain_length = 5
};

texture_mipmap_t wall_wood_texture_mip = {
    .pixels = &wall_wood_mip_chain[0],
    .size_bits = 6,
    .size = 64,
    .mip_chain_length = 4
};

texture_mipmap_t wall_stone3_texture_mip = {
    .pixels = &wall_stone3_mip_chain[0],
    .size_bits = 6,
    .size = 64,
    .mip_chain_length = 5
};

texture_mipmap_t wall_stone2_texture_mip = {
    .pixels = &wall_stone2_mip_chain[0],
    .size_bits = 6,
    .size = 64,
    .mip_chain_length = 6
};

texture_mipmap_t *wall_textures[] = {
    NULL,
    &floor_texture_mip,
    &wall_wood_texture_mip,
    &wall_stone3_texture_mip,
    &wall_stone2_texture_mip,
    &floor_texture_mip,
    &wall_wood_texture_mip,
    &wall_stone3_texture_mip,
    &wall_stone2_texture_mip,
};

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
int brightness = 100;

void init()
{
}

uint8_t sample_world_map(int x, int y)
{
    return worldMap2[x + y * mapWidth];
}

void __time_critical_func(update)(uint32_t tick)
{
    if (button(B))
        brightness = brightness <= 15 ? 15 : (brightness - 1);
    else if (button(X))
        brightness = brightness >= 100 ? 100 : (brightness + 1);
    backlight(brightness);

    float rotSpeed = _PI * 0.8 / 40.0;
    float cosrs, sinrs;
    bool turn = false;
    if (button(RIGHT))
    {
        cosrs = cos(-rotSpeed);
        sinrs = sin(-rotSpeed);
        turn = true;
    }
    else if (button(LEFT))
    {
        cosrs = cos(rotSpeed);
        sinrs = sin(rotSpeed);
        turn = true;
    }

    if (turn)
    {
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
    render_walls_in_range(0, 120, &cam_state, worldMap2, mapWidth, mapHeight, &wall_textures[0]);
    render_walls_in_range(120, 240, &cam_state, worldMap2, mapWidth, mapHeight, &wall_textures[0]);
}

void __time_critical_func(draw_floor)()
{
    render_planes(0, 60, &cam_state, &wall_stone2_texture_mip);
    render_planes(60, 120, &cam_state, &wall_stone2_texture_mip);
}

void __time_critical_func(draw)(uint32_t tick)
{
    // Draw 3D world
    draw_floor();
    draw_walls();

    // Draw HUD
    draw_battery_indicator();

    // Debug stuff
    pen(rgb(14, 14, 14));
    text(str(stats.fps, 1), 0, 0);
    text(str(stats.update_us, 2), 0, 15);
    text(str(stats.draw_us, 2), 0, 30);
    text(str(stats.flip_us, 2), 0, 45);
}
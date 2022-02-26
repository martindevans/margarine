#include <cmath>

#include "picosystem.hpp"

#include "pico/float.h"
#include "hardware/dma.h"
#include "hardware/interp.h"

#include "render/dda.h"

using namespace picosystem;

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
int clear_dma_channel;

void init()
{
    clear_dma_channel = dma_claim_unused_channel(true);
}

uint8_t sample_world_map(int x, int y)
{
    uint8_t worldMapWidth = 24;
    return worldMap2[x + y * worldMapWidth];
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

inline void fast_vline(int16_t x, int16_t y, int16_t c)
{
    color_t *dst = _dt->p(x, y);
    while(c-- > 0)
    {
        *dst = _pen;
        dst += _dt->w;
    }
}

inline void clamp_vline(int16_t *start, int16_t *end)
{
    if (*start < 0)
        *start = 0;
    if (*end > SCREEN->h)
        *end = SCREEN->h;
}

inline void draw_wall(uint16_t half_h, uint16_t x, uint16_t lineHeightInt, uint8_t wall_type, int side)
{
    // Determine the range of pixels to fill
    int16_t drawStart = half_h - lineHeightInt / 2;
    int16_t drawEnd = drawStart + lineHeightInt;
    clamp_vline(&drawStart, &drawEnd);
    int16_t lineHeight = drawEnd - drawStart;

    // Choose wall color
    color_t color = worldCol[wall_type];
    if (side == 1)
        color = color / 2;

    // Draw the pixels of the stripe as a vertical line
    pen(color);
    fast_vline(x, drawStart, lineHeight);
}

inline void draw_dda(uint16_t half_h, uint16_t x, dda_out_t *dda_result)
{
    draw_wall(half_h, x, uint16_t(dda_result->lineHeight), dda_result->wall_type, dda_result->side);
}

void __time_critical_func(draw)(uint32_t tick)
{
    blend(COPY);
    pen(rgb(0, 0, 0));
    clear();

    uint16_t half_h = SCREEN->h / 2;
    dda_in_t dda_in = {
        .w = SCREEN->w,
        .h = SCREEN->h
    };

    // Setup interpolators
    interp_config cfg = interp_default_config();
    interp_config_set_blend(&cfg, true);
    interp_set_config(interp0, 0, &cfg);
    cfg = interp_default_config();
    interp_set_config(interp0, 1, &cfg);

    const int bundle_width = 5;
    const int bundle_lerp_step = (int)(255 / (float)(bundle_width - 1));
    const float bundle_step = 1 / (float)(bundle_width - 1);
    for (int x = 0; x < SCREEN->w; x += bundle_width)
    {
        // DDA the left and right side of this range of 5 pixels
        dda_out_t dda_result_l = dda(x + 0, &dda_in, &cam_state, &worldMap2[0], 24);
        draw_dda(half_h, x, &dda_result_l);
        dda_out_t dda_result_r = dda(x + 4, &dda_in, &cam_state, &worldMap2[0], 24);
        draw_dda(half_h, x + bundle_width - 1, &dda_result_r);
        
        if (dda_result_l.wall_x == dda_result_r.wall_x && dda_result_l.wall_y == dda_result_r.wall_y && dda_result_l.wall_type == dda_result_r.wall_type && dda_result_l.side == dda_result_r.side)
        {
            // Render intermediate columns by interpolating the two end results
            interp0->base[0] = dda_result_l.lineHeight;
            interp0->base[1] = dda_result_r.lineHeight;
            for (int j = 1; j < bundle_width - 1; j++)
            {
                interp0->accum[1] = j * bundle_lerp_step;
                draw_wall(half_h, x + j, uint16_t(interp0->peek[1]), dda_result_l.wall_type, dda_result_l.side);
            }
        }
        else
        {
            // Two rays hit different walls, do all of the intermediate rays
            for (int j = 1; j < bundle_width - 1; j++)
            {
                dda_out_t dda_result_j = dda(x + j, &dda_in, &cam_state, &worldMap2[0], 24);
                draw_dda(half_h, x + j, &dda_result_j);
            }
        }
    }

    pen(rgb(14, 14, 14));
    text(str(stats.idle, 2), 0, 0);
    text(str(stats.update_us, 2), 0, 15);
    text(str(stats.draw_us, 2), 0, 30);
    text(str(stats.flip_us, 2), 0, 45);
}
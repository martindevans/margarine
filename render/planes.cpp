#include "picosystem.hpp"

#include "texture_mapping.h"
#include "texture.h"
#include "planes.h"

using namespace picosystem;

void __time_critical_func(render_planes)(int min_y, int max_y, camera_state_t *cam_state, texture_mipmap_t *floor_texture)
{
    uint16_t w = _dt->w;
    uint16_t h = _dt->h;
    uint16_t half_h = h / 2;

    // rayDir for leftmost ray (x = 0) and rightmost ray (x = w)
    float rayDirX0 = cam_state->dirX - cam_state->planeX;
    float rayDirY0 = cam_state->dirY - cam_state->planeY;
    float rayDirX1 = cam_state->dirX + cam_state->planeX;
    float rayDirY1 = cam_state->dirY + cam_state->planeY;

    uint16_t floor_pixels_w = floor_texture->size;
    uint16_t floor_pixels_h = floor_texture->size;

    texture_mapping_setup(floor_texture->size_bits, 16);

    for (int y = min_y; y < max_y; y++)
    {
        uint mip_level = 0;
        switch (y)
        {
            case 0   ... 60:  mip_level = 0; break;
            case 61  ... 85:  mip_level = 1; break;
            case 86  ... 100: mip_level = 2; break;
            case 101 ... 107: mip_level = 3; break;
            case 108 ... 112: mip_level = 4; break;
            default:          mip_level = 5; break;
        }
        //MIN((y * y) > 11, floor_texture->mip_chain_length - 1);
        uint mip_level_size_bits = floor_texture->size_bits - mip_level;

        uint16_t *dst_top = _dt->p(0, y);
        uint16_t *dst_bot = _dt->p(0, h - y - 1);

        // Current y position compared to the center of the screen (the horizon)
        int p = y - half_h;

        // Vertical position of the camera.
        float posZ = 1 - half_h;

        // Horizontal distance from the camera to the floor for the current row.
        // 0.5 is the z position exactly in the middle between floor and ceiling.
        float rowDistance = posZ / p;

        // calculate the real world step vector we have to add for each x (parallel to camera plane)
        // adding step by step avoids multiplications with a weight in the inner loop
        float floorStepX = rowDistance * (rayDirX1 - rayDirX0) / w;
        float floorStepY = rowDistance * (rayDirY1 - rayDirY0) / w;

        // real world coordinates of the leftmost column. This will be updated as we step to the right.
        float floorX = cam_state->posX + rowDistance * rayDirX0;
        float floorY = cam_state->posY + rowDistance * rayDirY0;

        // Correct negative values into positive range
        float u = (floorX - int(floorX)) * floor_pixels_w;
        while (u < 0)
            u += floor_pixels_w;
        float v = (floorY - int(floorY)) * floor_pixels_h;
        while (v < 0)
            v += floor_pixels_h;

        // Replace negative steps (e.g. -1) with large positive steps (e.g. width-1)
        float du = floorStepX * floor_pixels_w;
        while (du < 0)
            du += floor_pixels_w;
        float dv = floorStepY * floor_pixels_h;
        while (dv < 0)
            dv += floor_pixels_h;     

        texture_mapped_span_begin(uint32_t(65536 * u), uint32_t(65536 * v), uint32_t(65536 * du), uint32_t(65536 * dv));
        for (int x = 0; x < w; x++)
        {
            uint pixel_idx = texture_mapped_span_next();

            color_t color;
            if (mip_level == 0)
            {
                color = sample_texture(floor_texture, pixel_idx, 0);
            }
            else
            {
                int px_x = pixel_idx >> (floor_texture->size_bits * 2 - mip_level_size_bits);
                int px_y = (pixel_idx & (floor_texture->size - 1)) >> (floor_texture->size_bits - mip_level_size_bits);
                color = sample_texture(floor_texture, px_x, px_y, mip_level);
            }
            
            *dst_top = color;
            dst_top++;

            *dst_bot = color;
            dst_bot++;
        }
    }
}
#include "picosystem.hpp"

#include "texture_mapping.h"
#include "texture.h"
#include "planes.h"

using namespace picosystem;

void __time_critical_func(render_planes)(int min_y, int max_y, camera_state_t *cam_state, texture_mipmap_t *floor_texture, uint8_t *wall_heights)
{
    uint16_t w = _dt->w;
    uint16_t h = _dt->h;
    uint16_t half_h = h / 2;
    uint max_mip_level = floor_texture->mip_chain_length - 1;

    uint8_t min_wall_height = wall_heights[0];
    uint8_t wall_heights_prepped[w];
    for (int i = 0; i < w; i++)
    {
        wall_heights_prepped[i] = (h - wall_heights[i] + 1) / 2;
        min_wall_height = MIN(min_wall_height, wall_heights[i]);
    }
    min_wall_height = (240 - min_wall_height) / 2;

    // Early exit if no work needs doing (because it's all occluded by the walls)
    if (min_y > min_wall_height)
        return;

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
        if (y > min_wall_height)
            break;

        uint mip_level = 0;
        switch (y)
        {
            case 0   ... 45:  mip_level = 0; break;
            case 46  ... 70:  mip_level = 1; break;
            case 71  ... 85:  mip_level = 2; break;
            case 86  ... 95:  mip_level = 3; break;
            case 96  ... 105: mip_level = 4; break;
            case 106 ... 111: mip_level = 5; break;
            default:          mip_level = 6; break;
        }
        mip_level = MIN(mip_level, max_mip_level);
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

            if (y < wall_heights_prepped[x])
            {
                color_t c;
                if (mip_level == 0)
                {
                    c = sample_texture(floor_texture, pixel_idx, mip_level);
                }
                else
                {
                    int px_x = pixel_idx >> floor_texture->size_bits;
                    int px_y = pixel_idx & (floor_texture->size - 1);
                    c = sample_texture(floor_texture, px_y, px_x, mip_level);
                }
                
                *dst_top = c;
                *dst_bot = c;
            }

            dst_top++;
            dst_bot++;
        }
    }
}
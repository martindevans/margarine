#include "picosystem.hpp"

#include "hardware/interp.h"

#include "dda.h"
#include "../profiler/profiler.h"

using namespace picosystem;

dda_out_t dda(int xpos, dda_in_t *dda_in, camera_state_t *cam_in, map_t *map)
{
    // Calculate ray position and direction
    float cameraX = 2 * xpos / (float)dda_in->w - 1; //x-coordinate in camera space
    float rayDirX = cam_in->dirX + cam_in->planeX * cameraX;
    float rayDirY = cam_in->dirY + cam_in->planeY * cameraX;

    // Which box of the map we're in
    uint8_t mapX = uint8_t(cam_in->posX);
    uint8_t mapY = uint8_t(cam_in->posY);

    //length of ray from one x or y-side to next x or y-side
    //these are derived as:
    //deltaDistX = sqrt(1 + (rayDirY * rayDirY) / (rayDirX * rayDirX))
    //deltaDistY = sqrt(1 + (rayDirX * rayDirX) / (rayDirY * rayDirY))
    //which can be simplified to abs(|rayDir| / rayDirX) and abs(|rayDir| / rayDirY)
    //where |rayDir| is the length of the vector (rayDirX, rayDirY). Its length,
    //unlike (dirX, dirY) is not 1, however this does not matter, only the
    //ratio between deltaDistX and deltaDistY matters, due to the way the DDA
    //stepping further below works. So the values can be computed as below.
    // Division through zero is prevented, even though technically that's not
    // needed in C++ with IEEE 754 floating point values.
    float deltaDistX = (rayDirX == 0) ? 1e30 : std::abs(1 / rayDirX);
    float deltaDistY = (rayDirY == 0) ? 1e30 : std::abs(1 / rayDirY);

    // Length of ray from current position to next x or y-side
    float sideDistX;
    float sideDistY;

    //what direction to step in x or y-direction (either +1 or -1)
    int stepX;
    int stepY;

    //calculate step and initial sideDist
    if(rayDirX < 0)
    {
        stepX = -1;
        sideDistX = (cam_in->posX - mapX) * deltaDistX;
    }
    else
    {
        stepX = 1;
        sideDistX = (mapX + 1.0 - cam_in->posX) * deltaDistX;
    }
    if(rayDirY < 0)
    {
        stepY = -1;
        sideDistY = (cam_in->posY - mapY) * deltaDistY;
    }
    else
    {
        stepY = 1;
        sideDistY = (mapY + 1.0 - cam_in->posY) * deltaDistY;
    }

    // Convert delta dists and side dists to fixed point
    const float fixed_scale = 8192;
    int32_t deltaDistX_fixed = int32_t(deltaDistX * fixed_scale);
    int32_t deltaDistY_fixed = int32_t(deltaDistY * fixed_scale);
    int32_t sideDistX_fixed = int32_t(sideDistX * fixed_scale);
    int32_t sideDistY_fixed = int32_t(sideDistY * fixed_scale);

    // Perform DDA
    uint8_t side;
    uint8_t wall_type = 0;
    while (wall_type <= 0)
    {
        PROFILER_ADD(ProfilerValue_TotalDdaSteps, 1);

        //jump to next map square, either in x-direction, or in y-direction
        if (sideDistX_fixed < sideDistY_fixed)
        {
            sideDistX_fixed += deltaDistX_fixed;
            mapX += stepX;
            side = 0;
        }
        else
        {
            sideDistY_fixed += deltaDistY_fixed;
            mapY += stepY;
            side = 1;
        }

        // Check if sampling is out of bounds
        if (mapX < 0 || mapY < 0 || mapX >= map->map_width || mapY >= map->map_width)
            wall_type = 1;

        // Sample map
        wall_type = map->wall_map[mapX + mapY * map->map_width];
    }

    //Calculate distance projected on camera direction. This is the shortest distance from the point where the wall is
    //hit to the camera plane. Euclidean to center camera point would give fisheye effect!
    //This can be computed as (mapX - posX + (1 - stepX) / 2) / rayDirX for side == 0, or same formula with Y
    //for size == 1, but can be simplified to the code below thanks to how sideDist and deltaDist are computed:
    //because they were left scaled to |rayDir|. sideDist is the entire length of the ray above after the multiple
    //steps, but we subtract deltaDist once because one step more into the wall was taken above.
    int32_t depth_fixed;
    if(side == 0)
        depth_fixed = sideDistX_fixed - deltaDistX_fixed;
    else
        depth_fixed = sideDistY_fixed - deltaDistY_fixed;
    float perpWallDist = depth_fixed / fixed_scale;

    // Calculate height of line to draw on screen
    float lineHeight = dda_in->h / perpWallDist;
    if (lineHeight > 32768)
        lineHeight = 32768;

    // Calculate where exactly the wall was hit
    float wallX;
    if (side == 0)
        wallX = cam_in->posY + perpWallDist * rayDirY;
    else
        wallX = cam_in->posX + perpWallDist * rayDirX;
    wallX -= int(wallX);

    // x coordinate on the texture
    uint16_t texX = uint16_t(wallX * 8192);
    if ((side == 0 && rayDirX > 0) || (side == 1 && rayDirY < 0))
        texX = 8192 - texX;

    return dda_out_t
    {
        .wall_x = mapX,
        .wall_y = mapY,
        .wall_type = wall_type,
        .side = side,
        .lineHeight = lineHeight,
        .texture_coord = texX,
        .depth = depth_fixed,
    };
}

// draw a vertical wall slice
// - half_h: Half screen height
// - x: x coordnate of the slice on the screen
// - lineHeightInt: Integer height of the line
// - wall_type: type of the wall
// - wall_textures: set of wall textures to use
// - uf_coord: (u texture coordinate) * 8192
// - side: the direction this wall is being looked at
inline uint8_t draw_wall(uint16_t half_h, uint16_t x, uint16_t lineHeightInt, uint8_t wall_type, texture_mipmap **wall_textures, uint32_t uf_coord, bool side)
{
    // Choose wall texture
    texture_mipmap *tex = wall_textures[wall_type];
    uint32_t u_coord = uint32_t((uf_coord * tex->size) / 8192);

    // Determine y step of texture coordinates
    uint32_t v_coord = 0;
    uint32_t v_step = uint32_t((65536 * tex->size) / lineHeightInt);

    // Choose mip level based on wall height (bias slightly towards less aliasing)
    int mip_level = select_mip_level(tex, lineHeightInt - 10);

    // Determine the range of pixels to fill
    int16_t drawStart = half_h - lineHeightInt / 2;
    int16_t drawEnd = drawStart + lineHeightInt;
    if (drawStart < 0)
    {
        v_coord += v_step * -drawStart;
        drawStart = 0;
    }
    if (drawEnd > _dt->h)
        drawEnd = _dt->h;    

    // Darken some sides
    //todo: sample from a per-tile lightmap
    color_t light_map_colour = rgb(0, 0, 0);
    uint8_t light_map_blend = 0;
    if (side)
        light_map_blend = 5;

    // Draw the pixels of the stripe as a vertical line
    uint count = drawEnd - drawStart;
    PROFILER_ADD(ProfilerValue_PaintedWallPixels, count);
    color_t *dst = _dt->p(x, drawStart);
    while (count-- > 0)
    {
        //todo: lightmapping is too slow :(
        //color_t c = mix(sample_texture(tex, u_coord, v_coord >> 16, mip_level), light_map_colour, light_map_blend);

        color_t c = sample_texture(tex, u_coord, v_coord >> 16, mip_level);
        *dst = c;
        v_coord += v_step;
        dst += _dt->w;
    }

    return uint8_t(drawEnd - drawStart);
}

inline uint8_t draw_dda(uint16_t half_h, uint16_t x, dda_out_t *dda_result, texture_mipmap **wall_textures)
{
    return draw_wall(half_h, x, uint16_t(dda_result->lineHeight), dda_result->wall_type, wall_textures, dda_result->texture_coord, dda_result->side == 0);
}

void __time_critical_func(render_walls_in_range)(int min_x, int max_x, camera_state_t *cam_state, map_t *map, texture_mipmap **wall_textures, uint8_t *out_wall_heights, int32_t *out_wall_depths)
{
    uint16_t half_h = _dt->h / 2;
    dda_in_t dda_in = {
        .w = _dt->w,
        .h = _dt->h
    };

    const int bundle_width = 6;
    const int bundle_lerp_step = (int)(255 / (float)(bundle_width - 1));
    const float bundle_step = 1 / (float)(bundle_width - 1);

    dda_out_t dda_result_l = dda(min_x, &dda_in, cam_state, map);

    for (int x = min_x; x < max_x - bundle_width + 1; x += bundle_width)
    {
        out_wall_depths[x] = dda_result_l.depth;
        out_wall_heights[x] = draw_dda(half_h, x, &dda_result_l, wall_textures);

        dda_out_t dda_result_r = dda(x + bundle_width, &dda_in, cam_state, map);
    
        for (int j = 1; j < bundle_width; j++)
        {
            dda_out_t dda_result_j = dda(x + j, &dda_in, cam_state, map);
            out_wall_depths[x + j] = dda_result_j.depth;
            out_wall_heights[x + j] = draw_dda(half_h, x + j, &dda_result_j, wall_textures);
        }

        dda_result_l = dda_result_r;
    }
}
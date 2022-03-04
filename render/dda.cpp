#include "picosystem.hpp"

#include "hardware/interp.h"

#include "dda.h"

using namespace picosystem;

dda_out_t dda(int xpos, dda_in_t *dda_in, camera_state_t *cam_in, uint8_t *map, uint8_t map_width)
{
    // Calculate ray position and direction
    float cameraX = 2 * xpos / (float)dda_in->w - 1; //x-coordinate in camera space
    float rayDirX = cam_in->dirX + cam_in->planeX * cameraX;
    float rayDirY = cam_in->dirY + cam_in->planeY * cameraX;

    // Which box of the map we're in
    uint8_t mapX = uint8_t(cam_in->posX);
    uint8_t mapY = uint8_t(cam_in->posY);

    // Length of ray from current position to next x or y-side
    float sideDistX;
    float sideDistY;

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

    float perpWallDist;

    //what direction to step in x or y-direction (either +1 or -1)
    int stepX;
    int stepY;

    uint8_t side; //was a NS or a EW wall hit?
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

    // Perform DDA
    uint8_t wall_type = 0;
    while (wall_type <= 0)
    {
        //jump to next map square, either in x-direction, or in y-direction
        if (sideDistX < sideDistY)
        {
            sideDistX += deltaDistX;
            mapX += stepX;
            side = 0;
        }
        else
        {
            sideDistY += deltaDistY;
            mapY += stepY;
            side = 1;
        }

        // Sample map
        wall_type = map[mapX + mapY * map_width];
    }

    //Calculate distance projected on camera direction. This is the shortest distance from the point where the wall is
    //hit to the camera plane. Euclidean to center camera point would give fisheye effect!
    //This can be computed as (mapX - posX + (1 - stepX) / 2) / rayDirX for side == 0, or same formula with Y
    //for size == 1, but can be simplified to the code below thanks to how sideDist and deltaDist are computed:
    //because they were left scaled to |rayDir|. sideDist is the entire length of the ray above after the multiple
    //steps, but we subtract deltaDist once because one step more into the wall was taken above.
    if(side == 0)
        perpWallDist = (sideDistX - deltaDistX);
    else
        perpWallDist = (sideDistY - deltaDistY);

    // Calculate height of line to draw on screen
    float lineHeight = dda_in->h / perpWallDist;

    // Calculate where exactly the wall was hit
    float wallX;
    if (side == 0)
        wallX = cam_in->posY + perpWallDist * rayDirY;
    else
        wallX = cam_in->posX + perpWallDist * rayDirX;
    wallX -= int(wallX);

    // x coordinate on the texture
    float texX = wallX;
    if ((side == 0 && rayDirX > 0) || (side == 1 && rayDirY < 0))
        texX = 1 - texX;

    return dda_out_t
    {
        .wall_x = mapX,
        .wall_y = mapY,
        .wall_type = wall_type,
        .side = side,
        .lineHeight = lineHeight,
        .texture_coord = texX
    };
}

inline void draw_wall(uint16_t half_h, uint16_t x, uint16_t lineHeightInt, uint8_t wall_type, texture_mipmap **wall_textures, float uf_coord)
{
    // Choose wall texture
    texture_mipmap *tex = wall_textures[wall_type];
    uint u_coord = uint(uf_coord * tex->size);

    // Determine y step of texture coordinates
    uint32_t v_coord = 0;
    uint32_t v_step = uint32_t(65536 * ((float)tex->size / (float)lineHeightInt));

    // Choose mip level based on wall height
    uint mip_level = 0;
    switch (lineHeightInt)
    {
        case 0   ... 20:  mip_level = 5; break;
        case 21  ... 30:  mip_level = 4; break;
        case 31  ... 45:  mip_level = 3; break;
        case 46  ... 70:  mip_level = 2; break;
        case 71  ... 140: mip_level = 1; break;
        case 141 ... 240: mip_level = 0; break;
        default:          mip_level = 0; break;
    }
    mip_level = MIN(mip_level, tex->mip_chain_length - 1);

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

    // Setup interpolator
    interp_config cfg = interp_default_config();
    interp_set_config(interp1, 0, &cfg);
    interp1->accum[0] = v_coord;
    interp1->base[0] = v_step;

    // Draw the pixels of the stripe as a vertical line
    uint count = drawEnd - drawStart;
    color_t *dst = _dt->p(x, drawStart);
    while (count-- > 0)
    {
        *dst = sample_texture(tex, u_coord, interp1->pop[0] >> 16, mip_level);
        v_coord += v_step;
        dst += _dt->w;
    }
}

inline void draw_dda(uint16_t half_h, uint16_t x, dda_out_t *dda_result, texture_mipmap **wall_textures)
{
    draw_wall(half_h, x, uint16_t(dda_result->lineHeight), dda_result->wall_type, wall_textures, dda_result->texture_coord);
}

void __time_critical_func(render_walls_in_range)(int min_x, int max_x, camera_state_t *cam_state, uint8_t* worldMap, int mapWidth, texture_mipmap **wall_textures)
{
    uint16_t half_h = _dt->h / 2;
    dda_in_t dda_in = {
        .w = _dt->w,
        .h = _dt->h
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

    dda_out_t dda_result_l = dda(min_x, &dda_in, cam_state, worldMap, mapWidth);

    for (int x = min_x; x < max_x - bundle_width + 1; x += bundle_width)
    {
        draw_dda(half_h, x, &dda_result_l, wall_textures);
        dda_out_t dda_result_r = dda(x + bundle_width, &dda_in, cam_state, worldMap, mapWidth);
    
        // todo: restore this optimisation
        if (false && dda_result_l.wall_x == dda_result_r.wall_x && dda_result_l.wall_y == dda_result_r.wall_y && dda_result_l.wall_type == dda_result_r.wall_type && dda_result_l.side == dda_result_r.side)
        {
            if (dda_result_l.lineHeight > _dt->h && dda_result_r.lineHeight > _dt->h)
            {
                // Both sides are taller than the screen, fill the area
                uint16_t lineHeight = uint16_t(dda_result_l.lineHeight);
                for (int j = 1; j < bundle_width; j++)
                    draw_wall(half_h, x + j, lineHeight, dda_result_l.wall_type, wall_textures, 0.5f); //todo: texcoord
            }
            else
            {
                // Render intermediate columns by interpolating the two end results
                interp0->base[0] = int32_t(dda_result_l.lineHeight);
                interp0->base[1] = int32_t(dda_result_r.lineHeight);
                interp0->accum[1] = bundle_lerp_step;
                for (int j = 1; j < bundle_width; j++)
                {
                    draw_wall(half_h, x + j, interp0->peek[1], dda_result_l.wall_type, wall_textures, 0.5f); //todo: texcoord
                    interp0->accum[1] += bundle_lerp_step;
                }
            }
        }
        else
        {
            // Two rays hit different walls, do all of the intermediate rays
            for (int j = 1; j < bundle_width; j++)
            {
                dda_out_t dda_result_j = dda(x + j, &dda_in, cam_state, worldMap, mapWidth);
                draw_dda(half_h, x + j, &dda_result_j, wall_textures);
            }
        }

        dda_result_l = dda_result_r;
    }
}
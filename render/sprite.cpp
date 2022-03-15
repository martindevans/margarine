#include "sprite.h"

#include <bits/stdc++.h>
#include "picosystem.hpp"

using namespace picosystem;

bool compare_sprite3d(sprite3d_t a, sprite3d_t b)
{
    return a.distanceSqr > b.distanceSqr;
}

void sort_sprites(camera_state_t *camera, sprite3d_t *sprites, int count)
{
    for (int i = 0; i < count; i++)
    {
        sprite3d_t *sprite = &sprites[i];
        float dx = camera->posX - sprite->posX;
        float dy = camera->posY - sprite->posY;
        sprite->distanceSqr = dx * dx + dy * dy;
    }

    std::stable_sort(sprites, sprites + count, compare_sprite3d);
}

void __time_critical_func(render_sprites)(int min_x, int max_x, camera_state_t *camera, sprite3d_t *sprites, int count, int32_t *wall_depths, texture_mipmap **textures)
{
    float invDet = 1.0 / (camera->planeX * camera->dirY - camera->dirX * camera->planeY);
    int screen_width = _dt->w;
    int half_width = screen_width / 2;
    int screen_height = _dt->h;
    int half_height = screen_height / 2;

    for(int i = 0; i < count; i++)
    {
        // Skip sprites using a null texture
        texture_mipmap *tex = textures[sprites[i].texture];
        if (tex == NULL)
            continue;

        // Translate sprite position to relative to camera
        float spriteX = sprites[i].posX - camera->posX;
        float spriteY = sprites[i].posY - camera->posY;

        // Transform sprite with the inverse camera matrix
        // [ planeX   dirX ] -1                                       [ dirY      -dirX ]
        // [               ]       =  1/(planeX*dirY-dirX*planeY) *   [                 ]
        // [ planeY   dirY ]                                          [ -planeY  planeX ]
        float transformX = invDet * (camera->dirY * spriteX - camera->dirX * spriteY);
        float transformY = invDet * (-camera->planeY * spriteX + camera->planeX * spriteY);
        int spriteScreenX = int(half_width * (1 + transformX / transformY));
        int yMoveScreen = int(sprites[i].yMove / transformY);

        // Skip sprite if it is behind camera
        if (transformY <= 0)
            continue;

        // Calculate size of the sprite on screen
        // Using 'transformY' instead of the real distance prevents fisheye
        int spriteWidth = abs(int(sprites[i].xSize * screen_height / transformY));
        int spriteHeight = abs(int(sprites[i].ySize * screen_height / transformY));
        if (spriteHeight <= 0 || spriteWidth <= 0)
            continue;

        // Calculate the texture mapping parameters
        uint32_t u_coord = 0;
        uint32_t u_step = uint32_t((65536 * tex->size) / spriteWidth);
        uint32_t v_base_coord = 0;
        uint32_t v_step = uint32_t((65536 * tex->size) / spriteHeight);

        // Calculate extent to draw on screen (vertically)
        int drawStartY = half_height - spriteHeight / 2 + yMoveScreen;
        int drawEndY = drawStartY + spriteHeight;
        if (drawStartY < 0)  
        {
            v_base_coord += v_step * -drawStartY;
            drawStartY = 0;
        }
        if (drawEndY >= screen_height)
            drawEndY = screen_height;

        // Calculate extent to draw on screen (horizontally)
        int drawStartX = spriteScreenX - spriteWidth / 2;
        int drawEndX = drawStartX + spriteWidth;
        if (drawStartX < min_x)
        {
            u_coord += u_step * (min_x - drawStartX);
            drawStartX = min_x;
        }
        if (drawEndX >= max_x)
            drawEndX = max_x;

        // Choose mip level based on sprite height
        int mip_level = select_mip_level(tex, spriteHeight);

        // loop through every vertical stripe of the sprite on screen
        int32_t transformY8192 = int32_t(transformY * 8192);
        for (int stripe = drawStartX; stripe < drawEndX; stripe++)
        {
            // Check if this stripe is occluded by the zbuffer
            if (transformY8192 < wall_depths[stripe])
            {
                // Copy vertical strip of pixels
                uint32_t v_coord = v_base_coord;
                color_t *dst = _dt->p(stripe, drawStartY);
                for (int y = drawStartY; y < drawEndY; y++)
                {
                    color_t c = sample_texture(tex, u_coord >> 16, v_coord >> 16, mip_level);
                    if ((c & 0x00f0) > 0)
                        *dst = c;
                    dst += screen_width;
                    v_coord += v_step;
                }
            }

            u_coord += u_step;
        }
    }
}
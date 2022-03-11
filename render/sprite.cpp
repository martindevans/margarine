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

void __time_critical_func(render_sprites)(camera_state_t *camera, sprite3d_t *sprites, int count, int32_t *wall_depths)
{
    float invDet = 1.0 / (camera->planeX * camera->dirY - camera->dirX * camera->planeY);
    int screen_width = _dt->w;
    int half_width = screen_width / 2;
    int screen_height = _dt->h;
    int half_height = screen_height / 2;

    for(int i = 0; i < count; i++)
    {
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

        // Skip sprite if it is behind camera
        if (transformY <= 0)
            continue;

        // Calculate height of the sprite on screen
        // Using 'transformY' instead of the real distance prevents fisheye
        int spriteHeight = abs(int(screen_height / transformY));

        // Calculate lowest and highest pixel to fill in current stripe
        int drawStartY = half_height - spriteHeight / 2;
        int drawEndY = drawStartY + spriteHeight;
        if (drawStartY < 0)
            drawStartY = 0;
        if (drawEndY >= screen_height)
            drawEndY = screen_height;

        // Calculate width of the sprite
        int spriteWidth = abs(int(screen_height / transformY));
        int drawStartX = spriteScreenX - spriteWidth / 2;
        int drawEndX = drawStartX + spriteWidth;
        if (drawStartX < 0)
            drawStartX = 0;
        if (drawEndX >= screen_width)
            drawEndX = screen_width;

        // loop through every vertical stripe of the sprite on screen
        int32_t transformY8192 = int32_t(transformY * 8192);
        for (int stripe = drawStartX; stripe < drawEndX; stripe++)
        {
            //int texX = int(256 * (stripe - (-spriteWidth / 2 + spriteScreenX)) * texWidth / spriteWidth) / 256;

            // Check if this stripe is occluded by the zbuffer
            if (transformY8192 >= wall_depths[stripe])
                continue;
            
            // Copy vertical strip of pixels
            color_t *dst = _dt->p(stripe, drawStartY);
            for(int y = drawStartY; y < drawEndY; y++)
            {
                // int d = y * 256 - h * 128 + spriteHeight * 128;
                // int texY = ((d * texHeight) / spriteHeight) / 256;
                // Uint32 color = texture[sprite[spriteOrder[i]].texture][texWidth * texY + texX]; //get current color from the texture
                // if((color & 0x00FFFFFF) != 0) buffer[y][stripe] = color; //paint pixel if it isn't black, black is the invisible color

                *dst = rgb(10, i, 0);
                dst += screen_width;
            }
        }
    }
}
#include "picosystem.hpp"

#include "pico/float.h"
#include "hardware/dma.h"

using namespace picosystem;

uint8_t worldMap[24][24]=
{
  {4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,7,7,7,7,7,7,7,7},
  {4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,7,0,0,0,0,0,0,7},
  {4,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,7},
  {4,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,7},
  {4,0,3,0,0,0,0,0,0,0,0,0,0,0,0,0,7,0,0,0,0,0,0,7},
  {4,0,4,0,0,0,0,5,5,5,5,5,5,5,5,5,7,7,0,7,7,7,7,7},
  {4,0,5,0,0,0,0,5,0,5,0,5,0,5,0,5,7,0,0,0,7,7,7,1},
  {4,0,6,0,0,0,0,5,0,0,0,0,0,0,0,5,7,0,0,0,0,0,0,8},
  {4,0,7,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,7,7,7,1},
  {4,0,8,0,0,0,0,5,0,0,0,0,0,0,0,5,7,0,0,0,0,0,0,8},
  {4,0,0,0,0,0,0,5,0,0,0,0,0,0,0,5,7,0,0,0,7,7,7,1},
  {4,0,0,0,0,0,0,5,5,5,5,0,5,5,5,5,7,7,7,7,7,7,7,1},
  {6,6,6,6,6,6,6,6,6,6,6,0,6,6,6,6,6,6,6,6,6,6,6,6},
  {8,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4},
  {6,6,6,6,6,6,0,6,6,6,6,0,6,6,6,6,6,6,6,6,6,6,6,6},
  {4,4,4,4,4,4,0,4,4,4,6,0,6,2,2,2,2,2,2,2,3,3,3,3},
  {4,0,0,0,0,0,0,0,0,4,6,0,6,2,0,0,0,0,0,2,0,0,0,2},
  {4,0,0,0,0,0,0,0,0,0,0,0,6,2,0,0,5,0,0,2,0,0,0,2},
  {4,0,0,0,0,0,0,0,0,4,6,0,6,2,0,0,0,0,0,2,2,0,2,2},
  {4,0,6,0,6,0,0,0,0,4,6,0,0,0,0,0,5,0,0,0,0,0,0,2},
  {4,0,0,5,0,0,0,0,0,4,6,0,6,2,0,0,0,0,0,2,2,0,2,2},
  {4,0,6,0,6,0,0,0,0,4,6,0,6,2,0,0,5,0,0,2,0,0,0,2},
  {4,0,0,0,0,0,0,0,0,4,6,0,6,2,0,0,0,0,0,2,0,0,0,2},
  {4,4,4,4,4,4,4,4,4,4,1,1,1,2,2,2,2,2,2,3,3,3,3,3}
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
};

float posX = 22;
float posY = 12;
float dirX = -1;
float dirY = 0;
float planeX = 0;
float planeY = 0.66;

color_t bg = 0;
int clear_dma_channel;

void init()
{
    clear_dma_channel = dma_claim_unused_channel(true);
}

void __time_critical_func(update)(uint32_t tick)
{
    float rotSpeed = _PI * 1.0 / 40.0;

    if(button(RIGHT))
    {
        float cosrs = cos(-rotSpeed);
        float sinrs = sin(-rotSpeed);

        //both camera direction and camera plane must be rotated
        float oldDirX = dirX;
        dirX = dirX * cosrs - dirY * sinrs;
        dirY = oldDirX * sinrs + dirY * cosrs;
        float oldPlaneX = planeX;
        planeX = planeX * cosrs - planeY * sinrs;
        planeY = oldPlaneX * sinrs + planeY * cosrs;
    }

    if(button(LEFT))
    {
        float cosrs = cos(rotSpeed);
        float sinrs = sin(rotSpeed);

        float oldDirX = dirX;
        dirX = dirX * cosrs - dirY * sinrs;
        dirY = oldDirX * sinrs + dirY * cosrs;
        float oldPlaneX = planeX;
        planeX = planeX * cosrs - planeY * sinrs;
        planeY = oldPlaneX * sinrs + planeY * cosrs;
    }

    float moveSpeed = 2 * 1.0 / 40.0;

    //move forward if no wall in front of you
    if(button(UP))
    {
        if(worldMap[int(posX + dirX * moveSpeed)][int(posY)] == 0)
            posX += dirX * moveSpeed;
        if(worldMap[int(posX)][int(posY + dirY * moveSpeed)] == 0)
            posY += dirY * moveSpeed;
    }

    //move backwards if no wall behind you
    if(button(DOWN))
    {
        if(worldMap[int(posX - dirX * moveSpeed)][int(posY)] == 0)
            posX -= dirX * moveSpeed;
        if(worldMap[int(posX)][int(posY - dirY * moveSpeed)] == 0)
            posY -= dirY * moveSpeed;
    }
}

inline void fast_vline(int32_t x, int32_t y, int32_t c)
{
    color_t *dst = _dt->p(x, y);
    while(c-- > 0)
    {
        *dst = _pen;
        dst += _dt->w;
    }
}

void __time_critical_func(draw)(uint32_t tick)
{
    blend(COPY);
    pen(rgb(0, 0, 0));
    clear();

    int32_t w = SCREEN->w;
    int32_t h = SCREEN->h;
    int32_t half_h = h / 2;

    for(int x = 0; x < w; x++)
    {
        // Calculate ray position and direction
        float cameraX = 2 * x / (float)w - 1; //x-coordinate in camera space
        float rayDirX = dirX + planeX * cameraX;
        float rayDirY = dirY + planeY * cameraX;

        // Which box of the map we're in
        int mapX = int(posX);
        int mapY = int(posY);

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

        int side; //was a NS or a EW wall hit?
        //calculate step and initial sideDist
        if(rayDirX < 0)
        {
            stepX = -1;
            sideDistX = (posX - mapX) * deltaDistX;
        }
        else
        {
            stepX = 1;
            sideDistX = (mapX + 1.0 - posX) * deltaDistX;
        }
        if(rayDirY < 0)
        {
            stepY = -1;
            sideDistY = (posY - mapY) * deltaDistY;
        }
        else
        {
            stepY = 1;
            sideDistY = (mapY + 1.0 - posY) * deltaDistY;
        }

        // Perform DDA
        uint8_t hit = 0;
        while (hit <= 0)
        {
            //jump to next map square, either in x-direction, or in y-direction
            if(sideDistX < sideDistY)
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
            hit = worldMap[mapX][mapY];
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

        //Calculate height of line to draw on screen
        int lineHeight = (int)(h / perpWallDist);

        // Calculate pixel strip to fill
        int drawStart = half_h - lineHeight / 2;
        if (drawStart < 0)
        {
            lineHeight += drawStart;
            drawStart = 0;
        }
        if (drawStart + lineHeight >= h)
            lineHeight = h - drawStart;

        // Choose wall color
        color_t color = worldCol[hit];
        if (side == 1)
            color = color / 2;

        // Draw the pixels of the stripe as a vertical line
        pen(color);
        fast_vline(x, drawStart, lineHeight);
    }

    pen(rgb(14, 14, 14));
    text(str(stats.idle, 2), 0, 0);
    text(str(stats.update_us, 2), 0, 15);
    text(str(stats.draw_us, 2), 0, 30);
    text(str(stats.flip_us, 2), 0, 45);
}
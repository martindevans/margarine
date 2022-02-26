#include "picosystem.hpp"

#include "dda.h"

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

    return dda_out_t
    {
        .wall_x = mapX,
        .wall_y = mapY,
        .wall_type = wall_type,
        .side = side,
        .lineHeight = lineHeight
    };
}
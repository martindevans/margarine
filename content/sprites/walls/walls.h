#include "wood/wood.h"
#include "stone3/stone3.h"
#include "stone2/stone2.h"
#include "stone1/stone1.h"
#include "stoneblue/stoneblue.h"
#include "brickred/brickred.h"

texture_mipmap_t *wall_textures[] = {
    NULL,
    &wall_stone1_texture,
    &wall_stone2_texture,
    &wall_stone3_texture,
    &wall_stoneblue_texture,
    &wall_brickred_texture,
    &wall_wood_texture,
    &wall_wood_texture,
    &wall_wood_texture,
};
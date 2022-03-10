#include <cmath>

#include "picosystem.hpp"

#include "pico/float.h"
#include "hardware/dma.h"
#include "hardware/interp.h"

#include "content/sprites/test_tile/tile.h"
#include "content/sprites/walls/wood/wood.h"
#include "content/sprites/walls/stone3/stone3.h"
#include "content/sprites/walls/stone2/stone2.h"
#include "content/sprites/walls/stone1/stone1.h"
#include "content/sprites/walls/stoneblue/stoneblue.h"
#include "content/sprites/walls/brickred/brickred.h"

#include "render/dda.h"
#include "render/planes.h"
#include "render/texture_mapping.h"
#include "render/texture.h"
#include "render/sprite.h"
#include "render/hud.h"
#include "render/camera.h"

#include "profiler/profiler.h"

#include "content/maps/test_map/test_map.h"

using namespace picosystem;

map_t *map;

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

camera_state_t cam_state = 
{
    .posX = 4,
    .posY = 4,
    .dirX = -1,
    .dirY = 0,
    .planeX = 0,
    .planeY = 0.66,
};

int brightness = 90;

void init()
{
    PROFILER_BEGIN_TIMING(ProfilerValue_InitTime);
    {
        profiler_init();
        map = get_test_map();
    }
    PROFILER_END_TIMING(ProfilerValue_InitTime);
}

void __time_critical_func(update)(uint32_t tick)
{
    profiler_clear();

    // Screen brightness
    if (button(B))
        brightness = brightness <= 15 ? 15 : (brightness - 1);
    else if (button(X))
        brightness = brightness >= 100 ? 100 : (brightness + 1);
    backlight(brightness);

    // Turn camera
    const float rotSpeed = _PI * 0.8 / 40.0;
    if (button(RIGHT))
        rotate(&cam_state, -rotSpeed);
    else if (button(LEFT))
        rotate(&cam_state, rotSpeed);

    // Move camera
    const float moveSpeed = 2 * 1.0 / 40.0;
    if (button(UP))
        move(&cam_state, map, moveSpeed);
    else if (button(DOWN))
        move(&cam_state, map, -moveSpeed);
}

void __time_critical_func(draw_walls)(uint8_t *wall_heights, int32_t *wall_depths)
{
    render_walls_in_range(0, 120, &cam_state, map, &wall_textures[0], wall_heights, wall_depths);
    render_walls_in_range(120, 240, &cam_state, map, &wall_textures[0], wall_heights, wall_depths);
}

void __time_critical_func(draw_floor)(uint8_t *wall_heights)
{
    render_planes(0, 60, &cam_state, &wall_stone1_texture, wall_heights);
    render_planes(60, 120, &cam_state, &wall_stone1_texture, wall_heights);
}

void __time_critical_func(draw)(uint32_t tick)
{
    // Draw 3D world
    uint8_t wall_heights[240];
    int32_t wall_depths[240];
    draw_walls(wall_heights, wall_depths);
    draw_floor(wall_heights);
    
#if DEBUG_DRAW_DEPTH_BUFFER
    // Draw depth buffer
    for (int i = 0; i < 240; i++)
    {
        int32_t d = (30 * wall_depths[i]) / 8192;
        if (d < 0) d = 0;
        if (d > 239) d = 239;
        *(_dt->p(i, d)) = rgb(15, 0, 0);
    }
#endif

    // Draw HUD
    draw_battery_indicator();

    // Debug stuff
    pen(rgb(2, 2, 2));
    text("FPS: " + str(stats.fps, 1));
    text("UPDATE: " + str(stats.update_us, 2));
    text("DRAW: " + str(stats.draw_us, 2)); 

    if (profiler_is_enabled())
        text("PROFILER: " + str(*profiler_get(ProfilerValue_TotalDdaSteps), 0));
}
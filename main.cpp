#include <cmath>

#include "picosystem.hpp"

#include "pico/float.h"
#include "hardware/dma.h"
#include "hardware/interp.h"

#include "content/sprites/walls/walls.h"

#include "render/dda.h"
#include "render/planes.h"
#include "render/texture_mapping.h"
#include "render/texture.h"
#include "render/sprite.h"
#include "render/hud.h"
#include "render/camera.h"

#include "multithreading/multithreading.h"

#include "profiler/profiler.h"

#include "content/maps/test_map/test_map.h"

using namespace picosystem;

map_t *map;

#define sprite_count 3
sprite3d_t sprites[sprite_count] = {
    { .posX = 4.5, .posY = 3.5, .texture = 1 },
    { .posX = 4.5, .posY = 4.5, .texture = 2 },
    { .posX = 4.5, .posY = 5.5, .texture = 3 },
};

texture_mipmap_t *sprite_textures[] = {
    NULL,
    &wall_stone1_texture,
    &wall_brickred_texture,
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

uint8_t wall_heights[240];
int32_t wall_depths[240];

void init()
{
    PROFILER_BEGIN_TIMING(ProfilerValue_InitTime);
    {
        profiler_init();
        launch_multicore();
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

    // Sort sprites into distance order
    sort_sprites(&cam_state, sprites, sprite_count);
}

int32_t __time_critical_func(draw_walls_core1)(int32_t parameter)
{
    render_walls_in_range(120, 240, &cam_state, map, wall_textures, wall_heights, wall_depths);
    return 0;
}

void __time_critical_func(draw_walls)()
{
    start_multicore_work(&draw_walls_core1, 0);
    render_walls_in_range(0, 120, &cam_state, map, wall_textures, wall_heights, wall_depths);
    end_multicore_work_blocking();
}

int32_t __time_critical_func(draw_floor_core1)(int32_t parameter)
{
    render_planes(60, 120, &cam_state, &wall_stone1_texture, wall_heights);
    return 0;
}

void __time_critical_func(draw_floor)()
{
    start_multicore_work(&draw_floor_core1, 0);
    render_planes(0, 60, &cam_state, &wall_stone1_texture, wall_heights);
    end_multicore_work_blocking();
}

int32_t __time_critical_func(draw_sprites_core1)(int32_t parameter)
{
    render_sprites(120, 240, &cam_state, sprites, sprite_count, wall_depths, sprite_textures);
    return 0;
}

void __time_critical_func(draw_sprites)(camera_state_t *camera, sprite3d_t *sprites, int count)
{
    start_multicore_work(&draw_sprites_core1, 0);
    render_sprites(0, 120, &cam_state, sprites, sprite_count, wall_depths, sprite_textures);
    end_multicore_work_blocking();
}

void __time_critical_func(draw)(uint32_t tick)
{
    // Draw 3D world
    draw_walls();
    draw_floor();
    draw_sprites(&cam_state, sprites, sprite_count);
    
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
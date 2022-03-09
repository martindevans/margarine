#include "sprite.h"

#include <bits/stdc++.h>

bool compare_sprite3d(sprite3d_t a, sprite3d_t b)
{
    return a.distanceSqr < b.distanceSqr;
}

void sort_sprites(sprite3d_t *sprites, int count)
{
    std::stable_sort(sprites, sprites + count, compare_sprite3d);
}
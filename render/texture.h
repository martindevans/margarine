#pragma once

#include "picosystem.hpp"

typedef struct texture
{
    const picosystem::color_t *pixels;
    const uint8_t size;
    const uint8_t size_bits;
} texture_t;

typedef struct texture_mipmap
{
    const picosystem::color_t **pixels;
    const uint8_t size_bits;
    const uint8_t size;
    const uint8_t size_minus_1;
    const uint8_t mip_chain_length;
} texture_mipmap_t;

void load_texture(texture_mipmap_t &texture, uint8_t max_size);

inline __attribute__((always_inline)) picosystem::color_t sample_texture(const texture_t *texture, uint x, uint y)
{
    return texture->pixels[x + y * texture->size];
}

inline __attribute__((always_inline)) picosystem::color_t sample_texture(const texture_mipmap_t *texture, uint x, uint y, uint mip)
{
    x >>= mip;
    y >>= mip;

    uint size = 1 << (texture->size_bits - mip);
    return texture->pixels[mip][x + y * size];
}

inline __attribute__((always_inline)) picosystem::color_t sample_texture(const texture_mipmap_t *texture, uint index, uint mip)
{
    if (mip == 0)
        return texture->pixels[mip][index];

    uint px_x = index >> texture->size_bits;
    uint px_y = index & (texture->size_minus_1);
    return sample_texture(texture, px_y, px_x, mip);
}

inline __attribute__((always_inline)) int select_mip_level(const texture_mipmap_t *texture, int screen_size)
{
    uint tex_size = texture->size;

    uint mip_level = 0;
    while (screen_size < tex_size)
    {
        mip_level++;
        tex_size /= 2;
    }

    return MIN(mip_level, texture->mip_chain_length - 1);
}
#pragma once

#include "picosystem.hpp"

typedef struct texture
{
    picosystem::color_t *pixels;
    uint8_t size;
    uint8_t size_bits;
} texture_t;

typedef struct texture_mipmap
{
    picosystem::color_t **pixels;
    uint8_t size_bits;
    uint8_t size;
    uint8_t mip_chain_length;
} texture_mipmap_t;

inline picosystem::color_t sample_texture(const texture_t *texture, uint x, uint y)
{
    return texture->pixels[x + y * texture->size];
}

inline picosystem::color_t sample_texture(const texture_mipmap_t *texture, uint x, uint y, uint mip)
{
    x >>= mip;
    y >>= mip;

    uint size = 1 << (texture->size_bits - mip);
    return texture->pixels[mip][x + y * size];
}

inline picosystem::color_t sample_texture(const texture_mipmap_t *texture, uint index, uint mip)
{
    return texture->pixels[mip][index];
}

inline int select_mip_level(const texture_mipmap_t *texture, int screen_size)
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
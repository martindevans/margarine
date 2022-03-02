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
    uint size_bits = 1 << (texture->size_bits - mip);
    return texture->pixels[mip][x + y * size_bits];
}

inline picosystem::color_t sample_texture(const texture_mipmap_t *texture, uint index, uint mip)
{
    return texture->pixels[mip][index];
}
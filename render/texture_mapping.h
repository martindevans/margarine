#pragma once

#include "hardware/interp.h"

inline void texture_mapping_setup(uint texture_size_bits, uint uv_fractional_bits)
{
    interp_config cfg = interp_default_config();
    // set add_raw flag to use raw (un-shifted and un-masked) lane accumulator value when adding
    // it to the the lane base to make the lane result
    interp_config_set_add_raw(&cfg, true);
    interp_config_set_shift(&cfg, uv_fractional_bits);
    interp_config_set_mask(&cfg, 0, texture_size_bits - 1);
    interp_set_config(interp0, 0, &cfg);

    interp_config_set_shift(&cfg, uv_fractional_bits - texture_size_bits);
    interp_config_set_mask(&cfg, texture_size_bits, texture_size_bits + texture_size_bits - 1);
    interp_set_config(interp0, 1, &cfg);

    //interp0->base[2] = (uintptr_t) texture;
    interp0->base[2] = 0;
}

inline void texture_mapped_span_begin(uint32_t u, uint32_t v, uint32_t du, uint32_t dv)
{
    // u, v are texture coordinates in fixed point with uv_fractional_bits fractional bits
    // du, dv are texture coordinate steps across the span in same fixed point.
    interp0->accum[0] = u;
    interp0->base[0] = du;
    interp0->accum[1] = v;
    interp0->base[1] = dv;
}

inline uint16_t texture_mapped_span_next()
{
    // equivalent to
    // uint32_t sm_result0 = (accum0 >> uv_fractional_bits) & ((1 << texture_width_bits) - 1);
    // uint32_t sm_result1 = (accum1 >> uv_fractional_bits) & ((1 << texture_height_bits) - 1);
    // uint8_t *address = texture + sm_result0 + (sm_result1 << texture_width_bits);
    // output[i] = *address;
    // accum0 = du + accum0;
    // accum1 = dv + accum1;

    // result2 is the texture address for the current pixel;
    // popping the result advances to the next iteration
    return interp0->pop[2];
}
#include "texture.h"
#include <cstring>
#include <limits>

void load_texture(texture_mipmap_t &texture, uint8_t max_size)
{
    const picosystem::color_t **mem_mip_chain = (const picosystem::color_t**)malloc(sizeof(picosystem::color_t*) * texture.mip_chain_length);

    uint32_t size = texture.size;
    for (size_t i = 0; i < texture.mip_chain_length; i++)
    {
        // Get buffer in flash
        const picosystem::color_t *flash_mip_item = texture.pixels[i];

        if (size > max_size)
        {
            // Just store a reference to the flash directly for this mip level
            mem_mip_chain[i] = flash_mip_item;
        }
        else
        {
            // Create new buffer
            size_t bytes = sizeof(picosystem::color_t) * size * size;
            picosystem::color_t *mem_mip_item = (picosystem::color_t*)malloc(bytes);
            mem_mip_chain[i] = mem_mip_item;

            // Copy it
            std::memcpy(mem_mip_item, flash_mip_item, bytes);
        }

        size >>= 1;
    }

    texture.pixels = mem_mip_chain;
}
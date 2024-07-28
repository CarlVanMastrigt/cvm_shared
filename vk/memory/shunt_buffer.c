/**
Copyright 2024 Carl van Mastrigt

This file is part of cvm_shared.

cvm_shared is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

cvm_shared is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with cvm_shared.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "cvm_shared.h"

void cvm_vk_staging_shunt_buffer_initialise(cvm_vk_staging_shunt_buffer * buffer, VkDeviceSize alignment, VkDeviceSize max_size, bool multithreaded)
{
    assert((alignment & (alignment-1)) == 0);///alignment must be a power of 2
    buffer->alignment = alignment;
    buffer->multithreaded = multithreaded;
    buffer->max_size = cvm_vk_align(max_size, alignment);
    if(multithreaded)
    {
        atomic_init(&buffer->atomic_offset, 0);
        buffer->size = buffer->max_size;
    }
    else
    {
        buffer->offset=0;
        buffer->size = CVM_MAX(16384, alignment * 4);
        assert(buffer->size <= buffer->max_size);/// specified size too small
    }
    buffer->backing = malloc(buffer->size);
}

void cvm_vk_staging_shunt_buffer_terminate(cvm_vk_staging_shunt_buffer * buffer)
{
    free(buffer->backing);
}


void cvm_vk_staging_shunt_buffer_reset(cvm_vk_staging_shunt_buffer * buffer)
{
    if(buffer->multithreaded)
    {
        atomic_store_explicit(&buffer->atomic_offset, 0, memory_order_relaxed);
    }
    else
    {
        buffer->offset=0;
    }
}

void * cvm_vk_staging_shunt_buffer_reserve_bytes(cvm_vk_staging_shunt_buffer * buffer, VkDeviceSize byte_count, VkDeviceSize * offset)
{
    uint_fast64_t current_offset;
    byte_count = cvm_vk_align(byte_count, buffer->alignment);

    if(buffer->multithreaded)
    {
        /// this implementation is a little more expensive but ensures that anything that would consume the buffer can actually use it
        current_offset=atomic_load_explicit(&buffer->atomic_offset, memory_order_relaxed);
        do
        {
            if(current_offset + byte_count > buffer->size)
            {
                return NULL;
            }
        }
        while (atomic_compare_exchange_weak_explicit(&buffer->atomic_offset, &current_offset, current_offset+byte_count, memory_order_relaxed, memory_order_relaxed));
        *offset = current_offset;
    }
    else
    {
        *offset = buffer->offset;
        buffer->offset += byte_count;

        if(buffer->offset > buffer->size)
        {
            do buffer->size = cvm_allocation_increase_step(buffer->size);
            while(buffer->offset > buffer->size);

            buffer->size = CVM_MIN(buffer->size, buffer->max_size);

            if(buffer->offset > buffer->size)
            {
                /// allocation cannot fit!
                buffer->offset -= byte_count;
                return NULL;
            }

            buffer->backing = realloc(buffer->backing, buffer->size);
        }
    }

    return buffer->backing + *offset;
}

VkDeviceSize cvm_vk_staging_shunt_buffer_get_space_used(cvm_vk_staging_shunt_buffer * buffer)
{
    if(buffer->multithreaded)
    {
        return atomic_load_explicit(&buffer->atomic_offset, memory_order_relaxed);
    }
    else
    {
        return buffer->offset;
    }
}

void cvm_vk_staging_shunt_buffer_copy(cvm_vk_staging_shunt_buffer * buffer, void * dst)
{
    memcpy(dst, buffer->backing, cvm_vk_staging_shunt_buffer_get_space_used(buffer));
}





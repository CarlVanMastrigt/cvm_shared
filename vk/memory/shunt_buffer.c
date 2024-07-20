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

void cvm_vk_staging_shunt_buffer_initialise(cvm_vk_staging_shunt_buffer * buffer, VkDeviceSize alignment/**, VkDeviceSize max_size, bool multithreaded*/)
{
    buffer->size=CVM_MAX(4096, alignment);
    buffer->backing=malloc(buffer->size);
    buffer->alignment=alignment;
    buffer->offset=0;
}

void cvm_vk_staging_shunt_buffer_terminate(cvm_vk_staging_shunt_buffer * buffer)
{
    free(buffer->backing);
}


void cvm_vk_staging_shunt_buffer_reset(cvm_vk_staging_shunt_buffer * buffer)
{
    buffer->offset=0;
}

void * cvm_vk_staging_shunt_buffer_reserve_bytes(cvm_vk_staging_shunt_buffer * buffer, VkDeviceSize byte_count)
{
    #warning have max viable size and assert that we are under it!
    void * ptr;

    if(buffer->offset+byte_count > buffer->size)
    {
        buffer->backing = realloc(buffer->backing, (buffer->size*=2));
    }
    ptr=buffer->backing+buffer->offset;
    buffer->offset+=byte_count;

    return ptr;
}

VkDeviceSize cvm_vk_staging_shunt_buffer_new_segment(cvm_vk_staging_shunt_buffer * buffer)
{
    buffer->offset = cvm_vk_align(buffer->offset, buffer->alignment);
    return buffer->offset;
}

void cvm_vk_staging_shunt_buffer_copy(cvm_vk_staging_shunt_buffer * buffer, void * dst)
{
    memcpy(dst,buffer->backing,buffer->offset);
}





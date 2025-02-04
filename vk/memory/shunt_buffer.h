/**
Copyright 2024 Carl van Mastrigt

This file is part of solipsix.

solipsix is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

solipsix is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with solipsix.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef solipsix_H
#include "solipsix.h"
#endif


#ifndef CVM_VK_SHUNT_BUFFER_H
#define CVM_VK_SHUNT_BUFFER_H

struct cvm_vk_shunt_buffer
{
    /// multithreaded variant is WAY more complicated, harsher restrictions on memory usage &c.
    bool multithreaded;
//    mtx_t access_mutex;
    char * backing;
    VkDeviceSize alignment;
    VkDeviceSize size;
    VkDeviceSize max_size;
    union
    {
        VkDeviceSize offset;/// non-multithreaded
        atomic_uint_fast64_t atomic_offset;/// multithreaded
    };

    /// add check to make sure nothing added after buffer gets copied?
};

void cvm_vk_shunt_buffer_initialise(struct cvm_vk_shunt_buffer * buffer, VkDeviceSize alignment, VkDeviceSize max_size, bool multithreaded);
void cvm_vk_shunt_buffer_terminate(struct cvm_vk_shunt_buffer * buffer);


void cvm_vk_shunt_buffer_reset(struct cvm_vk_shunt_buffer * buffer);
/// returns pointer to location which can be written, this pointer is only valid until next use, unless mltithreaded in which case it will return a persistently valid pointer or NULL
void * cvm_vk_shunt_buffer_reserve_bytes(struct cvm_vk_shunt_buffer * buffer, VkDeviceSize byte_count, VkDeviceSize * offset);

VkDeviceSize cvm_vk_shunt_buffer_get_space_used(struct cvm_vk_shunt_buffer * buffer);
void cvm_vk_shunt_buffer_copy(struct cvm_vk_shunt_buffer * buffer, void * dst);

#endif




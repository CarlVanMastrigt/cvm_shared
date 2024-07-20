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

#ifndef CVM_SHARED_H
#include "cvm_shared.h"
#endif


#ifndef CVM_VK_SHUNT_BUFFER_H
#define CVM_VK_SHUNT_BUFFER_H

typedef struct cvm_vk_staging_shunt_buffer
{
    /// multithreaded variant is WAY more complicated, harsher restrictions on memory usage &c.
//    bool multithreaded;
//    mtx_t access_mutex;
    char * backing;
    VkDeviceSize alignment;
    VkDeviceSize size;
    VkDeviceSize offset;
    /// add check to make sure nothing added after buffer gets copied?
}
cvm_vk_staging_shunt_buffer;

void cvm_vk_staging_shunt_buffer_initialise(cvm_vk_staging_shunt_buffer * buffer, VkDeviceSize alignment);
void cvm_vk_staging_shunt_buffer_terminate(cvm_vk_staging_shunt_buffer * buffer);


void cvm_vk_staging_shunt_buffer_reset(cvm_vk_staging_shunt_buffer * buffer);
/// returns pointer to location which can be written, this pointer is only valid until next use
void * cvm_vk_staging_shunt_buffer_reserve_bytes(cvm_vk_staging_shunt_buffer * buffer, VkDeviceSize byte_count);
VkDeviceSize cvm_vk_staging_shunt_buffer_new_segment(cvm_vk_staging_shunt_buffer * buffer);

void cvm_vk_staging_shunt_buffer_copy(cvm_vk_staging_shunt_buffer * buffer, void * dst);

#endif




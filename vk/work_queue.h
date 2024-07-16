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

#ifndef CVM_VK_H
#include "cvm_vk.h"
#endif


#ifndef CVM_VK_WORK_QUEUE_H
#define CVM_VK_WORK_QUEUE_H

#warning shunt_queue, holding_queue

typedef struct cvm_vk_work_queue_setup
{
    void * shared_data;
    size_t entry_size;
    uint32_t entry_count;

    void (*entry_initialise_func)(void*,void*);
    void (*entry_terminate_func)(void*,void*);
    void (*entry_reset_func)(void*,void*);
}
cvm_vk_work_queue_setup;

///name really needs work
typedef struct cvm_vk_work_queue
{
    void * shared_data;
    size_t entry_size;
    uint32_t entry_count;

    uint32_t available_index;
    uint32_t available_count;
    bool entry_in_use;//debugging

    /// basically entries form a queue
    cvm_vk_timeline_semaphore_moment * entry_completion_moments;///not sure, but this might want to be an array per moment
    char * entry_data;

    void (*entry_terminate_func)(void*,void*);
    void (*entry_reset_func)(void*,void*);
}
cvm_vk_work_queue;

void cvm_vk_work_queue_initialise(cvm_vk_work_queue * queue, const cvm_vk_work_queue_setup * setup);
void cvm_vk_work_queue_terminate(cvm_vk_device * device, cvm_vk_work_queue * queue);/// stalls on entries?

void * cvm_vk_work_queue_acquire_entry(cvm_vk_device * device, cvm_vk_work_queue * queue);///can stall while waiting on prior gpu work, will also cleanup any completed work while its at it
void cvm_vk_work_queue_release_entry(cvm_vk_work_queue * queue, void * entry, const cvm_vk_timeline_semaphore_moment * completion_moment);

#endif




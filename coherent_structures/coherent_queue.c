/**
Copyright 2024,2025 Carl van Mastrigt

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

#include "coherent_structures/coherent_queue.h"
#include <stdlib.h>
#include <assert.h>

void cvm_coherent_queue_initialise(cvm_coherent_queue* queue,cvm_lockfree_pool* pool)
{
    queue->entry_data = pool->available_entries.entry_data;
    queue->entry_size = pool->available_entries.entry_size;
    queue->capacity_exponent = pool->available_entries.capacity_exponent;///already guaranteed to respect limit
    queue->entry_indices = malloc(sizeof(uint32_t) << pool->available_entries.capacity_exponent);
    queue->entry_mask = (((uint_fast64_t)1) << pool->available_entries.capacity_exponent)-1;
    atomic_init(&queue->add_index,0);
    atomic_init(&queue->add_fence,0);
    atomic_init(&queue->get_index,0);
}

void cvm_coherent_queue_terminate(cvm_coherent_queue* queue)
{
    /// could/should check all atomics are equal
    free(queue->entry_indices);
}

void cvm_coherent_queue_push(cvm_coherent_queue* queue, void* entry)
{
    uint_fast64_t queue_index,expected_queue_index;
    uint32_t entry_index;

    assert((char*)entry >= queue->entry_data);
    assert((char*)entry < queue->entry_data + (queue->entry_size << queue->capacity_exponent));

    entry_index = (uint32_t)(((char*)entry - queue->entry_data) / (ptrdiff_t)queue->entry_size);

    queue_index = atomic_fetch_add_explicit(&queue->add_index, 1, memory_order_relaxed);

    queue->entry_indices[queue_index & queue->entry_mask] = entry_index;

    do expected_queue_index = queue_index;
    while(!atomic_compare_exchange_weak_explicit(&queue->add_fence,&expected_queue_index,queue_index+1,memory_order_release,memory_order_relaxed));
}

void* cvm_coherent_queue_pull(cvm_coherent_queue* queue)
{
    uint_fast64_t queue_index,current_fence_value;
    uint32_t entry_index;

    queue_index = atomic_load_explicit(&queue->get_index, memory_order_acquire);

    /** by releasing the prior get index upon success we also release (and carry)
     *  the fence being greater than (or equal to the new) get index
     *  and all the changes that occurr with it to the next get index acquire */

    do
    {
        current_fence_value = atomic_load_explicit(&queue->add_fence,memory_order_acquire);
        if(queue_index==current_fence_value)
        {
            return NULL;
        }
    }
    while(!atomic_compare_exchange_weak_explicit(&queue->get_index, &queue_index, queue_index+1, memory_order_release, memory_order_acquire));

    entry_index = queue->entry_indices[queue_index & queue->entry_mask];

    return queue->entry_data + entry_index * queue->entry_size;
}



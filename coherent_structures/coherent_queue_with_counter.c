/**
Copyright 2024,2025 Carl van Mastrigt

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

#include <stdlib.h>
#include <assert.h>

#include "coherent_structures/coherent_queue_with_counter.h"
#include "coherent_structures/lockfree_pool.h"


/// these can support up to u32 sized queue and up to 2^31-1 fails tracked (need one more bit than count represents to handle wrapping properly)
#define SOL_COHERENT_QUEUE_FENCE_MASK ((uint_fast64_t)0x00000001FFFFFFFF)
#define SOL_COHERENT_QUEUE_COUNT_MASK ((uint_fast64_t)0xFFFFFFFE00000000)
#define SOL_COHERENT_QUEUE_COUNT_UNIT ((uint_fast64_t)0x0000000200000000)

void sol_coherent_queue_with_counter_initialise(sol_coherent_queue_with_counter* queue,sol_lockfree_pool* pool)
{
    queue->entry_data = pool->available_entries.entry_data;
    queue->entry_size = pool->available_entries.entry_size;
    queue->capacity_exponent = pool->available_entries.capacity_exponent;
    queue->entry_indices = malloc(sizeof(uint32_t) << pool->available_entries.capacity_exponent);
    queue->entry_mask = ((uint_fast64_t)1 << pool->available_entries.capacity_exponent)-1;
    atomic_init(&queue->add_index,0);
    atomic_init(&queue->add_fence,0);
    atomic_init(&queue->get_index,0);
}

void sol_coherent_queue_with_counter_terminate(sol_coherent_queue_with_counter* queue)
{
    /// could/should check all atomics are equal (except fence which needs to have its comparisons masked by SOL_COHERENT_QUEUE_FENCE_MASK
    free(queue->entry_indices);
}

void sol_coherent_queue_with_counter_push(sol_coherent_queue_with_counter* queue, void* entry)
{
    uint_fast64_t queue_index, fence_value, replacement_fence_value, counter;
    uint32_t entry_index;

    assert((char*)entry >= queue->entry_data);
    assert((char*)entry < queue->entry_data + (queue->entry_size << queue->capacity_exponent));

    entry_index = (uint32_t) (((char*)entry - queue->entry_data) / (ptrdiff_t)queue->entry_size);

    queue_index = atomic_fetch_add_explicit(&queue->add_index, 1, memory_order_relaxed);

    queue->entry_indices[queue_index & queue->entry_mask] = entry_index;

    ///assume no count by default
    fence_value = (queue_index & SOL_COHERENT_QUEUE_FENCE_MASK);
    replacement_fence_value = ((queue_index+1) & SOL_COHERENT_QUEUE_FENCE_MASK);

    while(!atomic_compare_exchange_weak_explicit(&queue->add_fence, &fence_value, replacement_fence_value, memory_order_release, memory_order_relaxed))
    {
        /// expected_fence_value has current fence value here
        counter = (fence_value & SOL_COHERENT_QUEUE_COUNT_MASK);
        fence_value = (queue_index & SOL_COHERENT_QUEUE_FENCE_MASK) | counter;
        replacement_fence_value = ((queue_index+1) & SOL_COHERENT_QUEUE_FENCE_MASK) | counter;
    }
}

void* sol_coherent_queue_with_counter_pull(sol_coherent_queue_with_counter* queue)
{
    uint_fast64_t queue_index, fence_value;
    uint32_t entry_index;

    queue_index = atomic_load_explicit(&queue->get_index, memory_order_acquire);

    do
    {
        fence_value = atomic_load_explicit(&queue->add_fence, memory_order_acquire);
        if((queue_index&SOL_COHERENT_QUEUE_FENCE_MASK) == (fence_value&SOL_COHERENT_QUEUE_FENCE_MASK))
        {
            return NULL;
        }
    }
    while(!atomic_compare_exchange_weak_explicit(&queue->get_index, &queue_index, queue_index+1, memory_order_release, memory_order_acquire));

    entry_index=queue->entry_indices[queue_index&queue->entry_mask];
    return queue->entry_data + entry_index*queue->entry_size;
}

bool sol_coherent_queue_with_counter_push_and_decrement(sol_coherent_queue_with_counter* queue, void* entry)
{
    uint_fast64_t queue_index, fence_value, replacement_fence_value, counter;
    uint32_t entry_index;
    bool nonzero_count;

    nonzero_count = false;

    assert((char*)entry >= queue->entry_data);
    assert((char*)entry < queue->entry_data + (queue->entry_size << queue->capacity_exponent));

    entry_index = (uint32_t) (((char*)entry - queue->entry_data) / (ptrdiff_t)queue->entry_size);

    queue_index = atomic_fetch_add_explicit(&queue->add_index, 1, memory_order_relaxed);

    queue->entry_indices[queue_index & queue->entry_mask] = entry_index;

    ///assume no stalls by default
    fence_value = (queue_index & SOL_COHERENT_QUEUE_FENCE_MASK);
    replacement_fence_value = ((queue_index+1) & SOL_COHERENT_QUEUE_FENCE_MASK);

    while(!atomic_compare_exchange_weak_explicit(&queue->add_fence, &fence_value, replacement_fence_value, memory_order_release, memory_order_relaxed))
    {
        /// expected_fence_value has current fence value here
        counter = (fence_value & SOL_COHERENT_QUEUE_COUNT_MASK);
        fence_value = (queue_index & SOL_COHERENT_QUEUE_FENCE_MASK) | counter;
        nonzero_count = counter != 0;
        assert(counter>=SOL_COHERENT_QUEUE_COUNT_UNIT || counter==0);

        replacement_fence_value = ((queue_index+1) & SOL_COHERENT_QUEUE_FENCE_MASK) | (counter - (nonzero_count ? SOL_COHERENT_QUEUE_COUNT_UNIT : 0));
    }

    return nonzero_count;
}

void* sol_coherent_queue_with_counter_pull_or_increment(sol_coherent_queue_with_counter* queue)
{
    uint_fast64_t queue_index, fence_value, replacement_fence_value;
    uint32_t entry_index;

    queue_index = atomic_load_explicit(&queue->get_index, memory_order_acquire);

    do
    {
        fence_value = atomic_load_explicit(&queue->add_fence, memory_order_acquire);

        while((queue_index&SOL_COHERENT_QUEUE_FENCE_MASK) == (fence_value&SOL_COHERENT_QUEUE_FENCE_MASK))
        {
            /// despite being in a loop i'm unsure this should be weak or strong
            replacement_fence_value = fence_value + SOL_COHERENT_QUEUE_COUNT_UNIT;
            /// acquire only technically needed on failure as that represents external change to data that needs to be acquired, otherwise no changes need to be acquired
            if(atomic_compare_exchange_weak_explicit(&queue->add_fence, &fence_value, replacement_fence_value, memory_order_acquire, memory_order_acquire))
            {
                /// if we sucessfully marked the fence as having been stalled on before anything was added to the queue we can return null to signify a sucessful stall
                return NULL;
            }
        }
    }
    while(!atomic_compare_exchange_weak_explicit(&queue->get_index, &queue_index, queue_index+1, memory_order_release, memory_order_acquire));

    entry_index = queue->entry_indices[queue_index & queue->entry_mask];
    return queue->entry_data + entry_index * queue->entry_size;
}






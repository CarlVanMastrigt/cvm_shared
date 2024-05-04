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




void cvm_coherent_queue_initialise(cvm_coherent_queue * queue,cvm_lockfree_pool * pool)
{
    queue->entry_data=pool->available_entries.entry_data;
    queue->entry_size=pool->available_entries.entry_size;
    queue->capacity_exponent=pool->available_entries.capacity_exponent;
    queue->entry_indices=malloc(sizeof(uint16_t) << pool->available_entries.capacity_exponent);
    queue->entry_mask=(((uint_fast64_t)1) << pool->available_entries.capacity_exponent)-1;
    atomic_init(&queue->add_index,0);
    atomic_init(&queue->add_fence,0);
    atomic_init(&queue->get_index,0);
}

void cvm_coherent_queue_terminate(cvm_coherent_queue * queue)
{
    free(queue->entry_indices);
}

void cvm_coherent_queue_add(cvm_coherent_queue * queue, void * entry)
{
    uint_fast64_t queue_index,expected_queue_index,entry_index;
    size_t entry_offset;

    assert((char*)entry >= queue->entry_data);

    entry_offset = (char*)entry - queue->entry_data;

    entry_index = (uint_fast64_t) (entry_offset / queue->entry_size);

    assert(entry_index < ((uint_fast64_t)1 << queue->capacity_exponent));

    queue_index = atomic_fetch_add_explicit(&queue->add_index, 1, memory_order_relaxed);

    queue->entry_indices[queue_index & queue->entry_mask] = entry_index;

    do expected_queue_index = queue_index;
    while(!atomic_compare_exchange_weak_explicit(&queue->add_fence,&expected_queue_index,queue_index+1,memory_order_release,memory_order_relaxed));
}

void * cvm_coherent_queue_get(cvm_coherent_queue * queue)
{
    uint_fast64_t queue_index,entry_index,current_fence_value;

    queue_index = atomic_load_explicit(&queue->get_index, memory_order_relaxed);

    do
    {
        current_fence_value = atomic_load_explicit(&queue->add_fence,memory_order_acquire);
        if(queue_index==current_fence_value)
        {
            return NULL;
        }
    }
    while(!atomic_compare_exchange_weak_explicit(&queue->get_index,&queue_index,queue_index+1,memory_order_release,memory_order_relaxed));

    entry_index=queue->entry_indices[queue_index&queue->entry_mask];
    return queue->entry_data + entry_index*queue->entry_size;
}





#define CVM_COHERENT_QUEUE_FENCE_MASK ((uint_fast64_t)0x00000000FFFFFFFF)
#define CVM_COHERENT_QUEUE_FAILURE_MASK ((uint_fast64_t)0xFFFFFFFF00000000)
#define CVM_COHERENT_QUEUE_FAILURE_UNIT ((uint_fast64_t)0x0000000100000000)

void cvm_coherent_queue_fail_tracking_initialise(cvm_coherent_queue_fail_tracking * queue,cvm_lockfree_pool * pool)
{
    queue->entry_data=pool->available_entries.entry_data;
    queue->entry_size=pool->available_entries.entry_size;
    queue->capacity_exponent=pool->available_entries.capacity_exponent;
    queue->entry_indices=malloc(sizeof(uint16_t) << pool->available_entries.capacity_exponent);
    queue->entry_mask=((uint_fast64_t)1 << pool->available_entries.capacity_exponent)-1;
    atomic_init(&queue->add_index,0);
    atomic_init(&queue->add_fence,0);
    atomic_init(&queue->get_index,0);
}

void cvm_coherent_queue_fail_tracking_terminate(cvm_coherent_queue_fail_tracking * queue)
{
    free(queue->entry_indices);
}

void cvm_coherent_queue_fail_tracking_add(cvm_coherent_queue_fail_tracking * queue, void * entry)
{
    uint_fast64_t queue_index,expected_fence_value,replacement_fence_value,entry_index,failure_counter;
    size_t entry_offset;

    assert((char*)entry >= queue->entry_data);

    entry_offset = (char*)entry - queue->entry_data;

    entry_index = (uint_fast64_t) (entry_offset / queue->entry_size);

    assert(entry_index < ((uint_fast64_t)1 << queue->capacity_exponent));

    queue_index = atomic_fetch_add_explicit(&queue->add_index, 1, memory_order_relaxed);

    queue->entry_indices[queue_index & queue->entry_mask] = entry_index;

    ///assume no stalls by default
    expected_fence_value = (queue_index & CVM_COHERENT_QUEUE_FENCE_MASK);
    replacement_fence_value = ((queue_index+1) & CVM_COHERENT_QUEUE_FENCE_MASK);

    while(!atomic_compare_exchange_weak_explicit(&queue->add_fence,&expected_fence_value,replacement_fence_value,memory_order_release,memory_order_relaxed))
    {
        /// expected_fence_value has current fence value here
        failure_counter = (expected_fence_value & CVM_COHERENT_QUEUE_FAILURE_MASK);
        expected_fence_value = (queue_index & CVM_COHERENT_QUEUE_FENCE_MASK) | failure_counter;
        replacement_fence_value = ((queue_index+1) & CVM_COHERENT_QUEUE_FENCE_MASK) | failure_counter;
    }
}

void * cvm_coherent_queue_fail_tracking_get(cvm_coherent_queue_fail_tracking * queue)
{
    uint_fast64_t queue_index,entry_index,current_fence_value;

    queue_index = atomic_load_explicit(&queue->get_index, memory_order_relaxed);

    do
    {
        current_fence_value = atomic_load_explicit(&queue->add_fence,memory_order_acquire);
        if((queue_index&CVM_COHERENT_QUEUE_FENCE_MASK) == (current_fence_value&CVM_COHERENT_QUEUE_FENCE_MASK))
        {
            return NULL;
        }
    }
    while(!atomic_compare_exchange_weak_explicit(&queue->get_index,&queue_index,queue_index+1,memory_order_release,memory_order_relaxed));

    entry_index=queue->entry_indices[queue_index&queue->entry_mask];
    return queue->entry_data + entry_index*queue->entry_size;
}

bool cvm_coherent_queue_fail_tracking_add_and_decrement_fails(cvm_coherent_queue_fail_tracking * queue, void * entry)
{
    uint_fast64_t queue_index,expected_fence_value,replacement_fence_value,entry_index,failure_counter;
    size_t entry_offset;
    bool decremented_failure_counter = false;

    assert((char*)entry >= queue->entry_data);

    entry_offset = (char*)entry - queue->entry_data;

    entry_index = (uint_fast64_t) (entry_offset / queue->entry_size);

    assert(entry_index < ((uint_fast64_t)1 << queue->capacity_exponent));

    queue_index = atomic_fetch_add_explicit(&queue->add_index, 1, memory_order_relaxed);

    queue->entry_indices[queue_index & queue->entry_mask] = entry_index;

    ///assume no stalls by default
    expected_fence_value = (queue_index & CVM_COHERENT_QUEUE_FENCE_MASK);
    replacement_fence_value = ((queue_index+1) & CVM_COHERENT_QUEUE_FENCE_MASK);

    while(!atomic_compare_exchange_weak_explicit(&queue->add_fence,&expected_fence_value,replacement_fence_value,memory_order_release,memory_order_relaxed))
    {
        /// expected_fence_value has current fence value here
        failure_counter = (expected_fence_value & CVM_COHERENT_QUEUE_FAILURE_MASK);
        decremented_failure_counter = failure_counter > 0;
        if(decremented_failure_counter)
        {
            failure_counter -= CVM_COHERENT_QUEUE_FAILURE_UNIT;
        }
        expected_fence_value = (queue_index & CVM_COHERENT_QUEUE_FENCE_MASK) | failure_counter;
        replacement_fence_value = ((queue_index+1) & CVM_COHERENT_QUEUE_FENCE_MASK) | failure_counter;
    }

    return decremented_failure_counter;
}
void * cvm_coherent_queue_fail_tracking_get_or_increment_fails(cvm_coherent_queue_fail_tracking * queue)
{
    uint_fast64_t queue_index,entry_index,current_fence_value,replacement_fence_value;

    queue_index = atomic_load_explicit(&queue->get_index, memory_order_relaxed);

    do
    {
        current_fence_value = atomic_load_explicit(&queue->add_fence,memory_order_acquire);

        while((queue_index&CVM_COHERENT_QUEUE_FENCE_MASK) == (current_fence_value&CVM_COHERENT_QUEUE_FENCE_MASK))
        {
            /// despite being in a loop i'm unsure this should be weak instead of strong
            replacement_fence_value = current_fence_value + CVM_COHERENT_QUEUE_FAILURE_UNIT;
            /// acquire only technically needed on failure as that represents external change to data that needs to be acquired, otherwise no changes need to be acquired
            if(atomic_compare_exchange_weak_explicit(&queue->add_fence,&current_fence_value,replacement_fence_value,memory_order_acquire,memory_order_acquire))
            {
                /// if we sucessfully stalled before anything was added to the queue we can stall (return null)
                return NULL;
            }
        }
    }
    while(!atomic_compare_exchange_weak_explicit(&queue->get_index,&queue_index,queue_index+1,memory_order_release,memory_order_relaxed));

    entry_index=queue->entry_indices[queue_index&queue->entry_mask];
    return queue->entry_data + entry_index*queue->entry_size;
}






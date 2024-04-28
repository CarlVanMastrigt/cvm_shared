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


void cvm_coherent_queue_pool_initialise(cvm_coherent_queue_pool * pool, size_t capacity_exponent, size_t entry_size)
{
    size_t i,count;

    assert(capacity_exponent<16);

    pool->entry_data=malloc(entry_size << capacity_exponent);
    pool->next_buffer=malloc(sizeof(uint16_t) << capacity_exponent);
    pool->capacity_exponent=capacity_exponent;
    pool->entry_size=entry_size;

    count=(size_t)1 << capacity_exponent;

    atomic_init(&pool->next_stack_head,0);
    for(i=0;i<count-1;i++)// all entries start off in the available queue
    {
        pool->next_buffer[i]=i+1;
    }
    pool->next_buffer[count-1]=CVM_INVALID_U16_INDEX;
}
void cvm_coherent_queue_pool_terminate(cvm_coherent_queue_pool * pool)
{
    free(pool->next_buffer);
    free(pool->entry_data);
}



void * cvm_coherent_queue_pool_acquire_entry(cvm_coherent_queue_pool * pool)
{
    uint_fast64_t entry_index,current_header_value,replacement_header_value;

    current_header_value=atomic_load_explicit(&pool->next_stack_head, memory_order_acquire);
    do
    {
        entry_index = current_header_value & CVM_U16_U64_LOWER_MASK;
        /// if there are no more entries in this list then return NULL
        if(entry_index == CVM_INVALID_U16_INDEX)
        {
            return NULL;
        }

        replacement_header_value = ((current_header_value & CVM_U16_U64_UPPER_MASK) + CVM_U16_U64_UPPER_UNIT) | (uint_fast64_t)(pool->next_buffer[entry_index]);
    }
    while(!atomic_compare_exchange_weak_explicit(&pool->next_stack_head, &current_header_value, replacement_header_value, memory_order_acquire, memory_order_acquire));
    /// success memory order could (conceptually) be relaxed here as we don't need to release/acquire any changes when that happens as nothing outside the atomic has changed
    /// but fail must acquire as the "next" member of "first" element may have changed

    assert(entry_index < ((uint_fast64_t)1 << pool->capacity_exponent));

    return pool->entry_data + entry_index*pool->entry_size;
}
void cvm_coherent_queue_pool_relinquish_entry(cvm_coherent_queue_pool * pool, void * entry)
{
    uint_fast64_t entry_index,current_header_value,replacement_header_value;
    entry_index = (uint_fast64_t)(( ((char*)entry) - pool->entry_data )/pool->entry_size);
    assert(entry_index < ((uint_fast64_t)1 << pool->capacity_exponent));

    current_header_value=atomic_load_explicit(&pool->next_stack_head, memory_order_relaxed);
    do
    {
        pool->next_buffer[entry_index] = (uint16_t)(current_header_value & CVM_U16_U64_LOWER_MASK);
        replacement_header_value=((current_header_value & CVM_U16_U64_UPPER_MASK) + CVM_U16_U64_UPPER_UNIT) | entry_index;
    }
    while(!atomic_compare_exchange_weak_explicit(&pool->next_stack_head, &current_header_value, replacement_header_value, memory_order_release, memory_order_relaxed));
}






void cvm_coherent_queue_initiasise(cvm_coherent_queue_pool * pool,cvm_coherent_queue * queue)
{
    queue->entry_data=pool->entry_data;
    queue->entry_size=pool->entry_size;
    queue->capacity_exponent=pool->capacity_exponent;
    queue->entry_indices=malloc(sizeof(uint16_t) << pool->capacity_exponent);
    queue->entry_mask=(((uint_fast64_t)1) << pool->capacity_exponent)-1;
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

void cvm_coherent_fail_tracking_queue_initiasise(cvm_coherent_queue_pool * pool,cvm_coherent_fail_tracking_queue * queue)
{
    queue->entry_data=pool->entry_data;
    queue->entry_size=pool->entry_size;
    queue->capacity_exponent=pool->capacity_exponent;
    queue->entry_indices=malloc(sizeof(uint16_t) << pool->capacity_exponent);
    queue->entry_mask=((uint_fast64_t)1 << pool->capacity_exponent)-1;
    atomic_init(&queue->add_index,0);
    atomic_init(&queue->add_fence,0);
    atomic_init(&queue->get_index,0);
}

void cvm_coherent_fail_tracking_queue_terminate(cvm_coherent_fail_tracking_queue * queue)
{
    free(queue->entry_indices);
}

void cvm_coherent_fail_tracking_queue_add(cvm_coherent_fail_tracking_queue * queue, void * entry)
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

void * cvm_coherent_fail_tracking_queue_get(cvm_coherent_fail_tracking_queue * queue)
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

bool cvm_coherent_fail_tracking_queue_add_and_decrement_fails(cvm_coherent_fail_tracking_queue * queue, void * entry)
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
void * cvm_coherent_fail_tracking_queue_get_or_increment_fails(cvm_coherent_fail_tracking_queue * queue)
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






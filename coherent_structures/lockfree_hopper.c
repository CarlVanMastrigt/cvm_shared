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

#include "coherent_structures.h"
#include <assert.h>


#define CVM_LOCKFREE_HOPPER_LOCK_BIT  ((uint_fast64_t)0x8000000000000000llu)



void cvm_lockfree_hopper_initialise(cvm_lockfree_hopper* hopper, cvm_lockfree_pool* pool)
{
    assert(pool->available_entries.capacity_exponent <= 16);///capacities over 2^16 not supported by lockfree hopper as is
    atomic_init(&hopper->head, CVM_LOCKFREE_HOPPER_LOCK_BIT);
}
void cvm_lockfree_hopper_terminate(cvm_lockfree_hopper* hopper)
{
    assert(atomic_load_explicit(&hopper->head, memory_order_relaxed) == CVM_LOCKFREE_HOPPER_LOCK_BIT);/// hopper should be locked/invalid upon termination
}

void cvm_lockfree_hopper_reset(cvm_lockfree_hopper* hopper)
{
    uint_fast64_t prev_value;

    prev_value = atomic_exchange_explicit(&hopper->head, CVM_LOCKFREE_STACK_INVALID_ENTRY, memory_order_relaxed);

    assert(prev_value == CVM_LOCKFREE_HOPPER_LOCK_BIT);/// hopper should be locked/invalid upon reset
}

bool cvm_lockfree_hopper_push(cvm_lockfree_hopper * hopper, cvm_lockfree_pool * pool, void* entry)
{
    uint_fast64_t current_head, replacement_head;
    ptrdiff_t entry_index;

    assert((char*)entry >= pool->available_entries.entry_data);

    entry_index = (ptrdiff_t)((char*)entry - pool->available_entries.entry_data) / pool->available_entries.entry_size;

    assert(entry_index < ((uint_fast64_t)1 << pool->available_entries.capacity_exponent));

    current_head=atomic_load_explicit(&hopper->head, memory_order_relaxed);
    do
    {
        if(current_head & CVM_LOCKFREE_HOPPER_LOCK_BIT)
        {
            assert(current_head == CVM_LOCKFREE_HOPPER_LOCK_BIT);// if locked should ONLY be locked
            return false;
        }

        assert((current_head & CVM_LOCKFREE_STACK_ENTRY_MASK) == current_head);

        pool->available_entries.next_buffer[entry_index] = (uint32_t)current_head;
        replacement_head = entry_index;
    }
    while(!atomic_compare_exchange_weak_explicit(&hopper->head, &current_head, replacement_head, memory_order_release, memory_order_relaxed));

    return true;
}

bool cvm_lockfree_hopper_check_if_locked(cvm_lockfree_hopper * hopper)
{
    return atomic_load_explicit(&hopper->head, memory_order_relaxed) & CVM_LOCKFREE_HOPPER_LOCK_BIT;
}

void * cvm_lockfree_hopper_lock(cvm_lockfree_hopper * hopper, cvm_lockfree_pool * pool, uint32_t* first_entry_index)
{
    uint_fast64_t entry_index;

    entry_index = atomic_load_explicit(&hopper->head, memory_order_acquire);
    do
    {
        assert( !(entry_index & CVM_LOCKFREE_HOPPER_LOCK_BIT));/// cannot lock the hopper twice
    }
    while(!atomic_compare_exchange_weak_explicit(&hopper->head, &entry_index, CVM_LOCKFREE_HOPPER_LOCK_BIT, memory_order_acquire, memory_order_relaxed));

    assert((entry_index & CVM_LOCKFREE_STACK_ENTRY_MASK) == entry_index);

    // need to know if the entry is invalid
    *first_entry_index = entry_index;

    /// if there are no entries in this hopper at all then return NULL
    if(entry_index == CVM_LOCKFREE_STACK_INVALID_ENTRY)
    {
        return NULL;
    }

    assert(entry_index < ((uint_fast64_t)1 << pool->available_entries.capacity_exponent));

    return pool->available_entries.entry_data + entry_index * pool->available_entries.entry_size;
}

void* cvm_lockfree_hopper_iterate(cvm_lockfree_pool* pool, uint32_t* entry_index)
{
    uint32_t next_entry_index;

    next_entry_index = pool->available_entries.next_buffer[*entry_index];

    if(next_entry_index == CVM_LOCKFREE_STACK_INVALID_ENTRY)
    {
        return NULL;
    }
    else
    {
        *entry_index = next_entry_index;
        return pool->available_entries.entry_data + next_entry_index * pool->available_entries.entry_size;
    }
}

void cvm_lockfree_hopper_relinquish_range(cvm_lockfree_pool* pool, uint32_t first_entry_index, uint32_t last_entry_index)
{
    // could have been given an empty range by a hopper
    if(first_entry_index == CVM_LOCKFREE_STACK_INVALID_ENTRY)
    {
        assert(last_entry_index == CVM_LOCKFREE_STACK_INVALID_ENTRY);
    }
    else
    {
        cvm_lockfree_pool_relinquish_entry_index_range(pool, first_entry_index, last_entry_index);
    }
}
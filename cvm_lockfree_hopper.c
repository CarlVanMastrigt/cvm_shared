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

#define CVM_LOCKFREE_HOPPER_CHECK_MASK ((uint_fast64_t)0xFFFFFFFFFFFE0000llu)
#define CVM_LOCKFREE_HOPPER_ENTRY_MASK ((uint_fast64_t)0x000000000000FFFFllu)

#define CVM_LOCKFREE_HOPPER_INVALID_ENTRY ((uint_fast64_t)0x000000000000FFFFllu)

#define CVM_LOCKFREE_HOPPER_LOCK_BIT  ((uint_fast64_t)0x0000000000010000llu)

#define CVM_LOCKFREE_HOPPER_CHECK_UNIT ((uint_fast64_t)0x0000000000020000llu)
#define CVM_LOCKFREE_HOPPER_ENTRY_UNIT ((uint_fast64_t)0x0000000000000001llu)


void cvm_lockfree_hopper_initialise(cvm_lockfree_hopper * hopper, cvm_lockfree_pool * pool)
{
    assert(pool->available_entries.capacity_exponent <= 16);///capacities over 2^16 not supported by lockfree hopper as is
    atomic_init(&hopper->head,CVM_LOCKFREE_HOPPER_INVALID_ENTRY);
}
void cvm_lockfree_hopper_terminate(cvm_lockfree_hopper * hopper)
{
    assert((atomic_load_explicit(&hopper->head, memory_order_relaxed) & CVM_LOCKFREE_HOPPER_ENTRY_MASK) == CVM_LOCKFREE_HOPPER_INVALID_ENTRY);/// hopper should be empty upon termination
}

void cvm_lockfree_hopper_reset(cvm_lockfree_hopper * hopper)
{
    assert((atomic_load_explicit(&hopper->head, memory_order_relaxed) & CVM_LOCKFREE_HOPPER_ENTRY_MASK) == CVM_LOCKFREE_HOPPER_INVALID_ENTRY);/// hopper should be empty upon reset

    atomic_store_explicit(&hopper->head, CVM_LOCKFREE_HOPPER_INVALID_ENTRY, memory_order_relaxed);
}

bool cvm_lockfree_hopper_add(cvm_lockfree_hopper * hopper, cvm_lockfree_pool * pool, void * entry)
{
    uint_fast64_t entry_index,current_head,replacement_head;
    size_t entry_offset;

    assert((char*)entry >= pool->available_entries.entry_data);

    entry_offset = (char*)entry - pool->available_entries.entry_data;
    entry_index = (uint_fast64_t) (entry_offset / pool->available_entries.entry_size);

    assert(entry_index < ((uint_fast64_t)1 << pool->available_entries.capacity_exponent));

    current_head=atomic_load_explicit(&hopper->head, memory_order_relaxed);
    do
    {
        if(current_head & CVM_LOCKFREE_HOPPER_LOCK_BIT)
        {
            return false;
        }
        pool->available_entries.next_buffer[entry_index] = (uint16_t)(current_head & CVM_LOCKFREE_HOPPER_ENTRY_MASK);
        replacement_head=((current_head & CVM_LOCKFREE_HOPPER_CHECK_MASK) + CVM_LOCKFREE_HOPPER_CHECK_UNIT) | entry_index;
    }
    while(!atomic_compare_exchange_weak_explicit(&hopper->head, &current_head, replacement_head, memory_order_release, memory_order_relaxed));

    return true;
}

bool cvm_lockfree_hopper_check_if_locked(cvm_lockfree_hopper * hopper)
{
    return atomic_load_explicit(&hopper->head, memory_order_relaxed) & CVM_LOCKFREE_HOPPER_LOCK_BIT;
}

void * cvm_lockfree_hopper_lock_and_get_first(cvm_lockfree_hopper * hopper, cvm_lockfree_pool * pool)
{
    uint_fast64_t entry_index,current_head,replacement_head;

    current_head=atomic_load_explicit(&hopper->head, memory_order_acquire);
    do
    {
        assert((current_head & CVM_LOCKFREE_HOPPER_LOCK_BIT)==0);/// cannot lock the hopper twice
        replacement_head = CVM_LOCKFREE_HOPPER_INVALID_ENTRY | CVM_LOCKFREE_HOPPER_LOCK_BIT;/// lock and empty contents
    }
    while(!atomic_compare_exchange_weak_explicit(&hopper->head, &current_head, replacement_head, memory_order_acquire, memory_order_acquire));

    entry_index = current_head & CVM_LOCKFREE_HOPPER_ENTRY_MASK;

    /// if there are no entries in this hopper at all then return NULL
    if(entry_index == CVM_LOCKFREE_HOPPER_INVALID_ENTRY)
    {
        return NULL;
    }

    assert(entry_index < ((uint_fast64_t)1 << pool->available_entries.capacity_exponent));

    return pool->available_entries.entry_data + entry_index*pool->available_entries.entry_size;
}

void * cvm_lockfree_hopper_get_next(cvm_lockfree_pool * pool, void * previous_entry)
{
    uint_fast64_t previous_entry_index;
    size_t previous_entry_offset;

    assert(previous_entry!=NULL);
    assert((char*)previous_entry >= pool->available_entries.entry_data);

    previous_entry_offset = (char*)previous_entry - pool->available_entries.entry_data;
    previous_entry_index = (uint_fast64_t) (previous_entry_offset / pool->available_entries.entry_size);

    assert(previous_entry_index < ((uint_fast64_t)1 << pool->available_entries.capacity_exponent));

    if(previous_entry_index == CVM_LOCKFREE_HOPPER_INVALID_ENTRY)
    {
        return NULL;
    }
    else
    {
        return pool->available_entries.entry_data + pool->available_entries.next_buffer[previous_entry_index]*pool->available_entries.entry_size;
    }
}

void * cvm_lockfree_hopper_relinquish_and_get_next(cvm_lockfree_pool * pool, void * current_entry)
{
    uint_fast64_t current_entry_index,next_entry_index;
    size_t current_entry_offset;
    void * next_entry;

    assert(current_entry!=NULL);
    assert((char*)current_entry >= pool->available_entries.entry_data);

    current_entry_offset = (char*)current_entry - pool->available_entries.entry_data;
    current_entry_index = (uint_fast64_t) (current_entry_offset / pool->available_entries.entry_size);

    assert(current_entry_index < ((uint_fast64_t)1 << pool->available_entries.capacity_exponent));

    next_entry_index = pool->available_entries.next_buffer[current_entry_index];

    if(next_entry_index == CVM_LOCKFREE_HOPPER_INVALID_ENTRY)
    {
        next_entry = NULL;
    }
    else
    {
        next_entry = pool->available_entries.entry_data + next_entry_index*pool->available_entries.entry_size;
    }

    cvm_lockfree_pool_relinquish_entry(pool, current_entry);

    return next_entry;
}

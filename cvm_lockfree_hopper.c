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

#warning check mask shouldn't even be necessary, as hopper should ONLY add contents between being reset...
#warning this doesnt *really* support 2^16 (last element would be invalid)

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

#warning as check mask is unnecessary: can do validation more easily and require lock before reset is called (default state)
void cvm_lockfree_hopper_reset(cvm_lockfree_hopper * hopper)
{
    assert((atomic_load_explicit(&hopper->head, memory_order_relaxed) & CVM_LOCKFREE_HOPPER_ENTRY_MASK) == CVM_LOCKFREE_HOPPER_INVALID_ENTRY);/// hopper should be empty upon reset

    atomic_store_explicit(&hopper->head, CVM_LOCKFREE_HOPPER_INVALID_ENTRY, memory_order_relaxed);
}

bool cvm_lockfree_hopper_push(cvm_lockfree_hopper * hopper, cvm_lockfree_pool * pool, void * entry)
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

void * cvm_lockfree_hopper_lock_and_get_first(cvm_lockfree_hopper * hopper, cvm_lockfree_pool * pool, uint32_t* entry_index_dst)
{
    uint_fast64_t entry_index, current_head, replacement_head;

    current_head=atomic_load_explicit(&hopper->head, memory_order_acquire);
    do
    {
        assert((current_head & CVM_LOCKFREE_HOPPER_LOCK_BIT)==0);/// cannot lock the hopper twice
        replacement_head = CVM_LOCKFREE_HOPPER_INVALID_ENTRY | CVM_LOCKFREE_HOPPER_LOCK_BIT;/// lock and empty contents
    }
    while(!atomic_compare_exchange_weak_explicit(&hopper->head, &current_head, replacement_head, memory_order_acquire, memory_order_acquire));

    entry_index = current_head & CVM_LOCKFREE_HOPPER_ENTRY_MASK;

    // need to know if the entry is invalid
    if(entry_index_dst)
    {
        *entry_index_dst = entry_index;
    }

    /// if there are no entries in this hopper at all then return NULL
    if(entry_index == CVM_LOCKFREE_HOPPER_INVALID_ENTRY)
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

    if(next_entry_index == CVM_LOCKFREE_HOPPER_INVALID_ENTRY)
    {
        return NULL;
    }
    else
    {
        *entry_index = next_entry_index;
        return pool->available_entries.entry_data + next_entry_index * pool->available_entries.entry_size;
    }
}

void* cvm_lockfree_hopper_relinquish_range(cvm_lockfree_pool* pool, uint32_t first_entry_index, uint32_t last_entry_index)
{
    // could have been given an empty range by a hopper
    if(first_entry_index==CVM_LOCKFREE_STACK_INVALID_ENTRY)
    {
        assert(last_entry_index==CVM_LOCKFREE_STACK_INVALID_ENTRY);
        return;
    }

    cvm_lockfree_pool_relinquish_entry_index_range(pool, first_entry_index, last_entry_index);
}

#warning invalid entry identifier REALLY WANTS to be standardised across stack/pool and hopper to allow seamless interoperability and have fewer defines
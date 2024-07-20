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

void cvm_lockfree_stack_initialise(cvm_lockfree_stack * stack, cvm_lockfree_pool * pool)
{
    atomic_init(&stack->head,CVM_LOCKFREE_STACK_INVALID_ENTRY);
    stack->next_buffer=pool->available_entries.next_buffer;
    stack->entry_data=pool->available_entries.entry_data;
    stack->entry_size=pool->available_entries.entry_size;
    stack->capacity_exponent=pool->available_entries.capacity_exponent;
}

void cvm_lockfree_stack_terminate(cvm_lockfree_stack * stack)
{
    assert((atomic_load_explicit(&stack->head, memory_order_relaxed) & CVM_LOCKFREE_STACK_ENTRY_MASK) == CVM_LOCKFREE_STACK_INVALID_ENTRY);/// stack should be empty upon termination
}

void cvm_lockfree_stack_push(cvm_lockfree_stack * stack, void * entry)
{
    uint_fast64_t entry_index,current_head,replacement_head;
    size_t entry_offset;

    assert((char*)entry >= stack->entry_data);

    entry_offset = (char*)entry - stack->entry_data;
    entry_index = (uint_fast64_t) (entry_offset / stack->entry_size);

    assert(entry_index < ((uint_fast64_t)1 << stack->capacity_exponent));

    current_head=atomic_load_explicit(&stack->head, memory_order_relaxed);
    do
    {
        stack->next_buffer[entry_index] = (uint16_t)(current_head & CVM_LOCKFREE_STACK_ENTRY_MASK);
        replacement_head=((current_head & CVM_LOCKFREE_STACK_CHECK_MASK) + CVM_LOCKFREE_STACK_CHECK_UNIT) | entry_index;
    }
    while(!atomic_compare_exchange_weak_explicit(&stack->head, &current_head, replacement_head, memory_order_release, memory_order_relaxed));
}

void * cvm_lockfree_stack_pull(cvm_lockfree_stack * stack)
{
    uint_fast64_t entry_index,current_head,replacement_head;

    current_head=atomic_load_explicit(&stack->head, memory_order_acquire);
    do
    {
        entry_index = current_head & CVM_LOCKFREE_STACK_ENTRY_MASK;
        /// if there are no more entries in this list then return NULL
        if(entry_index == CVM_LOCKFREE_STACK_INVALID_ENTRY)
        {
            return NULL;
        }

        replacement_head = ((current_head & CVM_LOCKFREE_STACK_CHECK_MASK) + CVM_LOCKFREE_STACK_CHECK_UNIT) | (uint_fast64_t)(stack->next_buffer[entry_index]);
    }
    while(!atomic_compare_exchange_weak_explicit(&stack->head, &current_head, replacement_head, memory_order_acquire, memory_order_acquire));
    /// success memory order could (conceptually) be relaxed here as we don't need to release/acquire any changes when that happens as nothing outside the atomic has changed
    /// but fail must acquire as the "next" member of "first" element may have changed


    assert(entry_index < ((uint_fast64_t)1 << stack->capacity_exponent));

    return stack->entry_data + entry_index*stack->entry_size;
}


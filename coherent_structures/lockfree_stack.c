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

#include <assert.h>

#include "coherent_structures/lockfree_stack.h"

#define SOL_LOCKFREE_STACK_CHECK_MASK    ((uint_fast64_t)0xFFFFFFFFFF000000llu)
#define SOL_LOCKFREE_STACK_ENTRY_MASK    ((uint_fast64_t)0x0000000000FFFFFFllu)
#define SOL_LOCKFREE_STACK_INVALID_ENTRY ((uint_fast64_t)0x0000000000FFFFFFllu)
#define SOL_LOCKFREE_STACK_CHECK_UNIT    ((uint_fast64_t)0x0000000001000000llu)

void sol_lockfree_stack_initialise(struct sol_lockfree_stack* stack, struct sol_lockfree_pool* pool)
{
    atomic_init(&stack->head,SOL_LOCKFREE_STACK_INVALID_ENTRY);
    stack->next_buffer = pool->next_buffer;
    stack->entry_data = pool->entry_data;
    stack->entry_size = pool->entry_size;
    stack->capacity_exponent = pool->capacity_exponent;
}

void sol_lockfree_stack_terminate(struct sol_lockfree_stack* stack)
{
    assert((atomic_load_explicit(&stack->head, memory_order_relaxed) & SOL_LOCKFREE_STACK_ENTRY_MASK) == SOL_LOCKFREE_STACK_INVALID_ENTRY);/// stack should be empty upon termination
}


void sol_lockfree_stack_push_index_range(struct sol_lockfree_stack* stack, uint32_t first_entry_index, uint32_t last_entry_index)
{
    uint_fast64_t current_head, replacement_head;

    assert(first_entry_index < ((uint_fast64_t)1 << stack->capacity_exponent));
    assert(last_entry_index  < ((uint_fast64_t)1 << stack->capacity_exponent));

    current_head = atomic_load_explicit(&stack->head, memory_order_relaxed);
    do
    {
        stack->next_buffer[last_entry_index] = (uint32_t)(current_head & SOL_LOCKFREE_STACK_ENTRY_MASK);
        replacement_head = ((current_head & SOL_LOCKFREE_STACK_CHECK_MASK) + SOL_LOCKFREE_STACK_CHECK_UNIT) | first_entry_index;
    }
    while(!atomic_compare_exchange_weak_explicit(&stack->head, &current_head, replacement_head, memory_order_release, memory_order_relaxed));
}

void sol_lockfree_stack_push_index(struct sol_lockfree_stack* stack, uint32_t entry_index)
{
    sol_lockfree_stack_push_index_range(stack, entry_index, entry_index);
}

uint32_t sol_lockfree_stack_pull_index(struct sol_lockfree_stack* stack)
{
    uint_fast64_t entry_index,current_head,replacement_head;

    current_head=atomic_load_explicit(&stack->head, memory_order_acquire);
    do
    {
        entry_index = current_head & SOL_LOCKFREE_STACK_ENTRY_MASK;
        /// if there are no more entries in this list then return NULL / SOL_LOCKFREE_STACK_INVALID_ENTRY_U32
        if(entry_index == SOL_LOCKFREE_STACK_INVALID_ENTRY)
        {
            return (uint32_t)SOL_LOCKFREE_STACK_INVALID_ENTRY;
        }

        replacement_head = ((current_head & SOL_LOCKFREE_STACK_CHECK_MASK) + SOL_LOCKFREE_STACK_CHECK_UNIT) | (uint_fast64_t)(stack->next_buffer[entry_index]);
    }
    while(!atomic_compare_exchange_weak_explicit(&stack->head, &current_head, replacement_head, memory_order_acquire, memory_order_acquire));
    /// success memory order could (conceptually) be relaxed here as we don't need to release/acquire any changes when that happens as nothing outside the atomic has changed
    /// but fail must acquire as the "next" member of "first" element may have changed

    assert(entry_index < ((uint_fast64_t)1 << stack->capacity_exponent));
    assert(entry_index <= UINT32_MAX);

    return entry_index;
}


void sol_lockfree_stack_push_range(struct sol_lockfree_stack* stack, void* first_entry, void* last_entry)
{
    uint32_t first_entry_index, last_entry_index;

    assert((char*)first_entry >= stack->entry_data);
    assert((char*)last_entry  >= stack->entry_data);

    assert((char*)first_entry < stack->entry_data + (stack->entry_size << stack->capacity_exponent));
    assert((char*)last_entry  < stack->entry_data + (stack->entry_size << stack->capacity_exponent));

    first_entry_index = (uint32_t) (((char*)first_entry - stack->entry_data) / (ptrdiff_t)stack->entry_size);
    last_entry_index  = (uint32_t) (((char*)last_entry  - stack->entry_data) / (ptrdiff_t)stack->entry_size);

    sol_lockfree_stack_push_index_range(stack, first_entry_index, last_entry_index);
}

void sol_lockfree_stack_push(struct sol_lockfree_stack* stack, void* entry)
{
    uint32_t entry_index;

    assert((char*)entry >= stack->entry_data);
    assert((char*)entry < stack->entry_data + (stack->entry_size << stack->capacity_exponent));

    entry_index = (uint32_t) (((char*)entry - stack->entry_data) / (ptrdiff_t)stack->entry_size);

    sol_lockfree_stack_push_index_range(stack, entry_index, entry_index);
}

void * sol_lockfree_stack_pull(struct sol_lockfree_stack* stack)
{
    uint32_t entry_index;

    entry_index = sol_lockfree_stack_pull_index(stack);

    if(entry_index == (uint32_t)SOL_LOCKFREE_STACK_INVALID_ENTRY)
    {
        return NULL;
    }

    return stack->entry_data + entry_index * stack->entry_size;
}



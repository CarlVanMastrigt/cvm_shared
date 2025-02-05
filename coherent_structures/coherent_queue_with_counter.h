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

#pragma once

#include <inttypes.h>
#include <stdatomic.h>
#include <stdbool.h>

typedef struct sol_lockfree_pool sol_lockfree_pool;

typedef struct sol_coherent_queue_with_counter
{
    /// faster under contention, slower when not contended
    _Alignas(128) atomic_uint_fast64_t add_index;
    _Alignas(128) atomic_uint_fast64_t add_fence;/// will contain the stall count information
    _Alignas(128) atomic_uint_fast64_t get_index;
    uint32_t* entry_indices;
    uint_fast64_t entry_mask;/// (2 ^ size factor) -1
    char* entry_data;/// data stored by queue, poijnter managed by the pool
    size_t entry_size;
    size_t capacity_exponent;
}
sol_coherent_queue_with_counter;

void sol_coherent_queue_with_counter_initialise(sol_coherent_queue_with_counter* queue,sol_lockfree_pool* pool);
void sol_coherent_queue_with_counter_terminate(sol_coherent_queue_with_counter* queue);

/// these have no effect on the fail counter
void  sol_coherent_queue_with_counter_push(sol_coherent_queue_with_counter* queue, void* entry);
void* sol_coherent_queue_with_counter_pull(sol_coherent_queue_with_counter* queue);///returns NULL on failure (b/c no elements remain)

/// these will attempt to add/get and increment/decrement a counter which tracks getters that have been stalled waiting on new elements
bool  sol_coherent_queue_with_counter_push_and_decrement(sol_coherent_queue_with_counter* queue, void* entry);///returns true if counter was nonzero, also decrements in this case
void* sol_coherent_queue_with_counter_pull_or_increment(sol_coherent_queue_with_counter* queue);///returns NULL if no elements remain and increments counter, otherwise returns acquired element

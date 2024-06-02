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

#ifndef CVM_SHARED_H
#include "cvm_shared.h"
#endif

typedef struct cvm_coherent_queue
{
    /// faster under contention, slower when not contended
    _Alignas(128) atomic_uint_fast64_t add_index; /// will contain the stall count information
    _Alignas(128) atomic_uint_fast64_t add_fence;
    _Alignas(128) atomic_uint_fast64_t get_index;
    /// usually 65536 max elements?
    uint16_t * entry_indices;
    uint_fast64_t entry_mask;/// (2 ^ size factor) -1
    char * entry_data;/// data stored by queue, poijnter managed by the pool
    size_t entry_size;
    size_t capacity_exponent;
}
cvm_coherent_queue;


void cvm_coherent_queue_initialise(cvm_coherent_queue * queue,cvm_lockfree_pool * pool);
void cvm_coherent_queue_terminate(cvm_coherent_queue * queue);

void cvm_coherent_queue_add(cvm_coherent_queue * queue, void * entry);
void * cvm_coherent_queue_get(cvm_coherent_queue * queue);///returns NULL on failure (b/c no elements remain)



typedef struct cvm_coherent_queue_with_counter
{
    /// faster under contention, slower when not contended
    _Alignas(128) atomic_uint_fast64_t add_index; /// will contain the stall count information
    _Alignas(128) atomic_uint_fast64_t add_fence;
    _Alignas(128) atomic_uint_fast64_t get_index;
    /// usually 65536 max elements?
    uint16_t * entry_indices;
    uint_fast64_t entry_mask;/// (2 ^ size factor) -1
    char * entry_data;/// data stored by queue, poijnter managed by the pool
    size_t entry_size;
    size_t capacity_exponent;
}
cvm_coherent_queue_with_counter;

void cvm_coherent_queue_with_counter_initialise(cvm_coherent_queue_with_counter * queue,cvm_lockfree_pool * pool);
void cvm_coherent_queue_with_counter_terminate(cvm_coherent_queue_with_counter * queue);

/// these have no effect on the fail counter
void   cvm_coherent_queue_with_counter_add(cvm_coherent_queue_with_counter * queue, void * entry);
void * cvm_coherent_queue_with_counter_get(cvm_coherent_queue_with_counter * queue);///returns NULL on failure (b/c no elements remain)

/// these will attempt to add/get and increment/decrement a counter which tracks getters that have been stalled waiting on new elements
bool   cvm_coherent_queue_with_counter_add_and_decrement(cvm_coherent_queue_with_counter * queue, void * entry);///returns true if fail counter was nonzero, also decrements in this case
void * cvm_coherent_queue_with_counter_get_or_increment(cvm_coherent_queue_with_counter * queue);///returns NULL on failure (b/c no elements remain) and increments fail counter

/// can use similar to cvm_thread_safe_queue but with managed pool











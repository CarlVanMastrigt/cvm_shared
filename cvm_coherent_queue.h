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

typedef struct cvm_coherent_queue_pool cvm_coherent_queue_pool;

typedef struct cvm_coherent_queue
{
    /// faster under contention, slower when not contended
    alignas(128) atomic_uint_fast32_t add_index; /// will contain the stall count information
    alignas(128) atomic_uint_fast32_t add_fence;
    alignas(128) atomic_uint_fast32_t get_index;
    /// usually 65536 max elements?
    uint16_t * entry_indices;
    uint_fast32_t entry_mask;/// (2 ^ size factor) -1
    char * entry_data;/// data stored by queue, poijnter managed by the pool
    size_t entry_size;
    size_t capacity_exponent;
//    atomic_flag * locks;
}
cvm_coherent_queue;


void cvm_coherent_queue_initiasise(cvm_coherent_queue_pool * pool,cvm_coherent_queue * queue);
void cvm_coherent_queue_terminate(cvm_coherent_queue * queue);

//bool cvm_coherent_queue_add_safely(cvm_coherent_queue * queue, void * entry);

void   cvm_coherent_queue_add(cvm_coherent_queue * queue, void * entry);
void * cvm_coherent_queue_get(cvm_coherent_queue * queue);///returns NULL on failure (b/c no elements remain)

/// can use similar to cvm_thread_safe_queue but with managed pool

struct cvm_coherent_queue_pool
{
    //available entries stored as stack
    alignas(128) atomic_uint_fast64_t next_stack_head;
    uint16_t * next_buffer;

    ///data backing, extensible?
    char * entry_data;
    size_t entry_size;
    size_t capacity_exponent;
};

void cvm_coherent_queue_pool_initialise(cvm_coherent_queue_pool * pool, size_t capacity_exponent, size_t entry_size);
void cvm_coherent_queue_pool_terminate(cvm_coherent_queue_pool * pool);

void * cvm_coherent_queue_pool_acquire_entry(cvm_coherent_queue_pool * pool);
void   cvm_coherent_queue_pool_relinquish_entry(cvm_coherent_queue_pool * pool, void * entry);









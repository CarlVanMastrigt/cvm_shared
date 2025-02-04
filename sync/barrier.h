/**
Copyright 2025 Carl van Mastrigt

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
along with barrier.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "cvm_sync.h"

#ifndef CVM_BARRIER_H
#define CVM_BARRIER_H

struct cvm_barrier_pool
{
    cvm_lockfree_pool barrier_pool;
    cvm_lockfree_pool successor_pool;
};

void cvm_barrier_pool_initialise(struct cvm_barrier_pool* pool, size_t total_barrier_exponent, size_t total_successor_exponent);
void cvm_barrier_pool_terminate(struct cvm_barrier_pool* pool);

struct cvm_barrier
{
    const struct cvm_sync_primitive_functions* sync_functions;

    struct cvm_barrier_pool* pool;

    atomic_uint_fast32_t condition_count;
    atomic_uint_fast32_t reference_count;

    cvm_lockfree_hopper successor_hopper;
};



struct cvm_barrier* cvm_barrier_prepare(struct cvm_barrier_pool* pool);

void cvm_barrier_activate(struct cvm_barrier* barrier);





/// corresponds to the number of times a matching `cvm_barrier_signal_conditions` must be called for the barrier before it can be executed
/// at least one dependency must be held in order to set up a dependency to that barrier if it has already been enqueued
void cvm_barrier_impose_conditions(struct cvm_barrier* barrier, uint_fast32_t count);

/// use this to signal that some set of data and/or dependencies required by the barrier have been set up, total must be matched to the count provided to cvm_barrier_impose_conditions
void cvm_barrier_signal_conditions(struct cvm_barrier* barrier, uint_fast32_t count);

/// execution dependencies also act as retained references (because we cannot clean up a barrier until it has been executed) as such if a dependency is required anyway then a refernce need not be held as well

/// corresponds to the number of times a matching `cvm_barrier_release_retainer` must be called for the barrier before it can be destroyed/released (i.e. the pointer becomes an invalid way to refernce this barrier)
/// at least one retainer must be held in order to set up a successor to that barrier if it has already been enqueued
void cvm_barrier_retain_references(struct cvm_barrier* barrier, uint_fast32_t count);

/// signal that we are done setting up things that must happen before cleanup (e.g. setting up successors barriers/gates)
void cvm_barrier_release_references(struct cvm_barrier* barrier, uint_fast32_t count);





static inline union cvm_sync_primitive* cvm_barrier_as_sync_primitive(struct cvm_barrier* barrier)
{
    return (union cvm_sync_primitive*)barrier;
}


#endif



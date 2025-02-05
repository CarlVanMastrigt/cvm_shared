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

#pragma once

#include <inttypes.h>
#include <stdatomic.h>

#include "coherent_structures/lockfree_pool.h"
#include "coherent_structures/lockfree_hopper.h"

// struct sol_sync_primitive_functions;
// union sol_sync_primitive;

struct sol_barrier_pool
{
    sol_lockfree_pool barrier_pool;
    sol_lockfree_pool successor_pool;
};

void sol_barrier_pool_initialise(struct sol_barrier_pool* pool, size_t total_barrier_exponent, size_t total_successor_exponent);
void sol_barrier_pool_terminate(struct sol_barrier_pool* pool);

struct sol_barrier
{
    const struct sol_sync_primitive_functions* sync_functions;

    struct sol_barrier_pool* pool;

    atomic_uint_fast32_t condition_count;
    atomic_uint_fast32_t reference_count;

    sol_lockfree_hopper successor_hopper;
};



struct sol_barrier* sol_barrier_prepare(struct sol_barrier_pool* pool);

void sol_barrier_activate(struct sol_barrier* barrier);





/// corresponds to the number of times a matching `sol_barrier_signal_conditions` must be called for the barrier before it can be executed
/// at least one dependency must be held in order to set up a dependency to that barrier if it has already been enqueued
void sol_barrier_impose_conditions(struct sol_barrier* barrier, uint_fast32_t count);

/// use this to signal that some set of data and/or dependencies required by the barrier have been set up, total must be matched to the count provided to sol_barrier_impose_conditions
void sol_barrier_signal_conditions(struct sol_barrier* barrier, uint_fast32_t count);

/// execution dependencies also act as retained references (because we cannot clean up a barrier until it has been executed) as such if a dependency is required anyway then a refernce need not be held as well

/// corresponds to the number of times a matching `sol_barrier_release_retainer` must be called for the barrier before it can be destroyed/released (i.e. the pointer becomes an invalid way to refernce this barrier)
/// at least one retainer must be held in order to set up a successor to that barrier if it has already been enqueued
void sol_barrier_retain_references(struct sol_barrier* barrier, uint_fast32_t count);

/// signal that we are done setting up things that must happen before cleanup (e.g. setting up successors barriers/gates)
void sol_barrier_release_references(struct sol_barrier* barrier, uint_fast32_t count);





static inline union sol_sync_primitive* sol_barrier_as_sync_primitive(struct sol_barrier* barrier)
{
    return (union sol_sync_primitive*)barrier;
}



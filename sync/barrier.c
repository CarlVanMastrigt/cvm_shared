/**
Copyright 2025 Carl van Mastrigt

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

#include "barrier.h"



static void cvm_barrier_impose_condition(union cvm_sync_primitive* primitive)
{
    cvm_barrier_impose_conditions(&primitive->barrier, 1);
}
static void cvm_barrier_signal_condition(union cvm_sync_primitive* primitive)
{
    cvm_barrier_signal_conditions(&primitive->barrier, 1);
}
static void cvm_barrier_attach_successor(union cvm_sync_primitive* primitive, union cvm_sync_primitive* successor)
{
    struct cvm_barrier* barrier;
    struct cvm_lockfree_pool* successor_pool;
    union cvm_sync_primitive** successor_ptr;

    barrier = &primitive->barrier;

    assert(atomic_load_explicit(&barrier->reference_count, memory_order_relaxed));
    /// barrier must be retained to set up successors (can technically be satisfied illegally, using queue for re-use will make detection better but not infallible)

    if(cvm_lockfree_hopper_check_if_locked(&barrier->successor_hopper))
    {
        /// if hopper already locked then barrier has had all conditions satisfied/signalled, so can signal this successor
        cvm_sync_primitive_signal_condition(successor);
    }
    else
    {
        // create sucessor and add it to the hopper
        successor_pool = &barrier->pool->successor_pool;

        successor_ptr = cvm_lockfree_pool_acquire_entry(successor_pool);
        assert(successor_ptr);///ran out of successors

        *successor_ptr = successor;

        if(!cvm_lockfree_hopper_push(&barrier->successor_hopper, successor_pool, successor_ptr))
        {
            /// if we failed to add the successor then the barrier has already been completed, relinquish the storage and signal the successor
            cvm_lockfree_pool_relinquish_entry(successor_pool, successor_ptr);
            cvm_sync_primitive_signal_condition(successor);
        }
    }
}
static void cvm_barrier_retain_reference(union cvm_sync_primitive* primitive)
{
    cvm_barrier_retain_references(&primitive->barrier, 1);
}
static void cvm_barrier_release_reference(union cvm_sync_primitive* primitive)
{
    cvm_barrier_release_references(&primitive->barrier, 1);
}

const static struct cvm_sync_primitive_functions barrier_sync_functions =
{
    .impose_condition  = &cvm_barrier_impose_condition,
    .signal_condition  = &cvm_barrier_signal_condition,
    .attach_successor  = &cvm_barrier_attach_successor,
    .retain_reference  = &cvm_barrier_retain_reference,
    .release_reference = &cvm_barrier_release_reference,
};

static void cvm_barrier_initialise(void* elem, void* data)
{
    struct cvm_barrier* barrier = elem;
    struct cvm_barrier_pool* pool = data;

    barrier->sync_functions = &barrier_sync_functions;
    barrier->pool = pool;

    cvm_lockfree_hopper_initialise(&barrier->successor_hopper, &pool->successor_pool);

    atomic_init(&barrier->condition_count, 0);
    atomic_init(&barrier->reference_count, 0);
}

void cvm_barrier_pool_initialise(struct cvm_barrier_pool* pool, size_t total_barrier_exponent, size_t total_successor_exponent)
{
    cvm_lockfree_pool_initialise(&pool->barrier_pool, total_barrier_exponent, sizeof(struct cvm_barrier));
    cvm_lockfree_pool_initialise(&pool->successor_pool, total_successor_exponent, sizeof(union cvm_sync_primitive**));
}

void cvm_barrier_pool_terminate(struct cvm_barrier_pool* pool)
{
    cvm_lockfree_pool_terminate(&pool->successor_pool);
    cvm_lockfree_pool_terminate(&pool->barrier_pool);
}




void cvm_barrier_impose_conditions(struct cvm_barrier* barrier, uint_fast32_t count)
{
    uint_fast32_t old_count = atomic_fetch_add_explicit(&barrier->condition_count, count, memory_order_relaxed);
    assert(old_count>0);/// should not be adding dependencies when none still exist (need held dependencies to addsetup more dependencies)
}

void cvm_barrier_signal_conditions(struct cvm_barrier* barrier, uint_fast32_t count)
{
    uint_fast32_t old_count;
    struct cvm_barrier_pool* pool;
    union cvm_sync_primitive** successor_ptr;

    /// this is responsible for coalescing all modifications, but also for making them available to the next thread/atomic to recieve this memory (after the potential release in this function)
    old_count = atomic_fetch_sub_explicit(&barrier->condition_count, count, memory_order_acq_rel);
    assert(old_count >= count);/// condition count cannot go negative, more conditions have been signalled than were created

    if(old_count == count)/// this is the last dependency this barrier was waiting on, put it on list of available barriers and make sure there's a worker thread to satisfy it
    {
        pool = barrier->pool;

        successor_ptr = cvm_lockfree_hopper_lock_and_get_first(&barrier->successor_hopper, &pool->successor_pool);

        cvm_barrier_release_references(barrier, 1);

        while(successor_ptr)
        {
            cvm_sync_primitive_signal_condition(*successor_ptr);
            successor_ptr = cvm_lockfree_hopper_relinquish_and_get_next(&pool->successor_pool, successor_ptr);
        }
    }
}



void cvm_barrier_retain_references(struct cvm_barrier * barrier, uint_fast32_t count)
{
    uint_fast32_t old_count=atomic_fetch_add_explicit(&barrier->reference_count, count, memory_order_relaxed);
    assert(old_count!=0);/// should not be adding successors reservations when none still exist (need held successors reservations to addsetup more successors reservations)
}

void cvm_barrier_release_references(struct cvm_barrier * barrier, uint_fast32_t count)
{
    /// need to release to prevent reads/writes of successor/completion data being moved after this operation
    uint_fast32_t old_count=atomic_fetch_sub_explicit(&barrier->reference_count, count, memory_order_release);

    assert(old_count >= count);/// have completed more successor reservations than were made

    if(old_count==count)
    {
        cvm_lockfree_pool_relinquish_entry(&barrier->pool->barrier_pool, barrier);
    }
}

struct cvm_barrier* cvm_barrier_prepare(struct cvm_barrier_pool* pool)
{
    struct cvm_barrier* barrier;

    barrier = cvm_lockfree_pool_acquire_entry(&pool->barrier_pool);

    cvm_lockfree_hopper_reset(&barrier->successor_hopper);

    /// need to wait on "enqueue" op before beginning, ergo need one extra dependency for that (enqueue is really just a signal)
    atomic_store_explicit(&barrier->condition_count, 1, memory_order_relaxed);
    /// need to retain a reference until the successors are actually signalled, ergo one extra reference that will be released after completing the barrier
    atomic_store_explicit(&barrier->reference_count, 1, memory_order_relaxed);

    return barrier;
}

void cvm_barrier_activate(struct cvm_barrier* barrier)
{
    /// this is basically just called differently to account for the "hidden" wait counter added on barrier creation
    cvm_barrier_signal_conditions(barrier, 1);
}





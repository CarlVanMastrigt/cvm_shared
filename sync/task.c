/**
Copyright 2024,2025 Carl van Mastrigt

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

#include "task.h"






static inline struct cvm_task* cvm_task_worker_thread_get_task(struct cvm_task_system* task_system)
{
    struct cvm_task* task;

    while(true)
    {
        task = cvm_coherent_queue_with_counter_pull(&task_system->pending_tasks);
        if(task)
        {
            return task;
        }

        /// there were no more tasks to perform
        mtx_lock(&task_system->worker_thread_mutex);

        /// this will increment local stall counter if it fails (which note: is inside the mutex lock)
        /// needs to be inside mutex lock such that when we add a task we KNOW there is a worker thread (at least this one) that has already stalled by the time that queue addition tries to wake a worker
        ///     ^ (queue addition wakes workers inside this mutex)
        task = cvm_coherent_queue_with_counter_pull_or_increment(&task_system->pending_tasks);
        if(task)
        {
            mtx_unlock(&task_system->worker_thread_mutex);
            return task;
        }

        /// if this thread was going to sleep but something else grabbed the mutex first and requested a wakeup it makes sense to snatch that first and never actually wait
        ///     ^ and let a later wakeup be treated as spurrious
        while(task_system->signalled_unstalls==0)
        {
            task_system->stalled_thread_count++;

            /// if all threads are stalled (there is no work left to do) and we've asked the system to shut down (this requires there are no more tasks being created) then we can finalise shutdown
            if(task_system->shutdown_initiated && (task_system->stalled_thread_count == task_system->worker_thread_count))
            {
                task_system->stalled_thread_count--;
                task_system->shutdown_completed=true;
                cnd_broadcast(&task_system->worker_thread_condition);/// wake up all stalled threads so that they can exit
                mtx_unlock(&task_system->worker_thread_mutex);
                return NULL;
            }

            /// wait untill more workers are needed or we're shutting down (with appropriate checks in case of spurrious wakeup)
            cnd_wait(&task_system->worker_thread_condition, &task_system->worker_thread_mutex);

            task_system->stalled_thread_count--;

            if(task_system->shutdown_completed)
            {
                mtx_unlock(&task_system->worker_thread_mutex);
                return NULL;
            }
        }

        task_system->signalled_unstalls--;/// we have unstalled successfully

        mtx_unlock(&task_system->worker_thread_mutex);
    }
}


static int cvm_task_worker_thread_function(void* in)
{
    struct cvm_task_system* task_system = in;
    struct cvm_task* task;
    union cvm_sync_primitive** successor_ptr;

    while((task = cvm_task_worker_thread_get_task(task_system)))
    {
        task->task_function(task->task_function_data);

        ///could be for loop but w/e
        successor_ptr = cvm_lockfree_hopper_lock_and_get_first(&task->successor_hopper, &task_system->successor_pool);

        cvm_task_release_references(task, 1);// the sucessors dont require the hopper anymore, task is done with, so can release

        while(successor_ptr)
        {
            cvm_sync_primitive_signal_condition(*successor_ptr);
            successor_ptr = cvm_lockfree_hopper_relinquish_and_get_next(&task_system->successor_pool, successor_ptr);
        }
    }

    return 0;
}

static void cvm_task_impose_condition(union cvm_sync_primitive* primitive)
{
    cvm_task_impose_conditions(&primitive->task, 1);
}
static void cvm_task_signal_condition(union cvm_sync_primitive* primitive)
{
    cvm_task_signal_conditions(&primitive->task, 1);
}
static void cvm_task_attach_successor(union cvm_sync_primitive* primitive, union cvm_sync_primitive* successor)
{
    struct cvm_task* task;
    struct cvm_lockfree_pool* successor_pool;
    union cvm_sync_primitive** successor_ptr;

    task = &primitive->task;

    assert(atomic_load_explicit(&task->reference_count, memory_order_relaxed));
    /// task must be retained to set up successors (can technically be satisfied illegally, using queue for re-use will make detection better but not infallible)

    if(cvm_lockfree_hopper_check_if_locked(&task->successor_hopper))
    {
        /// if hopper already locked then task has been completed, can signal
        cvm_sync_primitive_signal_condition(successor);
    }
    else
    {
        successor_pool = &task->task_system->successor_pool;

        successor_ptr = cvm_lockfree_pool_acquire_entry(successor_pool);
        assert(successor_ptr);///ran out of successors

        *successor_ptr = successor;

        if(!cvm_lockfree_hopper_push(&task->successor_hopper, successor_pool, successor_ptr))
        {
            /// if we failed to add the successor then the task has already been completed, relinquish the storage and signal the successor
            cvm_lockfree_pool_relinquish_entry(successor_pool, successor_ptr);
            cvm_sync_primitive_signal_condition(successor);
        }
    }
}
static void cvm_task_retain_reference(union cvm_sync_primitive* primitive)
{
    cvm_task_retain_references(&primitive->task, 1);
}
static void cvm_task_release_reference(union cvm_sync_primitive* primitive)
{
    cvm_task_release_references(&primitive->task, 1);
}

const static struct cvm_sync_primitive_functions task_sync_functions =
{
    .impose_condition  = &cvm_task_impose_condition,
    .signal_condition  = &cvm_task_signal_condition,
    .attach_successor  = &cvm_task_attach_successor,
    .retain_reference  = &cvm_task_retain_reference,
    .release_reference = &cvm_task_release_reference,
};

static void cvm_task_initialise(void* elem, void* data)
{
    struct cvm_task* task = elem;
    struct cvm_task_system* task_system = data;

    task->sync_functions = &task_sync_functions;
    task->task_system = task_system;

    cvm_lockfree_hopper_initialise(&task->successor_hopper,&task_system->successor_pool);

    atomic_init(&task->condition_count, 0);
    atomic_init(&task->reference_count, 0);
}

void cvm_task_system_initialise(struct cvm_task_system* task_system, uint32_t worker_thread_count, size_t total_task_exponent, size_t total_successor_exponent)
{
    uint32_t i;

    cvm_lockfree_pool_initialise(&task_system->task_pool, total_task_exponent, sizeof(struct cvm_task));
    cvm_lockfree_pool_initialise(&task_system->successor_pool, total_successor_exponent, sizeof(union cvm_sync_primitive**));

    cvm_lockfree_pool_call_for_every_entry(&task_system->task_pool, &cvm_task_initialise, task_system);

    cvm_coherent_queue_with_counter_initialise(&task_system->pending_tasks, &task_system->task_pool);

    task_system->worker_threads=malloc(sizeof(thrd_t)*worker_thread_count);
    task_system->worker_thread_count=worker_thread_count;

    cnd_init(&task_system->worker_thread_condition);
    mtx_init(&task_system->worker_thread_mutex, mtx_plain);

    task_system->shutdown_completed=false;
    task_system->shutdown_initiated=false;

    task_system->stalled_thread_count=0;
    task_system->signalled_unstalls=0;

    /// will need setup mutex locked here if we want to wait on all workers to start before progressing (maybe useful to have, but I can't think of a reason)
    for(i=0;i<worker_thread_count;i++)
    {
        thrd_create(task_system->worker_threads+i, cvm_task_worker_thread_function, task_system);
    }
}


void cvm_task_system_begin_shutdown(struct cvm_task_system* task_system)
{
    mtx_lock(&task_system->worker_thread_mutex);
    task_system->shutdown_initiated=true;
    cnd_signal(&task_system->worker_thread_condition);/// ensure at least one worker is awake to finalise shutdown
    mtx_unlock(&task_system->worker_thread_mutex);
}
/// ^ this is also necessary/useful for queues i believe,
///     ^ have to wait till all tasks that would use a queue have completed before attempting to terminate the queue, best/safest way to do this is to have the queue outlive the task system

void cvm_task_system_end_shutdown(struct cvm_task_system* task_system)
{
    uint32_t i;
    mtx_lock(&task_system->worker_thread_mutex);
    assert(task_system->shutdown_initiated);/// this will finalise shutdown of the task system, shutdown must have been initiated earlier
    while(!task_system->shutdown_completed)
    {
        cnd_wait(&task_system->worker_thread_condition, &task_system->worker_thread_mutex);
    }
    mtx_unlock(&task_system->worker_thread_mutex);

    for(i=0;i<task_system->worker_thread_count;i++)
    {
        /// if thread gets stuck here its possible that not all tasks were able to complete, perhaps because things werent shut down correctly and some tasks have outstanding dependencies
        thrd_join(task_system->worker_threads[i], NULL);
    }
    assert(task_system->stalled_thread_count==0);/// make sure everyone woke up okay
}

void cvm_task_system_terminate(struct cvm_task_system* task_system)
{
    free(task_system->worker_threads);

    cnd_destroy(&task_system->worker_thread_condition);
    mtx_destroy(&task_system->worker_thread_mutex);

    cvm_lockfree_pool_terminate(&task_system->successor_pool);

    cvm_coherent_queue_with_counter_terminate(&task_system->pending_tasks);
    cvm_lockfree_pool_terminate(&task_system->task_pool);
}




void cvm_task_impose_conditions(struct cvm_task* task, uint_fast32_t count)
{
    uint_fast32_t old_count=atomic_fetch_add_explicit(&task->condition_count, count, memory_order_relaxed);
    assert(old_count>0);/// should not be adding dependencies when none still exist (need held dependencies to addsetup more dependencies)
}

void cvm_task_signal_conditions(struct cvm_task* task, uint_fast32_t count)
{
    uint_fast32_t old_count;
    bool unstall_worker;
    struct cvm_task_system* task_system = task->task_system;

    /// this is responsible for coalescing all modifications, but also for making them available to the next thread/atomic to recieve this memory (after the potential release in this function)
    old_count=atomic_fetch_sub_explicit(&task->condition_count, count, memory_order_acq_rel);
    assert(old_count >= count);///must not make dep. count go negative

    if(old_count==count)/// this is the last dependency this task was waiting on, put it on list of available tasks and make sure there's a worker thread to satisfy it
    {
        /// acquire stall count here?
        unstall_worker=cvm_coherent_queue_with_counter_push_and_decrement(&task_system->pending_tasks, task);

        if(unstall_worker)
        {
            mtx_lock(&task_system->worker_thread_mutex);

            assert(!task_system->shutdown_completed);/// shouldnt have stopped running if tasks have yet to complete
            assert(task_system->stalled_thread_count > 0);/// weren't actually any stalled workers!
            assert(task_system->signalled_unstalls < task_system->stalled_thread_count);///invalid unstall request

            task_system->signalled_unstalls++;/// required for preventing spurrious wakeup

            cnd_signal(&task_system->worker_thread_condition);

            mtx_unlock(&task_system->worker_thread_mutex);
        }
    }
}



void cvm_task_retain_references(struct cvm_task * task, uint_fast32_t count)
{
    uint_fast32_t old_count=atomic_fetch_add_explicit(&task->reference_count, count, memory_order_relaxed);
    assert(old_count!=0);/// should not be adding successors reservations when none still exist (need held successors reservations to addsetup more successors reservations)
}

void cvm_task_release_references(struct cvm_task * task, uint_fast32_t count)
{
    /// need to release to prevent reads/writes of successor/completion data being moved after this operation
    uint_fast32_t old_count=atomic_fetch_sub_explicit(&task->reference_count, count, memory_order_release);

    assert(old_count >= count);/// have completed more successor reservations than were made

    if(old_count==count)
    {
        /// this should only happen after task completion, BUT the only time this counter can (if used properly) hit zero is if the task HAS completed!
        cvm_lockfree_pool_relinquish_entry(&task->task_system->task_pool, task);
    }
}

struct cvm_task* cvm_task_prepare(struct cvm_task_system* task_system, void(*task_function)(void*), void * data)
{
    struct cvm_task* task;

    task = cvm_lockfree_pool_acquire_entry(&task_system->task_pool);
    assert(task);//not enough tasks allocated

    task->task_function=task_function;
    task->task_function_data=data;

    cvm_lockfree_hopper_reset(&task->successor_hopper);

    /// need to wait on "enqueue" op before beginning, ergo need one extra dependency for that (enqueue is really just a signal)
    atomic_store_explicit(&task->condition_count, 1, memory_order_relaxed);
    /// need to retain a reference until the successors are actually signalled, ergo one extra reference that will be released after completing the task
    atomic_store_explicit(&task->reference_count, 1, memory_order_relaxed);

    return task;
}

void cvm_task_activate(struct cvm_task* task)
{
    /// this is basically just called differently to account for the "hidden" wait counter added on task creation
    cvm_task_signal_conditions(task, 1);
}





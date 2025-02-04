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

#include "cvm_sync.h"

#ifndef CVM_TASK_H
#define CVM_TASK_H

struct cvm_task_system
{
    cvm_lockfree_pool task_pool;
    cvm_coherent_queue_with_counter pending_tasks;

    cvm_lockfree_pool successor_pool;///pool for storing successors (linked list/hopper per task)

    thrd_t* worker_threads;/// make this system extensible (to a degree) should stalled threads want to be switched to a worker thread (can perhaps do this without relying on scheduler though)
    uint32_t worker_thread_count;

    cnd_t worker_thread_condition;/// multipurpose, used for setup, shutdown and for handling stalled threads
    mtx_t worker_thread_mutex;

    uint32_t signalled_unstalls;
    uint32_t stalled_thread_count;

    bool shutdown_completed;
    bool shutdown_initiated;

    /// pool size for types and expected pool count (to allow over use without pointer change in cases where a second pool would overrun memory capabilities and crash)
    /// is above too much to expose to user? maybe take expected pool size and quarter it?
};

void cvm_task_system_initialise(struct cvm_task_system* task_system, uint32_t worker_thread_count, size_t total_task_exponent, size_t total_successor_exponent);

/// invalid to add tasks *that don't depend on other tasks* after this has been called
/// this CAN be called inside a task!
void cvm_task_system_begin_shutdown(struct cvm_task_system* task_system);

/// waits for all tasks to complete and worker threads to join before returning, as such cannot be called inside a task
/// any outstanding tasks will be executed before this returns
/// as such any task still in flight must not have dependencies that would only be satisfied after this function has returned
void cvm_task_system_end_shutdown(struct cvm_task_system* task_system);

/// cleans up allocations
void cvm_task_system_terminate(struct cvm_task_system* task_system);



struct cvm_task
{
    const struct cvm_sync_primitive_functions* sync_functions;

    struct cvm_task_system* task_system;

    void(*task_function)(void*);
    void* task_function_data;

    /// need to only init atomics once: "If obj was not default-constructed, or this function is called twice on the same obj, the behavior is undefined."

    atomic_uint_fast32_t condition_count;
    atomic_uint_fast32_t reference_count;

    cvm_lockfree_hopper successor_hopper;
};


// task starts inert/unactivated and cannot run (so that order of execution relative to other primitives can be established) `cvm_task_activate` must be called for it to run
struct cvm_task* cvm_task_prepare(struct cvm_task_system* task_system, void(*task_function)(void*), void* data);

/// allows a task to be executed
/// must either add all associated dependencies/condition before calling this OR impose conditions and retain references the task as necessary to set up dependencies later
void cvm_task_activate(struct cvm_task* task);//commit?



// all conditions must be satisfied for a task to run

/// corresponds to the number of times a matching `cvm_task_signal_conditions` must be called for the task before it can be executed
/// at least one condition must be unsignalled in order to set up another condition to that task if it has already been enqueued
void cvm_task_impose_conditions(struct cvm_task* task, uint_fast32_t count);

/// use this to signal that some set of data and/or dependencies required by the task have been set up, total must be matched to the count provided to cvm_task_impose_conditions
void cvm_task_signal_conditions(struct cvm_task* task, uint_fast32_t count);


/// corresponds to the number of times a matching `cvm_task_release_reference` must be called for the task before it can be cleaned up
/// at least one retainer must be held in order to set up a successor to that task if it has already been enqueued
void cvm_task_retain_references(struct cvm_task* task, uint_fast32_t count);

/// signal that we are done setting up things that must happen before cleanup (e.g. setting up successors)
void cvm_task_release_references(struct cvm_task* task, uint_fast32_t count);





static inline union cvm_sync_primitive* cvm_task_as_sync_primitive(struct cvm_task* task)
{
    return (union cvm_sync_primitive*)task;
}


#endif



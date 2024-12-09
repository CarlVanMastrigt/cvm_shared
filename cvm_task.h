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

#ifndef CVM_TASK_H
#define CVM_TASK_H

typedef struct cvm_task_system cvm_task_system;

typedef void(cvm_task_function)(void*);

// probably want tie in to an "actor" subsystem (single threaded process) should that actually be determined to be a desirable thing to have
//      ^ could just have series of tasks that trigger each other and/or mailbox that ensures it either has a task executing or enqueues execution of a task to handle the work (unified work system)

typedef struct cvm_task
{
    cvm_sync_primitive_signal_function * signal_function;/// polymorphic base

    cvm_task_system * task_system;

    cvm_task_function * task_func;
    void * task_func_input;

    /// need to only init atomics once: "If obj was not default-constructed, or this function is called twice on the same obj, the behavior is undefined."

    atomic_uint_fast16_t dependency_count;
    atomic_uint_fast16_t reference_count;

    cvm_lockfree_hopper successor_hopper;
}
cvm_task;



struct cvm_task_system
{
    cvm_lockfree_pool task_pool;
    cvm_coherent_queue_with_counter pending_tasks;

    cvm_lockfree_pool successor_pool;///pool for storing successors (linked list/hopper per task)

    thrd_t * worker_threads;/// make this system extensible (to a degree) should stalled threads want to be switched to a worker thread (can perhaps do this without relying on scheduler though)
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

void cvm_task_system_initialise(cvm_task_system * task_system, uint32_t worker_thread_count, size_t total_task_exponent, size_t total_successor_exponent);

/// invalid to add tasks *that don't depend on other tasks* after this has been called
/// this CAN be called inside a task!
void cvm_task_system_begin_shutdown(cvm_task_system * task_system);

/// any outstanding tasks will be executed before this returns
/// as such any task still in flight must not have dependencies that would only be satisfied after this function has returned
void cvm_task_system_terminate(cvm_task_system * task_system);

cvm_task * cvm_create_task(cvm_task_system * task_system, cvm_task_function func, void * data);

/// allows a task to be executed (is basically just another signal task call lol)
/// must either add all associated dependencies before calling this or add dependencies and retain the task as necessary to set up the dependencies later
void cvm_task_enqueue(cvm_task * task);



/// everything from here is only useful if task successors/predecessors want to be set up AFTER the task has been enqueued



/// corresponds to the number of times a matching `cvm_task_signal_dependency` must be called for the task before it can be executed
/// at least one dependency must be held in order to set up a dependency to that task if it has already been enqueued
void cvm_task_add_dependencies(cvm_task * task, uint16_t count);

/// use this to signal that some set of data and/or dependencies required by the task have been set up, total must be matched to the count provided to cvm_ts_add_task_signal_dependencies
void cvm_task_signal_dependencies(cvm_task * task, uint16_t count);
void cvm_task_signal_dependency(cvm_task * task);


/// execution dependencies also act as retained references (because we cannot clean up a task until it has been executed) as such if a dependency is required anyway then a refernce need not be held as well



/// corresponds to the number of times a matching `cvm_task_release_retainer` must be called for the task before it can be destroyed/released (i.e. the pointer becomes an invalid way to refernce this task)
/// at least one retainer must be held in order to set up a successor to that task if it has already been enqueued
void cvm_task_retain_reference(cvm_task * task, uint16_t count);

/// signal that we are done setting up things that must happen before cleanup (e.g. setting up successors tasks/gates)
void cvm_task_release_references(cvm_task * task, uint16_t count);
void cvm_task_release_reference(cvm_task * task);





/// the dependency on the successor must have been set up before calling this function
/// this is because if the task has already been completed this will signal the successor immediately
/// should only be called as part of setting up a cvm_sync_primitive happens before relationship
void cvm_task_add_successor(cvm_task * task, cvm_sync_primitive * successor);





#endif



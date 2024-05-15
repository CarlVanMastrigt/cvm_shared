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


#define CVM_TASK_MAX_SUCESSORS 8

typedef struct cvm_task
{
    cvm_synchronization_primitive_signal_function * signal_function;/// polymorphic base

    cvm_task_system * task_system;

    cvm_task_function * task_func;
    void * task_func_input;

    /// need to only init atomics once: "If obj was not default-constructed, or this function is called twice on the same obj, the behavior is undefined."

    atomic_uint_fast16_t dependency_count;
    atomic_uint_fast16_t reference_count;

    atomic_flag sucessor_lock;

    bool complete;/// has the task completed
    uint32_t successor_count;

    cvm_synchronization_primitive * successors[CVM_TASK_MAX_SUCESSORS];
}
cvm_task;



struct cvm_task_system
{
    cvm_lockfree_pool task_pool;
    cvm_coherent_queue_fail_tracking pending_tasks;

    thrd_t * worker_threads;/// make this system extensible (to a degree) should stalled threads want to be switched to a worker thread (can perhaps do this without relying on scheduler though)
    uint32_t worker_thread_count;

    cnd_t worker_thread_condition;/// multipurpose, used for setup, shutdown and for handling stalled threads
    mtx_t worker_thread_mutex;

    uint32_t signalled_unstalls;
    uint32_t stalled_thread_count;

    bool running;
    bool shutdown_initiated;

    /// pool size for types and expected pool count (to allow over use without pointer change in cases where a second pool would overrun memory capabilities and crash)
    /// is above too much to expose to user? maybe take expected pool size and quarter it?
};

void cvm_task_system_initialise(cvm_task_system * task_system, uint32_t worker_thread_count, size_t total_task_exponent);

/// any outstanding tasks will be executed before this returns
/// as such any task still in flight must not have dependencies that would only be satisfied after this function has returned
void cvm_task_system_terminate(cvm_task_system * task_system);

cvm_task * cvm_create_task(cvm_task_system * task_system, cvm_task_function func, void * data);

/// allows a task to be executed (is basically just another signal task call lol)
/// must either add all associated dependencies before calling this or add dependencies and retain the task as necessary to set up the dependencies later
void cvm_task_enqueue(cvm_task * task);



/// corresponds to the number of times a matching `cvm_task_signal_dependency` must be called for the task before it can be executed
/// at least one dependency must be held in order to set up a dependency to that task if it has already been enqueued
void cvm_task_add_dependencies(cvm_task * task, uint16_t count);

// do we need a "mass signal" variant?
/// use this to signal that some set of data and/or dependencies required by the task have been set up, must be matched to the count provided to cvm_ts_add_task_signal_dependencies
void cvm_task_signal_dependencies(cvm_task * task, uint16_t count);
void cvm_task_signal_dependency(cvm_task * task);


/// execution dependencies also act as cleanup dependencies (because we cannot clean up a task ultil it has been executed)



/// corresponds to the number of times a matching `cvm_task_release_retainer` must be called for the task before it can be destroyed/released
/// at least one retainer must be held in order to set up a successor to that task if it has already been enqueued
void cvm_task_retain_reference(cvm_task * task, uint16_t count);

/// signal that we are done setting up things that must happen before cleanup (e.g. setting up sucessors tasks/gates)
void cvm_task_release_references(cvm_task * task, uint16_t count);
void cvm_task_release_reference(cvm_task * task);


/// the dependency on the sucessor must have been set up before calling this function
/// if the task has already been completed this will signal the sucessor immediately
void cvm_task_add_sucessor(cvm_task * task, cvm_synchronization_primitive * sucessor);





#endif



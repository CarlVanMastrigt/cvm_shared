/**
Copyright 2023,2024 Carl van Mastrigt

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

typedef struct cvm_task cvm_task;
typedef struct cvm_task_system cvm_task_system;

typedef struct cvm_task_pool cvm_task_pool;

typedef void(*cvm_task_function)(void*);

// generalised dependency chaining became too complicated for this application

// probably want tie in to an "actor" subsystem (single threaded process) should that actually be determined to be a desirable thing to have
//      ^ could just have series of tasks that trigger each other and/or mailbox that ensures it either has a task executing or enqueues execution of a task to handle the work (unified work system)


#define CVM_TASK_MAX_SUCESSOR_TASKS_EXPONENT 4
#define CVM_TASK_MAX_SUCESSOR_GATES_EXPONENT 2
#define CVM_TASK_POOL_MAX_WORKER_THREAD_EXPONENT 8
/// please note: max_worker_threads = 2^CVM_TASK_POOL_MAX_WORKER_THREAD_EXPONENT-1






#define CVM_TASK_MAX_SUCESSOR_TASKS ((1<<CVM_TASK_MAX_SUCESSOR_TASKS_EXPONENT)-1)
#define CVM_TASK_MAX_SUCESSOR_GATES ((1<<CVM_TASK_MAX_SUCESSOR_GATES_EXPONENT)-1)
#define CVM_TASK_POOL_MAX_WORKER_THREADS ((1<<CVM_TASK_POOL_MAX_WORKER_THREAD_EXPONENT)-1)

#define CVM_TASK_POOL_BATCH_COUNT_EXPONENT 12
#define CVM_TASK_POOL_BATCH_SIZE_EXPONENT 12



struct cvm_task
{
    cvm_task_system * task_system;
    cvm_task_pool * pool;

    cvm_task_function task_func;
    void * task_func_input;

    /// because everything is a singly linked queue/stack we need a link to the next, but also we need to know the location/identity of this task in memory when its a "free floating" pointer (not on either the pending or available task lists)
    uint32_t next;

    /// need to only init atomics once: "If obj was not default-constructed, or this function is called twice on the same obj, the behavior is undefined."

    atomic_uint_fast16_t execution_dependencies;
    atomic_uint_fast16_t cleanup_dependencies;

    atomic_flag sucessor_lock;

    uint_fast16_t complete : 1;/// has the task completed
    uint_fast16_t successor_task_count : CVM_TASK_MAX_SUCESSOR_TASKS_EXPONENT;
    uint_fast16_t successor_gate_count : CVM_TASK_MAX_SUCESSOR_GATES_EXPONENT;

    cvm_task * successor_tasks[CVM_TASK_MAX_SUCESSOR_TASKS];
    cvm_gate * successor_gates[CVM_TASK_MAX_SUCESSOR_GATES];
};


#define CVM_LOCKFREE_POOL_TYPE cvm_task_pool
#define CVM_LOCKFREE_POOL_ENTRY_TYPE cvm_task
#define CVM_LOCKFREE_POOL_FUNCTION_PREFIX cvm_task_pool
#define CVM_LOCKFREE_LIST_TYPE cvm_task_list
#define CVM_LOCKFREE_LIST_FUNCTION_PREFIX cvm_task_list

#include "cvm_lockfree_list.h"


/// do we want a syetem to spin up new workers if/when a gate is stalled?




struct cvm_task_system
{
    /// priors - subsequents/futures/following
    /// before - after
    /// past/history - future
    /// preceeding - following (gates/tasks)
    /// predecessors/successors


    cvm_task_pool task_pool;
    cvm_task_list pending_tasks;

    thrd_t * worker_threads;/// make this system extensible (to a degree) should stalled threads want to be switched to a worker thread (can perhaps do this without relying on scheduler though)
    uint32_t worker_thread_count;

    cnd_t worker_thread_condition;// multipurpose, used for setup, shutdown and for handling stalled threads
    mtx_t worker_thread_mutex;

    uint32_t signalled_unstalls;
    uint32_t debug_stalled_count;

    bool running;

    /// pool size for types and expected pool count (to allow over use without pointer change in cases where a second pool would overrun memory capabilities and crash)
    /// is above too much to expose to user? maybe take expected pool size and quarter it?
};

void cvm_task_system_initialise(cvm_task_system * task_system, uint32_t worker_thread_count);

/// must NOT add new tasks after this is called
void cvm_task_system_terminate(cvm_task_system * task_system);

/// this really just sets up a task after acquiring or allocating memory for one
cvm_task * cvm_create_task(cvm_task_system * task_system, cvm_task_function func, void * data);

/// allows a task to be executed (is basically just another signal task call lol)
/// must eother add all execution/cleanup dependencies before calling this or add dependencies of that type that will be used to set up further dependencies
void cvm_enqueue_task(cvm_task * task);



/// corresponds to the number of times `cvm_signal_task_execution_dependencies` must be explicitly called on task before it can be executed
/// an execution dependency must be held in order to set up a dependency to that task if it has already been enqueued
void cvm_task_add_execution_dependencies(cvm_task * task, uint32_t dependencies);
/// ^ can/should be mechanism for holding off on setting up external dependencies before execution (though i kind of dislike this)

// do we need a "mass signal" variant?
/// use this to signal that some set of data and/or dependencies required by the task have been set up, must be matched to the count provided to cvm_ts_add_task_signal_dependencies
void cvm_task_signal_execution_dependency(cvm_task * task);


/// execution dependencies also act as cleanup dependencies (because we cannot clean up a task ultil it has been executed)



/// corresponds to the number of times `cvm_task_signal_cleanup_dependency` must be explicitly called on task before it can be destroyed/released
/// a cleanup dependency must be held in order to set up a successor to that task if it has already been enqueued
void cvm_task_add_cleanup_dependencies(cvm_task * task, uint32_t dependencies);
/// ^ can/should be mechanism for holding off on setting up external dependencies before execution (though i kind of dislike this)

/// signal that we are done setting up things that must happen before cleanup (e.g. setting up sucessors tasks/gates)
void cvm_task_signal_cleanup_dependency(cvm_task * task);




/// adds a successor/dependency relationship to the 2 tasks
void cvm_add_task_task_dependency(cvm_task * a, cvm_task * b);/// names? a before b is actually kind of understandable, before and after was somewhat confusing
/// are aliases of above more confusing/undesirable? otherwise could alias as add_dependency and add_dependent

/// use to ensure a task runs before the gate is signalled
void cvm_add_task_gate_dependency(cvm_task * a, cvm_gate * b);


#endif



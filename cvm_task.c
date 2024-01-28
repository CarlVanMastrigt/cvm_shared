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

#include "cvm_shared.h"


static void cvm_task_init(cvm_task * task)
{
    task->sucessor_lock = (atomic_flag)ATOMIC_FLAG_INIT;
    atomic_init(&task->execution_dependencies,0);
    atomic_init(&task->cleanup_dependencies,0);
}

//static void cvm_task_destroy(cvm_task * task)
//{
//}


#define CVM_LOCKFREE_POOL_ENTRY_TYPE cvm_task

#define CVM_LOCKFREE_POOL_TYPE cvm_task_pool
#define CVM_LOCKFREE_POOL_FUNCTION_PREFIX cvm_task_pool

#define CVM_LOCKFREE_LIST_TYPE cvm_task_list
#define CVM_LOCKFREE_LIST_FUNCTION_PREFIX cvm_task_list

#define CVM_LOCKFREE_POOL_BATCH_COUNT_EXPONENT CVM_TASK_POOL_BATCH_COUNT_EXPONENT
#define CVM_LOCKFREE_POOL_BATCH_SIZE_EXPONENT  CVM_TASK_POOL_BATCH_SIZE_EXPONENT
#define CVM_LOCKFREE_POOL_STALL_COUNT_EXPONENT CVM_TASK_POOL_MAX_WORKER_THREAD_EXPONENT

#define CVM_LOCKFREE_POOL_ENTRY_INIT_FUNCTION cvm_task_init
//#define CVM_LOCKFREE_POOL_ENTRY_DESTROY_FUNCTION cvm_task_destroy

#include "cvm_lockfree_list.c"

void cvm_task_add_execution_dependencies(cvm_task * task, uint32_t dependencies)
{
    /// this operation doesn't form a gate, the paired signaling of this task (cvm_ts_signal_task_dependency) that does that and thus needs strict memory ordering
    uint_fast16_t old_count=atomic_fetch_add_explicit(&task->execution_dependencies, dependencies, memory_order_relaxed);
    assert(old_count>0);/// should not be adding dependencies when none still exist (need held dependencies to addsetup more dependencies)
}

void cvm_task_signal_execution_dependency(cvm_task * task)
{
    uint_fast16_t old_count;
    bool unstall_worker;
    cvm_task_system * task_system=task->task_system;

    /// this is responsible for coalescing all modifications, but also for making them available to the next thread/atomic to recieve this memory (after the potential release in this function)
    old_count=atomic_fetch_sub_explicit(&task->execution_dependencies, 1, memory_order_acq_rel);
    assert(old_count>0);///must not make dep. count go negative

    if(old_count==1)/// this is the last dependency this task was waiting on, put it on list of available tasks and make sure there's a worker thread to satisfy it
    {
        /// acquire stall count here?
        unstall_worker=cvm_task_list_add_and_try_unstall(&task_system->pending_tasks,task);

        if(unstall_worker)
        {
            mtx_lock(&task_system->worker_thread_mutex);

            assert(task_system->debug_stalled_count!=0);
            assert(task_system->signalled_unstalls < CVM_TASK_POOL_MAX_WORKER_THREADS);/// somehow exceeded maximum worker threads
            task_system->signalled_unstalls--;

            cnd_signal(&task_system->worker_thread_condition);
            mtx_unlock(&task_system->worker_thread_mutex);
        }
    }
}

void cvm_task_add_cleanup_dependencies(cvm_task * task, uint32_t dependencies)
{
    uint_fast16_t old_count=atomic_fetch_add_explicit(&task->cleanup_dependencies, dependencies, memory_order_relaxed);
    assert(old_count!=0);/// should not be adding successors reservations when none still exist (need held successors reservations to addsetup more successors reservations)
}

void cvm_task_signal_cleanup_dependency(cvm_task * task)
{
    /// need to release to prevent reads/writes of successor/completion data being moved after this operation
    uint_fast16_t old_count=atomic_fetch_sub_explicit(&task->cleanup_dependencies,1,memory_order_release);
    assert(old_count>0);/// have completed more sucessor reservations than were made
    if(old_count==1)
    {
        /// this should only happen after task completion, BUT the only time this counter can (if used properly) hit zero is if the task HAS completed!
        cvm_task_pool_relinquish_entry(task->pool,task);
    }
}



cvm_task * cvm_create_task(cvm_task_system * task_system, cvm_task_function function, void * data)
{
    cvm_task * task;

    task=cvm_task_pool_acquire_entry(&task_system->task_pool);

    task->task_system=task_system;

    task->task_func=function;
    task->task_func_input=data;

    task->complete=false;
    task->successor_task_count=0;
    task->successor_gate_count=0;

    /// need to wait on "enqueue" op before beginning, ergo need one extra wait for that (enqueue is really just a signal)
    atomic_store_explicit(&task->execution_dependencies,1,memory_order_relaxed);
    atomic_store_explicit(&task->cleanup_dependencies,1,memory_order_relaxed);

    /// task is about to become "free floating" so next should reference itself

    return task;
}

void cvm_enqueue_task(cvm_task * task)
{
    /// this is basically just called differently to account for the "hidden" wait counter added on task creation
    cvm_task_signal_execution_dependency(task);
}



static int cvm_task_worker_thread_function(void * in)
{
    cvm_task_system * task_system=in;
    cvm_task * task;
    uint_fast16_t i,task_count,gate_count;
    bool running;

    running=true;
/// thread starts stalled

    while(running)
    {
        /// if there are no more pending tasks
        while((task=cvm_task_list_get(&task_system->pending_tasks)))
        {
            ///stalled--
            task->task_func(task->task_func_input);

            while(atomic_flag_test_and_set_explicit(&task->sucessor_lock, memory_order_acq_rel));
            assert(!task->complete);
            task->complete=true;
            task_count=task->successor_task_count;
            gate_count=task->successor_gate_count;
            atomic_flag_clear_explicit(&task->sucessor_lock, memory_order_release);/// once completed is set to true no more sucessors will be added, so is safe to use these counts here

            for(i=0;i<task_count;i++)
            {
                cvm_task_signal_execution_dependency(task->successor_tasks[i]);// should release changes
            }

            for(i=0;i<gate_count;i++)
            {
                cvm_gate_signal(task->successor_gates[i]);// should release changes
            }

            cvm_task_signal_cleanup_dependency(task);
        }

        mtx_lock(&task_system->worker_thread_mutex);

        ///probably reasonable to make shutdown rely on all tasks having been completed in a way managed by user/programmer
        if(task_system->running)
        {
            task=cvm_task_list_get_or_stall(&task_system->pending_tasks);/// this will increment stall if it fails (which note: is inside the mutex lock)

            if(!task)
            {
                task_system->debug_stalled_count++;

                do
                {
                    cnd_wait(&task_system->worker_thread_condition, &task_system->worker_thread_mutex);
                }
                while(task_system->signalled_unstalls);

                task_system->signalled_unstalls--;/// we have unstalled successfully

                task_system->debug_stalled_count--;
            }
        }
        else
        {
            /// nothing left to do and no longer running
            running=false;
        }

        mtx_unlock(&task_system->worker_thread_mutex);
    }

    return 0;
}

void cvm_task_system_initialise(cvm_task_system * task_system, uint32_t worker_thread_count)
{
    uint32_t i;

    assert(worker_thread_count <= CVM_TASK_POOL_MAX_WORKER_THREADS);///trying to initialise task system with too many tasks

    cvm_task_pool_init(&task_system->task_pool);
    cvm_task_list_init(&task_system->task_pool,&task_system->pending_tasks);

    task_system->worker_threads=malloc(sizeof(thrd_t)*worker_thread_count);
    task_system->worker_thread_count=worker_thread_count;

    cnd_init(&task_system->worker_thread_condition);
    mtx_init(&task_system->worker_thread_mutex, mtx_plain);

    task_system->running=true;

    task_system->debug_stalled_count=0;
    task_system->signalled_unstalls=0;

    /// will need setup mutex locked here if we want to wait on all workers to start before progressing (maybe useful to have, but I can't think of a reason)

    for(i=0;i<worker_thread_count;i++)
    {
        thrd_create(task_system->worker_threads+i, cvm_task_worker_thread_function, task_system);
    }
}

void cvm_task_system_terminate(cvm_task_system * task_system)
{
    uint32_t i;

    mtx_lock(&task_system->worker_thread_mutex);
    task_system->running=false;
    cnd_broadcast(&task_system->worker_thread_condition);/// wake up all stalled threads
    mtx_unlock(&task_system->worker_thread_mutex);

    for(i=0;i<task_system->worker_thread_count;i++)
    {
        thrd_join(task_system->worker_threads[i], NULL);
    }

    free(task_system->worker_threads);

    cnd_destroy(&task_system->worker_thread_condition);
    mtx_destroy(&task_system->worker_thread_mutex);

    cvm_task_pool_destroy(&task_system->task_pool);
}



void cvm_add_task_task_dependency(cvm_task * a, cvm_task * b)
{
    bool already_executed;

    assert(atomic_load_explicit(&a->cleanup_dependencies,memory_order_relaxed)>0);/// must have dependency to set up other dependencies
    while(atomic_flag_test_and_set_explicit(&a->sucessor_lock, memory_order_acq_rel));
    assert(a->successor_task_count<CVM_TASK_MAX_SUCESSOR_TASKS);/// trying to set up too many sucessors (in debug could track this total separely to ones added prior to execution)
    already_executed=a->complete;
    if(!already_executed)
    {
        a->successor_tasks[a->successor_task_count++]=b;
    }
    atomic_flag_clear_explicit(&a->sucessor_lock, memory_order_release);

    assert(atomic_load_explicit(&b->execution_dependencies,memory_order_relaxed)>0);/// must have dependency to set up other dependencies
    if(!already_executed)
    {
        cvm_task_add_execution_dependencies(b,1);
    }
}

void cvm_add_task_gate_dependency(cvm_task * a, cvm_gate * b)
{
    bool already_executed;

    assert(atomic_load_explicit(&a->cleanup_dependencies,memory_order_relaxed)>0);/// must have dependency to set up other dependencies
    while(atomic_flag_test_and_set_explicit(&a->sucessor_lock, memory_order_acq_rel));
    assert(a->successor_gate_count<CVM_TASK_MAX_SUCESSOR_GATES);/// trying to set up too many sucessors (in debug could track this total separely to ones added prior to execution)
    already_executed=a->complete;
    if(!already_executed)
    {
        a->successor_gates[a->successor_gate_count++]=b;
    }
    atomic_flag_clear_explicit(&a->sucessor_lock, memory_order_release);

    if(!already_executed)
    {
        cvm_gate_add_dependencies(b,1);
    }
}


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


void cvm_task_add_dependencies(cvm_task * task, uint16_t count)
{
    uint_fast16_t old_count=atomic_fetch_add_explicit(&task->dependency_count, count, memory_order_relaxed);
    assert(old_count>0);/// should not be adding dependencies when none still exist (need held dependencies to addsetup more dependencies)
}

void cvm_task_signal_dependencies(cvm_task * task, uint16_t count)
{
    uint_fast16_t old_count;
    bool unstall_worker;
    cvm_task_system * task_system=task->task_system;

    /// this is responsible for coalescing all modifications, but also for making them available to the next thread/atomic to recieve this memory (after the potential release in this function)
    old_count=atomic_fetch_sub_explicit(&task->dependency_count, count, memory_order_acq_rel);
    assert(old_count >= count);///must not make dep. count go negative

    if(old_count==count)/// this is the last dependency this task was waiting on, put it on list of available tasks and make sure there's a worker thread to satisfy it
    {
        /// acquire stall count here?
        unstall_worker=cvm_coherent_queue_fail_tracking_add_and_decrement_fails(&task_system->pending_tasks, task);

        if(unstall_worker)
        {
            mtx_lock(&task_system->worker_thread_mutex);

            assert(task_system->debug_stalled_count > 0);/// weren't actually any stalled workers

            #warning remove
            if(task_system->signalled_unstalls >= task_system->worker_thread_count) printf("debug stall count: %u\n",task_system->debug_stalled_count);

            assert(task_system->signalled_unstalls < task_system->worker_thread_count);/// somehow exceeded maximum worker threads
            task_system->signalled_unstalls++;/// required for preventing spurrious wakeup

            cnd_signal(&task_system->worker_thread_condition);
            mtx_unlock(&task_system->worker_thread_mutex);
        }
    }
}

void cvm_task_signal_dependency(cvm_task * task)
{
    cvm_task_signal_dependencies(task, 1);
}



void cvm_task_retain_reference(cvm_task * task, uint16_t count)
{
    uint_fast16_t old_count=atomic_fetch_add_explicit(&task->reference_count, count, memory_order_relaxed);
    assert(old_count!=0);/// should not be adding successors reservations when none still exist (need held successors reservations to addsetup more successors reservations)
}

void cvm_task_release_references(cvm_task * task, uint16_t count)
{
    /// need to release to prevent reads/writes of successor/completion data being moved after this operation
    uint_fast16_t old_count=atomic_fetch_sub_explicit(&task->reference_count, count, memory_order_release);

    assert(old_count >= count);/// have completed more sucessor reservations than were made

    if(old_count==count)
    {
        /// this should only happen after task completion, BUT the only time this counter can (if used properly) hit zero is if the task HAS completed!
        cvm_lockfree_pool_relinquish_entry(&task->task_system->task_pool, task);
    }
}

void cvm_task_release_reference(cvm_task * task)
{
    cvm_task_release_references(task, 1);
}



cvm_task * cvm_create_task(cvm_task_system * task_system, cvm_task_function function, void * data)
{
    cvm_task * task;

    task = cvm_lockfree_pool_acquire_entry(&task_system->task_pool);

    task->task_system = task_system;

    task->task_func=function;
    task->task_func_input=data;

    task->complete=false;
    task->successor_count = 0;

    /// need to wait on "enqueue" op before beginning, ergo need one extra dependency for that (enqueue is really just a signal)
    atomic_store_explicit(&task->dependency_count, 1, memory_order_relaxed);
    /// need to retain a reference until the sucessors are actually signalled, ergo one extra reference that will be released after completing the task
    atomic_store_explicit(&task->reference_count, 1, memory_order_relaxed);

    return task;
}

void cvm_task_enqueue(cvm_task * task)
{
    /// this is basically just called differently to account for the "hidden" wait counter added on task creation
    cvm_task_signal_dependency(task);
}



static int cvm_task_worker_thread_function(void * in)
{
    cvm_task_system * task_system=in;
    cvm_task * task;
    uint_fast16_t i,sucessor_count;
    bool running;

    running=true;
    task=NULL;

    while(running)
    {
        if(task==NULL)/// task could be null b/c we're initialising or b/c this worker has just woken up from stalling on not being able to acquire a task
        {
            task=cvm_coherent_queue_fail_tracking_get(&task_system->pending_tasks);
        }

        /// if there are no more pending tasks
        while(task)
        {
            ///perform the task
            task->task_func(task->task_func_input);

            while(atomic_flag_test_and_set_explicit(&task->sucessor_lock, memory_order_acquire));
            assert(!task->complete);/// how did a task get completed twice!? was it re-used without being reset?
            task->complete=true;
            sucessor_count=task->successor_count;
            atomic_flag_clear_explicit(&task->sucessor_lock, memory_order_release);/// once completed is set to true no more sucessors will be added, so is safe to use these counts here

            for(i=0;i<sucessor_count;i++)
            {
                task->successors[i]->signal_function(task->successors[i]);/// this op should release changes
            }

            cvm_task_release_reference(task);

            task=cvm_coherent_queue_fail_tracking_get(&task_system->pending_tasks);
        }

        /// there were no more tasks to perform
        mtx_lock(&task_system->worker_thread_mutex);

        /// probably reasonable to make shutdown rely on all tasks having been completed in a way managed by user/programmer
        if(task_system->running)
        {
            task=cvm_coherent_queue_fail_tracking_get_or_increment_fails(&task_system->pending_tasks);/// this will increment local stall counter if it fails (which note: is inside the mutex lock)

            if(!task)
            {
                task_system->debug_stalled_count++;

                do
                {
                    /// wait untill more workers are needed or we're shutting down (with appropriate checks in case of spurrious wakeup)
                    cnd_wait(&task_system->worker_thread_condition, &task_system->worker_thread_mutex);
                }
                while(task_system->signalled_unstalls==0 && task_system->running);

                task_system->signalled_unstalls--;/// we have unstalled successfully

                task_system->debug_stalled_count--;
            }
        }
        else
        {
            /// nothing left to do and task system is being shut down, we can exit
            running=false;
        }

        mtx_unlock(&task_system->worker_thread_mutex);
    }

    return 0;
}



/// for polymorphism
static void cvm_task_signal_func(void * primitive)
{
    cvm_task_signal_dependency(primitive);
}

static void cvm_task_initialise(void * elem, void * data)
{
    cvm_task * task = elem;
    cvm_task_system * task_system = data;

    task->signal_function = cvm_task_signal_func;
    task->task_system = task_system;

    atomic_init(&task->dependency_count, 0);
    atomic_init(&task->reference_count, 0);
    task->sucessor_lock = (atomic_flag)ATOMIC_FLAG_INIT;
}

void cvm_task_system_initialise(cvm_task_system * task_system, uint32_t worker_thread_count, size_t total_task_exponent)
{
    uint32_t i;

    cvm_lockfree_pool_initialise(&task_system->task_pool, total_task_exponent, sizeof(cvm_task));
    cvm_lockfree_pool_call_for_every_entry(&task_system->task_pool, &cvm_task_initialise, &task_system);

    cvm_coherent_queue_fail_tracking_initialise(&task_system->pending_tasks, &task_system->task_pool);

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
        /// if thread gets stuck here its possible that not all tasks were able to complete, perhaps because things werent shut down correctly and some tasks have outstanding dependencies
        thrd_join(task_system->worker_threads[i], NULL);
    }

    free(task_system->worker_threads);

    cnd_destroy(&task_system->worker_thread_condition);
    mtx_destroy(&task_system->worker_thread_mutex);

    cvm_coherent_queue_fail_tracking_terminate(&task_system->pending_tasks);
    cvm_lockfree_pool_terminate(&task_system->task_pool);
}


void cvm_task_add_sucessor(cvm_task * task, cvm_synchronization_primitive * sucessor)
{
    bool already_completed;

    assert(atomic_load_explicit(&task->reference_count, memory_order_relaxed));/// task must be retained to set up sucessors (can technically be satisfied illegally, using queue for re-use will make detection better but not infallible)

    while(atomic_flag_test_and_set_explicit(&task->sucessor_lock, memory_order_acquire));

    assert(task->successor_count < CVM_TASK_MAX_SUCESSORS);/// trying to set up too many sucessors (in debug could track this total separely to ones added prior to execution)

    already_completed = task->complete;

    if(!already_completed)
    {
        task->successors[task->successor_count++] = sucessor;
    }

    atomic_flag_clear_explicit(&task->sucessor_lock, memory_order_release);

    if(already_completed)
    {
        sucessor->signal_function(sucessor);
    }
}


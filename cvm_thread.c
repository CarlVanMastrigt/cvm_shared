/**
Copyright 2021,2022 Carl van Mastrigt

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

#include "solipsix.h"





void cvm_synchronous_thread_group_create(cvm_synchronous_thread_group_data * group_data,cvm_thread_function * functions,void ** data,uint_fast32_t thread_count)
{
    uint_fast32_t i;
    group_data->threads=malloc(sizeof(cvm_synchronous_thread)*thread_count);
    group_data->thread_count=thread_count;

    cnd_init(&group_data->condition);
    atomic_init(&group_data->active_counter,0);///start with no threads active
    group_data->running=true;

    for(i=0;i<thread_count;i++)
    {
        mtx_init(&group_data->threads[i].mutex,mtx_plain);
        group_data->threads[i].group_data=group_data;
        group_data->threads[i].data=data[i];
        group_data->threads[i].waiting=false;

        thrd_create(&group_data->threads[i].thread,functions[i],group_data->threads+i);
    }
}

void cvm_synchronous_thread_group_join(cvm_synchronous_thread_group_data * group_data,int ** res)
{
    uint_fast32_t i;

    assert(!group_data->running);

    for(i=0;i<group_data->thread_count;i++)
    {
        thrd_join(group_data->threads[i].thread,res?res[i]:NULL);

        mtx_destroy(&group_data->threads[i].mutex);
    }

    free(group_data->threads);

    cnd_destroy(&group_data->condition);
}

void cvm_synchronous_thread_group_critical_section_begin(cvm_synchronous_thread_group_data * group_data)
{
    uint_fast32_t expected_active_counter,i;

    /// following is very unlikely to ever actually spin, but we need to ensure that all threads have actually been woken up and regained their mutex before we start them up again
    do expected_active_counter=group_data->thread_count;
    while( ! atomic_compare_exchange_weak_explicit(&group_data->active_counter,&expected_active_counter,0,memory_order_acquire,memory_order_relaxed));
    ///wait for thread to be active (worker thread has definitely gained control of mutex)

    for(i=0;i<group_data->thread_count;i++)
    {
        mtx_lock(&group_data->threads[i].mutex);
        assert(group_data->threads[i].waiting);///thread must be waiting
        ///main thread must have all relevant mutexes locked
    }
}

void cvm_synchronous_thread_group_critical_section_end(cvm_synchronous_thread_group_data * group_data,bool keep_running)
{
    uint_fast32_t i;

    group_data->running=keep_running;
    cnd_broadcast(&group_data->condition);

    for(i=0;i<group_data->thread_count;i++)
    {
        group_data->threads[i].waiting=false;
        mtx_unlock(&group_data->threads[i].mutex);
        ///main thread must unlock all mutexes so that all worker threads may begin again
    }
}

void cvm_synchronous_thread_critical_section_wait(cvm_synchronous_thread * thread)
{
    thread->waiting=true;
    do
    {
        cnd_wait(&thread->group_data->condition,&thread->mutex);///unlocks mutex (MUST have control at this point) then waits on condition and regains control of it's mutex once condition is signalled/broadcast
    }
    while(thread->waiting);
    atomic_fetch_add_explicit(&thread->group_data->active_counter,1,memory_order_release);///signal that this thread has started again and gained control of its mutex
}

void cvm_synchronous_thread_initialise(cvm_synchronous_thread * thread)
{
    mtx_lock(&thread->mutex);
    atomic_fetch_add_explicit(&thread->group_data->active_counter,1,memory_order_release);///signal that this thread has started and gained control of its mutex
}

void cvm_synchronous_thread_finalise(cvm_synchronous_thread * thread)
{
    mtx_unlock(&thread->mutex);
}

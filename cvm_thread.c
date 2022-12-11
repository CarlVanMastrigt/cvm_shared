/**
Copyright 2021,2022 Carl van Mastrigt

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





void cvm_thread_group_data_create(cvm_thread_group_data * group_data,cvm_thread_function func,void ** data,uint_fast32_t thread_count)
{
    uint_fast32_t i;
    group_data->threads=malloc(sizeof(cvm_thread)*thread_count);
    group_data->thread_count=thread_count;

    cnd_init(&group_data->condition);
    atomic_init(&group_data->active_counter,0);///start with no threads active
    group_data->running=true;

    for(i=0;i<thread_count;i++)
    {
        mtx_init(&group_data->threads[i].mutex,mtx_plain);
        group_data->threads[i].group_data=group_data;
        group_data->threads[i].data=data[i];

        thrd_create(&group_data->threads[i].thread,func,group_data->threads+i);
    }
}

void cvm_thread_group_data_join(cvm_thread_group_data * group_data,int ** res)
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

void cvm_thread_group_critical_section_start(cvm_thread_group_data * group_data)
{
    uint_fast32_t value,i;

    do value=group_data->thread_count;
    while( ! atomic_compare_exchange_weak(&group_data->active_counter,&value,0));///wait for signal to be active (worker thread has definitely gained control of mutex)

    for(i=0;i<group_data->thread_count;i++)
    {
        mtx_lock(&group_data->threads[i].mutex);
        ///main thread must have all relevant mutexes locked
    }
}

void cvm_thread_group_critical_section_end(cvm_thread_group_data * group_data,bool keep_running)
{
    uint_fast32_t i;

    group_data->running=keep_running;
    cnd_broadcast(&group_data->condition);

    for(i=0;i<group_data->thread_count;i++)
    {
        mtx_unlock(&group_data->threads[i].mutex);
        ///main thread must unlock all mutexes so that all worker threads may begin again
    }
}

void cvm_thread_critical_section_wait(cvm_thread * thread)
{
    cnd_wait(&thread->group_data->condition,&thread->mutex);///unlocks mutex (MUST have control at this point) then waits on condition and regains control of it's mutex once condition is signalled/broadcast
    atomic_fetch_add(&thread->group_data->active_counter,1);///signal that this thread has started again and gained control of its mutex
}

void cvm_thread_initialise(cvm_thread * thread)
{
    mtx_lock(&thread->mutex);
    atomic_fetch_add(&thread->group_data->active_counter,1);///signal that this thread has started and gained control of its mutex
}

void cvm_thread_finalise(cvm_thread * thread)
{
    mtx_unlock(&thread->mutex);
}

/**
Copyright 2021 Carl van Mastrigt

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

int cvm_synchronised_thread_create(cvm_synchronised_thread * thread,cvm_thread_function func,void * data)
{
    mtx_init(&thread->mutex,mtx_plain);
    cnd_init(&thread->condition);

    thread->waiting=false;
    thread->running=true;

    atomic_init(&thread->spinlock,1);

    thread->data=data;

    return thrd_create(&thread->thread,func,thread);
}

int cvm_synchronised_thread_join(cvm_synchronised_thread * thread,int * res)
{
    int r = thrd_join(thread->thread,res);

    cnd_destroy(&thread->condition);
    mtx_destroy(&thread->mutex);

    return r;
}

void cvm_synchronised_thread_critical_section_start(cvm_synchronised_thread * thread)
{
    uint_fast32_t lock;
    do lock=0;
    while( ! atomic_compare_exchange_weak(&thread->spinlock,&lock,1));

    mtx_lock(&thread->mutex);///critical section start
}

void cvm_synchronised_thread_critical_section_end(cvm_synchronised_thread * thread,bool keep_running)
{
    thread->running=keep_running;
    atomic_store(&thread->spinlock,1);
    thread->waiting=false;

    cnd_signal(&thread->condition);
    mtx_unlock(&thread->mutex);///critical section end
}

void cvm_synchronised_thread_critical_section_wait(cvm_synchronised_thread * thread)
{
    thread->waiting=true;
    do cnd_wait(&thread->condition,&thread->mutex);
    while(thread->waiting);

    atomic_store(&thread->spinlock,0);
}

void cvm_synchronised_thread_initialise(cvm_synchronised_thread * thread)
{
    mtx_lock(&thread->mutex);
    atomic_store(&thread->spinlock,0);
}

void cvm_synchronised_thread_finalise(cvm_synchronised_thread * thread)
{
    mtx_unlock(&thread->mutex);
}




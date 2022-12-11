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

#ifndef CVM_SHARED_H
#include "cvm_shared.h"
#endif

#ifndef CVM_THREAD_H
#define CVM_THREAD_H

typedef int(*cvm_thread_function)(void*);

//typedef struct cvm_synchronised_thread
//{
//    thrd_t thread;
//
//    mtx_t mutex;
//    cnd_t condition;
//
//    atomic_uint_fast32_t signal;
//
//    bool running;
//
//    void * data;
//}
//cvm_synchronised_thread;
//
//int cvm_synchronised_thread_create(cvm_synchronised_thread * thread,cvm_thread_function func,void * data);
//int cvm_synchronised_thread_join(cvm_synchronised_thread * thread,int * res);
//
/////called from parent thread
//void cvm_synchronised_thread_critical_section_start(cvm_synchronised_thread * thread);
//void cvm_synchronised_thread_critical_section_end(cvm_synchronised_thread * thread,bool keep_running);
/////called from child thread
//void cvm_synchronised_thread_critical_section_wait(cvm_synchronised_thread * thread);
//void cvm_synchronised_thread_initialise(cvm_synchronised_thread * thread);
//void cvm_synchronised_thread_finalise(cvm_synchronised_thread * thread);




static inline void cvm_atomic_lock_create(atomic_uint_fast32_t * spinlock)
{
    atomic_init(spinlock,0);
}

static inline void cvm_atomic_lock_acquire(atomic_uint_fast32_t * spinlock)
{
    uint_fast32_t lock;
    do lock=atomic_load(spinlock);
    while(lock!=0 || !atomic_compare_exchange_weak(spinlock,&lock,1));
}

static inline void cvm_atomic_lock_release(atomic_uint_fast32_t * spinlock)
{
    atomic_store(spinlock,0);
}


#define CVM_MAX_THREADS 16

typedef struct cvm_thread_group_data cvm_thread_group_data;

typedef struct cvm_thread
{
    thrd_t thread;
    mtx_t mutex;
    cvm_thread_group_data * group_data;
    void * data;
}
cvm_thread;

struct cvm_thread_group_data
{
    cvm_thread * threads;
    uint_fast32_t thread_count;

    cnd_t condition;
    atomic_uint_fast32_t active_counter;
    bool running;
};

void cvm_thread_group_data_create(cvm_thread_group_data * group_data,cvm_thread_function * functions,void ** data,uint_fast32_t thread_count);
void cvm_thread_group_data_join(cvm_thread_group_data * group_data,int ** res);///res must be a buffer of size CVM_MAX_THREADS or NULL

///called from parent thread
void cvm_thread_group_critical_section_start(cvm_thread_group_data * group_data);
void cvm_thread_group_critical_section_end(cvm_thread_group_data * group_data,bool keep_running);
///called from child thread
void cvm_thread_critical_section_wait(cvm_thread * thread);
void cvm_thread_initialise(cvm_thread * thread);
void cvm_thread_finalise(cvm_thread * thread);



#endif


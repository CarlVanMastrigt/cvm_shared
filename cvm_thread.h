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

#ifndef solipsix_H
#include "solipsix.h"
#endif

#ifndef CVM_THREAD_H
#define CVM_THREAD_H

typedef int(*cvm_thread_function)(void*);




/// 1 means locked, 0 unlocked
static inline void cvm_atomic_lock_create(atomic_uint_fast32_t * spinlock)
{
    atomic_init(spinlock,0);
}

static inline void cvm_atomic_lock_acquire(atomic_uint_fast32_t * spinlock)
{
    uint_fast32_t lock;
    do lock=atomic_load(spinlock);
    while(lock || !atomic_compare_exchange_weak(spinlock,&lock,1));
}

static inline void cvm_atomic_lock_release(atomic_uint_fast32_t * spinlock)
{
    atomic_store(spinlock,0);
}



/**
"synchronous" used here to communicate that threads run in parallel with some controlling thread, that is the check in at some point when necessary (the critical section)
*/

typedef struct cvm_synchronous_thread_group_data cvm_synchronous_thread_group_data;

typedef struct cvm_synchronous_thread
{
    cvm_synchronous_thread_group_data * group_data;
    void * data;
    thrd_t thread;
    mtx_t mutex;
    bool waiting;
}
cvm_synchronous_thread;

struct cvm_synchronous_thread_group_data
{
    cvm_synchronous_thread * threads;
    uint_fast32_t thread_count;

    cnd_t condition;
    atomic_uint_fast32_t active_counter;
    bool running;
};

void cvm_synchronous_thread_group_create(cvm_synchronous_thread_group_data * group_data,cvm_thread_function * functions,void ** data,uint_fast32_t thread_count);
void cvm_synchronous_thread_group_join(cvm_synchronous_thread_group_data * group_data,int ** res);///res must be a buffer of size thread_count NULL

///called from parent thread
void cvm_synchronous_thread_group_critical_section_begin(cvm_synchronous_thread_group_data * group_data);
void cvm_synchronous_thread_group_critical_section_end(cvm_synchronous_thread_group_data * group_data,bool keep_running);
///called from child thread
void cvm_synchronous_thread_critical_section_wait(cvm_synchronous_thread * thread);
void cvm_synchronous_thread_initialise(cvm_synchronous_thread * thread);
void cvm_synchronous_thread_finalise(cvm_synchronous_thread * thread);



#endif


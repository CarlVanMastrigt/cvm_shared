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

#ifndef CVM_SHARED_H
#include "cvm_shared.h"
#endif

#ifndef CVM_THREAD_H
#define CVM_THREAD_H

typedef int(*cvm_thread_function)(void*);

typedef struct cvm_synchronised_thread
{
    thrd_t thread;

    mtx_t mutex;
    cnd_t condition;
    bool waiting;

    atomic_uint_fast32_t spinlock;

    bool running;

    void * data;
}
cvm_synchronised_thread;

int cvm_synchronised_thread_create(cvm_synchronised_thread * thread,cvm_thread_function func,void * data);
int cvm_synchronised_thread_join(cvm_synchronised_thread * thread,int * res);

///called from parent thread
void cvm_synchronised_thread_critical_section_start(cvm_synchronised_thread * thread);
void cvm_synchronised_thread_critical_section_end(cvm_synchronised_thread * thread,bool keep_running);
///called from child thread
void cvm_synchronised_thread_critical_section_wait(cvm_synchronised_thread * thread);
void cvm_synchronised_thread_initialise(cvm_synchronised_thread * thread);
void cvm_synchronised_thread_finalise(cvm_synchronised_thread * thread);





#endif


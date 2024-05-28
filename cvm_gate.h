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

#ifndef CVM_GATE_H
#define CVM_GATE_H

typedef struct cvm_gate_pool
{
    cvm_lockfree_pool available_gates;
}
cvm_gate_pool;

typedef struct cvm_gate
{
    cvm_sync_primitive_signal_function * signal_function;/// polymorphic base

    cvm_gate_pool * pool;

    atomic_uint_fast32_t status;

    cnd_t condition;
    mtx_t mutex;
}
cvm_gate;

cvm_gate * cvm_gate_acquire(cvm_gate_pool * pool);

/// must be called once for every dependency added by a call to cvm_gate_add_dependencies
void cvm_gate_signal(cvm_gate * gate);

/// MUST be called at some point as this will clean up the acquired gate
void cvm_gate_wait_and_relinquish(cvm_gate * gate);

/// used to set up dependencies (calls to cvm_gate_signal) to wait on (dont return from cvm_gate_wait until all signals happen)
/// must know that an gate has at least one outstanding dependency and/or that the gate hasn't been waited on for calling this to be legal
void cvm_gate_add_dependencies(cvm_gate * gate, uint32_t dependency_count);

void cvm_gate_pool_initialise(cvm_gate_pool * pool, size_t capacity_exponent);
void cvm_gate_pool_terminate(cvm_gate_pool * pool);


#endif




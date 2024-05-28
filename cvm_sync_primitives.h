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

#ifndef CVM_SYNC_PRIMITIVES_H
#define CVM_SYNC_PRIMITIVES_H

///polymorphic base for primitives (gate, task &c.)
typedef void (cvm_sync_primitive_signal_function)(void * primitive);

typedef union cvm_sync_primitive cvm_sync_primitive;

#include "cvm_task.h"
#include "cvm_gate.h"

union cvm_sync_primitive
{
    cvm_sync_primitive_signal_function * signal_function;// base of all elements in union
    cvm_task task;
    cvm_gate gate;
};

/// adds a successor/dependency relationship to the 2 tasks
void cvm_sync_task_happens_before_task(cvm_task * a, cvm_task * b);
/// are aliases of above more confusing/undesirable? otherwise could alias as add_dependency and add_dependent

/// use to ensure a task runs before the gate is signalled
void cvm_sync_task_happens_before_gate(cvm_task * a, cvm_gate * b);

#endif

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

#ifndef CVM_SYNCHRONIZATION_PRIMITIVES_H
#define CVM_SYNCHRONIZATION_PRIMITIVES_H

///polymorphic base for primitives (gate, task &c.)
typedef void (cvm_synchronization_primitive_signal_function)(void * primitive);

typedef union cvm_synchronization_primitive cvm_synchronization_primitive;

#include "cvm_task.h"
#include "cvm_gate.h"

union cvm_synchronization_primitive
{
    cvm_synchronization_primitive_signal_function * signal_function;
    cvm_task task;
    cvm_gate gate;
};



/// adds a successor/dependency relationship to the 2 tasks
void cvm_add_task_task_dependency(cvm_task * a, cvm_task * b);/// names? a before b is actually kind of understandable, before and after was somewhat confusing
/// are aliases of above more confusing/undesirable? otherwise could alias as add_dependency and add_dependent

/// use to ensure a task runs before the gate is signalled
void cvm_add_task_gate_dependency(cvm_task * a, cvm_gate * b);

#endif

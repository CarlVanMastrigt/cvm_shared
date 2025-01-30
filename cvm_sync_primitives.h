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

union cvm_sync_primitive;

struct cvm_sync_primitive_functions
{
    void(*const add_dependency   )(union cvm_sync_primitive*);
    void(*const signal_dependency)(union cvm_sync_primitive*);
    void(*const add_successor    )(union cvm_sync_primitive*, union cvm_sync_primitive*);//primitive, successor
};

#include "cvm_task.h"
#include "cvm_gate.h"

union cvm_sync_primitive
{
    const struct cvm_sync_primitive_functions* sync_functions;
    struct cvm_task task;
    struct cvm_gate gate;
};

/**
 * a happens before b
 *
 * a must have a reference retained or an unsignalled dependency
 * b must have an unsignalled dependency
 *
 * (for tasks not yet having been enqueued on counts as an unsignalled dependency)
 * (for gates not yet having been waited on counts as an unsignalled dependency)
 */
void cvm_sync_establish_ordering_typed(union cvm_sync_primitive* a, union cvm_sync_primitive* b);
void cvm_sync_establish_ordering(void* a, void* b);


#endif

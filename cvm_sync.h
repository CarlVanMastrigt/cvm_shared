/**
Copyright 2024,2025 Carl van Mastrigt

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

#ifndef CVM_SYNC_H
#define CVM_SYNC_H

union cvm_sync_primitive;

struct cvm_sync_primitive_functions
{
    void(*const impose_condition )(union cvm_sync_primitive*);
    void(*const signal_condition )(union cvm_sync_primitive*);

    void(*const retain_reference )(union cvm_sync_primitive*);
    void(*const release_reference)(union cvm_sync_primitive*);

    void(*const attach_successor )(union cvm_sync_primitive*, union cvm_sync_primitive*);//primitive, successor
};

#include "sync/task.h"
#include "sync/gate.h"

union cvm_sync_primitive
{
    const struct cvm_sync_primitive_functions* sync_functions;
    struct cvm_task task;
    struct cvm_gate gate;
};

static inline void cvm_sync_primitive_impose_condition(union cvm_sync_primitive* primitive)
{
    primitive->sync_functions->impose_condition(primitive);
}
static inline void cvm_sync_primitive_signal_condition(union cvm_sync_primitive* primitive)
{
    primitive->sync_functions->signal_condition(primitive);
}
static inline void cvm_sync_primitive_retain_reference(union cvm_sync_primitive* primitive)
{
    primitive->sync_functions->retain_reference(primitive);
}
static inline void cvm_sync_primitive_release_reference(union cvm_sync_primitive* primitive)
{
    primitive->sync_functions->release_reference(primitive);
}

/**
 * establishes an ordering dependency such that: a happens before b
 *
 * a must have a reference retained or an unsignalled dependency
 * b must have an unsignalled dependency
 *
 * (for tasks not yet having been enqueued on counts as an unsignalled dependency)
 * (for gates not yet having been waited on counts as an unsignalled dependency)
 */
static inline void cvm_sync_primitive_attach_successor(union cvm_sync_primitive* primitive, union cvm_sync_primitive* successor)
{
    successor->sync_functions->impose_condition(successor);
    // because sucessor may satisfy the dependency/condition immediately, successor must have a condition set first
    primitive->sync_functions->attach_successor(primitive, successor);
}





struct cvm_sync_queue
{
    union cvm_sync_primitive* _Atomic end_primitive;
};

void cvm_sync_queue_initialise(struct cvm_sync_queue* queue);
void cvm_sync_queue_terminate(struct cvm_sync_queue* queue);

void cvm_sync_queue_enqueue_primitive(struct cvm_sync_queue* queue, union cvm_sync_primitive* primitive);
void cvm_sync_queue_enqueue_primitive_range(struct cvm_sync_queue* queue, union cvm_sync_primitive* first_primitive, union cvm_sync_primitive* last_primitive);


#endif

/**
Copyright 2024,2025 Carl van Mastrigt

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

#pragma once

#include <stddef.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <threads.h>

#include "sol_coherent_structures.h"

union sol_sync_primitive;

struct sol_sync_primitive_functions
{
    void(*const impose_condition )(union sol_sync_primitive*);
    void(*const signal_condition )(union sol_sync_primitive*);

    void(*const retain_reference )(union sol_sync_primitive*);
    void(*const release_reference)(union sol_sync_primitive*);

    void(*const attach_successor )(union sol_sync_primitive*, union sol_sync_primitive*);//primitive, successor
};

#include "sync/task.h"
#include "sync/gate.h"
#include "sync/barrier.h"

union sol_sync_primitive
{
    const struct sol_sync_primitive_functions* sync_functions;
    struct sol_task task;
    struct sol_gate gate;
    struct sol_barrier barrier;
};

static inline void sol_sync_primitive_impose_condition(union sol_sync_primitive* primitive)
{
    primitive->sync_functions->impose_condition(primitive);
}
static inline void sol_sync_primitive_signal_condition(union sol_sync_primitive* primitive)
{
    primitive->sync_functions->signal_condition(primitive);
}
static inline void sol_sync_primitive_retain_reference(union sol_sync_primitive* primitive)
{
    primitive->sync_functions->retain_reference(primitive);
}
static inline void sol_sync_primitive_release_reference(union sol_sync_primitive* primitive)
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
static inline void sol_sync_primitive_attach_successor(union sol_sync_primitive* primitive, union sol_sync_primitive* successor)
{
    successor->sync_functions->impose_condition(successor);
    // because sucessor may satisfy the dependency/condition immediately, successor must have a condition set first
    primitive->sync_functions->attach_successor(primitive, successor);
}


// these sync structs are built on top of other primitives
#include "sync/queue.h"


/**
Copyright 2025 Carl van Mastrigt

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

struct sol_sync_primitive;

struct sol_sync_primitive_functions
{
    void(*const impose_condition )(struct sol_sync_primitive*);
    void(*const signal_condition )(struct sol_sync_primitive*);

    void(*const retain_reference )(struct sol_sync_primitive*);
    void(*const release_reference)(struct sol_sync_primitive*);

    // because sucessor may satisfy the dependency/condition immediately, successor must have a condition set first
    void(*const attach_successor )(struct sol_sync_primitive*, struct sol_sync_primitive*);//primitive, successor
};

/// struct sol_sync_primitive MUST be the first element of any synchronization primitive
struct sol_sync_primitive
{
    const struct sol_sync_primitive_functions* sync_functions;
};

static inline void sol_sync_primitive_impose_condition(struct sol_sync_primitive* primitive)
{
    primitive->sync_functions->impose_condition(primitive);
}
static inline void sol_sync_primitive_signal_condition(struct sol_sync_primitive* primitive)
{
    primitive->sync_functions->signal_condition(primitive);
}
static inline void sol_sync_primitive_retain_reference(struct sol_sync_primitive* primitive)
{
    primitive->sync_functions->retain_reference(primitive);
}
static inline void sol_sync_primitive_release_reference(struct sol_sync_primitive* primitive)
{
    primitive->sync_functions->release_reference(primitive);
}

/**
 * establishes an ordering dependency such that: a happens before b
 *
 * a must have a reference retained or an unsignalled condition
 * b must have an unsignalled condition or not yet have been activated
 *
 * (for tasks not yet having been enqueued on counts as an unsignalled condition)
 * (for gates not yet having been waited on counts as an unsignalled condition)
 */
static inline void sol_sync_primitive_attach_successor(struct sol_sync_primitive* primitive, struct sol_sync_primitive* successor)
{
    primitive->sync_functions->attach_successor(primitive, successor);
}

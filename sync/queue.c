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

#include <assert.h>

#include "sol_sync.h"
#include "sync/queue.h"

void sol_sync_queue_initialise(struct sol_sync_queue* queue)
{
    atomic_init(&queue->end_primitive, NULL);
}

void sol_sync_queue_terminate(struct sol_sync_queue* queue)
{
    union sol_sync_primitive* end_primitive;

    end_primitive = atomic_exchange_explicit(&queue->end_primitive, NULL, memory_order_relaxed);

    if(end_primitive)
    {
        sol_sync_primitive_release_reference(end_primitive);
    }
}

void sol_sync_queue_enqueue_primitive_range(struct sol_sync_queue* queue, union sol_sync_primitive* first_primitive, union sol_sync_primitive* last_primitive)
{
    union sol_sync_primitive* old_end_primitive;

    sol_sync_primitive_retain_reference(last_primitive);/// move this to dedicated (static inline) function?

    old_end_primitive = atomic_exchange_explicit(&queue->end_primitive, last_primitive, memory_order_relaxed);

    if(old_end_primitive)
    {
        sol_sync_primitive_attach_successor(old_end_primitive, first_primitive);
        sol_sync_primitive_release_reference(old_end_primitive);
    }
}

void sol_sync_queue_enqueue_primitive(struct sol_sync_queue* queue, union sol_sync_primitive* primitive)
{
    sol_sync_queue_enqueue_primitive_range(queue, primitive, primitive);
}
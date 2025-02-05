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

#include <inttypes.h>
#include <stdatomic.h>
#include <stdbool.h>

#include "coherent_structures/lockfree_pool.h"

/** lockfree_hopper
 * a hopper is a simplified stack that supports atomic addition, but then must be locked before its entries are accessed
 * locking prevents addition (attempting to add will return false) and must be reset before it can be used again
 * locking also gets the first entry, subsequent entries must then be acquired using one of the iterate function
 * entries should be relinquished back to the hoppers backing pool after the next entry has been acquired
 * iterate will return null when the hopper has been exhausted, the range iterated should then be relinquished*/

/// as this structure is intended to be as lightweight as possible the backing pool must be provided whenever its required

struct sol_lockfree_hopper
{
    // this could probably be an atomic pointer
    atomic_uint_fast64_t head;
};

/// a hopper uses a pool for backing
void sol_lockfree_hopper_initialise(struct sol_lockfree_hopper* hopper, struct sol_lockfree_pool* pool);
void sol_lockfree_hopper_terminate(struct sol_lockfree_hopper* hopper);

void sol_lockfree_hopper_reset(struct sol_lockfree_hopper* hopper);

bool sol_lockfree_hopper_push(struct sol_lockfree_hopper* hopper, struct sol_lockfree_pool* pool, void* entry);/// returns true if it was added, false if not (fails if hopper is locked)
bool sol_lockfree_hopper_check_if_locked(struct sol_lockfree_hopper* hopper);


/** hopper locking
 * usage pattern is:  lock -> iterate -> relinquish_range
 *
 * MUST call `sol_lockfree_pool_relinquish_entry_index_range` after locking and processing & iterating hopper contents with:
 * `first_entry_index` with the value of `first_entry_index` from the initial `sol_lockfree_hopper_lock` call
 * `last_enrty_index` with the value of `entry_index` from failing (NULL return) `sol_lockfree_hopper_iterate` call
*/

// locking also gets the first elemnt, and begins the process of emptying the hopper (which must be done)
void* sol_lockfree_hopper_lock(struct sol_lockfree_hopper* hopper, struct sol_lockfree_pool* pool, uint32_t* first_entry_index);

//returns NULL when there are no further entries, WITHOUT altering entry_index (will only change entry index if the new one is valid)
void* sol_lockfree_hopper_iterate(struct sol_lockfree_pool* pool, uint32_t* entry_index);

void sol_lockfree_hopper_relinquish_range(struct sol_lockfree_pool* pool, uint32_t first_entry_index, uint32_t last_entry_index);

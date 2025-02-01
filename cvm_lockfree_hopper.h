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

#ifndef CVM_LOCKFREE_HOPPER_H
#define CVM_LOCKFREE_HOPPER_H

/// a hopper is a simplified stack that supports atomic addition, but then must be locked before its entries are accessed
/// locking prevents addition (attempting to add will return false) and must be reset before it can be used again
/// locking also gets the first entry, subsequent entries must then be acquired using one of the get_next functions
/// if the non-relinquishing one is called then entries muct be manually relinquished to the backing pool
/// entries should be relinquished back to the hoppers backing pool after the next entry has been acquired
/// any of the get functions will return null when the hopper has been exhausted

/// as this structure is intended to be as lightweight as possible the backing pool must be provided whenever its required

typedef struct cvm_lockfree_hopper
{
    // does alignas here actually improve performance?
    atomic_uint_fast64_t head;
}
cvm_lockfree_hopper;

/// a hopper uses a pool for backing
void cvm_lockfree_hopper_initialise(cvm_lockfree_hopper * hopper, cvm_lockfree_pool * pool);
void cvm_lockfree_hopper_terminate(cvm_lockfree_hopper * hopper);

void cvm_lockfree_hopper_reset(cvm_lockfree_hopper * hopper);

bool cvm_lockfree_hopper_push(cvm_lockfree_hopper * hopper, cvm_lockfree_pool * pool, void * entry);/// returns true if it was added, false if not (fails if hopper is locked)
bool cvm_lockfree_hopper_check_if_locked(cvm_lockfree_hopper * hopper);
void * cvm_lockfree_hopper_lock_and_get_first(cvm_lockfree_hopper * hopper, cvm_lockfree_pool * pool);

/// hopper not actually required for these functions, only for the initial lock and get operation
/// these should also ONLY be used passing a `previous_entry` returned by that function or these
void* cvm_lockfree_hopper_relinquish_and_get_next(cvm_lockfree_pool * pool, void * previous_entry);

void* cvm_lockfree_hopper_get_next(cvm_lockfree_pool * pool, void * previous_entry);
/// MUST be called with value returned by `cvm_lockfree_hopper_lock_and_get_first` and CANNOT have called `cvm_lockfree_hopper_relinquish_and_get_next` while iterating entries
void cvm_lockfree_hopper_relinquish_all(cvm_lockfree_pool * pool, void * first_entry, void * last_entry);

#endif


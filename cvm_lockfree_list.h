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

/// this file should be included as needed

#ifndef CVM_LOCKFREE_POOL_ENTRY_TYPE
#error must define CVM_LOCKFREE_POOL_ENTRY_TYPE to include cvm_lockfree_list.h
#endif

#ifndef CVM_LOCKFREE_POOL_TYPE
#error must define CVM_LOCKFREE_POOL_TYPE to include cvm_lockfree_list.h
#endif

#ifndef CVM_LOCKFREE_POOL_FUNCTION_PREFIX
#error must define CVM_LOCKFREE_POOL_FUNCTION_PREFIX to include cvm_lockfree_list.h
#endif

#ifndef CVM_LOCKFREE_LIST_TYPE
#error must define CVM_LOCKFREE_LIST_TYPE to include cvm_lockfree_list.h
#endif

#ifndef CVM_LOCKFREE_LIST_FUNCTION_PREFIX
#error must define CVM_LOCKFREE_LIST_FUNCTION_PREFIX to include cvm_lockfree_list.h
#endif

typedef struct CVM_LOCKFREE_LIST_TYPE
{
    atomic_uint_fast64_t header;/// as long as this union is init using designated initializer then this can be larger than 64 safely
    void * pool;
}
CVM_LOCKFREE_LIST_TYPE;

void CVM_CAT2(CVM_LOCKFREE_LIST_FUNCTION_PREFIX,_init)(CVM_LOCKFREE_POOL_TYPE * pool, CVM_LOCKFREE_LIST_TYPE * list);

/// returns if there are any stalls (should ignore if stall exponent 0/undefined)
void CVM_CAT2(CVM_LOCKFREE_LIST_FUNCTION_PREFIX,_add)(CVM_LOCKFREE_LIST_TYPE * list,CVM_LOCKFREE_POOL_ENTRY_TYPE * entry);

/// will return NULL if list is empty
CVM_LOCKFREE_POOL_ENTRY_TYPE * CVM_CAT2(CVM_LOCKFREE_LIST_FUNCTION_PREFIX,_get)(CVM_LOCKFREE_LIST_TYPE * list);


/// following irregular "stall" related actions are only implememted if CVM_LOCKFREE_POOL_STALL_COUNT_EXPONENT is defined with a nonzero value
/// they allow a thread that would be getting values from the list to indicate that it's stopped because there was nothing left when it attemped to get a value
/// adding a value can detect that there are threads that stalled/stopped that may want to have some action performed (e.g. wakeup or transition current thread to processing the get-op instead of add-op)

/// will add and decrement stall counter if nonzero, returns true if stall counter was decremented
bool CVM_CAT2(CVM_LOCKFREE_LIST_FUNCTION_PREFIX,_add_and_try_unstall)(CVM_LOCKFREE_LIST_TYPE * list,CVM_LOCKFREE_POOL_ENTRY_TYPE * entry);

/// if nonzero will decrement stall counter, otherwise add entry to the list, returns true if stall counter was decremented with entry in free floating state
bool CVM_CAT2(CVM_LOCKFREE_LIST_FUNCTION_PREFIX,_add_or_unstall)(CVM_LOCKFREE_LIST_TYPE * list,CVM_LOCKFREE_POOL_ENTRY_TYPE * entry);

/// will get an entry or stall one and return NULL if it can't
CVM_LOCKFREE_POOL_ENTRY_TYPE * CVM_CAT2(CVM_LOCKFREE_LIST_FUNCTION_PREFIX,_get_or_stall)(CVM_LOCKFREE_LIST_TYPE * list);


struct CVM_LOCKFREE_POOL_TYPE
{
    CVM_LOCKFREE_LIST_TYPE available_entries;

    CVM_LOCKFREE_POOL_ENTRY_TYPE ** entries;
    uint32_t batches_init;
    mtx_t batches_mutex;
};

void CVM_CAT2(CVM_LOCKFREE_POOL_FUNCTION_PREFIX,_init)(CVM_LOCKFREE_POOL_TYPE * pool);
/// all entries must be relinquished before this is called, and acquire must not be called on this
void CVM_CAT2(CVM_LOCKFREE_POOL_FUNCTION_PREFIX,_destroy)(CVM_LOCKFREE_POOL_TYPE * pool);

CVM_LOCKFREE_POOL_ENTRY_TYPE * CVM_CAT2(CVM_LOCKFREE_POOL_FUNCTION_PREFIX,_acquire_entry)(CVM_LOCKFREE_POOL_TYPE * pool);
void CVM_CAT2(CVM_LOCKFREE_POOL_FUNCTION_PREFIX,_relinquish_entry)(CVM_LOCKFREE_POOL_TYPE * pool,CVM_LOCKFREE_POOL_ENTRY_TYPE * entry);

#undef CVM_LOCKFREE_POOL_ENTRY_TYPE

#undef CVM_LOCKFREE_POOL_TYPE
#undef CVM_LOCKFREE_POOL_FUNCTION_PREFIX

#undef CVM_LOCKFREE_LIST_TYPE
#undef CVM_LOCKFREE_LIST_FUNCTION_PREFIX




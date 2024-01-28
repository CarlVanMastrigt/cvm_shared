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

//#include "cvm_shared.h"

#ifndef CVM_LOCKFREE_POOL_ENTRY_TYPE
#error must define CVM_LOCKFREE_POOL_ENTRY_TYPE to include cvm_lockfree_list.c
#endif


#ifndef CVM_LOCKFREE_POOL_TYPE
#error must define CVM_LOCKFREE_POOL_TYPE to include cvm_lockfree_list.c
#endif

#ifndef CVM_LOCKFREE_POOL_FUNCTION_PREFIX
#error must define CVM_LOCKFREE_POOL_FUNCTION_PREFIX to include cvm_lockfree_list.c
#endif

#ifndef CVM_LOCKFREE_LIST_TYPE
#error must define CVM_LOCKFREE_LIST_TYPE to include cvm_lockfree_list.h
#endif

#ifndef CVM_LOCKFREE_LIST_FUNCTION_PREFIX
#error must define CVM_LOCKFREE_LIST_FUNCTION_PREFIX to include cvm_lockfree_list.c
#endif

#ifndef CVM_LOCKFREE_POOL_BATCH_COUNT_EXPONENT
#error must define CVM_LOCKFREE_POOL_BATCH_COUNT_EXPONENT to include cvm_lockfree_list.c
#endif

#ifndef CVM_LOCKFREE_POOL_BATCH_SIZE_EXPONENT
#error must define CVM_LOCKFREE_POOL_BATCH_SIZE_EXPONENT to include cvm_lockfree_list.c
#endif

#ifndef CVM_LOCKFREE_POOL_STALL_COUNT_EXPONENT
#define CVM_LOCKFREE_POOL_STALL_COUNT_EXPONENT 0
#endif

#if (CVM_LOCKFREE_POOL_BATCH_COUNT_EXPONENT+CVM_LOCKFREE_POOL_BATCH_SIZE_EXPONENT)>32
#error must define pool index and batch id must fit in a u32 together
#endif

#if (CVM_LOCKFREE_POOL_BATCH_COUNT_EXPONENT+CVM_LOCKFREE_POOL_BATCH_SIZE_EXPONENT+CVM_LOCKFREE_POOL_STALL_COUNT_EXPONENT)>32
#warning this many worker threads combined with max total lockfree elements is not recommended as it will degrade the safety of the CAS lockfree mechanism
#endif



#define CVM_LOCKFREE_LIST_LINK_NULL_BATCH ((((uint32_t)1)<<CVM_LOCKFREE_POOL_BATCH_COUNT_EXPONENT)-1)
#define CVM_LOCKFREE_LIST_LINK_LAST_INDEX ((((uint32_t)1)<<CVM_LOCKFREE_POOL_BATCH_SIZE_EXPONENT)-1)

#define CVM_LOCKFREE_LIST_LINK_BATCH_MASK CVM_LOCKFREE_LIST_LINK_NULL_BATCH
#define CVM_LOCKFREE_LIST_LINK_INDEX_MASK CVM_LOCKFREE_LIST_LINK_LAST_INDEX

#define CVM_LOCKFREE_LIST_HEADER_STALL_MASK ( ((((uint64_t)1)<<CVM_LOCKFREE_POOL_STALL_COUNT_EXPONENT)-1) << (CVM_LOCKFREE_POOL_BATCH_COUNT_EXPONENT+CVM_LOCKFREE_POOL_BATCH_SIZE_EXPONENT))
#define CVM_LOCKFREE_LIST_HEADER_STALL_COUNT_NON_MAX(value) (((value) & CVM_LOCKFREE_LIST_HEADER_STALL_MASK) != CVM_LOCKFREE_LIST_HEADER_STALL_MASK)
#define CVM_LOCKFREE_LIST_HEADER_STALL_COUNT_NONZERO(value) (((value) & CVM_LOCKFREE_LIST_HEADER_STALL_MASK) != 0)

/// these can also be used to get the batch/index from a header value
#define CVM_LOCKFREE_LIST_LINK_GET_BATCH(v) (((v)>>CVM_LOCKFREE_POOL_BATCH_SIZE_EXPONENT) & CVM_LOCKFREE_LIST_LINK_BATCH_MASK)
#define CVM_LOCKFREE_LIST_LINK_GET_INDEX(v) ((v) & CVM_LOCKFREE_LIST_LINK_INDEX_MASK)

#define CVM_LOCKFREE_LIST_LINK_SET(batch,index) (((index) & CVM_LOCKFREE_LIST_LINK_INDEX_MASK) | ( ((batch) & CVM_LOCKFREE_LIST_LINK_BATCH_MASK) <<CVM_LOCKFREE_POOL_BATCH_SIZE_EXPONENT))

#define CVM_LOCKFREE_LIST_HEADER_LINK_MASK ((((uint32_t)1)<<(CVM_LOCKFREE_POOL_BATCH_COUNT_EXPONENT+CVM_LOCKFREE_POOL_BATCH_SIZE_EXPONENT))-1)
#define CVM_LOCKFREE_LIST_HEADER_GET_LINK(v) ((v) & CVM_LOCKFREE_LIST_HEADER_LINK_MASK)

#define CVM_LOCKFREE_LIST_HEADER_IDENTIFIER_VALUE (((uint64_t)1)<<(CVM_LOCKFREE_POOL_BATCH_COUNT_EXPONENT+CVM_LOCKFREE_POOL_BATCH_SIZE_EXPONENT+CVM_LOCKFREE_POOL_STALL_COUNT_EXPONENT))
#define CVM_LOCKFREE_LIST_HEADER_STALL_VALUE (((uint64_t)1)<<(CVM_LOCKFREE_POOL_BATCH_COUNT_EXPONENT+CVM_LOCKFREE_POOL_BATCH_SIZE_EXPONENT))

#define CVM_LOCKFREE_LIST_HEADER_IDENTIFIER_STALL_MASK (~((uint64_t)CVM_LOCKFREE_LIST_HEADER_LINK_MASK))

#define CVM_LOCKFREE_LIST_HEADER_SET_REPLACEMENT_VALUE_NEW_LINK(old_value,link) \
( (((old_value)+CVM_LOCKFREE_LIST_HEADER_IDENTIFIER_VALUE) & CVM_LOCKFREE_LIST_HEADER_IDENTIFIER_STALL_MASK)  |  ((link) & CVM_LOCKFREE_LIST_HEADER_LINK_MASK) )

#define CVM_LOCKFREE_LIST_HEADER_SET_REPLACEMENT_VALUE_NEW_LINK_AND_UNSTALL(old_value,link) \
( (((old_value)+CVM_LOCKFREE_LIST_HEADER_IDENTIFIER_VALUE-CVM_LOCKFREE_LIST_HEADER_STALL_VALUE) & CVM_LOCKFREE_LIST_HEADER_IDENTIFIER_STALL_MASK)  |  ((link) & CVM_LOCKFREE_LIST_HEADER_LINK_MASK) )

#define CVM_LOCKFREE_LIST_HEADER_SET_REPLACEMENT_VALUE_STALL(old_value) ((old_value)+CVM_LOCKFREE_LIST_HEADER_IDENTIFIER_VALUE+CVM_LOCKFREE_LIST_HEADER_STALL_VALUE)
#define CVM_LOCKFREE_LIST_HEADER_SET_REPLACEMENT_VALUE_UNSTALL(old_value) ((old_value)+CVM_LOCKFREE_LIST_HEADER_IDENTIFIER_VALUE-CVM_LOCKFREE_LIST_HEADER_STALL_VALUE)





void CVM_CAT2(CVM_LOCKFREE_LIST_FUNCTION_PREFIX,_static_assert_validation)(CVM_LOCKFREE_POOL_ENTRY_TYPE entry)
{
    static_assert(_Generic(entry.next, uint32_t: true, default: false));/// VM_LOCKFREE_POOL_ENTRY_TYPE must contain a variable called next with type uint32_t
    static_assert(_Generic(entry.pool, CVM_LOCKFREE_POOL_TYPE*: true, default: false));/// VM_LOCKFREE_POOL_ENTRY_TYPE must contain a variable called pool with type CVM_LOCKFREE_POOL_TYPE*
    printf("%lx\n",CVM_LOCKFREE_LIST_HEADER_IDENTIFIER_STALL_MASK);
}






void CVM_CAT2(CVM_LOCKFREE_LIST_FUNCTION_PREFIX,_init)(CVM_LOCKFREE_POOL_TYPE * pool, CVM_LOCKFREE_LIST_TYPE * list)
{
    atomic_init(&list->header,CVM_LOCKFREE_LIST_LINK_NULL_BATCH<<CVM_LOCKFREE_POOL_BATCH_SIZE_EXPONENT);
    list->pool=pool;
}

void CVM_CAT2(CVM_LOCKFREE_LIST_FUNCTION_PREFIX,_add)(CVM_LOCKFREE_LIST_TYPE * list,CVM_LOCKFREE_POOL_ENTRY_TYPE * entry)
{
    uint_fast64_t current_header_value,replacement_header_value;
    uint32_t link_to_entry;

    assert(list->pool==entry->pool);/// entry and list must use the same pool
    assert(entry==entry->pool->entries[CVM_LOCKFREE_LIST_LINK_GET_BATCH(entry->next)]+CVM_LOCKFREE_LIST_LINK_GET_INDEX(entry->next));/// entry must link to itself when its free floating, and must be free floating to be added to a list

    /// if we're here entry must have been free floating and thus next should link to itself
    link_to_entry=entry->next;

    current_header_value=atomic_load_explicit(&list->header, memory_order_relaxed);
    do
    {
        entry->next=CVM_LOCKFREE_LIST_HEADER_GET_LINK(current_header_value);
        replacement_header_value=CVM_LOCKFREE_LIST_HEADER_SET_REPLACEMENT_VALUE_NEW_LINK(current_header_value,link_to_entry);
    }
    while(!atomic_compare_exchange_weak_explicit(&list->header, &current_header_value, replacement_header_value, memory_order_release, memory_order_relaxed));
}

CVM_LOCKFREE_POOL_ENTRY_TYPE * CVM_CAT2(CVM_LOCKFREE_LIST_FUNCTION_PREFIX,_get)(CVM_LOCKFREE_LIST_TYPE * list)
{
    CVM_LOCKFREE_POOL_TYPE * pool;
    CVM_LOCKFREE_POOL_ENTRY_TYPE * entry;
    uint_fast64_t current_header_value,replacement_header_value;

    pool=list->pool;

    current_header_value=atomic_load_explicit(&list->header, memory_order_acquire);
    do
    {
        /// if there are no more entries in this list then return NULL
        if(CVM_LOCKFREE_LIST_LINK_GET_BATCH(current_header_value)==CVM_LOCKFREE_LIST_LINK_NULL_BATCH)
        {
            return NULL;
        }

        entry=pool->entries[CVM_LOCKFREE_LIST_LINK_GET_BATCH(current_header_value)]+CVM_LOCKFREE_LIST_LINK_GET_INDEX(current_header_value);

        replacement_header_value=CVM_LOCKFREE_LIST_HEADER_SET_REPLACEMENT_VALUE_NEW_LINK(current_header_value,entry->next);
    }
    while(!atomic_compare_exchange_weak_explicit(&list->header, &current_header_value, replacement_header_value, memory_order_acquire, memory_order_acquire));
    /// success memory order could (conceptually) be relaxed here as we don't need to release/acquire any changes when that happens as nothing outside the atomic has changed
    /// but fail must acquire as the "next" member of "first" element may have changed

    /// entry is about to become "free floating" so next should reference itself
    entry->next=CVM_LOCKFREE_LIST_HEADER_GET_LINK(current_header_value);
    assert(entry->pool==pool);/// pools somehow got out of alignment

    return entry;
}

#if CVM_LOCKFREE_POOL_STALL_COUNT_EXPONENT > 0
bool CVM_CAT2(CVM_LOCKFREE_LIST_FUNCTION_PREFIX,_add_and_try_unstall)(CVM_LOCKFREE_LIST_TYPE * list,CVM_LOCKFREE_POOL_ENTRY_TYPE * entry)
{
    uint_fast64_t current_header_value,replacement_header_value;
    uint32_t link_to_entry;
    bool unstall;

    assert(list->pool==entry->pool);/// entry and list must use the same pool
    assert(entry==entry->pool->entries[CVM_LOCKFREE_LIST_LINK_GET_BATCH(entry->next)]+CVM_LOCKFREE_LIST_LINK_GET_INDEX(entry->next));/// entry must link to itself when its free floating, and must be free floating to be added to a list

    /// if we're here entry must have been free floating and thus next should link to itself
    link_to_entry=entry->next;

    current_header_value=atomic_load_explicit(&list->header, memory_order_relaxed);
    do
    {
        entry->next=CVM_LOCKFREE_LIST_HEADER_GET_LINK(current_header_value);
        unstall=CVM_LOCKFREE_LIST_HEADER_STALL_COUNT_NONZERO(current_header_value);
        if(unstall)
        {
            replacement_header_value=CVM_LOCKFREE_LIST_HEADER_SET_REPLACEMENT_VALUE_NEW_LINK_AND_UNSTALL(current_header_value,link_to_entry);
        }
        else
        {
            replacement_header_value=CVM_LOCKFREE_LIST_HEADER_SET_REPLACEMENT_VALUE_NEW_LINK(current_header_value,link_to_entry);
        }
    }
    while(!atomic_compare_exchange_weak_explicit(&list->header, &current_header_value, replacement_header_value, memory_order_acq_rel, memory_order_relaxed));
    /// (not sure the acquire is actually necessary upon success to ensure future mutex lock is ordered after this, i.e. in sync)

    return unstall;
}

bool CVM_CAT2(CVM_LOCKFREE_LIST_FUNCTION_PREFIX,_add_or_unstall)(CVM_LOCKFREE_LIST_TYPE * list,CVM_LOCKFREE_POOL_ENTRY_TYPE * entry)
{
    uint_fast64_t current_header_value,replacement_header_value;
    uint32_t link_to_entry;
    bool unstall;

    assert(list->pool==entry->pool);/// entry and list must use the same pool
    assert(entry==entry->pool->entries[CVM_LOCKFREE_LIST_LINK_GET_BATCH(entry->next)]+CVM_LOCKFREE_LIST_LINK_GET_INDEX(entry->next));/// entry must link to itself when its free floating, and must be free floating to be added to a list

    /// if we're here entry must have been free floating and thus next should link to itself
    link_to_entry=entry->next;

    current_header_value=atomic_load_explicit(&list->header, memory_order_relaxed);
    do
    {
        unstall=CVM_LOCKFREE_LIST_HEADER_STALL_COUNT_NONZERO(current_header_value);
        if(unstall)
        {
            entry->next=link_to_entry;
            replacement_header_value=CVM_LOCKFREE_LIST_HEADER_SET_REPLACEMENT_VALUE_UNSTALL(current_header_value);
        }
        else
        {
            entry->next=CVM_LOCKFREE_LIST_HEADER_GET_LINK(current_header_value);
            replacement_header_value=CVM_LOCKFREE_LIST_HEADER_SET_REPLACEMENT_VALUE_NEW_LINK(current_header_value,link_to_entry);
        }
    }
    while(!atomic_compare_exchange_weak_explicit(&list->header, &current_header_value, replacement_header_value, memory_order_acq_rel, memory_order_relaxed));
    /// (not sure the acquire is actually necessary upon success to ensure future mutex lock is ordered after this, i.e. in sync)

    return unstall;
}

CVM_LOCKFREE_POOL_ENTRY_TYPE * CVM_CAT2(CVM_LOCKFREE_LIST_FUNCTION_PREFIX,_get_or_stall)(CVM_LOCKFREE_LIST_TYPE * list)
{
    CVM_LOCKFREE_POOL_TYPE * pool;
    CVM_LOCKFREE_POOL_ENTRY_TYPE * entry;
    uint_fast64_t current_header_value,replacement_header_value;

    pool=list->pool;

    current_header_value=atomic_load_explicit(&list->header, memory_order_acquire);
    do
    {
        /// if there are no more entries in this list then return NULL
        if(CVM_LOCKFREE_LIST_LINK_GET_BATCH(current_header_value)==CVM_LOCKFREE_LIST_LINK_NULL_BATCH)
        {
            entry=NULL;
            assert(CVM_LOCKFREE_LIST_HEADER_STALL_COUNT_NON_MAX(current_header_value));
            replacement_header_value=CVM_LOCKFREE_LIST_HEADER_SET_REPLACEMENT_VALUE_STALL(current_header_value);
        }
        else
        {
            entry=pool->entries[CVM_LOCKFREE_LIST_LINK_GET_BATCH(current_header_value)]+CVM_LOCKFREE_LIST_LINK_GET_INDEX(current_header_value);
            replacement_header_value=CVM_LOCKFREE_LIST_HEADER_SET_REPLACEMENT_VALUE_NEW_LINK(current_header_value,entry->next);
        }
    }
    while(!atomic_compare_exchange_weak_explicit(&list->header, &current_header_value, replacement_header_value, memory_order_acq_rel, memory_order_acquire));
    /// (not sure the release is actually necessary upon success when stalling to ensure prior mutex lock is ordered before this, i.e. in sync)
    /// on success when NOT stalling memory order could (conceptually) be relaxed as the atomic itself is the mechanism that communicates all relevant changes and all modifications are already acquired that this point

    if(entry)
    {
        /// entry is about to become "free floating" so next should reference itself
        entry->next=CVM_LOCKFREE_LIST_HEADER_GET_LINK(current_header_value);
        assert(entry->pool==pool);/// entry somehow ended up in incorrect pool
    }

    return entry;
}
#endif



void CVM_CAT2(CVM_LOCKFREE_POOL_FUNCTION_PREFIX,_init)(CVM_LOCKFREE_POOL_TYPE * pool)
{
    CVM_CAT2(CVM_LOCKFREE_LIST_FUNCTION_PREFIX,_init)(pool,&pool->available_entries);

    pool->entries=malloc(sizeof(CVM_LOCKFREE_POOL_ENTRY_TYPE*)<<CVM_LOCKFREE_POOL_BATCH_COUNT_EXPONENT);
    pool->batches_init=0;
    mtx_init(&pool->batches_mutex,mtx_plain);
}

void CVM_CAT2(CVM_LOCKFREE_POOL_FUNCTION_PREFIX,_destroy)(CVM_LOCKFREE_POOL_TYPE * pool)
{
    CVM_LOCKFREE_POOL_ENTRY_TYPE * entry;
    uint32_t count,i;

    i=CVM_LOCKFREE_LIST_HEADER_GET_LINK(atomic_load(&pool->available_entries.header));
    count=0;

    while(CVM_LOCKFREE_LIST_LINK_GET_BATCH(i)!=CVM_LOCKFREE_LIST_LINK_NULL_BATCH)
    {
        entry=pool->entries[CVM_LOCKFREE_LIST_LINK_GET_BATCH(i)]+CVM_LOCKFREE_LIST_LINK_GET_INDEX(i);
        assert(entry->pool==pool);///entries must be a part of their own pool

        i=entry->next;
        #ifdef CVM_LOCKFREE_POOL_ENTRY_DESTROY_FUNCTION
        CVM_LOCKFREE_POOL_ENTRY_DESTROY_FUNCTION(entry);
        #endif

        count++;
    }

    assert(count==(pool->batches_init<<CVM_LOCKFREE_POOL_BATCH_SIZE_EXPONENT));

    for(i=0;i<pool->batches_init;i++)
    {
        free(pool->entries[i]);
    }
    free(pool->entries);

    mtx_destroy(&pool->batches_mutex);
}

CVM_LOCKFREE_POOL_ENTRY_TYPE * CVM_CAT2(CVM_LOCKFREE_POOL_FUNCTION_PREFIX,_acquire_entry)(CVM_LOCKFREE_POOL_TYPE * pool)
{
    CVM_LOCKFREE_POOL_ENTRY_TYPE * entry;
    uint_fast64_t current_header_value,replacement_header_value;
    uint32_t batch,index;

    current_header_value=atomic_load_explicit(&pool->available_entries.header, memory_order_acquire);

    do
    {
        /// if there are no more available entries we need to allocate some
        if(CVM_LOCKFREE_LIST_LINK_GET_BATCH(current_header_value)==CVM_LOCKFREE_LIST_LINK_NULL_BATCH)
        {
            assert(CVM_LOCKFREE_LIST_LINK_GET_INDEX(current_header_value)==0);/// if the batch is null, the index should always be 0

            mtx_lock(&pool->batches_mutex);/// need to double check this inside a mutex to ensure that only one extra allocation actually happens when necessary

            current_header_value=atomic_load_explicit(&pool->available_entries.header, memory_order_acquire);

            /// check list of available entries is still empty once we have exclusive access
            if(CVM_LOCKFREE_LIST_LINK_GET_BATCH(current_header_value)==CVM_LOCKFREE_LIST_LINK_NULL_BATCH)
            {
                assert(CVM_LOCKFREE_LIST_LINK_GET_INDEX(current_header_value)==0);/// if the batch is null, the index should always be 0

                batch=pool->batches_init++;
                assert(batch!=CVM_LOCKFREE_LIST_LINK_NULL_BATCH);/// HAVE EXHAUSTED ALL ENTRY SPACE

                ///entry technically used to reference an array here...
                entry=pool->entries[batch]=malloc(sizeof(CVM_LOCKFREE_POOL_ENTRY_TYPE)<<CVM_LOCKFREE_POOL_BATCH_SIZE_EXPONENT);

                for(index=0;index<=CVM_LOCKFREE_LIST_LINK_LAST_INDEX;index++)
                {
                    #ifdef CVM_LOCKFREE_POOL_ENTRY_INIT_FUNCTION
                    CVM_LOCKFREE_POOL_ENTRY_INIT_FUNCTION(entry+index);
                    #endif
                    entry[index].pool=pool;
                }

                for(index=1;index<CVM_LOCKFREE_LIST_LINK_LAST_INDEX;index++)
                {
                    ///nothing else needs to be init here!
                    entry[index].next=CVM_LOCKFREE_LIST_LINK_SET(batch,index+1);
                }

                do
                {
                    /// link the end of the list into the extant part of the available list (likely to be empty but not guaranteed)
                    entry[CVM_LOCKFREE_LIST_LINK_LAST_INDEX].next=CVM_LOCKFREE_LIST_HEADER_GET_LINK(current_header_value);
                    ///replace the header with the start of (the unused part of) this list (0th entry will be returned by this function call)
                    replacement_header_value=CVM_LOCKFREE_LIST_HEADER_SET_REPLACEMENT_VALUE_NEW_LINK(current_header_value,CVM_LOCKFREE_LIST_LINK_SET(batch,1));
                }
                while(!atomic_compare_exchange_weak_explicit(&pool->available_entries.header, &current_header_value, replacement_header_value, memory_order_release, memory_order_relaxed));

                mtx_unlock(&pool->batches_mutex);

                entry->next=CVM_LOCKFREE_LIST_LINK_SET(batch,0);

                return entry;
            }

            mtx_unlock(&pool->batches_mutex);
        }

        entry=pool->entries[CVM_LOCKFREE_LIST_LINK_GET_BATCH(current_header_value)]+CVM_LOCKFREE_LIST_LINK_GET_INDEX(current_header_value);

        replacement_header_value=CVM_LOCKFREE_LIST_HEADER_SET_REPLACEMENT_VALUE_NEW_LINK(current_header_value,entry->next);
    }
    while(!atomic_compare_exchange_weak_explicit(&pool->available_entries.header, &current_header_value, replacement_header_value, memory_order_release, memory_order_acquire));

    /// entry is about to become "free floating" so next should reference itself
    entry->next=CVM_LOCKFREE_LIST_HEADER_GET_LINK(current_header_value);

    return entry;
}

void CVM_CAT2(CVM_LOCKFREE_POOL_FUNCTION_PREFIX,_relinquish_entry)(CVM_LOCKFREE_POOL_TYPE * pool,CVM_LOCKFREE_POOL_ENTRY_TYPE * entry)
{
    assert(entry->pool==pool);///entries must only be relinquished to the pool they were acquired from
    assert(entry==pool->entries[CVM_LOCKFREE_LIST_LINK_GET_BATCH(entry->next)]+CVM_LOCKFREE_LIST_LINK_GET_INDEX(entry->next));
    CVM_CAT2(CVM_LOCKFREE_LIST_FUNCTION_PREFIX,_add)(&pool->available_entries,entry);
}

#undef CVM_LOCKFREE_LIST_LINK_NULL_BATCH
#undef CVM_LOCKFREE_LIST_LINK_LAST_INDEX

#undef CVM_LOCKFREE_LIST_HEADER_STALL_MASK
#undef CVM_LOCKFREE_LIST_HEADER_STALL_COUNT_NON_MAX
#undef CVM_LOCKFREE_LIST_HEADER_STALL_COUNT_NONZERO

#undef CVM_LOCKFREE_LIST_LINK_GET_BATCH
#undef CVM_LOCKFREE_LIST_LINK_GET_INDEX

#undef CVM_LOCKFREE_LIST_LINK_SET

#undef CVM_LOCKFREE_LIST_HEADER_LINK_MASK
#undef CVM_LOCKFREE_LIST_HEADER_GET_LINK

#undef CVM_LOCKFREE_LIST_HEADER_IDENTIFIER_VALUE
#undef CVM_LOCKFREE_LIST_HEADER_STALL_VALUE

#undef CVM_LOCKFREE_LIST_HEADER_IDENTIFIER_STALL_MASK

#undef CVM_LOCKFREE_LIST_HEADER_SET_REPLACEMENT_VALUE_NEW_LINK

#undef CVM_LOCKFREE_LIST_HEADER_SET_REPLACEMENT_VALUE_NEW_LINK_AND_UNSTALL

#undef CVM_LOCKFREE_LIST_HEADER_SET_REPLACEMENT_VALUE_STALL
#undef CVM_LOCKFREE_LIST_HEADER_SET_REPLACEMENT_VALUE_UNSTALL


/// needs to be defined externally

#undef CVM_LOCKFREE_POOL_ENTRY_TYPE

#undef CVM_LOCKFREE_POOL_TYPE
#undef CVM_LOCKFREE_POOL_FUNCTION_PREFIX

#undef CVM_LOCKFREE_LIST_TYPE
#undef CVM_LOCKFREE_LIST_FUNCTION_PREFIX

#undef CVM_LOCKFREE_POOL_BATCH_COUNT_EXPONENT
#undef CVM_LOCKFREE_POOL_BATCH_SIZE_EXPONENT

#undef CVM_LOCKFREE_POOL_STALL_COUNT_EXPONENT

#ifdef CVM_LOCKFREE_POOL_ENTRY_INIT_FUNCTION
#undef CVM_LOCKFREE_POOL_ENTRY_INIT_FUNCTION
#endif

#ifdef CVM_LOCKFREE_POOL_ENTRY_DESTROY_FUNCTION
#undef CVM_LOCKFREE_POOL_ENTRY_DESTROY_FUNCTION
#endif

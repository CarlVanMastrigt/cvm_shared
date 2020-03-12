/**
Copyright 2020 Carl van Mastrigt

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

#ifndef CVM_LIST_H
#define CVM_LIST_H


///S   LIST
#ifndef CVM_LIST
#define CVM_LIST(type,name)                                                     \
                                                                                \
typedef struct name##_list                                                      \
{                                                                               \
    type * list;                                                                \
    uint32_t space;                                                             \
    uint32_t count;                                                             \
}                                                                               \
name##_list;                                                                    \
                                                                                \
                                                                                \
static inline void name##_list_ini( name##_list * l )                           \
{                                                                               \
    l->list=malloc( sizeof( type ) );                                           \
    l->space=1;                                                                 \
    l->count=0;                                                                 \
}                                                                               \
                                                                                \
static inline uint32_t name##_list_add( name##_list * l , type value )          \
{                                                                               \
    if(l->count==l->space)                                                      \
    {                                                                           \
        l->list=realloc(l->list,sizeof( type )*(l->space*=2));                  \
    }                                                                           \
    l->list[l->count]=value;                                                    \
    return l->count++;                                                          \
}                                                                               \
                                                                                \
static inline void name##_list_clr( name##_list * l )                           \
{                                                                               \
    l->count=0;                                                                 \
}                                                                               \
                                                                                \
static inline int name##_list_get( name##_list * l , type * value )             \
{                                                                               \
    if(l->count==0)return 0;                                                    \
    *value=l->list[--l->count];                                                 \
    return 1;                                                                   \
}                                                                               \
                                                                                \
static inline void name##_list_del( name##_list * l )                           \
{                                                                               \
    free(l->list);                                                              \
}                                                                               \
                                                                                \
                                                                                \
                                                                                \
static inline uint32_t name##_list_add_by_ptr( name##_list * l , type * value ) \
{                                                                               \
    if(l->count==l->space)                                                      \
    {                                                                           \
        l->list=realloc(l->list,sizeof( type )*(l->space*=2));                  \
    }                                                                           \
    l->list[l->count]=*value;                                                   \
    return l->count++;                                                          \
}                                                                               \
                                                                                \
static inline uint32_t name##_list_add_multiple                                 \
( name##_list * l , type * values , uint32_t count)                             \
{                                                                               \
    while((l->count+count) > l->space)                                          \
    {                                                                           \
        l->list=realloc(l->list,sizeof( type )*(l->space*=2));                  \
    }                                                                           \
    memcpy(l->list+l->count,values,sizeof( type )*count);                       \
    return (l->count+=count);                                                   \
}                                                                               \
                                                                                \
static inline type * name##_list_get_ptr( name##_list * l )                     \
{                                                                               \
    return l->list;                                                             \
}                                                                               \
                                                                                \
static inline uint32_t name##_list_get_count( name##_list * l )                 \
{                                                                               \
    return l->count;                                                            \
}                                                                               \
                                                                                \
static inline type * name##_list_ensure_space                                   \
( name##_list * l , uint32_t required_space )                                   \
{                                                                               \
    while((l->count+required_space) > l->space)                                 \
    {                                                                           \
        l->list=realloc(l->list,sizeof( type )*(l->space*=2));                  \
    }                                                                           \
    return (l->list+l->count);                                                  \
}                                                                               \



#endif
///E   LIST

CVM_LIST(uint32_t,uint)
CVM_LIST(uintptr_t,uintptr)
















#ifndef CVM_BIN_HEAP
#define CVM_BIN_HEAP(data_type,test_type,name,comparison)                       \
                                                                                \
typedef struct name##_bin_heap_entry                                            \
{                                                                               \
    data_type data;                                                             \
    test_type test;                                                             \
}                                                                               \
name##_bin_heap_entry;                                                          \
                                                                                \
typedef struct name##_bin_heap                                                  \
{                                                                               \
    name##_bin_heap_entry * heap;                                               \
    uint32_t space;                                                             \
    uint32_t count;                                                             \
}                                                                               \
name##_bin_heap;                                                                \
                                                                                \
                                                                                \
static inline void name##_bin_heap_ini( name##_bin_heap * h )                   \
{                                                                               \
    h->heap=malloc( sizeof( name##_bin_heap_entry ) );                          \
    h->space=1;                                                                 \
    h->count=0;                                                                 \
}                                                                               \
                                                                                \
static inline void name##_bin_heap_add( name##_bin_heap * h , data_type data , test_type test) \
{                                                                               \
    if(h->count==h->space)                                                      \
    {                                                                           \
        h->heap=realloc( h->heap , sizeof( name##_bin_heap_entry ) * ( h->space*=2 ) ); \
    }                                                                           \
                                                                                \
    uint32_t i1,i2;                                                             \
    i1=h->count++;                                                              \
                                                                                \
    while(i1)                                                                   \
    {                                                                           \
        i2=(i1-1)>>1;                                                           \
        if( test comparison h->heap[i2].test)                                   \
        {                                                                       \
            h->heap[i1]=h->heap[i2];                                            \
        }                                                                       \
        else break;                                                             \
        i1=i2;                                                                  \
    }                                                                           \
                                                                                \
    h->heap[i1].data=data;                                                      \
    h->heap[i1].test=test;                                                      \
}                                                                               \
                                                                                \
static inline void name##_bin_heap_clr( name##_bin_heap * h )                   \
{                                                                               \
    h->count=0;                                                                 \
}                                                                               \
                                                                                \
static inline int name##_bin_heap_get( name##_bin_heap * h , data_type * data ) \
{                                                                               \
    if(h->count==0)return 0;                                                    \
                                                                                \
    *data=h->heap[0].data;                                                      \
                                                                                \
    uint32_t current,child;                                                     \
    name##_bin_heap_entry removed=h->heap[--(h->count)];                        \
    current=0;                                                                  \
                                                                                \
    while((child=(current<<1)+1) comparison (h->count))                         \
    {                                                                           \
        child+=((child+1 < h->count)&&(h->heap[child+1].test < h->heap[child].test));\
                                                                                \
        if(removed.test comparison h->heap[child].test) break;                  \
                                                                                \
        h->heap[current]=h->heap[child];                                        \
        current=child;                                                          \
    }                                                                           \
                                                                                \
    h->heap[current]=removed;                                                   \
    return 1;                                                                   \
}                                                                               \
                                                                                \
static inline void name##_bin_heap_del( name##_bin_heap * h )                   \
{                                                                               \
    free(h->heap);                                                              \
}                                                                               \
                                                                                \



#endif



CVM_BIN_HEAP(uint32_t,uint32_t,uint_min,<)













typedef uint8_t cvm_atomic_pad[256-sizeof(atomic_uint_fast32_t)];///+sizeof(uint32_t) is 128, required memory separation on most, if not all CPU's test this for actual performance difference




typedef struct cvm_expanding_mp_mc_list
{
    size_t type_size;
    uint_fast32_t block_size;
    char * in_block;
    char * out_block;
    char * end_block;
    cvm_atomic_pad start_pad;
    atomic_uint_fast32_t spinlock;
    cvm_atomic_pad spinlock_pad;
    atomic_uint_fast32_t in;
    cvm_atomic_pad in_pad;
    atomic_uint_fast32_t out;
    cvm_atomic_pad out_pad;
    atomic_uint_fast32_t in_buffer_fence;
    cvm_atomic_pad in_buffer_fence_pad;
    atomic_uint_fast32_t out_buffer_fence;
    cvm_atomic_pad out_buffer_fence_pad;
    atomic_uint_fast32_t in_completions;
    cvm_atomic_pad in_completions_pad;
    atomic_uint_fast32_t out_completions;
    cvm_atomic_pad out_completions_pad;
}
cvm_expanding_mp_mc_list;

void cvm_expanding_mp_mc_list_ini( cvm_expanding_mp_mc_list * list , uint_fast32_t block_size , size_t type_size );///not coherent
void cvm_expanding_mp_mc_list_add( cvm_expanding_mp_mc_list * list , void * value );
bool cvm_expanding_mp_mc_list_get( cvm_expanding_mp_mc_list * list , void * value );
void cvm_expanding_mp_mc_list_del( cvm_expanding_mp_mc_list * list );///not coherent



typedef struct cvm_fixed_size_mp_mc_list
{
    size_t type_size;
    uint_fast32_t max_entry_count;
    char * data;
    cvm_atomic_pad start_pad;
    atomic_uint_fast32_t in;
    cvm_atomic_pad in_pad;
    atomic_uint_fast32_t out;
    cvm_atomic_pad out_pad;
    atomic_uint_fast32_t in_fence;
    cvm_atomic_pad in_fence_pad;
    atomic_uint_fast32_t out_fence;
    cvm_atomic_pad out_fence_pad;
}
cvm_fixed_size_mp_mc_list;

void cvm_fixed_size_mp_mc_list_ini( cvm_fixed_size_mp_mc_list * list , uint_fast32_t max_entry_count , size_t type_size );///not coherent
bool cvm_fixed_size_mp_mc_list_add( cvm_fixed_size_mp_mc_list * list , void * value );
bool cvm_fixed_size_mp_mc_list_get( cvm_fixed_size_mp_mc_list * list , void * value );
void cvm_fixed_size_mp_mc_list_del( cvm_fixed_size_mp_mc_list * list );///not coherent





typedef struct cvm_lockfree_mp_mc_stack_head cvm_lockfree_mp_mc_stack_head;

struct cvm_lockfree_mp_mc_stack_head
{
    uint_fast64_t change_count;
    char * first;
};

///not recently tested
///allocate memory in chunks (to improve performance) and record to free accordingly (maybe mark each element as start of block, record # of blocks in atomic, then go through list at end adding starts to alloced list and free when through all)
typedef struct cvm_lockfree_mp_mc_stack
{
    void * unit_allocation;
    size_t type_size;
    cvm_atomic_pad start_pad;
    _Atomic cvm_lockfree_mp_mc_stack_head allocated;
    cvm_atomic_pad allocated_pad;
    _Atomic cvm_lockfree_mp_mc_stack_head available;
    cvm_atomic_pad available_pad;
}
cvm_lockfree_mp_mc_stack;

void cvm_lockfree_mp_mc_stack_ini( cvm_lockfree_mp_mc_stack * stack , uint32_t num_units , size_t type_size );///not lockfree
bool cvm_lockfree_mp_mc_stack_add( cvm_lockfree_mp_mc_stack * stack , void * value );
bool cvm_lockfree_mp_mc_stack_get( cvm_lockfree_mp_mc_stack * stack , void * value );
void cvm_lockfree_mp_mc_stack_del( cvm_lockfree_mp_mc_stack * stack );///not lockfree




///    is faster than the general type under all usage patterns.
///
///    will work forever unless:
///        -a thread unexpectedly dies in the middle of an add/get operation.
///        -total stored entries exceed max_entry_count (unsafe because of this).

typedef struct cvm_fast_unsafe_mp_mc_list
{
    size_t type_size;
    uint_fast32_t max_entry_count;

    uint8_t * data;
    cvm_atomic_pad start_pad;
    atomic_uint_fast32_t fence;
    cvm_atomic_pad fence_pad;
    atomic_uint_fast32_t in;
    cvm_atomic_pad in_pad;
    atomic_uint_fast32_t out;
    cvm_atomic_pad out_pad;
}
cvm_fast_unsafe_mp_mc_list;

void cvm_fast_unsafe_mp_mc_list_ini( cvm_fast_unsafe_mp_mc_list * list , uint_fast32_t max_entry_count , size_t type_size );
void cvm_fast_unsafe_mp_mc_list_add( cvm_fast_unsafe_mp_mc_list * list , void * value );
bool cvm_fast_unsafe_mp_mc_list_get( cvm_fast_unsafe_mp_mc_list * list , void * value );
void cvm_fast_unsafe_mp_mc_list_del( cvm_fast_unsafe_mp_mc_list * list );




#endif

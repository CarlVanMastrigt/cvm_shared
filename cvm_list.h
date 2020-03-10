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
        if((child+1) comparison (h->count))                                     \
        {                                                                       \
            if(h->heap[child+1].test < h->heap[child].test)child++;             \
        }                                                                       \
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























//#define CVM_ZERO_8  0x00
//#define CVM_ZERO_16 0x0000
//#define CVM_ZERO_32 0x00000000
//#define CVM_ZERO_64 0x0000000000000000
//
//#define CVM_ONE_8  0x01
//#define CVM_ONE_16 0x0001
//#define CVM_ONE_32 0x00000001
//#define CVM_ONE_64 0x0000000000000001
//
//#define CVM_MID_8  0x80
//#define CVM_MID_16 0x8000
//#define CVM_MID_32 0x80000000
//#define CVM_MID_64 0x8000000000000000
//
//#define CVM_QTR_8  0x40
//#define CVM_QTR_16 0x4000
//#define CVM_QTR_32 0x40000000
//#define CVM_QTR_64 0x4000000000000000


typedef uint8_t cvm_atomic_pad[124];///+sizeof(uint32_t) is 128, required memory separation on most, if not all CPU's test this for actual performance difference




/**
    general multi-producer multi-consumer concurrent list

    allocates memory as needed.

    starts deleting free chunks when fraction in use is below some threshold and total chunks allocated is above some threshold.
        -these threshold should be set at creation time after initialisation.

    will work forever unless:
        -a thread unexpectedly dies in the middle of an add/get operation.
        -total stored datum goes above 2^32 (this can be increased to 2^64 by changing all 32 refrences in code to 64, which comes at the cost of about 5-20% time increase).
*/

//typedef struct cvm_coherent_large_data_mp_mc_list_element cvm_coherent_large_data_mp_mc_list_element;
//
//struct cvm_coherent_large_data_mp_mc_list_element
//{
//    cvm_coherent_large_data_mp_mc_list_element * next_element;
//    uint8_t * data;
//};
//
//typedef struct
//{
//    size_t type_size;
//    uint32_t block_size;
//    uint32_t active_chunks;
//    uint32_t total_chunks;
//    uint32_t del_numerator;
//    uint32_t del_denominator;
//    uint32_t min_total_chunks;
//    cvm_coherent_large_data_mp_mc_list_element * in_element;
//    cvm_coherent_large_data_mp_mc_list_element * out_element;
//    cvm_coherent_large_data_mp_mc_list_element * end_element;
//    cvm_atomic_pad start_pad;
//    _Atomic uint32_t spinlock;
//    cvm_atomic_pad spinlock_pad;
//    _Atomic uint32_t in;
//    cvm_atomic_pad in_pad;
//    _Atomic uint32_t out;
//    cvm_atomic_pad out_pad;
//    _Atomic uint32_t in_fence;
//    cvm_atomic_pad in_fence_pad;
//    _Atomic uint32_t in_op_count;
//    cvm_atomic_pad in_op_count_pad;
//    _Atomic uint32_t out_op_count;
//    cvm_atomic_pad out_op_count_pad;
//}
//cvm_coherent_large_data_mp_mc_list;
//
//void cvm_coherent_large_data_mp_mc_list_ini( cvm_coherent_large_data_mp_mc_list * list , uint32_t block_size , size_t type_size );///not coherent
//void cvm_coherent_large_data_mp_mc_list_add( cvm_coherent_large_data_mp_mc_list * list , void * value );
//bool cvm_coherent_large_data_mp_mc_list_get( cvm_coherent_large_data_mp_mc_list * list , void * value );
//void cvm_coherent_large_data_mp_mc_list_del( cvm_coherent_large_data_mp_mc_list * list );///not coherent
//
//void cvm_coherent_large_data_mp_mc_list_set_deletion_data( cvm_coherent_large_data_mp_mc_list * list , uint32_t numerator , uint32_t denominator , uint32_t min_total_chunks );













typedef struct cvm_coherent_mp_mc_list_element cvm_coherent_mp_mc_list_element;

struct cvm_coherent_mp_mc_list_element
{
    cvm_coherent_mp_mc_list_element * next_element;
    uint8_t * data;
};

typedef struct cvm_coherent_mp_mc_list
{
    size_t type_size;
    uint32_t block_size;
    cvm_coherent_mp_mc_list_element * in_element;
    cvm_coherent_mp_mc_list_element * out_element;
    cvm_coherent_mp_mc_list_element * end_element;
    cvm_atomic_pad start_pad;
    _Atomic uint32_t spinlock;
    cvm_atomic_pad spinlock_pad;
    _Atomic uint32_t in;
    cvm_atomic_pad in_pad;
    _Atomic uint32_t out;
    cvm_atomic_pad out_pad;
    _Atomic uint32_t in_buffer_fence;
    cvm_atomic_pad in_buffer_fence_pad;
    _Atomic uint32_t out_buffer_fence;
    cvm_atomic_pad out_buffer_fence_pad;
    _Atomic uint32_t in_completions;
    cvm_atomic_pad out_fence_pad;
    _Atomic uint32_t out_completions;
    cvm_atomic_pad out_completions_pad;
}
cvm_coherent_mp_mc_list;

void cvm_coherent_mp_mc_list_ini( cvm_coherent_mp_mc_list * list , uint32_t block_size , size_t type_size );///not coherent
void cvm_coherent_mp_mc_list_add( cvm_coherent_mp_mc_list * list , void * value );
bool cvm_coherent_mp_mc_list_get( cvm_coherent_mp_mc_list * list , void * value );
void cvm_coherent_mp_mc_list_del( cvm_coherent_mp_mc_list * list );///not coherent
















typedef struct cvm_lockfree_mp_mc_stack_head cvm_lockfree_mp_mc_stack_head;

struct cvm_lockfree_mp_mc_stack_head
{
    uintptr_t count;
    void * top;
};

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
int  cvm_lockfree_mp_mc_stack_add( cvm_lockfree_mp_mc_stack * stack , void * value );
int  cvm_lockfree_mp_mc_stack_get( cvm_lockfree_mp_mc_stack * stack , void * value );
void cvm_lockfree_mp_mc_stack_del( cvm_lockfree_mp_mc_stack * stack );///not lockfree











/**
    fast unsafe multi-producer multi-consumer concurrent list

    allocates set amount of memory at initialisation.

    is faster than the general type under all usage patterns.

    will work forever unless:
        -a thread unexpectedly dies in the middle of an add/get operation.
        -total stored datum goes above the ammount set at startup (unsafe because of this).
*/

typedef struct cvm_fast_unsafe_mp_mc_list
{
    size_t type_size;
    uint32_t list_size;
    cvm_atomic_pad start_pad;
    uint8_t * data;
    cvm_atomic_pad fence_status_pad;
    _Atomic uint32_t in;
    cvm_atomic_pad in_pad;
    _Atomic uint32_t out;
    cvm_atomic_pad out_pad;
}
cvm_fast_unsafe_mp_mc_list;

void cvm_fast_unsafe_mp_mc_list_ini( cvm_fast_unsafe_mp_mc_list * list , uint32_t list_size , size_t type_size );
void cvm_fast_unsafe_mp_mc_list_add( cvm_fast_unsafe_mp_mc_list * list , void * value );
int  cvm_fast_unsafe_mp_mc_list_get( cvm_fast_unsafe_mp_mc_list * list , void * value );
void cvm_fast_unsafe_mp_mc_list_del( cvm_fast_unsafe_mp_mc_list * list );



/**
    fast unsafe single-producer multi-consumer concurrent list

    allocates set amount of memory at initialisation.

    is faster than the general type under all usage patterns.

    will work forever unless:
        -a thread unexpectedly dies in the middle of an add/get operation.
        -total stored datum goes above the ammount set at startup (unsafe because of this).
        -cvm_fast_unsafe_sp_mc_list_add is called from more than 1 thread simultaneously for the same cvm_fast_unsafe_sp_mc_list object
*/

typedef struct cvm_fast_unsafe_sp_mc_list
{
    size_t type_size;
    uint32_t list_size;
    uint8_t * data;
    cvm_atomic_pad start_pad;
    _Atomic uint32_t in;
    cvm_atomic_pad in_pad;
    _Atomic uint32_t fence;
    cvm_atomic_pad fence_pad;
    _Atomic uint32_t out;
    cvm_atomic_pad out_pad;
}
cvm_fast_unsafe_sp_mc_list;

void cvm_fast_unsafe_sp_mc_list_ini( cvm_fast_unsafe_sp_mc_list * list , uint32_t list_size , size_t type_size );
void cvm_fast_unsafe_sp_mc_list_add( cvm_fast_unsafe_sp_mc_list * list , void * value );
int  cvm_fast_unsafe_sp_mc_list_get( cvm_fast_unsafe_sp_mc_list * list , void * value );
void cvm_fast_unsafe_sp_mc_list_del( cvm_fast_unsafe_sp_mc_list * list );



void test_coherent_data_structures(void);

#endif

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

#ifndef CVM_DATA_STRUCTURES_H
#define CVM_DATA_STRUCTURES_H





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











#ifndef CVM_BIN_HEAP
#define CVM_BIN_HEAP(type,name,cmp_func)                                        \
                                                                                \
typedef struct name##_bin_heap                                                  \
{                                                                               \
    type * heap;                                                                \
    uint_fast32_t space;                                                        \
    uint_fast32_t count;                                                        \
}                                                                               \
name##_bin_heap;                                                                \
                                                                                \
static inline void name##_bin_heap_ini( name##_bin_heap * h )                   \
{                                                                               \
    h->heap=malloc( sizeof( type ) );                                           \
    h->space=1;                                                                 \
    h->count=0;                                                                 \
}                                                                               \
                                                                                \
static inline void name##_bin_heap_add( name##_bin_heap * h ,   type data )     \
{                                                                               \
    if(h->count==h->space)h->heap=realloc(h->heap,sizeof(type)*(h->space*=2));  \
                                                                                \
    uint32_t u,d;                                                               \
    d=h->count++;                                                               \
                                                                                \
    while(d)                                                                    \
    {                                                                           \
        u=(d-1)>>1;                                                             \
        if(cmp_func(data,h->heap[u])) h->heap[d]=h->heap[u];                    \
        else break;                                                             \
        d=u;                                                                    \
    }                                                                           \
                                                                                \
    h->heap[d]=data;                                                            \
}                                                                               \
                                                                                \
static inline void name##_bin_heap_clr( name##_bin_heap * h )                   \
{                                                                               \
    h->count=0;                                                                 \
}                                                                               \
                                                                                \
static inline bool name##_bin_heap_get( name##_bin_heap * h , type * data )     \
{                                                                               \
    if(h->count==0)return false;                                                \
                                                                                \
    *data=h->heap[0];                                                           \
                                                                                \
    uint32_t u,d;                                                               \
    type r=h->heap[--(h->count)];                                               \
    u=0;                                                                        \
                                                                                \
    while((d=(u<<1)+1) < (h->count))                                            \
    {                                                                           \
        d+=((d+1 < h->count)&&(cmp_func(h->heap[d+1],h->heap[d])));             \
        if(cmp_func(r,h->heap[d])) break;                                       \
        h->heap[u]=h->heap[d];                                                  \
        u=d;                                                                    \
    }                                                                           \
                                                                                \
    h->heap[u]=r;                                                               \
    return 1;                                                                   \
}                                                                               \
                                                                                \
static inline void name##_bin_heap_del( name##_bin_heap * h )                   \
{                                                                               \
    free(h->heap);                                                              \
}                                                                               \

#endif














typedef uint8_t cvm_atomic_pad[256-sizeof(atomic_uint_fast32_t)];///+sizeof(uint32_t) is 128, required memory separation on most, if not all CPU's test this for actual performance difference




typedef struct cvm_thread_safe_expanding_queue
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
cvm_thread_safe_expanding_queue;

void cvm_thread_safe_expanding_queue_ini( cvm_thread_safe_expanding_queue * list , uint_fast32_t block_size , size_t type_size );///not coherent
void cvm_thread_safe_expanding_queue_add( cvm_thread_safe_expanding_queue * list , void * value );
bool cvm_thread_safe_expanding_queue_get( cvm_thread_safe_expanding_queue * list , void * value );
void cvm_thread_safe_expanding_queue_del( cvm_thread_safe_expanding_queue * list );///not coherent



typedef struct cvm_thread_safe_queue
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
cvm_thread_safe_queue;

void cvm_thread_safe_queue_ini( cvm_thread_safe_queue * list , uint_fast32_t max_entry_count , size_t type_size );///not coherent
bool cvm_thread_safe_queue_add( cvm_thread_safe_queue * list , void * value );
bool cvm_thread_safe_queue_get( cvm_thread_safe_queue * list , void * value );
void cvm_thread_safe_queue_del( cvm_thread_safe_queue * list );///not coherent






typedef struct cvm_lock_free_stack_head
{
    uint_fast64_t change_count;
    char * first;
}
cvm_lock_free_stack_head;

typedef struct cvm_lock_free_stack
{
    void * unit_allocation;
    size_t type_size;
    cvm_atomic_pad start_pad;
    _Atomic cvm_lock_free_stack_head allocated;
    cvm_atomic_pad allocated_pad;
    _Atomic cvm_lock_free_stack_head available;
    cvm_atomic_pad available_pad;
}
cvm_lock_free_stack;

void cvm_lock_free_stack_ini( cvm_lock_free_stack * stack , uint32_t num_units , size_t type_size );///not lockfree
bool cvm_lock_free_stack_add( cvm_lock_free_stack * stack , void * value );
bool cvm_lock_free_stack_get( cvm_lock_free_stack * stack , void * value );
void cvm_lock_free_stack_del( cvm_lock_free_stack * stack );///not lockfree




///    is faster than the general type under all usage patterns.
///    thread safe unless total stored entries exceed max_entry_count (not thread-safe because of this).

typedef struct cvm_coherent_limted_queue
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
cvm_coherent_limted_queue;

void cvm_coherent_limted_queue_ini( cvm_coherent_limted_queue * list , uint_fast32_t max_entry_count , size_t type_size );
void cvm_coherent_limted_queue_add( cvm_coherent_limted_queue * list , void * value );
bool cvm_coherent_limted_queue_get( cvm_coherent_limted_queue * list , void * value );
void cvm_coherent_limted_queue_del( cvm_coherent_limted_queue * list );




#endif
/**
Copyright 2020,2021,2022,2023,2024 Carl van Mastrigt

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

#ifndef solipsix_H
#include "solipsix.h"
#endif

#ifndef CVM_DATA_STRUCTURES_H
#define CVM_DATA_STRUCTURES_H


///add->push get->pull

#ifndef CVM_STACK
#define CVM_STACK(type,name,start_size)                                         \
                                                                                \
typedef struct name##_stack                                                     \
{                                                                               \
    type * data;                                                                \
    uint_fast32_t space;                                                        \
    uint_fast32_t count;                                                        \
}                                                                               \
name##_stack;                                                                   \
                                                                                \
                                                                                \
static inline void name##_stack_initialise( name##_stack * s )                  \
{                                                                               \
    assert(start_size>3 && !(start_size & (start_size-1)));                     \
    s->data=malloc( sizeof( type ) * start_size );                              \
    s->space=start_size;                                                        \
    s->count=0;                                                                 \
}                                                                               \
                                                                                \
static inline type * name##_stack_new( name##_stack * s )                       \
{                                                                               \
    if(s->count==s->space)                                                      \
    {                                                                           \
        s->space *= 2;                                                          \
        s->data=realloc(s->data, sizeof( type ) * s->space);                    \
    }                                                                           \
    return s->data+s->count++;                                                  \
}                                                                               \
                                                                                \
static inline void name##_stack_push( name##_stack * s , type value )           \
{                                                                               \
    *( name##_stack_new( s ) ) = value;                                         \
}                                                                               \
                                                                                \
static inline bool name##_stack_pull( name##_stack * s , type * value )         \
{                                                                               \
    if(s->count==0)return false;                                                \
    *value=s->data[--s->count];                                                 \
    return true;                                                                \
}                                                                               \
                                                                                \
static inline type * name##_stack_pull_ptr( name##_stack * s )                  \
{                                                                               \
    if(s->count==0)return NULL;                                                 \
    return s->data + --s->count;                                                \
}                                                                               \
                                                                                \
static inline void name##_stack_terminate( name##_stack * s )                   \
{                                                                               \
    free(s->data);                                                              \
}                                                                               \
                                                                                \
static inline void name##_stack_reset( name##_stack * s )                       \
{                                                                               \
    s->count=0;                                                                 \
}                                                                               \
                                                                                \
static inline void name##_stack_push_multiple                                   \
( name##_stack * s , uint_fast32_t count , const type * values)                 \
{                                                                               \
    uint_fast32_t n;                                                            \
    while((s->count+count) > s->space)                                          \
    {                                                                           \
        s->space *= 2;                                                          \
        s->data=realloc(s->data, sizeof( type ) * s->space);                    \
    }                                                                           \
    memcpy(s->data+s->count,values,sizeof( type )*count);                       \
    s->count+=count;                                                            \
}                                                                               \
                                                                                \
static inline size_t name##_stack_size( name##_stack * s )                      \
{                                                                               \
    return sizeof( type ) * s->count;                                           \
}                                                                               \
                                                                                \
static inline void name##_stack_copy( name##_stack * s , void * dst )           \
{                                                                               \
    memcpy( dst , s->data , sizeof( type ) * s->count );                        \
}                                                                               \
                                                                                \
static inline type * name##_stack_get_ptr( name##_stack * s , uint_fast32_t i ) \
{                                                                               \
    return s->data + i;                                                         \
}                                                                               \
                                                                                \
static inline void name##_stack_remove( name##_stack * s , uint_fast32_t i )    \
{                                                                               \
    memmove( s->data + i, s->data + i + 1, sizeof(type) * ( --s->count-i));     \
}                                                                               \

#endif

CVM_STACK(uint32_t,u32,16)
///used pretty universally, including below





#ifndef CVM_ARRAY
#define CVM_ARRAY(type,name,start_size)                                         \
                                                                                \
typedef struct name##_array                                                     \
{                                                                               \
    u32_stack available_indices;                                                \
    type * array;                                                               \
    uint_fast32_t space;                                                        \
    uint_fast32_t count;                                                        \
}                                                                               \
name##_array;                                                                   \
                                                                                \
                                                                                \
static inline void name##_array_initialise( name##_array * l )                  \
{                                                                               \
    assert(start_size>3 && !(start_size & (start_size-1)));                     \
    u32_stack_initialise(&l->available_indices);                                \
    l->array=malloc( sizeof( type ) * start_size );                             \
    l->space=start_size;                                                        \
    l->count=0;                                                                 \
}                                                                               \
                                                                                \
static inline uint32_t name##_array_add( name##_array * l , type value )        \
{                                                                               \
    uint_fast32_t n;                                                            \
    uint32_t i;                                                                 \
    if(!u32_stack_pull(&l->available_indices,&i))                               \
    {                                                                           \
        if(l->count==l->space)                                                  \
        {                                                                       \
            l->space *= 2;                                                      \
            l->array=realloc(l->array, sizeof( type ) * l->space);              \
        }                                                                       \
        i=l->count++;                                                           \
    }                                                                           \
    l->array[i]=value;                                                          \
    return i;                                                                   \
}                                                                               \
                                                                                \
static inline uint32_t name##_array_add_ptr                                     \
( name##_array * l , const type * value )                                       \
{                                                                               \
    uint_fast32_t n;                                                            \
    uint32_t i;                                                                 \
    if(!u32_stack_pull(&l->available_indices,&i))                               \
    {                                                                           \
        if(l->count==l->space)                                                  \
        {                                                                       \
            l->space *= 2;                                                      \
            l->array=realloc(l->array, sizeof( type ) * l->space);              \
        }                                                                       \
        i=l->count++;                                                           \
    }                                                                           \
    l->array[i]=*value;                                                         \
    return i;                                                                   \
}                                                                               \
                                                                                \
static inline type name##_array_get( name##_array * l , uint32_t i )            \
{                                                                               \
    u32_stack_push(&l->available_indices,i);                                    \
    return l->array[i];                                                         \
}                                                                               \
                                                                                \
/** returned pointer cannot be used after any other operation has occurred*/    \
static inline const type * name##_array_get_ptr( name##_array * l , uint32_t i )\
{                                                                               \
    u32_stack_push(&l->available_indices,i);                                    \
    return l->array+i;                                                          \
}                                                                               \
                                                                                \
static inline void name##_array_terminate( name##_array * l )                   \
{                                                                               \
    u32_stack_terminate(&l->available_indices);                                 \
    free(l->array);                                                             \
}                                                                               \
                                                                                \
static inline void name##_array_reset( name##_array * l )                       \
{                                                                               \
    u32_stack_reset(&l->available_indices);                                     \
    l->count=0;                                                                 \
}                                                                               \

#endif


CVM_ARRAY(uint32_t,u32,16)


/**
may be worth looking into adding support for random location deletion and heap
location tracking to support that, though probably not as adaptable as might be
desirable
*/


/**
CVM_BIN_HEAP_CMP(A,B) must be declared, with true resulting in the value A
moving UP the heap

TODO: make any inputs to heap_cmp pointers and const

TODO: use test sort heap performance improvements
*/

#ifndef CVM_BIN_HEAP
#define CVM_BIN_HEAP(type,name)                                                 \
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
static inline void name##_bin_heap_add( name##_bin_heap * h , const type data ) \
{                                                                               \
    if(h->count==h->space)h->heap=realloc(h->heap,sizeof(type)*(h->space*=2));  \
                                                                                \
    uint32_t u,d;                                                               \
    d=h->count++;                                                               \
                                                                                \
    while(d && (CVM_BIN_HEAP_CMP(data,h->heap[(u=(d-1)>>1)])))                  \
    {                                                                           \
        h->heap[d]=h->heap[u];                                                  \
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
    if(h->count==0)                                                             \
    {                                                                           \
        return false;                                                           \
    }                                                                           \
                                                                                \
    *data=h->heap[0];                                                           \
                                                                                \
    uint32_t u,d;                                                               \
    type r=h->heap[--(h->count)];                                               \
    u=0;                                                                        \
                                                                                \
    while((d=(u<<1)+1) < (h->count))                                            \
    {                                                                           \
        d+=((d+1 < h->count)&&(CVM_BIN_HEAP_CMP(h->heap[d+1],h->heap[d])));     \
        if(CVM_BIN_HEAP_CMP(r,h->heap[d]))                                      \
        {                                                                       \
            break;                                                              \
        }                                                                       \
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








#ifndef CVM_QUEUE
#define CVM_QUEUE(type,name,start_size)                                         \
                                                                                \
typedef struct name##_queue                                                     \
{                                                                               \
    type* data;                                                                 \
    uint_fast32_t space;                                                        \
    uint_fast32_t count;                                                        \
    uint_fast32_t front;                                                        \
}                                                                               \
name##_queue;                                                                   \
                                                                                \
                                                                                \
static inline void name##_queue_initialise( name##_queue* q )                   \
{                                                                               \
    assert(( (start_size) & ( (start_size) - 1 )) == 0);                        \
    q->data=malloc( sizeof( type ) * start_size );                              \
    q->space=start_size;                                                        \
    q->count=0;                                                                 \
    q->front=0;                                                                 \
}                                                                               \
                                                                                \
static inline uint32_t name##_queue_new_index( name##_queue* q )                \
{                                                                               \
    uint_fast32_t front_offset, move_count;                                     \
    type * src;                                                                 \
    if(q->count == q->space)                                                    \
    {                                                                           \
        q->data = realloc(q->data, sizeof(type) * q->count * 2);                \
        front_offset = q->front & (q->space - 1);                               \
        if(q->front & q->space)                                                 \
        {                                                                       \
            src = q->data + front_offset;                                       \
            move_count = q->space - front_offset;                               \
        }                                                                       \
        else                                                                    \
        {                                                                       \
            src = q->data;                                                      \
            move_count = front_offset;                                          \
        }                                                                       \
        memcpy(src + q->space, src, sizeof(type) * move_count);                 \
        q->space *= 2;                                                          \
    }                                                                           \
    return q->front + q->count++;                                               \
}                                                                               \
                                                                                \
static inline type * name##_queue_get_ptr( name##_queue* q , uint32_t index )   \
{                                                                               \
    return q->data + (index & (q->space - 1));                                  \
}                                                                               \
                                                                                \
static inline type * name##_queue_new( name##_queue* q )                        \
{                                                                               \
    return q->data + ( name##_queue_new_index(q) & (q->space - 1));             \
}                                                                               \
                                                                                \
static inline uint32_t name##_queue_enqueue( name##_queue* q , type value )     \
{                                                                               \
    uint_fast32_t i;                                                            \
    i = name##_queue_new_index(q);                                              \
    q->data[i & (q->space - 1)] = value;                                        \
    return i;                                                                   \
}                                                                               \
                                                                                \
static inline bool name##_queue_dequeue( name##_queue* q , type* value )        \
{                                                                               \
    if(q->count == 0)                                                           \
    {                                                                           \
        return false;                                                           \
    }                                                                           \
    if(value)                                                                   \
    {                                                                           \
        *value = q->data[ q->front & (q->space - 1) ];                          \
    }                                                                           \
    q->front++;                                                                 \
    q->count--;                                                                 \
    return true;                                                                \
}                                                                               \
                                                                                \
static inline type * name##_queue_dequeue_ptr( name##_queue* q )                \
{                                                                               \
    type * ptr;                                                                 \
    if(q->count == 0)                                                           \
    {                                                                           \
        return NULL;                                                            \
    }                                                                           \
    ptr = q->data + (q->front & (q->space - 1));                                \
    q->front++;                                                                 \
    q->count--;                                                                 \
    return ptr;                                                                 \
}                                                                               \
                                                                                \
static inline void name##_queue_terminate( name##_queue* q )                    \
{                                                                               \
    free(q->data);                                                              \
}                                                                               \
                                                                                \
static inline type * name##_queue_get_front_ptr( name##_queue* q )              \
{                                                                               \
    if(q->count == 0)                                                           \
    {                                                                           \
        return NULL;                                                            \
    }                                                                           \
    return q->data + (q->front & (q->space - 1));                               \
}                                                                               \
                                                                                \
static inline type * name##_queue_get_back_ptr( name##_queue* q )               \
{                                                                               \
    if(q->count == 0)                                                           \
    {                                                                           \
        return NULL;                                                            \
    }                                                                           \
    return q->data + ((q->front + q->count - 1) & (q->space - 1));              \
}                                                                               \
                                                                                \
static inline uint32_t name##_queue_front_index( name##_queue* q )              \
{                                                                               \
    return q->front;                                                            \
}                                                                               \
                                                                                \
static inline uint32_t name##_queue_back_index( name##_queue* q )               \
{                                                                               \
    return q->front + q->count - 1;                                             \
}                                                                               \


#endif

/**
brief on queue resizing:
need to move the correct part of the buffer to maintain (modulo) indices after resizing:
|+++o+++|
|---o+++++++----|
vs
        |+++o+++|
|+++--------o+++|
^ realloced segment array with alignment of relative (intended) indices/offsets


iterating a queue
for(i=0;i<q->count;i++) queue_get_ptr(q, q->front + i)
*/

// CVM_QUEUE(uint32_t,u32,16)







struct cvm_cache_link
{
    uint16_t older;
    uint16_t newer;
};

/**
CVM_CACHE_CMP( entry , key ) must be declared, is used for equality checking when finding
entries in the cache; this comparison should take pointers of the type (a) and
the key_type (B)
*/

#ifndef CVM_CACHE
#define CVM_CACHE(type,key_type,name)                                           \
typedef struct name##_cache                                                     \
{                                                                               \
    type* entries;                                                              \
    struct cvm_cache_link* links;                                               \
    uint16_t oldest;                                                            \
    uint16_t newest;                                                            \
    uint16_t first_free;                                                        \
    uint16_t count;                                                             \
}                                                                               \
name##_cache;                                                                   \
                                                                                \
static inline void name##_cache_initialise                                      \
( name##_cache * cache , uint32_t size)                                         \
{                                                                               \
    assert(size >= 2);                                                          \
    cache->links = malloc(sizeof(struct cvm_cache_link) * size);                \
    cache->entries = malloc(sizeof( type ) * size);                             \
    cache->oldest = CVM_INVALID_U16;                                            \
    cache->newest = CVM_INVALID_U16;                                            \
    cache->count  = 0;                                                          \
    cache->first_free = size - 1;                                               \
    while(size--)                                                               \
    {                                                                           \
        cache->links[size].newer = size - 1;                                    \
        cache->links[size].older = CVM_INVALID_U16;/*not needed*/               \
    }                                                                           \
    assert(cache->links[0].newer == CVM_INVALID_U16);                           \
}                                                                               \
                                                                                \
static inline void name##_cache_terminate( name##_cache * cache )               \
{                                                                               \
    free(cache->links);                                                         \
    free(cache->entries);                                                       \
}                                                                               \
                                                                                \
static inline type * name##_cache_find                                          \
( name##_cache * cache , const key_type key )                                   \
{                                                                               \
    uint16_t i,p;                                                               \
    const type * e;                                                             \
    for(i = cache->newest; i != CVM_INVALID_U16; i = cache->links[i].older)     \
    {                                                                           \
        e = cache->entries + i;                                                 \
        if(CVM_CACHE_CMP( e , key ))                                            \
        {/** move to newest slot in cache */                                    \
            if(cache->newest != i)                                              \
            {                                                                   \
                if(cache->oldest == i)                                          \
                {                                                               \
                    cache->oldest = cache->links[i].newer;                      \
                }                                                               \
                else                                                            \
                {                                                               \
                    p = cache->links[i].older;                                  \
                    cache->links[p].newer = cache->links[i].newer;              \
                }                                                               \
                cache->links[cache->newest].newer = i;                          \
                cache->links[i].older = cache->newest;                          \
                cache->links[i].newer = CVM_INVALID_U16;                        \
                cache->newest = i;                                              \
            }                                                                   \
            return cache->entries + i;                                          \
        }                                                                       \
    }                                                                           \
                                                                                \
    return NULL;                                                                \
}                                                                               \
                                                                                \
static inline type * name##_cache_new( name##_cache * cache , bool * evicted )  \
{                                                                               \
    uint16_t i;                                                                 \
    if(cache->first_free != CVM_INVALID_U16)                                    \
    {                                                                           \
        if(evicted)                                                             \
        {                                                                       \
            *evicted = false;                                                   \
        }                                                                       \
        i = cache->first_free;                                                  \
        cache->first_free = cache->links[i].newer;                              \
        if(cache->newest == CVM_INVALID_U16)                                    \
        {                                                                       \
            cache->oldest = i;                                                  \
        }                                                                       \
        else                                                                    \
        {                                                                       \
            cache->links[cache->newest].newer = i;                              \
        }                                                                       \
    }                                                                           \
    else                                                                        \
    {                                                                           \
        if(evicted)                                                             \
        {                                                                       \
            *evicted = true;                                                    \
        }                                                                       \
        i = cache->oldest;                                                      \
        cache->oldest = cache->links[i].newer;                                  \
        cache->links[cache->oldest].older = CVM_INVALID_U16;                    \
        cache->links[cache->newest].newer = i;                                  \
    }                                                                           \
    cache->links[i].newer = CVM_INVALID_U16;                                    \
    cache->links[i].older = cache->newest;                                      \
    cache->newest = i;                                                          \
    cache->count++;                                                             \
    return cache->entries + i;                                                  \
}                                                                               \
                                                                                \
static inline type * name##_cache_evict( name##_cache * cache )                 \
{                                                                               \
    uint16_t i;                                                                 \
    i = cache->oldest;                                                          \
    if(i == CVM_INVALID_U16)                                                    \
    {                                                                           \
        return NULL;                                                            \
    }                                                                           \
    cache->oldest = cache->links[i].newer;                                      \
    if(cache->oldest == CVM_INVALID_U16)                                        \
    {                                                                           \
        cache->newest = CVM_INVALID_U16;                                        \
    }                                                                           \
    else                                                                        \
    {                                                                           \
        cache->links[cache->oldest].older = CVM_INVALID_U16;                    \
    }                                                                           \
    cache->links[i].newer = cache->first_free;                                  \
    cache->links[i].older = CVM_INVALID_U16;/*not needed*/                      \
    cache->first_free = i;                                                      \
    cache->count--;                                                             \
    return cache->entries + i;                                                  \
}                                                                               \
                                                                                \
static inline void name##_cache_remove( name##_cache * cache , type * entry)    \
{                                                                               \
    uint16_t i = entry - cache->entries;                                        \
    if(cache->oldest == i)                                                      \
    {                                                                           \
        cache->oldest = cache->links[i].newer;                                  \
        cache->links[cache->oldest].older = CVM_INVALID_U16;                    \
    }                                                                           \
    else                                                                        \
    {                                                                           \
        cache->links[cache->links[i].older].newer = cache->links[i].newer;      \
    }                                                                           \
    if(cache->newest == i)                                                      \
    {                                                                           \
        cache->newest = cache->links[i].older;                                  \
        cache->links[cache->newest].newer = CVM_INVALID_U16;                    \
    }                                                                           \
    else                                                                        \
    {                                                                           \
        cache->links[cache->links[i].newer].older = cache->links[i].older;      \
    }                                                                           \
    cache->links[i].newer = cache->first_free;                                  \
    cache->links[i].older = CVM_INVALID_U16;/*not needed*/                      \
    cache->first_free = i;                                                      \
    cache->count--;                                                             \
}                                                                               \
                                                                                \
static inline type * name##_cache_get_oldest_ptr( name##_cache * cache )        \
{                                                                               \
    if(cache->oldest == CVM_INVALID_U16)                                        \
    {                                                                           \
        return NULL;                                                            \
    }                                                                           \
    return cache->entries + cache->oldest;                                      \
}                                                                               \

#endif


















#endif
